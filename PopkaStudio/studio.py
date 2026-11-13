"""The studio moved to studio/studio.py (2.1). What still starts this file — the restart after an update
from 2.0 or older, an old launcher — is handed over to it."""
import os
import sys

VERSION = "2.1"  # (the updater of 2.0 and older reads it here)
NEW = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "studio", "studio.py")
os.chdir(os.path.dirname(NEW))
os.execv(sys.executable, [sys.executable, NEW] + sys.argv[1:])
