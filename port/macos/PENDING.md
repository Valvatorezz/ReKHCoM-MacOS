# Current macOS development status

Save-state prototype: active work on `quicksaveload`, based on `macos-port`
at `13e543eef` (the later upstream merge in main is not included). The opt-in
`--experimental-states` build uses separate `states[-release]/<region>` output.
See SAVE_STATES.md for architecture, controls and isolated test commands.
Normal builds retain disabled quick states; boss rematches remain disabled.
This is experimental work, not permission to commit, push or replace the
player's working app. Manual field/battle, audio/input, save-point/continue and
longer-session checks remain pending after the automated regressions.

Active checkout: `/Users/randellevergara/GitHub/ReKHCoM-MacOS`. The older
`/Users/randellevergara/ChatGPT/Games/ReKHCoM` folder is reference material only.
The user requested a prebuilt app where users supply their own ROM on 23 October
2026. The new `--release` build mode stages generated text as runtime ROM
references; see README.md and VALIDATION.md. Build outputs are separate from
the default private local build, under `build/macos/release/us/`.
The additive-blending fix is applied locally. Do not assume changes are
committed or pushed; inspect Git state. This work does not authorise publishing.

Controls localisation: the user reported Spanish labels on the English controls
page on 23 October 2026. Nine English-column strings in `port/src/port_ui.c`
were Spanish. These are corrected in source for both keyboard and controller
pages; targeted US/macOS compilation passed. The existing release ZIP has not
been rebuilt with this label correction. Rebuild and verify the controls page.

## Earlier pause (historical)

Stopped at the user's explicit request on 22 October 2026 to conserve remaining
usage until 11:50 PM Asia/Manila. Do not schedule or start work automatically.

## Finish the current update first

1. Verify normal physical mouse interaction in the upstream launcher. Automated
   CUA clicks yielded correct mouse-down positions but displaced mouse-up/global
   pointer readings. Keyboard navigation worked: Escape exits a child panel;
   arrows select tabs and Return activates. Distinguish tooling from an app bug.
2. Verify Play visibly starts khcom_us, hides the launcher, and restores it on
   game exit. Test child failure and reset behavior without touching player saves.
3. Toggle HD text through Enhancements → Experimental, launch a dialogue scene,
   compare against pixel text and inspect scaling/clipping. Font/settings checks
   already pass. The final HD-dialogue fixture also loaded fonts and completed
   905 frames without a trap; visual comparison and GUI-toggle testing remain.
4. Exercise ROM file picker/drag-drop and save import, backup, export and editor
   on disposable synthetic saves. The native same-directory atomic replacement
   is implemented, but the complete UI paths need validation.
5. Validate native crash-signal delivery and Apple's crash report preservation
   using an isolated diagnostic fixture. Installation/record-writing pass.
6. Characterise the rough glyphs and automated deck-fixture discrepancy. On
   23 October 2026 the user reported that the deck CP numbers display correctly
   during normal gameplay; the fixture observation is not a confirmed gameplay
   bug. Test a physical controller and audible audio, and test display
   changes/Retina scaling.
7. Test a real save point/continue and longer play. Test a second Apple Silicon
   Mac (minimum target macOS 15); Intel is not built. Refresh validation docs.
8. Review staged launcher replacements, remove stale Windows tooltips and ensure
   deferred controls clearly describe limitations. Current upstream source is
   untouched; adaptations live in port/macos/build_launcher.py.

## User-reported Jafar battle flicker — reproduction pending

- Jumping high toward Iago (the bird) and the lamp during the Jafar fight causes
  black flickering, with assets appearing to struggle to load. Zoom is the user's
  suspected cause, not a confirmed diagnosis.
- Prioritize reproduction alongside the existing rendering artifacts, after
  launcher interaction/Play-return, HD text and broader gameplay validation.
  Compare Original 240×160 versus Auto view and battle zoom settings using
  isolated copies of saves/settings. Do not modify the player's live data.

## Deferred by the user

- Replacement HD music and optional HD movies: decoder/mixer and user imports.
- Wider save-state portability and boss rematches after states are reliable;
  experimental quick save/load is now active work on `quicksaveload` above.
- RetroAchievements: retain shared achievement code/data; add runtime/account
  integration later. Discord is not requested.
- Other regions only if wanted: EU languages/future achievement compatibility;
  JP Japanese presentation. The current US English game needs neither.

## Resume pointers

Other platform sources: the 176 removed PS4, PS5, Vita and Web files and dedicated
tools have been restored from the macOS port's upstream base, revision
42da59f14c0b534fdc66f585670ecb29b552b9f7. Keep these upstream files in place to
simplify future merges. Required PS5-derived macOS dependencies remain under
port/macos; the IR helper remains port/macos/wasm_ir.py. Rebuild and runtime
validation of those reorganised macOS dependencies remain pending; restoring
the other platforms does not complete that validation. Existing packaged builds
and the player's working app were not replaced. Newer upstream updates have
not been merged as part of this restoration.

Repository: /Users/randellevergara/GitHub/ReKHCoM-MacOS
ROM: /Users/randellevergara/Games/Kingdom Hearts - Chain of Memories (USA).gba
Relocation: generated build files and the Python virtual environment may retain
old absolute paths. Inspect and regenerate affected files/environment as needed
before the next build; relocation alone does not validate existing outputs.
Latest signed app: read build/macos/us/app-path.txt (temporary staging location).
Archive: build/macos/us/ReKHCoM-macos-arm64-us.zip and SHA256SUMS.
Build/test instructions: port/macos/README.md.
Logs: build/macos/build-v02.log, test-v02.log, final-package-check.log.
Scene evidence: newest build/macos/tests/run-*/report.json and frames.
Isolated GUI test app: build/macos/launcher-ui-app.txt, currently closed. This is
an older instrumented copy, not the latest final archive. Do not change or close
any original game instance the user may be playing.

Do not replace the player's working app or modify live saves/settings without
asking. Use isolated test copies for validation.

No ROM is bundled. Restored upstream artwork has its original ownership notice;
source CC0 does not grant rights to that artwork or ROM-derived game content.
Fonts carry upstream's free-project-use notice. Keep this personal build private.
