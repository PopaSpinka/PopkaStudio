"""stream.py — clips for the P4 board's Video app straight from this computer over Wi-Fi (boards/p4lcd7/p4-stream.md).

The studio's Stream button runs it (studio.py, /api/stream); on its own: $PY tools/p4stream.py [FOLDER].

  GET /list          a line a clip: name, size, seconds, title (tab between); the board makes tiles of them
  GET /NAME          the file, by pieces: "Range: bytes=A-B" (206); the connection stays open for the next piece
Nothing else: no other paths, nothing but the .p4v files of one folder, nothing written. While it runs it says on the network
once a second where it is (UDP broadcast to HELLO_PORT: "POPKASTREAM <port>"): the board hears it and shows the clips;
a few seconds of silence after stop() and they go from the board.

What it can carry is set by the board: its Wi-Fi chip keeps what arrives in a small internal memory, so no more than
sndbuf bytes a connection are on their way unacknowledged. Steady ~1.0 MB/s: clips made with p4v.py --q 18 (~0.7 MB/s)
play with room to spare.
"""
import http.server
import os
import re
import socket
import struct
import subprocess
import threading
import time

PORT = 8470
HELLO_PORT = 8471
SNDBUF = 12 * 1024


def lan_address():
    """This computer's address in the local network (the board connects to it)."""
    for dev in ("en0", "en1"):
        try:
            r = subprocess.run(["ipconfig", "getifaddr", dev], capture_output=True, text=True, timeout=2)
            if r.stdout.strip():
                return r.stdout.strip()
        except (OSError, subprocess.SubprocessError):
            pass
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("10.255.255.255", 1))
        return s.getsockname()[0]
    except OSError:
        return ""
    finally:
        s.close()


def clip_info(path):
    """(size, seconds, title) of a .p4v, or None. The title: the name without its ending, _ as spaces."""
    try:
        with open(path, "rb") as f:
            h = struct.unpack("<4s15I", f.read(64))
    except (OSError, struct.error):
        return None
    if h[0] != b"P4V2" or not h[4] or not h[5]:
        return None
    seconds = h[6] * h[5] / h[4]
    size = os.path.getsize(path)
    return size, seconds, os.path.basename(path)[:-4].replace("_", " ")


def clips(folder):
    """The folder's clips: [{name, title, seconds, mb_s}] by name."""
    out = []
    if os.path.isdir(folder):
        for n in sorted(os.listdir(folder), key=str.lower):
            if n.lower().endswith(".p4v") and not n.startswith("."):
                info = clip_info(os.path.join(folder, n))
                if info:
                    out.append({"name": n, "title": info[2], "seconds": round(info[1], 1), "mb_s": round(info[0] / 1048576 / info[1], 2)})
    return out


class _Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"  # (keep-alive: the board asks for piece after piece on the same connection)
    server_version = "PopkaStream"

    def setup(self):
        # A small send buffer: no more than that many bytes on their way to the board unacknowledged (its Wi-Fi driver keeps
        # what arrives in internal memory; with the whole 64 KB TCP window in flight the link to its Wi-Fi chip stalls)
        if self.server.owner.sndbuf:
            self.request.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, self.server.owner.sndbuf)
        super().setup()

    def log_message(self, *a):
        pass

    def _reply(self, code, body=b"", headers=()):
        self.send_response(code)
        for k, v in headers:
            self.send_header(k, v)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        owner = self.server.owner
        name = self.path.split("?")[0].lstrip("/")
        if name == "list":
            lines = [f"{c['name']}\t{os.path.getsize(os.path.join(owner.folder, c['name']))}\t{c['seconds']:.1f}\t{c['title']}" for c in clips(owner.folder)]
            return self._reply(200, ("\n".join(lines) + "\n").encode(), [("Content-Type", "text/plain")])
        path = os.path.join(owner.folder, name)
        if "/" in name or "\\" in name or name.startswith(".") or not name.lower().endswith(".p4v") or not os.path.isfile(path):
            return self._reply(404)
        size = os.path.getsize(path)
        a, b = 0, size - 1
        m = re.match(r"bytes=(\d+)-(\d*)", self.headers.get("Range", ""))
        if m:
            a = int(m.group(1))
            b = min(int(m.group(2)) if m.group(2) else size - 1, size - 1)
        if a > b:
            return self._reply(416, headers=[("Content-Range", f"bytes */{size}")])
        self.send_response(206 if m else 200)
        self.send_header("Content-Length", str(b - a + 1))
        if m:
            self.send_header("Content-Range", f"bytes {a}-{b}/{size}")
        self.send_header("Accept-Ranges", "bytes")
        self.end_headers()
        try:
            with open(path, "rb") as f:
                f.seek(a)
                left = b - a + 1
                while left:
                    chunk = f.read(min(left, 256 * 1024))
                    if not chunk:
                        break
                    self.wfile.write(chunk)
                    left -= len(chunk)
                    owner.count(len(chunk))
        except (BrokenPipeError, ConnectionResetError):
            pass  # (the board went elsewhere in the clip, or closed it)


class _Server(http.server.ThreadingHTTPServer):
    daemon_threads = True

    def handle_error(self, request, client_address):
        pass  # (the board drops a connection when it goes elsewhere in a clip: not worth a trace in the studio's log)


class Stream:
    """One stream: start() serves the folder and says so on the network, stop() ends both. state() for the studio's page."""

    def __init__(self, folder, port=PORT, sndbuf=SNDBUF):
        self.folder, self.port, self.sndbuf = os.path.abspath(folder), port, sndbuf
        self._srv = None
        self._sent = 0
        self._rate = 0.0
        self._lock = threading.Lock()
        self.address = ""

    def count(self, n):
        with self._lock:
            self._sent += n

    @property
    def on(self):
        return self._srv is not None

    def start(self):
        if self._srv:
            return
        os.makedirs(self.folder, exist_ok=True)
        srv = _Server(("0.0.0.0", self.port), _Handler)
        srv.owner = self
        self._srv = srv
        self.address = lan_address()
        threading.Thread(target=srv.serve_forever, daemon=True).start()
        threading.Thread(target=self._hello, args=(srv,), daemon=True).start()

    def stop(self):
        srv, self._srv = self._srv, None
        if srv:
            srv.shutdown()
            srv.server_close()

    def _hello(self, srv):
        """Once a second: "here is a stream" to the whole local network; and how many MB/s went out."""
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        msg = f"POPKASTREAM {self.port}\n".encode()
        try:
            while self._srv is srv:
                for to in ("255.255.255.255", re.sub(r"\.\d+$", ".255", self.address) if self.address else ""):
                    if to:
                        try:
                            s.sendto(msg, (to, HELLO_PORT))
                        except OSError:
                            pass
                time.sleep(1)
                with self._lock:
                    self._rate, self._sent = self._sent / 1048576, 0
        finally:
            s.close()

    def state(self):
        return {"on": self.on, "address": self.address, "port": self.port, "out_mb_s": round(self._rate, 2) if self.on else 0, "clips": clips(self.folder)}
