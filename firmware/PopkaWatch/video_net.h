// video_net.h — clips straight from PopkaStudio on the Mac over Wi-Fi (its Stream button; tools/p4stream.py does the same),
// boards/p4lcd7/p4-stream.md. Included from video_hw.h, on the board and with Wi-Fi only.
//
// The studio says once a second on the network (UDP broadcast to VN_HELLO_PORT, "POPKASTREAM <port>") that it streams; the
// board hears it, lists its clips (GET /list) and shows them on the screen of tiles; a few seconds of silence — they go.
// A clip comes by pieces (Range): one task on core 0 keeps VN_CONNS connections open and fetches pieces of VN_CHUNK into a
// cache of VN_CHUNKS of them in PSRAM, from the place being watched on. The reader (vh_reader) takes a clip's records from
// the cache as from the card (vn_read) and waits when they are not there yet. The player starts, and goes on after a stop,
// with VN_START bytes ahead.
//
// What limits it is the internal memory of the P4: the C6's Wi-Fi driver keeps what arrives there, and when it can't get a
// buffer the link to the C6 stalls for good. So the reader's and this file's tasks keep their stacks in PSRAM, the driver
// may fall back on PSRAM when internal memory refuses (vn_hook_driver), and the computer sends no more than 12 KB a
// connection unacknowledged (--sndbuf). Wi-Fi stays connected and awake. That carries ~1.0 MB/s; the link may still stall
// for good after minutes (a restart brings it back): the driver itself, boards/p4lcd7/p4-stream.md.
//
// Commands: vnet — how the stream goes (vnet HOST PORT — a computer that doesn't announce itself, vnet off — forget it);
// vget NAME FROM BYTES — one piece, how fast.
#include <WiFiUdp.h>
#include <esp_memory_utils.h>
#include "esp_hosted_os_abstraction.h"

#define VN_CHUNK (512 * 1024)
#define VN_CHUNKS 24                // 12 MB: ~12 s of a clip of 1 MB/s
#define VN_CONNS 3                  // 3 x 12 KB on their way: 2 x 16 was slower, 2 x 32 stalled the link (internal memory)
#define VN_START (5u << 20)         // bytes ahead to start and to go on after a stop (or the file's end): ~7 s of a 0.7 MB/s clip
#define VN_HEAD_MAX 512
#define VN_HELLO_PORT 8471          // where the studio says it streams
#define VN_GONE_MS 6000             // silence after which its clips go

static char vn_host[48];
static int vn_port;
static volatile bool vn_present;    // a computer streams (heard, or set by vnet)
static bool vn_manual;              // (set by vnet: it never goes)
static uint32_t vn_seen;            // when it was last heard
static uint8_t *vn_mem;             // the cache: chunk k of the open file in slot k % VN_CHUNKS
static volatile int32_t vn_have[VN_CHUNKS];   // which chunk a slot holds (-1 — none)
static volatile bool vn_busy[VN_CHUNKS];      // a connection is filling it
static volatile uint32_t vn_lo;     // the chunk being read: those before it are let go
static volatile uint32_t vn_gen;    // a new place or a new clip: what is in flight is dropped
static volatile bool vn_on;         // a network clip is open
static char vn_name[64];
static uint32_t vn_size, vn_bps;    // the file's bytes; the clip's bytes a second (its average)
static volatile uint64_t vn_rx;     // bytes received, all along
static volatile uint32_t vn_stalls, vn_go_ms;
static TaskHandle_t vn_task;

struct VnConn {
  WiFiClient c;
  int32_t chunk = -1;               // the chunk it fetches (-1 — idle)
  uint32_t gen, got, want, last;  // (last: when it last received something)
  int hn;
  bool body;
  char head[VN_HEAD_MAX];
};
static VnConn *vn_conns;
static void vh_begin();  // (video_hw.h: the reader task, which lists the clips)

// The C6's driver (ESP-Hosted 2.12.11 in the core) takes a buffer of internal memory for every read from the C6 — 4 to 19 KB,
// tens of thousands a minute, and keeps ~50 KB of them once a stream has run — and when one is refused the link stalls for
// good: with the interface, the sound and the player there are 20-50 KB left (boards/p4lcd7/p4-stream.md). In its table of OS
// functions what internal memory refuses comes from PSRAM (the driver's own MEMPOOL_PREFER_SPIRAM, off in the core's
// build, does that on the P4). Only then: its buffers from PSRAM as a rule restarted the board. Tried and dropped (docs/
// p4-stream.md): the frame's tiles in PSRAM while a clip streams (frames twice as long, and they never got their memory back:
// the driver keeps ~50 KB once a stream has run), a private heap of the tiles' memory (crashed: some buffers are freed by
// free() itself), bands of 8 or 14 rows (frames 7 → 13 and 8 ms)
static void *(*vn_os_malloc_align)(size_t, size_t), *(*vn_os_malloc)(size_t), *(*vn_os_calloc)(size_t, size_t);
static void (*vn_os_free_align)(void *), (*vn_os_free)(void *);
static volatile uint32_t vn_os_ext;
static void *vn_from_psram(size_t n, size_t a) {
  void *p = heap_caps_aligned_alloc(a > 128 ? a : 128, (n + 127) & ~(size_t)127, MALLOC_CAP_SPIRAM);  // (a PSRAM line: 128)
  if (p) vn_os_ext++;
  return p;
}
// PSRAM only when internal memory refuses: the driver's buffers from PSRAM as a rule broke the link (the reads of 4 KB and
// more from PSRAM restarted the board as soon as the clips were listed: the C6 resets, the driver restarts the P4)
static void *vn_hk_malloc_align(size_t n, size_t a) {
  void *p = vn_os_malloc_align(n, a);
  return p ? p : vn_from_psram(n, a);
}
static void *vn_hk_malloc(size_t n) {
  void *p = vn_os_malloc(n);
  return p ? p : vn_from_psram(n, 8);
}
static void *vn_hk_calloc(size_t k, size_t n) {
  void *p = vn_os_calloc(k, n);
  if (!p && (p = vn_from_psram(k * n, 8))) memset(p, 0, k * n);
  return p;
}
static void vn_hk_free_align(void *p) {
  if (esp_ptr_external_ram(p)) heap_caps_free(p);
  else vn_os_free_align(p);
}
static void vn_hk_free(void *p) {
  if (esp_ptr_external_ram(p)) heap_caps_free(p);
  else vn_os_free(p);
}
static void vn_hook_driver() {
  if (vn_os_malloc_align) return;
  vn_os_malloc_align = g_hosted_osi_funcs._h_malloc_align, vn_os_free_align = g_hosted_osi_funcs._h_free_align, vn_os_free = g_hosted_osi_funcs._h_free;
  vn_os_malloc = g_hosted_osi_funcs._h_malloc, vn_os_calloc = g_hosted_osi_funcs._h_calloc;
  g_hosted_osi_funcs._h_free_align = vn_hk_free_align, g_hosted_osi_funcs._h_free = vn_hk_free;
  g_hosted_osi_funcs._h_malloc_align = vn_hk_malloc_align, g_hosted_osi_funcs._h_malloc = vn_hk_malloc, g_hosted_osi_funcs._h_calloc = vn_hk_calloc;
}
// Wi-Fi up. With Video the board stays on the network all the time: the clock's task never drops it (hclk_hold) and power
// save is off (with it on the access point holds the packets back: 0.03 MB/s instead of 1.5) — both from vn_commands at
// the start. Here it is only waited for. Wi-Fi's own calls are never made from the reader's or this file's task: from a
// task whose stack is in PSRAM they never reach the C6
static bool vn_wifi(uint32_t wait_ms) {
  static uint32_t asked;
  for (const uint32_t t0 = millis(); WiFi.status() != WL_CONNECTED; delay(50)) {
    // (no network: the clock's task joins it again at once, not in its 10 minutes — HaloClock.h, hclk_task)
    if (hclk_handle && (!asked || millis() - asked > 20000)) xTaskNotifyGive(hclk_handle), asked = millis();
    if (millis() - t0 >= wait_ms) return false;
  }
  return true;
}

// One whole answer on its own connection (the list, a header, a picture): n bytes from a into to; the length, or -1
static int vn_get(const char *path, uint32_t a, uint32_t n, uint8_t *to, uint32_t cap) {
  if (!vn_wifi(20000)) return -1;
  WiFiClient c;
  if (!c.connect(vn_host, vn_port, 3000)) return -1;
  if (n) c.printf("GET /%s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%lu-%lu\r\nConnection: close\r\n\r\n", path, vn_host, (unsigned long)a, (unsigned long)(a + n - 1));
  else c.printf("GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, vn_host);
  char line[128];
  int len = -1, code = 0;
  const uint32_t t0 = millis();
  for (;;) {  // the head, line by line
    const int k = c.readBytesUntil('\n', line, sizeof line - 1);
    if (k <= 0 && (!c.connected() || millis() - t0 > 5000)) return -1;
    line[k > 0 ? k : 0] = 0;
    if (!code) sscanf(line, "HTTP/%*s %d", &code);
    if (!strncasecmp(line, "Content-Length:", 15)) len = atoi(line + 15);
    if (k <= 1) break;  // (the empty line)
  }
  if (code != 200 && code != 206) return -1;
  uint32_t got = 0;
  const uint32_t want = len >= 0 ? min((uint32_t)len, cap) : cap;
  while (got < want && (c.connected() || c.available()) && millis() - t0 < 15000) {
    const int k = c.read(to + got, want - got);
    if (k > 0) got += k;
    else delay(1);
  }
  return got;
}

// The computer's clips onto the screen of tiles, after the card's (vh_scan): their pictures as the card's
static void vn_scan_list(int &n);
static void vn_scan(int &n) {
  if (vn_present) vn_scan_list(n);
}
static void vn_scan_list(int &n) {
  if (!vn_wifi(25000)) return;
  static char *list;
  if (!list) list = (char *)heap_caps_malloc(8192, MALLOC_CAP_SPIRAM);
  static uint8_t *poster;
  if (!poster) poster = (uint8_t *)heap_caps_aligned_alloc(128, VH_POSTER_MAX, MALLOC_CAP_SPIRAM);
  const int k = list && poster ? vn_get("list", 0, 0, (uint8_t *)list, 8191) : -1;
  if (k <= 0) return (void)LOG("video: no list from %s:%d\n", vn_host, vn_port);
  list[k] = 0;
  int found = 0;
  for (char *ln = strtok(list, "\n"); ln && n < VH_MAX; ln = strtok(nullptr, "\n")) {
    char name[64], title[48];
    unsigned long size = 0;
    float secs = 0;
    if (sscanf(ln, "%63[^\t]\t%lu\t%f\t%47[^\t\n]", name, &size, &secs, title) != 4) continue;
    uint32_t h[16];
    if (vn_get(name, 0, 64, (uint8_t *)h, 64) != 64 || memcmp(h, "P4V2", 4)) continue;
    VhFile &F = vh_files[n];
    if (!F.thumb) F.thumb = (uint16_t *)heap_caps_aligned_alloc(128, VH_TW * VH_TH * 2, MALLOC_CAP_SPIRAM);
    if (!F.thumb) break;
    memset(F.thumb, 0x22, VH_TW * VH_TH * 2);
    if (h[9] && h[10] && h[10] <= VH_POSTER_MAX && vn_get(name, h[9], h[10], poster, VH_POSTER_MAX) == (int)h[10]) {
      esp_cache_msync(poster, (h[10] + 127) & ~127, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
      if (vh_decode(poster, h[10], F.thumb, VH_TW * VH_TH * 2)) {
        esp_cache_msync(F.thumb, VH_TW * VH_TH * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
        vh_bake(F.thumb);
        esp_cache_msync(F.thumb, VH_TW * VH_TH * 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
      }
    }
    strlcpy(F.name, name, sizeof F.name), strlcpy(F.title, title, sizeof F.title);
    F.seconds = (uint32_t)((uint64_t)h[6] * h[5] / h[4]), F.net = 1, F.bytes = size;
    vh_duration(F);
    vh_fit_title(F);
    __sync_synchronize();
    vh_n = ++n, found++;
  }
  LOG("video: %d clips from %s:%d\n", found, vn_host, vn_port);
}

// The header and the tables of a network clip (vh_load): h — its 64 bytes, *t — the tables (count + 1 offsets, count sizes)
static bool vn_head(const VhFile &F, uint32_t *h, uint32_t **t) {
  if (vn_get(F.name, 0, 64, (uint8_t *)h, 64) != 64 || memcmp(h, "P4V2", 4) || h[1]) return LOG("video: no header from the network\n"), false;
  const uint32_t bytes = 4 * (2 * h[6] + 1);
  *t = (uint32_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
  if (!*t || vn_get(F.name, 64, bytes, (uint8_t *)*t, bytes) != (int)bytes) return heap_caps_free(*t), *t = nullptr, LOG("video: no table from the network\n"), false;
  strlcpy(vn_name, F.name, sizeof vn_name);
  vn_size = F.bytes, vn_bps = F.seconds ? F.bytes / F.seconds : 1 << 20;
  return true;
}

// From byte at on (a new clip or a new place): what was in the cache is forgotten
static void vn_go(uint32_t at) {
  vn_gen = vn_gen + 1;
  for (int s = 0; s < VN_CHUNKS; s++) vn_have[s] = -1;
  vn_lo = at / VN_CHUNK, vn_go_ms = millis();
  vn_on = true;
  if (vn_task) xTaskNotifyGive(vn_task);
}
static void vn_stop() {
  vn_on = false;
}

// Bytes ahead of the place being read, in a row
static uint32_t vn_ahead() {
  uint32_t k = vn_lo, n = 0;
  while (k < vn_lo + VN_CHUNKS && (uint64_t)k * VN_CHUNK < vn_size && vn_have[k % VN_CHUNKS] == (int32_t)k) n += VN_CHUNK, k++;
  return n;
}
// Enough ahead to start (or to go on after a stop): VN_START, the file's end, or 12 s of trying
static bool vn_ready() {
  const bool r = !vn_on || vn_ahead() >= min<uint32_t>(VN_START, vn_size - min<uint32_t>(vn_size, vn_lo * VN_CHUNK)) || millis() - vn_go_ms > 12000;
  static uint32_t told;
  if (r && vn_on && told != vn_go_ms) told = vn_go_ms, LOG("video: %u KB ahead in %lu ms\n", (unsigned)(vn_ahead() >> 10), (unsigned long)(millis() - vn_go_ms));
  return r;
}

// n bytes of the open clip from at (a record, vh_reader) out of the cache, waiting for them; false — an order came
// (rd_gen, rd_cmd) or nothing for 20 s. They are written for the JPEG decoder's DMA: out of the processor's cache after
static bool vn_read(uint32_t at, uint32_t n, uint8_t *to) {
  const int cmd = rd_cmd;  // (the open itself reads its first frame under RD_OPEN: only a change is an order)
  const uint32_t gen = rd_gen;
  for (uint32_t done = 0; done < n;) {
    const uint32_t p = at + done, k = p / VN_CHUNK, s = k % VN_CHUNKS, in = p % VN_CHUNK;
    if (k != vn_lo) vn_lo = k;  // (sequential: the chunks before it are free for those ahead)
    const uint32_t t0 = millis();
    while (vn_have[s] != (int32_t)k) {
      if (rd_gen != gen || rd_cmd != cmd || millis() - t0 > 20000) return false;
      if (vn_task) xTaskNotifyGive(vn_task);
      delay(2);
    }
    const uint32_t m = min(n - done, VN_CHUNK - in);
    memcpy(to + done, vn_mem + (size_t)s * VN_CHUNK + in, m);
    done += m;
  }
  esp_cache_msync(to, (n + 127) & ~127, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
  return true;
}

// The next chunk to fetch: the first in the window ahead that is neither here nor on its way; -1 — none
static int32_t vn_next() {
  for (uint32_t k = vn_lo; k < vn_lo + VN_CHUNKS && (uint64_t)k * VN_CHUNK < vn_size; k++) {
    const uint32_t s = k % VN_CHUNKS;
    if (vn_have[s] != (int32_t)k && !vn_busy[s]) return k;
  }
  return -1;
}

static void vn_drop(VnConn &q) {  // (a connection that broke, or an answer not wanted any more)
  if (q.chunk >= 0) vn_busy[q.chunk % VN_CHUNKS] = false;
  q.chunk = -1, q.c.stop();
}

// One connection, a step: ask for a chunk, read its head, its body into the cache. true — it did something
static bool vn_step(VnConn &q) {
  if (q.chunk < 0) {
    const int32_t k = vn_next();
    if (k < 0) return false;
    if (!q.c.connected() && !q.c.connect(vn_host, vn_port, 3000)) return false;
    q.c.setNoDelay(true);
    const uint32_t a = (uint32_t)k * VN_CHUNK, b = min((uint32_t)(k + 1) * VN_CHUNK, vn_size) - 1;
    q.c.printf("GET /%s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%lu-%lu\r\n\r\n", vn_name, vn_host, (unsigned long)a, (unsigned long)b);
    vn_busy[k % VN_CHUNKS] = true;
    q.chunk = k, q.gen = vn_gen, q.got = 0, q.want = b - a + 1, q.hn = 0, q.body = false, q.last = millis();
    return true;
  }
  const int av = q.c.available();
  if (av <= 0) {
    // (nothing for 2.5 s: a lost packet that never came again — seen on the last 1436 bytes of a chunk while the others
    // waited for exactly that chunk. The connection goes, the chunk is fetched anew)
    if (!q.c.connected() || millis() - q.last > 2500) vn_drop(q);
    return false;
  }
  q.last = millis();
  uint8_t *slot = vn_mem + (size_t)(q.chunk % VN_CHUNKS) * VN_CHUNK;
  if (!q.body) {  // the head: up to the empty line; what comes after it is the body's start
    const int k = q.c.read((uint8_t *)q.head + q.hn, min(av, VN_HEAD_MAX - 1 - q.hn));
    if (k <= 0) return false;
    q.hn += k, q.head[q.hn] = 0;
    char *e = strstr(q.head, "\r\n\r\n");
    if (!e) {
      if (q.hn >= VN_HEAD_MAX - 1) vn_drop(q);
      return true;
    }
    if (strncmp(q.head + 9, "206", 3)) return vn_drop(q), true;
    q.body = true;
    const int rest = q.hn - (int)(e + 4 - q.head);
    if (rest > 0) memcpy(slot, e + 4, min((uint32_t)rest, q.want)), q.got = min((uint32_t)rest, q.want), vn_rx += q.got;
  } else {
    const int k = q.c.read(slot + q.got, min((uint32_t)av, q.want - q.got));
    if (k <= 0) return false;
    q.got += k, vn_rx += k;
  }
  if (q.got >= q.want) {  // the whole chunk: it is in the cache, if still wanted
    const uint32_t s = q.chunk % VN_CHUNKS;
    if (q.gen == vn_gen && (uint32_t)q.chunk >= vn_lo) vn_have[s] = q.chunk;
    vn_busy[s] = false, q.chunk = -1;
  }
  return true;
}

// The studio's hello: its address and port. A computer heard anew, or silent for VN_GONE_MS — the screen of tiles lists again
static void vn_hello() {
  static WiFiUDP udp;
  static bool bound;
  if (WiFi.status() != WL_CONNECTED) return;
  if (!bound) bound = udp.begin(VN_HELLO_PORT);
  bool fresh = false;
  for (int k = udp.parsePacket(); k > 0; k = udp.parsePacket()) {
    char msg[48] = "";
    udp.read(msg, sizeof msg - 1);
    int port = 0;
    if (sscanf(msg, "POPKASTREAM %d", &port) != 1 || port <= 0 || vn_manual) continue;
    const String ip = udp.remoteIP().toString();
    fresh |= !vn_present || strcmp(vn_host, ip.c_str()) || vn_port != port;
    strlcpy(vn_host, ip.c_str(), sizeof vn_host), vn_port = port, vn_seen = millis();
  }
  if (fresh) vn_present = true, LOG("video: a stream from %s:%d\n", vn_host, vn_port), vh_rescan = true;
  if (vn_present && !vn_manual && millis() - vn_seen > VN_GONE_MS) vn_present = false, LOG("video: the stream went\n"), vh_rescan = true;
}

static void vn_loop(void *) {
  uint32_t lost = 0;
  for (;;) {
    // (no network for 20 s — after a fault the C6 may not join at once: the clock's task joins it again, not in 10 minutes)
    if (WiFi.status() == WL_CONNECTED) lost = 0;
    else if (!lost) lost = millis();
    else if (millis() - lost > 20000 && hclk_handle) xTaskNotifyGive(hclk_handle), lost = millis();
    vn_hello();
    if (!vn_on || !vn_conns) {
      if (vn_conns)
        for (int i = 0; i < VN_CONNS; i++) vn_drop(vn_conns[i]);
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(250));
      continue;
    }
    if (WiFi.status() != WL_CONNECTED) {
      for (int i = 0; i < VN_CONNS; i++) vn_drop(vn_conns[i]);
      vn_wifi(500);
      continue;
    }
    bool any = false;
    for (int i = 0; i < VN_CONNS; i++) {
      VnConn &q = vn_conns[i];
      if (q.chunk >= 0 && q.gen != vn_gen) vn_drop(q);  // (a new place: the old answer is not read to its end)
      any |= vn_step(q);
    }
    if (!any) ulTaskNotifyTake(pdTRUE, 1);
  }
}

// The cache and the connections, at the first network clip (the task runs from the start: it listens for the studio)
static bool vn_begin() {
  if (vn_conns) return true;
  vn_mem = (uint8_t *)heap_caps_aligned_alloc(128, (size_t)VN_CHUNK * VN_CHUNKS, MALLOC_CAP_SPIRAM);
  VnConn *c = (VnConn *)heap_caps_malloc(sizeof(VnConn) * VN_CONNS, MALLOC_CAP_SPIRAM);
  if (!vn_mem || !c) return LOG("video: no memory for the network's cache\n"), false;
  for (int i = 0; i < VN_CONNS; i++) new (&c[i]) VnConn();  // (one by one: an array's placement new may put a count before it)
  for (int s = 0; s < VN_CHUNKS; s++) vn_have[s] = -1;
  __sync_synchronize();
  vn_conns = c;
  return true;
}

// How the stream goes, for the player's title and vnet: what comes in, how much is ahead (s), how many stops
static void vn_status(char *out, int n) {
  static uint32_t t0;
  static uint64_t rx0;
  static float rate;
  const uint32_t now = millis();
  if (now - t0 >= 1000) rate = (vn_rx - rx0) / 1048576.0f / ((now - t0) / 1000.0f), rx0 = vn_rx, t0 = now;
  snprintf(out, n, "net %.1f MB/s   ahead %.0f s   stops %lu", rate, vn_ahead() / (float)max<uint32_t>(vn_bps, 1), (unsigned long)vn_stalls);
}

static void vn_commands() {
  hclk_hold = true, WiFi.setSleep(false), vn_hook_driver();  // (the network stays up and awake: vn_wifi)
  // (above the reader: what arrives must not wait in internal memory. Its stack in PSRAM: internal memory is the stream's;
  // it never calls Wi-Fi itself — from a task whose stack is in PSRAM those calls never reach the C6)
  xTaskCreatePinnedToCoreWithCaps(vn_loop, "video_net", 6144, nullptr, 6, &vn_task, 0, MALLOC_CAP_SPIRAM);
  halo_command("vget", [](const char *a) {  // vget NAME FROM BYTES — one piece from the computer, how fast (a test)
    char name[64];
    unsigned long at = 0, n = 0;
    if (sscanf(a, "%63s %lu %lu", name, &at, &n) != 3 || !n) return;
    static uint8_t *buf;
    if (!buf) buf = (uint8_t *)heap_caps_malloc(64 << 10, MALLOC_CAP_SPIRAM);
    if (!vn_wifi(20000)) return (void)LOG("vget: no wifi\n");
    WiFiClient c;
    if (!c.connect(vn_host, vn_port, 3000)) return (void)LOG("vget: no connection\n");
    c.setNoDelay(true);
    c.printf("GET /%s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%lu-%lu\r\nConnection: close\r\n\r\n", name, vn_host, at, at + n - 1);
    uint32_t got = 0, win = 0;
    const uint32_t t0 = millis();
    uint32_t tw = t0, last = t0;
    char line[160];
    int p = 0;
    while (got < n && millis() - last < 3000) {
      const int av = c.available();
      int k = 0;
      if (av > 0) k = c.read(buf, min(av, 64 << 10));
      const uint32_t now = millis();
      if (k > 0) got += k, win += k, last = now;
      else delay(1);
      if (now - tw >= 250) {
        p += snprintf(line + p, sizeof line - p, " %lu", (unsigned long)(win / 1024));
        win = 0, tw = now;
        if (p > 120) LOG("vget: KB/250ms%s\n", line), p = 0;
      }
    }
    if (p) LOG("vget: KB/250ms%s\n", line);
    const uint32_t ms = millis() - t0;
    LOG("vget: %lu bytes in %lu ms, %.2f MB/s, wifi %d, internal %u KB, dma %u KB\n", (unsigned long)got, (unsigned long)ms, got / 1048576.0f / (ms / 1000.0f + 0.001f), WiFi.status() == WL_CONNECTED,
        (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024), (unsigned)(heap_caps_get_free_size(MALLOC_CAP_DMA) / 1024));
  });
  halo_command("vnet", [](const char *a) {
    char h[48] = "";
    int port = 8470;
    if (sscanf(a, "%47s %d", h, &port) >= 1) {
      vn_manual = strcmp(h, "off") != 0;
      if (vn_manual) strlcpy(vn_host, h, sizeof vn_host), vn_port = port;
      vn_present = vn_manual, vh_rescan = true;
    }
    char s[96];
    vn_status(s, sizeof s);
    LOG("video: stream %s:%d%s, %s, wifi %d, driver buffers from psram %lu, dma free %d KB\n", vn_present ? vn_host : "none", vn_port, vn_manual ? " (set)" : "", s,
        WiFi.status() == WL_CONNECTED, (unsigned long)vn_os_ext, (int)(heap_caps_get_free_size(MALLOC_CAP_DMA) >> 10));
  });
}
