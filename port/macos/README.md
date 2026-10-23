# ReKHCoM on Apple Silicon — local experimental port

This target builds an ordinary arm64 macOS application with SDL3. It runs the
recompiled game code, not a GBA CPU emulator. The build uses wasm32 as an
intermediate representation, translates it to C with w2c2, then compiles that
C to native Apple Silicon instructions. The installed app uses no browser,
JavaScript engine, WASI runtime, JIT, Wine, or Rosetta.

Primary play/test system: M2 MacBook Air (Mac14,2), **macOS 27.0.1**, confirmed
by the user. The app retains a macOS 15.0 minimum deployment target so that
using it on a different Apple Silicon Mac does not unnecessarily require 27.
Older supported OS versions still need their own real-machine testing.

## Portability between Macs

- This build is for **Apple Silicon** (M-series), not Intel Macs. Its compiler
  settings target baseline arm64 and do not require M2-specific instructions.
  Intel support would need a separate x86_64 build and validation.
- SDL is bundled. The other runtime libraries are macOS system libraries;
  another Mac does not need Homebrew or the build tools. The builder checks
  these requirements before packaging so local dependencies cannot silently
  become requirements on another machine.
- On macOS 27 the app uses SDL's native macOS graphics, audio and controller
  backends. Keeping a lower deployment target does not force software rendering
  or Rosetta. Optimizations should be measured and preserve game timing and
  the original GBA presentation; there is no reason to raise the OS minimum
  or add a chip-specific requirement for the current build.
- Transfer the app and your own matching-region ROM separately. To transfer
  progress, quit the game on both Macs and copy the Application Support folder
  described below, backing up the destination first. Ad-hoc signing may require
  normal per-app approval on the receiving Mac.

## Install and play

1. Build with your own supported cartridge dump as described below. Extract
   `build/macos/<region>/ReKHCoM-macos-arm64-<region>.zip` and move `ReKHCoM.app`
   to Applications, or a local folder outside cloud synchronization.
2. Double-click the app to open the upstream launcher. Use **Install ROM…**
   to choose your own `.gba` file. It verifies SHA-1 and copies the dump to
   Application Support/roms, separately from the app. Select USA and press **Play**.
   Other regions require their own matching game executable built from that dump.
3. Play using the keyboard or an SDL-supported controller. Close the window or
   use Command-Q to quit normally. Keep a backup of normal cartridge saves.

The app is locally ad-hoc signed, not notarized or signed with a Developer ID.
No global security settings need changing. If macOS blocks it, use the normal
per-app Open approval in Finder/System Settings after verifying your local build.

The initial preset preserves the 240×160 GBA view at its 3:2 aspect ratio,
nearest-neighbor scaling, pixel text, original audio, and original button
combinations. New Game may still ask the upstream Original/Re:CoM question;
choose **Original**. The upstream options can change the presentation.

| Control | Keyboard | Controller position |
|---|---|---|
| GBA A / B | X or K / Z or J | South / east |
| GBA L / R | A / S | Left / right shoulder or trigger |
| Start / Select | Return / Backspace or right Shift | Start / Back |
| Movement | Arrow keys (W also up) | D-pad / left stick |
| Options | Escape, F1, or Start+Select | Start+Back |
| Fullscreen | F11 or Option-Return | Use options |

On a Mac keyboard, F-keys may require Fn. One-button dodge and stock are off in
the original preset; the game's original combinations still work. Keyboard and
controller bindings are retained in `kh_com.ini` using upstream's names. The
upstream launcher opens first, with graphics, sound, enhancements, controls and
save tools. Play is wired to hide the launcher and return when the game exits;
that full GUI round trip still needs verification. HD text can be enabled under
**Enhancements → Experimental**; pixel text remains the default. The bundled
Latin fonts support US/EU text; JP HD text remains disabled. See VALIDATION.md
for the current launcher input-testing limitation.

## Saves, settings, and logs

Normal launches use:

```
~/Library/Application Support/ReKHCoM/ReKHCoM/
```

US saves are `kh_com_us.sav`; EU is `kh_com.sav`; JP is `kh_com_jp.sav`.
These are normal 64 KiB SRAM files. The game writes temporary files and renames
them; launcher save imports/edits back up the existing save before atomic replacement. Settings are `kh_com.ini`; diagnostics are `kh_com.log` and
`kh_com_host.log`. Native faults also append `kh_com_crash.log`; macOS retains
its normal crash reporting. Guest bounds traps log frame and memory details
before exiting, rather than pretending an invalid memory access succeeded. Updating/replacing the app does not replace this directory.
For imports, quit first, back up the existing file, and use a matching-region
raw GBA SRAM save. Emulator container formats/save states are not SRAM saves.

For an isolated run or troubleshooting, launch the executable from Terminal:

```sh
/path/to/ReKHCoM.app/Contents/MacOS/khcom_us \
  --rom '/path/to/your-dump.gba' --data-dir '/path/to/isolated-test-directory'
```

`KHCOM_ROM` and `KHCOM_DATA` are equivalent game environment defaults. Explicit
game arguments win. The launcher accepts `--rom`; use `KHCOM_DATA` to isolate
its data folder. Launching the bundled game executable directly skips the launcher. Very long data paths inherit upstream's roughly 250-character
base-path limit. Prefer the default folder or a short test path.

## Repository and platform audit

Inspected before modifying source: upstream revision
`42da59f14c0b534fdc66f585670ecb29b552b9f7`, `README.md`, `CONTRIBUTING.md`,
`LICENSE.md`, `requirements.txt`, `configure_pc.py`, Linux/web/console builds,
Android64 notes/host, and font/launcher asset notices. No upstream game source
was edited. The build stages a few checked substitutions into ignored `build/`
files; a changed integration anchor stops the build.

| Existing assumption/dependency | macOS treatment |
|---|---|
| Windows i686 + llvm-mingw, PE sections and linker symbols | Native arm64 host; wasm32 core uses linear-memory offsets |
| 32-bit pointers embedded in cartridge structures | Keep core pointers 32-bit; translate offsets at the host boundary |
| Original GBA memory addresses / Windows VirtualAlloc | Addresses are offsets inside checked 384 MiB linear memory |
| Linux i386/armhf, ELF sysroot, fixed mmap, bundled glibc loader | None are needed by the macOS target |
| Windows GDI/FreeType/console HD text fonts | Optional HD text through the existing stb TrueType path and bundled fonts |
| X11/Wayland, ALSA/PulseAudio, udev/fbdev | SDL3 Cocoa/Metal, CoreAudio, native HID/GameController |
| Windows launcher, Win32 dialogs/process/shortcut APIs | Upstream Dear ImGui UI with Cocoa picker, POSIX process/save services |
| Windows/Linux relative writable executable directory | Application Support, or explicit isolated data directory |
| PS5/Switch quick states and setjmp reset | Quick states disabled; reset flushes and recreates the core |
| Optional HD replacements and achievement badge atlas | HD music/movies deferred; badge atlas retained |

The direct Switch/Android ARM64 ILP32 image loader is **not** a macOS 15 path:
Apple's ARM64 loader enforces a 4 GiB low-address guard. The local reduced-pagezero
probe was killed before main. See Apple's
[`mach_loader.c`](https://github.com/apple-oss-distributions/xnu/blob/xnu-11417.140.69/bsd/kern/mach_loader.c).
Rewriting the complete decompilation's structures for LP64 would be much more
invasive. Linear-memory translation avoids both restrictions. Binaryen's
function-pointer cast emulation is required before w2c2 because the original
code casts task/mode callbacks between signatures; runtime checks remain on.

## Build from source

This private checkout omits the PS4, PS5, Vita and Web platform folders and
their dedicated tools. The macOS target's reused PS5 guest headers, shims,
bridge declarations and stb font renderer now live under `port/macos/guest`,
`shared` and `third_party`. Their original contents and embedded notices are
retained; PS5-derived source filenames remain for traceability. The fixed-address
LLVM IR helper formerly imported from the Web builder is `wasm_ir.py` here.
Shared game, launcher and achievement code remains in place. Other platforms'
documentation and cross-platform tooling may still describe the upstream tree;
this checkout's supported local build path is the macOS helper below.

Requirements: Apple Command Line Tools (`clang`, `ld`, `codesign`, `make`),
Python 3, and network access for preparing build dependencies. The helper
installs everything else inside this checkout and creates a local virtualenv.
It does not install or modify Homebrew packages.

```sh
sh port/macos/prepare_dependencies.sh
.venv/bin/python port/macos/build_macos.py \
  --rom '/absolute/path/to/your-dump.gba' \
  --wasi-sdk build/macos/deps/wasi-sdk-34.0-arm64-macos \
  --w2c2 build/macos/deps/w2c2 \
  --wasm-opt build/macos/deps/binaryen-version_133/bin/wasm-opt \
  --sdl build/macos/deps/sdl --jobs 4
```

Pinned tools: WASI SDK 34, Binaryen 133, SDL 3.4.18, w2c2 revision
`700f556539ea7c7995e6645b9b80e3edf9c9b271`. Archive SHA-256 checks are in the
preparation script. Four compile jobs suit the 16 GB M2 Air; use two if needed.
Allow a few GB of disk space for tools and intermediate files.

The ROM is identified by upstream's canonical SHA-1, copied into ignored
`roms/` if absent, and processed by the existing asset generator. The original
file remains unchanged. Existing different ROMs are never overwritten.
EU/US/JP are recognized; the initial tested build is **US only**. One app is
built per region. See the root README for supported dump hashes.

`build/macos/<region>/` contains the install ZIP, `SHA256SUMS`, native build
metadata, and intermediate code. `app-path.txt` points to the signed staging
app in the temporary directory. Signing there avoids iCloud attaching Finder
metadata that invalidates signatures. The ZIP stores only normal files and
preserves executable modes, without Finder/resource-fork attributes. The app packages upstream fonts, launcher art/icon and badge atlas with notices.
No game ROM, save, or standalone extracted cartridge-asset directory is packaged.
The game binary contains ROM-derived compiled data; keep this personal build private.
Do not distribute local ROM-derived generation outputs or assume the source
code license grants rights to original game content.

## Verification and limitations

See [VALIDATION.md](VALIDATION.md) for the observed results and rendering issues.

Run the checks in fresh isolated directories; never against a player's saves:

```sh
.venv/bin/python port/macos/test_macos.py \
  --rom '/absolute/path/to/your-dump.gba' --region us
```

The native integration test checks keyboard press/release through the actual
guest input handler and clearing on focus loss; SDL virtual controller input/removal; audio stream queue,
frequency ratio and clearing; triangle rendering/readback; normal SRAM
write/read across fresh core instances; and invalid bridge-offset rejection.
Scene tests run title, deck, battle, and event fixtures and retain logs and BMPs
under a new `build/macos/tests/run-*/` directory. Inspect these frames as well
as exit codes: an unrecognized/incorrect harness request can merely run startup.

- Experimental, not a verified complete playthrough. Movies, endings, prolonged
  play, and all bosses/regions need further verification.
- Save states (F5/F9) and boss rematches that require them are unavailable.
- The host has one active gamepad; physical USB/Bluetooth controller models
  still need testing. Virtual controller checks do not prove hardware behavior.
- Dummy audio tests verify the path, not audible quality/latency on real output.
- Replacement HD music/movies and online services remain deferred. The upstream
  launcher and optional Latin HD text are now included. See [ROADMAP.md](ROADMAP.md).
- Fullscreen follows the desktop resolution/refresh rate. Exclusive display-mode
  switching and choosing a particular monitor are not implemented. Moving between
  Retina/external displays and unusual scales still needs physical testing.
- One active gamepad is enough for the single-player campaigns. Simultaneous
  pads, controller selection, automatic button-type labels and haptics are not
  validated. A second pad alone would not provide GBA link multiplayer.
- macOS 15 hardware testing remains required even though both binaries declare
  a 15.0 minimum. Initial direct GUI execution from the restricted build shell
  aborted inside macOS application registration; normal desktop launch was
  subsequently possible. Do not confuse that tooling failure with a proven
  in-game crash fix.

## Licensing

The repository's source license is CC0. Its own README explicitly excludes
Square Enix/Disney/Nintendo game content and trademarks from that grant.
`port/decompui/assets/NOTICE.md` identifies official launcher art and its owners;
these assets are restored for the requested private build, retaining that notice.
This does not grant redistribution rights. `port/assets/fonts/NOTICE.md` describes
the bundled fan fonts as free for project use; their earlier exclusion was not
an established licensing necessity. The macOS bundle includes CC0, SDL's zlib
license, w2c2 and Dear ImGui MIT licenses, and stb's license text. WASI SDK and Binaryen are
build-only tools under their respective upstream licenses. Your dump stays
local and is never uploaded.
