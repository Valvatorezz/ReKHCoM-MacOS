"""Fixed-address LLVM IR rewriting retained from the upstream web builder."""
import re


def rewrite_ir(source, symbols):
    # Only imported globals are fixed-address cartridge aliases. A definition
    # with the same name indicates a generator error; do not silently remove it.
    imported = set()
    lines = []
    for line in source.splitlines(keepends=True):
        match = re.match(r'@([\w.$]+)\s*=\s*(.*)', line)
        if match and match[1] in symbols:
            if not match[2].startswith('external '):
                raise ValueError(f'Fixed symbol has a native definition: {match[1]}')
            imported.add(match[1])
        else:
            lines.append(line)
    return re.sub(r'@([\w.$]+)', lambda m: f'inttoptr (i32 {symbols[m[1]]} to ptr)'
                  if m[1] in imported else m[0], ''.join(lines))
