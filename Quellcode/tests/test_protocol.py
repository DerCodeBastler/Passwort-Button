"""Native Prüfung des tatsächlichen Firmwarecodes mit ASan/UBSan."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory() as d:
    exe=Path(d)/'test_firmware'
    subprocess.run(['gcc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',str(root),str(root/'test_firmware.c'),'-o',str(exe)],check=True)
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
    subprocess.run([str(exe)],env=env,check=True)
