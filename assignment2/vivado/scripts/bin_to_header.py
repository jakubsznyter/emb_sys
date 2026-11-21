#!/usr/bin/env python3
"""Same output as `xxd -i -n NAME in.bin out.h` (for machines without xxd)."""
import sys
from pathlib import Path

name, src, dst = sys.argv[1], Path(sys.argv[2]), Path(sys.argv[3])
data = src.read_bytes()
lines = [', '.join(f'0x{b:02x}' for b in data[i:i + 12]) for i in range(0, len(data), 12)]
dst.write_text(f'unsigned char {name}[] = {{\n  ' + ',\n  '.join(lines) + f'\n}};\nunsigned int {name}_len = {len(data)};\n')
print(f'{dst}: {name} {len(data)} bytes')
