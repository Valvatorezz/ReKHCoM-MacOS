# macOS priorities and deferred features

User decisions recorded 22 October 2026. These are project notes, not scheduled
reminders or permission to publish anything. The app is for private use.

## Current priorities

- Open the upstream launcher first, with a working Play action, settings,
  controls, ROM verification/installation and save tools.
- Restore the upstream launcher cover/logo/icon and achievement badge artwork,
  retaining notices. Keep ROM files out of packages.
- Restore optional HD text using upstream's bundled KHMenu and ComicHearts
  fonts. Preserve pixel text as an option. Latin-font support does not imply
  Japanese HD font support.
- Improve macOS diagnostics while preserving Apple's crash reporting. Native
  crash logging and guest-memory recovery are separate concerns.

## Deferred, retained for later

1. Replacement HD music: add a native decoder/mixer to the existing bridge,
   then connect optional user-imported tracks. No priority for this update.
2. Portable quick save/load states: snapshot linear memory, function-table /
   runtime state and platform state at a known safe frame boundary; validate
   version/build compatibility and resynchronize audio/input on restore.
3. Boss rematches: restore after portable states are reliable.
4. RetroAchievements: preserve shared achievement code/data. Account integration,
   official runtime evaluation and submission need their own validation; the
   official upstream set targets the European cartridge.
5. Optional replacement HD movies/scenes remain outside the current work.

Discord presence is not requested and is out of scope.

## Compatibility decisions

One active gamepad is sufficient for the campaign. Selecting between several
connected pads, automatic controller-type labels and haptics are later polish.
A second pad alone would not implement GBA link battles.

Fullscreen currently follows the display's desktop mode. The launcher must not
claim exclusive resolution/refresh switching works while the game bridge lacks
it. Retain resizing, fullscreen, VSync and original/wide aspect options.

Keep other-region support in the builder without packaging unverified games or
ROMs. EU is useful for its five languages and future official achievement work;
JP for Japanese text/voices and optional undub sourcing. The US cartridge is
sufficient for the user's current English campaign.
