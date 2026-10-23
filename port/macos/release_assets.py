"""Keep generated cartridge text in the user's runtime ROM, not the executable.

Only ignored, staged build inputs are changed. Original game sources and the
shared asset generator are retained for other targets and private local builds.
"""
import json
import re
import shutil
import subprocess

import textgen


def prepare(root, out, region, rom, symbols):
    if region != 'us':
        raise RuntimeError('The release text path currently supports the US cartridge only')
    gen = out / 'release-gen'
    shutil.copytree(root / 'build' / region / 'gen', gen, dirs_exist_ok=True)
    charmap = textgen.load_charmap(region)
    samples = {}
    fragments = 0
    riku_start = None
    for path in sorted(textgen.POOL_DIR.glob('*.yaml')):
        pool = textgen.Pool(path)
        if not pool.present(region):
            continue
        encoded = textgen.read_source(pool, region, charmap)
        for unit in pool.units(region):
            placed, _ = unit.layout(lambda entry, _start: encoded[entry['name']])
            declarations = []
            for i, (entry, start, size) in enumerate(placed):
                name = entry['name']
                if 'record' not in entry:
                    data = encoded[name]
                    if rom[start - textgen.ROM_BASE:start - textgen.ROM_BASE + len(data)] != data:
                        raise RuntimeError('Generated text differs from the cartridge: ' + name)
                    if len(data) >= 32:
                        samples[name] = data
                    if unit.form == 'macro':
                        samples[name] = rom[start - textgen.ROM_BASE:start - textgen.ROM_BASE + size]
                if unit.kind != 'fragment':
                    continue
                if unit.form == 'macro':
                    if unit.name != 'riku_deck_names.inc' or unit.stride != 224 or unit.size != 20:
                        raise RuntimeError('Unsupported release text macro: ' + unit.name)
                    # The names are inline fields of Deck, rather than pointers.
                    # Stage the enclosing table as an external ROM-backed symbol.
                    riku_start = unit.start - 198
                    continue
                previous = symbols.setdefault(name, start)
                if previous != start:
                    raise RuntimeError('Conflicting text address: ' + name)
                if 'record' in entry:
                    ctype = pool.types[entry['record']].get('ctype', 'const ' + entry['record'])
                    declarations.append(f'extern {ctype} {name};')
                else:
                    width = unit.encoding(charmap).width
                    slot = (placed[i + 1][1] if i + 1 < len(placed) else unit.end) - start
                    count = slot // width if unit.count == 'slot' else len(encoded[name]) // width
                    ctype = entry.get('ctype', unit.ctype)
                    declarations.append(f'extern {ctype} {name}[{count}];')
            if unit.kind == 'fragment':
                (gen / unit.name).write_text('/* Runtime ROM declarations; no cartridge payload. */\n' + '\n'.join(declarations) + '\n')
                fragments += 1
    if riku_start is None:
        raise RuntimeError('Missing Riku deck text manifest')
    symbols['sRikuDecks'] = riku_start
    source = root / 'src/card/card_battle_riku.c'
    content, count = re.subn(
        r'static const Deck sRikuDecks\[21\] = \{.*?^\};',
        'extern const Deck sRikuDecks[21];\n'
        'typedef char ReleaseDeckLayout[(sizeof(Deck) == 224 && '
        '__builtin_offsetof(Deck, name) == 198) ? 1 : -1];',
        source.read_text(), flags=re.M | re.S)
    if count != 1:
        raise RuntimeError('Riku deck integration anchor changed')
    staged = out / 'staged/card_battle_riku.c'
    staged.parent.mkdir(parents=True, exist_ok=True)
    staged.write_text(content)
    # Fail closed if an upstream edit changes any cards, names or deck values.
    # The actual guest also asserts the Deck layout above, using its own header.
    table = re.search(r'static const Deck sRikuDecks\[21\] = \{.*?^\};', source.read_text(), re.M | re.S).group()
    fixture = out / 'release-deck-check.c'
    fixture.write_text('#include <stdint.h>\n#include <stdio.h>\n'
        'typedef struct { uint16_t cards[99]; unsigned char name[20]; '
        'uint16_t cpCost,cardCount,unk; } Deck;\n'
        '#include "riku_deck_names.inc"\n' + table +
        '\nint main(void) { return fwrite(sRikuDecks,1,sizeof(sRikuDecks),stdout) != sizeof(sRikuDecks); }\n')
    check = out / 'release-deck-check'
    subprocess.run(['clang','-I'+str(root/'build'/region/'gen'),str(fixture),'-o',str(check)],check=True)
    deck_bytes = subprocess.check_output([str(check)])
    offset = riku_start - textgen.ROM_BASE
    if len(deck_bytes) != 21 * 224 or deck_bytes != rom[offset:offset + len(deck_bytes)]:
        raise RuntimeError('The source Riku deck table differs from the runtime cartridge')
    (out / 'release-inputs.json').write_text(json.dumps({
        'region': region, 'text_fragments': fragments,
        'text_audit_samples': len(samples), 'riku_decks_address': riku_start,
        'riku_deck_table_matches_rom': True,
    }, indent=2) + '\n')
    return gen, staged, samples


def audit(binary, samples):
    data = binary.read_bytes()
    found = [name for name, sample in samples.items() if sample in data]
    if found:
        raise RuntimeError('Generated cartridge text remains in executable: ' + ', '.join(found[:10]))
    return {'generated_text_samples_checked': len(samples), 'embedded_samples_found': 0}
