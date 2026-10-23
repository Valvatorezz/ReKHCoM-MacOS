# Validation

## Experimental quick save/load — 23 October 2026

Branch: `quicksaveload`, based on `13e543eef`. Later upstream/main changes
are not included. Build flags: `--experimental-states --release`, US ROM.
Output: `build/macos/states-release/us/`. The normal build and player's
installed app were not replaced. See SAVE_STATES.md for limits and commands.

- Full experimental build and packaging succeeded. Generated-text payload
  audit checked 5,211 samples, with no embedded matches or packaged ROM/save/
  state files. Extracted ZIP signature, CRC, SHA-256 and executable identity
  checks passed (`states-release/us/package-check.json`).
- Title, battle, event/dialogue and deck: real F5/F9 events traverse the native
  bridge. At equivalent later frames, screenshots match baseline files byte
  for byte, both in the same process and after restarting the application.
  All eight comparisons retained identical synthetic cartridge-save files.
- Two successive loads of one state also produce the baseline screenshot.
  Missing, truncated, wrong-build and corrupted-payload files are rejected
  without changing later screenshots or synthetic SRAM files.
- The deterministic menu-animation clock is required for exact image
  comparison: the initial title comparison differed only in 400 pixels of the
  elapsed-time-driven cursor light. With the existing frame-clock test option,
  the images match. No gameplay code change was needed for that comparison.
- Existing integration suite: nine checks pass (including additive blending,
  keyboard/controller bridge input, dummy audio, SRAM and bounds trapping).
  All five 905-frame scenes, including HD dialogue, also pass.
- Example title state: 19,941,891 bytes, written in 1,070 ms on this machine.
  This is one observed sample, not a general performance guarantee.

Evidence: `build/macos/states-build.log`, `states-test.log`,
`states-integration.log`, `tests/states-g6zaeg5a/{report,identity-report}.json`
and `tests/run-u15hmsnp/report.json`. Separate identity tests repair the
metadata checksum after changing the build/ROM identity, to distinguish
compatibility rejection from damaged-header rejection.

Outstanding: manual field and battle use, F6/F7 slot changes, audio after loads,
held keys/physical controllers, real save-point/continue, display changes,
app relocation and longer sessions. The implementation remains opt-in and
experimental. Boss rematches remain disabled. The existing PortTitleLogoBg
linker warning is still present. No live player saves/settings were used.

## Controls localisation — source correction, 23 October 2026

The user's English controls page contained nine Spanish action/navigation
labels. `Lang()` already selects English for the US build; the English column
of `sLabels` held the wrong strings. Corrected those nine entries, shared by
the keyboard and controller pages. Targeted US/macOS wasm32 compilation of
`port_ui.c` passed, and comparison against HEAD confirmed all other language
columns are unchanged. The existing release ZIP predates this correction;
rebuilt-app visual verification remains pending.

## US release-mode build — 23 October 2026

Built locally in the active GitHub checkout with `--release`, without publishing.
The default private build is separate. Evidence:
`build/macos/release-build.log`, `release-test.log`, `release-baseline-test.log`,
and `build/macos/release/us/{release-inputs,release-audit,package-check}.json`.

- Generated text fragments are staged as external declarations and their
  addresses point into the user's loaded cartridge. All generated text bytes
  are checked against the cartridge when preparing the release inputs.
- The complete 4,704-byte Riku deck table matches the original ROM. Its inline
  names are no longer compiled into the game. The actual guest compilation
  asserts the 224-byte Deck size and the name field's offset.
- Executable audit: 5,211 generated text/inline-name samples checked, none found.
  Text samples shorter than 32 bytes are excluded except for inline deck names;
  the scan is not a proof that every short byte sequence is absent.
- ZIP CRC and SHA-256 checks passed. The extracted app's signature verifies;
  its game runs with a separately supplied ROM and rejects a missing ROM.
  No ROM, save or state file is present in the archive.
- All nine integration checks passed, including normal/additive blending and
  mode transitions for textured and untextured triangles, input, synthetic SRAM,
  dummy audio and bridge bounds checks.
- Title, deck, battle, event and HD-dialogue fixtures each completed 905 frames
  without a trap. Evidence: `build/macos/tests/run-65_ypdbn/`. The final packaged
  game executable is byte-identical to the executable used for these tests.
- Comparison with the private build (`run-_c2fazmf`): title, battle, event and
  HD-dialogue framebuffer files are byte-identical. Pixel-text dialogue was
  visually inspected. The deck images differ by 3,296 pixels within the inactive
  decks' CP digits (x=640–763, y=388–607); the baseline contains magenta pixels
  there. On 23 October 2026 the user checked the CP display during normal
  gameplay and reported that it displays correctly. Retain this discrepancy
  as an automated-fixture observation, not a confirmed normal-gameplay bug;
  its cause remains unconfirmed. A full playthrough and normal launcher
  interaction remain pending.
- The linker reports a `PortTitleLogoBg` signature mismatch between credits
  code and the PPU implementation. This existing integration warning is not
  resolved by moving generated text to runtime ROM addresses.

The technical payload check does not change the notices for upstream launcher
artwork, fonts, or decompiled source. The app is experimental and ad-hoc signed,
not Developer ID signed or notarised. No player's live saves/settings were used.

## Earlier private build — 22 October 2026

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
