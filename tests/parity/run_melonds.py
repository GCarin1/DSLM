#!/usr/bin/env python3
"""Unattended melonDS parity run (Windows): test-harness criterion 1.

Launches the portable melonDS from .tools/melonds (docs/building.md) with
dslm.nds and the .tools/card sync folder, presses START by posting a key to
its window (no mouse or focus change; melonDS must map START to Enter),
waits for the generation, closes melonDS normally so it writes the card back,
then runs `tools/dslm.py parity` on /dslm/tokens.txt.

    python tests/parity/run_melonds.py --steps 128 [--model fp32|int8]
"""
import argparse
import ctypes
import pathlib
import re
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
MELON = ROOT / ".tools" / "melonds" / "melonDS.exe"
CARD = ROOT / ".tools" / "card"
NDS = ROOT / "dslm.nds"

WM_KEYDOWN, WM_KEYUP, WM_CLOSE, VK_RETURN = 0x100, 0x101, 0x10, 0x0D


def find_window(pid, timeout=20.0):
    user32 = ctypes.windll.user32
    found = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
    def callback(hwnd, _):
        owner = ctypes.c_ulong()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid and user32.IsWindowVisible(hwnd) and user32.GetWindowTextLengthW(hwnd):
            found.append(hwnd)
        return True

    deadline = time.time() + timeout
    while time.time() < deadline:
        found.clear()
        user32.EnumWindows(callback, 0)
        if found:
            return found[0]
        time.sleep(0.5)
    raise SystemExit("error: melonDS window did not appear")


def press_start(hwnd):
    user32 = ctypes.windll.user32
    scan = user32.MapVirtualKeyW(VK_RETURN, 0)
    down = 1 | (scan << 16)
    user32.PostMessageW(hwnd, WM_KEYDOWN, VK_RETURN, down)
    time.sleep(0.15)
    user32.PostMessageW(hwnd, WM_KEYUP, VK_RETURN, down | (3 << 30))


CHECKPOINTS = {"int8": "/dslm/models/stories260K_q80.bin", "fp32": "/dslm/models/stories260K.bin"}


def set_run(ini, steps, model):
    text = ini.read_text(encoding="utf-8")
    new = re.sub(r"(?m)^steps\s*=.*$", f"steps       = {steps}", text)
    new = re.sub(r"(?m)^checkpoint\s*=.*$", f"checkpoint = {CHECKPOINTS[model]}", new)
    ini.write_text(new, encoding="utf-8", newline="\n")
    return text


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--steps", type=int, default=128)
    ap.add_argument("--model", choices=sorted(CHECKPOINTS), default="int8")
    ap.add_argument("--wait", type=float, help="seconds to let it generate (default: from steps)")
    args = ap.parse_args()
    if sys.platform != "win32":
        raise SystemExit("error: this runner drives the Windows melonDS build")
    for path in (MELON, NDS, CARD / "dslm" / "dslm.ini"):
        if not path.exists():
            raise SystemExit(f"error: {path} missing (see docs/building.md and tools/dslm.py stage)")

    ini = CARD / "dslm" / "dslm.ini"
    tokens = CARD / "dslm" / "tokens.txt"
    original = set_run(ini, args.steps, args.model)
    if tokens.exists():
        tokens.unlink()
    wait = args.wait if args.wait is not None else 15 + args.steps * (0.8 if args.model == "fp32" else 0.2)
    proc = subprocess.Popen([str(MELON), str(NDS)])
    try:
        hwnd = find_window(proc.pid)
        time.sleep(6)  # boot and model load
        press_start(hwnd)
        print(f"generating {args.steps} steps, waiting {wait:.0f} s")
        time.sleep(wait)
        ctypes.windll.user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
        proc.wait(timeout=30)
    except Exception:
        proc.kill()
        raise
    finally:
        ini.write_text(original, encoding="utf-8", newline="\n")

    if not tokens.exists():
        raise SystemExit("error: no tokens.txt (generation did not finish in time; raise --wait)")
    return subprocess.call([sys.executable, str(ROOT / "tools" / "dslm.py"), "parity", str(tokens)])


if __name__ == "__main__":
    sys.exit(main())
