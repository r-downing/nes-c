import sys
import subprocess

exe, nes_rom, nes_log = sys.argv[1:4]

with open(nes_log, "rt") as fp:
    nes_log_lines = fp.readlines()

sp = subprocess.run([exe, nes_rom, str(len(nes_log_lines))], capture_output=True)

assert sp.returncode == 0, f"{sp.returncode}"
output = sp.stdout.decode().strip().split("\n")

assert (len(output) == len(nes_log_lines)), f"{len(output)} == {len(nes_log_lines)}"

import re

pattern = r"(?P<PC>[0-9A-F]{4}).* A:(?P<A>[0-9A-F]{2}) X:(?P<X>[0-9A-F]{2}) Y:(?P<Y>[0-9A-F]{2}) P:(?P<P>[0-9A-F]{2}) SP:(?P<SP>[0-9A-F]{2}).*CYC:(?P<CYC>[0-9]+)"

for i, (line1, line2) in  enumerate(zip(nes_log_lines, output)):
    m1 = re.match(pattern, line1)
    m2 = re.match(pattern, line2)
    print(f"{i + 1:04d}: {line1.strip():100s} {line2.strip()}")
    gd1 = m1.groupdict()
    gd2 = m2.groupdict()
    if gd1 != gd2:
        print(f"mismatch {[k for k in gd1 if gd1[k] != gd2[k]]}")
        sys.exit(1)


