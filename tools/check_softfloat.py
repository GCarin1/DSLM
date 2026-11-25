#!/usr/bin/env python3
"""Fail if the fixed-point forward pass can reach a soft-float routine.

Disassembles build/dslm.elf, builds the call graph from the per-token entry
points (fx_forward, fx_sample, matmul_fx) and looks for libgcc soft-float
helpers (__aeabi_f*, __aeabi_d*, __*sf*, __*df*). Integer helpers such as
__aeabi_ldivmod are allowed. Run inside the Docker image after `make nds`.
"""
import os
import re
import subprocess
import sys

ELF = sys.argv[1] if len(sys.argv) > 1 else "build/dslm.elf"
OBJDUMP = os.environ.get("OBJDUMP", "/opt/wonderful/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-objdump")
ROOTS = tuple(os.environ.get("CHECK_ROOTS", "fx_forward,fx_sample,matmul_fx").split(","))
SOFTFLOAT = re.compile(r"^(__aeabi_[fd]\w*|__aeabi_[iu]2[fd]|__aeabi_l2[fd]|__aeabi_ul2[fd]|__\w*[sd]f\d?\w*)$")
INTEGER_OK = {"__aeabi_ldivmod", "__aeabi_uldivmod", "__aeabi_idiv", "__aeabi_uidiv", "__aeabi_idivmod",
              "__aeabi_uidivmod", "__aeabi_lmul", "__aeabi_llsl", "__aeabi_llsr", "__aeabi_lasr"}


def symbols(elf):
    """address -> function name (Thumb bit cleared), from nm."""
    nm = OBJDUMP.replace("objdump", "nm")
    out = subprocess.run([nm, "-n", elf], check=True, capture_output=True, text=True).stdout
    table = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "TtWw":
            table[int(parts[0], 16) & ~1] = parts[2]
    return table


def call_graph(text, table):
    graph, current = {}, None
    for line in text.splitlines():
        m = re.match(r"^[0-9a-f]+ <([^>]+)>:$", line)
        if m:
            current = m.group(1)
            graph.setdefault(current, set())
            continue
        if current is None:
            continue
        # direct calls and branches to other functions, and long-call literals (ITCM)
        for target in re.findall(r"\b(?:bl|blx|b)\s+[0-9a-f]+ <([^>+]+)>", line):
            if target != current:
                graph[current].add(target)
        # -mlong-calls (ARM-mode files, ITCM) call through literal-pool addresses
        for value in re.findall(r"\.word\s+0x([0-9a-f]+)", line):
            target = table.get(int(value, 16) & ~1)
            if target and target != current:
                graph[current].add(target)
    return graph


def main():
    text = subprocess.run([OBJDUMP, "-d", "--no-show-raw-insn", ELF], check=True,
                          capture_output=True, text=True).stdout
    graph = call_graph(text, symbols(ELF))
    roots = [f for f in graph if f.split(".")[0] in ROOTS]
    missing = [r for r in ROOTS if not any(f.split(".")[0] == r for f in graph)]
    seen, stack, bad = set(), list(roots), []
    while stack:
        f = stack.pop()
        if f in seen:
            continue
        seen.add(f)
        for g in graph.get(f, ()):
            if SOFTFLOAT.match(g) and g not in INTEGER_OK:
                bad.append((f, g))
            elif g in graph:
                stack.append(g)
    print("roots:", ", ".join(sorted(roots)), "| missing:", ", ".join(missing) or "none")
    print("reachable:", ", ".join(sorted(seen)))
    if bad:
        for f, g in sorted(set(bad)):
            print(f"SOFT-FLOAT: {f} calls {g}")
        return 1
    if missing:
        print("roots not found: is the fixed-point engine linked?")
        return 1
    print("no soft-float in the fixed-point forward pass")

    # fixed-point-inference: the matmul inner loop runs from ITCM (mirrored at 0x01000000, 32 KB)
    if "matmul_fx" in ROOTS:
        addr = next((a for a, n in symbols(ELF).items() if n.split(".")[0] == "matmul_fx"), None)
        if addr is None or not 0x01000000 <= addr < 0x01008000:
            print(f"matmul_fx is not in ITCM (address {addr and hex(addr)})")
            return 1
        print(f"matmul_fx in ITCM at {addr:#010x}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
