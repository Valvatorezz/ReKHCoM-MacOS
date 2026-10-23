# Resume after usage refresh

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
6. Resolve or characterize existing rough glyphs/magenta deck artifact, test a
   physical controller and audible audio, and test display changes/Retina scaling.
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
- Portable quick save/load states; boss rematches after states are reliable.
- RetroAchievements: retain shared achievement code/data; add runtime/account
  integration later. Discord is not requested.
- Other regions only if wanted: EU languages/future achievement compatibility;
  JP Japanese presentation. The current US English game needs neither.

## Resume pointers

Local source cleanup: PS4, PS5, Vita and Web platform folders and dedicated tools
were removed at the user's request. Required PS5-derived macOS dependencies are
preserved under port/macos; the IR helper is now port/macos/wasm_ir.py. Rebuild
and runtime validation of this source reorganisation remain pending. Existing
packaged builds and the player's working app were not replaced.

Repository: /Users/randellevergara/ChatGPT/Games/ReKHCoM
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
