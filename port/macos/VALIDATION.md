# Validation — 22 October 2026

Experimental local US build from ReKHCoM revision
`42da59f14c0b534fdc66f585670ecb29b552b9f7` with the changes in this folder.
Host: M2 MacBook Air (Mac14,2), macOS 27.0.1. Both the executable and bundled
SDL3 are arm64 Mach-O with a 15.0 minimum deployment target. The user confirmed
macOS 27 is the current play system; older OS testing is a portability check,
not a prerequisite for playing on this Mac.

The user's US dump matched SHA-1
`10729bd884f8fdca7a310b6d606c52e46657aa48`. The original file was not changed.

## Passed

- Signed app bundle verified with `codesign --verify --deep --strict`.
- ZIP extracted into an unrelated temporary directory: signature verified and
  startup completed using only its bundled SDL.
- v0.2 ZIP includes launcher and game executables, bundled SDL, upstream fonts,
  cover/logo/icon and badge atlas, with notices and dependency licenses. No ROMs
  or player saves are bundled. Extracted ZIP signature verification passed.
- Keyboard X press/release through SDL, the offset bridge, and the game's input
  handler. A held key clears on window focus loss.
- SDL virtual controller south-button press/release through the same guest
  input code; virtual controller removal.
- Audio device stream creation, sample queue, frequency ratio and clearing
  using the dummy audio device. Actual desktop run also opened a 48 kHz device;
  audible quality was not assessed.
- Native textured triangle rendering with pixel readback.
- Normal 64 KiB US SRAM write/rename/read, including another fresh core instance.
  These were synthetic SRAM bytes in a new test directory, not a player's save.
- Out-of-range bridge pointers trap instead of becoming native pointers.
- 905-frame isolated title, deck, battle and event fixtures, with zero exit
  codes and no memory/function traps. Frames inspected: Sora fighting a Shadow
  in Traverse Town, Review Deck, and Sora/Donald/Goofy dialogue in Castle Oblivion.
- Normal desktop launch in a separate test app, real keyboard navigation through
  the title, empty save slot, Original choice and confirmation. The cartridge's
  opening movie started and advanced with captions. The test app was closed.

Evidence logs and screenshots remain under ignored `build/macos/tests/`;
`build/macos/test.log` identifies the latest isolated run. The build ZIP's
checksum is `build/macos/us/SHA256SUMS`.

## Failures and limits observed

- The first direct GUI run from the restricted build shell aborted in macOS
  `_RegisterApplication` before SDL initialization completed. The crash report
  identified native ARM64 execution, not Rosetta. Desktop launch subsequently
  worked. This does not prove that all in-game crashes are fixed.
- Signing directly inside the cloud-synchronized workspace failed because Finder
  metadata was reattached to the .app directory. The build now signs in a clean
  temporary staging folder and creates a ZIP without those attributes.
- An installed Homebrew SDL required macOS 27; it was replaced in this bundle
  with SDL 3.4.18 rebuilt for macOS 15. The builder rejects a newer SDL minimum.
- The deck fixture contains a small magenta artifact near the bottom, and some
  title/overlay glyphs appear rough or incomplete. Their cause and whether they
  reproduce upstream have not been established. Original presentation is the
  preset; pixel-perfect equivalence to hardware has not been established.
- Full playthroughs, ending movies, Japanese/EU builds, prolonged play, actual
  macOS 15, physical controllers, audible sound/latency, and save import from a
  real cartridge/emulator remain unverified. Automatic SRAM I/O tests are not a
  substitute for playing to a real save point and continuing that save.
- Quick save states and dependent boss-rematch features are deliberately disabled.

## v0.2 launcher and HD text work (scope paused at user request)

- The actual upstream Dear ImGui launcher builds and opens as the bundle's main
  process. Home, Graphics and Enhancements pages were viewed with keyboard
  navigation. Cover/logo and launcher fonts render.
- An isolated non-GUI launcher check passed ROM verification/installation,
  game executable and font-file discovery, and saving/re-reading hd_text=1.
- Automated mouse-down coordinates matched the intended control, but mouse-up
  and global pointer polling reported another location. Ordinary physical mouse
  interaction remains unverified; do not assume it is either broken or fixed.
- Play uses posix_spawn to start the separate game executable. The full visible
  Play → game → launcher round trip remains unverified.
- Native signal handler installation and crash-record output passed in isolation.
  Real fault delivery and macOS crash-report preservation were not crash-tested.
- Latest build: build/macos/build-v02.log; focused test output:
  build/macos/test-v02.log; package check: build/macos/final-package-check.log.
  Treat these logs as authoritative for HD dialogue/game-scene test results.
- User requested scope reduction to conserve usage. Outstanding work is recorded
  in PENDING.md; this package remains experimental, not a fully validated upgrade.

Final focused run: build/macos/tests/run-0etfal6t. All eight integration checks
and all five 905-frame scenes (title, deck, battle, event, HD dialogue) passed.
HD dialogue loaded the bundled fonts and exited without a trap. Visual comparison
of HD/pixel text remains pending. Relocated ZIP signature verification and the
no-ROM/save package check passed. No player's save directory was used.
