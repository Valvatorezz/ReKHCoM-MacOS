# Experimental macOS quick save/load

This opt-in prototype lives on `quicksaveload`. It is based on the current
`macos-port` checkpoint, not the later upstream merge into `main`. It does not
enable boss rematches. The normal build continues to disable quick states.

## Build and controls

From the repository root, add `--experimental-states` to the existing build
command. Combine it with `--release` to retain runtime ROM text:

```sh
.venv/bin/python port/macos/build_macos.py \
  --experimental-states --release --rom roms/B8CE.gba \
  --wasi-sdk build/macos/deps/wasi-sdk-34.0-arm64-macos \
  --w2c2 build/macos/deps/w2c2 \
  --wasm-opt build/macos/deps/binaryen-version_133/bin/wasm-opt \
  --sdl build/macos/deps/sdl --jobs 4
```

The output is `build/macos/states-release/us/`; without `--release` it is
`build/macos/states/us/`. Neither overwrites the normal release ZIP or installs
an app. The region restrictions of `--release` still apply.

- F5 saves the current slot.
- F9 loads the current slot.
- F6/F7 select the previous/next slot, using upstream's slot controls.
- Mac keyboards may require Fn with the function keys.

Files are `states/slot1.macstate` through `slot9.macstate` inside the selected
data folder. States require the same core/build identity, ROM and data-folder
path. They are not compatible with Windows states or arbitrary future builds.
Moving the app after creating a state, other regions, display changes and
physical-controller input around a load remain unverified.

## Manual testing without touching player data

Run the test game directly using a fresh data folder, rather than installing
over the working app. From the repository root:

```sh
test_data=$(mktemp -d "$PWD/build/macos/manual-states.XXXXXX")
game=$(.venv/bin/python -c 'import json; print(json.load(open("build/macos/states-release/us/native-build.json"))["game_executable"])')
"$game" --rom "$PWD/roms/B8CE.gba" --data-dir "$test_data"
```

Use F5 in a field or battle, move or take damage, then use F9. Quit and relaunch
the same command using the same `$test_data` to check that F9 works after a
restart. The default slot is 1. Record keyboard/controller behaviour and listen
for stale or missing audio. Keep the regular player's app and data folder separate.

A request inside a nested menu/movie wait is deferred until the next normal
game-loop boundary. It is not an arbitrary native-stack checkpoint.

## Implementation

The builder stages a split of `AgbMain`: normal initialisation runs once, then
an exported loop resumes at an explicit boundary before `UpdateKeyState`.
The extracted loop has no persistent local variables and is compiled without
optimisation to prevent loop-carried native temporaries. Source game files are
not rewritten in the repository. The emitted LLVM loop was inspected to confirm
that the checkpoint comes before work for the next frame and has no allocations.

Saving captures the guest's 384 MiB linear memory plus CPU copies of active
texture pixels. The current SRAM, its dirty-check/bank bookkeeping and current
configuration are excluded from the saved payload. These live values survive a
load. Loading itself does not replace or write a cartridge `.sav`; subsequent
normal game saving still behaves as the game does.

The host validates the full state before applying it: format, build/ROM/data
identity, lengths, header and payload SHA-256, texture bounds and resource
availability. Texture recreation is prepared before changing the running guest.
Only the diagnostic game log may remain open; other open guest files or
directories cause a request to be rejected. Function-table contents must still
match the freshly instantiated module. No native function pointers or SDL
handles are serialised. A build-time guard rejects changes to the generated
runtime's supported global/table layout.

After validation, the host abandons the old native call stack, restores memory,
rebinds recreated textures and the existing log, resets the guest stack pointer,
clears queued audio/input, resets pacing and re-enters the exported game loop.
This avoids restoring a process stack or depending on macOS load addresses.

Writes use compression and a same-directory temporary file, flush/fsync and
rename. The first implementation favours completeness over small state files:
states contain local game memory, including cartridge data, and must not be
bundled in release archives. The release payload check rejects `.macstate` files.

## Automated checks

```sh
.venv/bin/python port/macos/test_states.py --rom roms/B8CE.gba
.venv/bin/python port/macos/test_macos.py --rom roms/B8CE.gba \
  --region us --build-dir build/macos/states-release/us
```

These run in newly created `build/macos/tests/` folders with synthetic SRAM and
dummy SDL video/audio. Save-state tests inject real F5/F9 events through the
native input bridge, compare later framebuffer files against baseline runs,
and check that the synthetic SRAM file is unchanged. The existing deterministic
menu-animation clock is enabled so elapsed-time blinking does not create false
image differences. Dummy audio verifies the path, not audible quality.

This remains experimental until manual field/battle, actual save-point/continue,
audio, held-input, repeated-load and longer gameplay checks are completed.
