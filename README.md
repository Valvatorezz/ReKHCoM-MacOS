# ReKHCoM-MacOS

This is an AI-assisted macOS port of [ReKHCoM](https://github.com/MatyMeatBoy/ReKHCoM), the native PC port of Kingdom Hearts: Chain of Memories. It adapts the upstream project to run on **Apple Silicon Macs (M-series), with macOS 15 or later**. This is an experimental port; some Windows features are still missing or need further testing. Intel Macs are not currently supported.

The supported and tested cartridge region is currently **US English**. You must supply your own cartridge dump; no ROM is included. The build scripts recognise other regions, but those builds have not been validated, and release packaging currently supports US only.

## Building and testing

See the [macOS build and usage guide](port/macos/README.md) for dependencies, build commands, controls and packaging. Use its `--release` build mode when preparing an app for users who supply their own ROM; the default private local build embeds generated game text.

The [validation notes](port/macos/VALIDATION.md) describe completed checks and their limits. The [pending list](port/macos/PENDING.md) tracks development priorities and remaining manual tests.

## Windows features still missing or incomplete on macOS

This comparison describes the current checkout. Shared upstream code may be present even when its macOS integration is disabled or incomplete.

| Windows feature | Current macOS status |
| --- | --- |
| HD asset import and replacement graphics | The launcher import workflow is unavailable. Shared HD sprite and layout rendering code is retained, but importing and using the complete set of replacement cards, HUD, menus and world artwork is not supported or validated end to end. Full HD support is deferred. |
| Replacement HD music | Playback is not implemented in the macOS host. It still needs decoding, mixing and asset import. Original cartridge audio is supported. |
| Replacement HD movies and story scenes, including imported subtitles | Playback and import are not implemented on macOS. Original cartridge movies remain supported. |
| Quick save/load states | Disabled in standard builds. An opt-in `--experimental-states` prototype provides F5 save, F9 load and F6/F7 slot selection. Automated checks pass, but manual gameplay testing remains pending. States require the same build, ROM and data folder and are not compatible with Windows `.khst` files. See the [save-state guide](port/macos/SAVE_STATES.md). |
| Boss rematches | Disabled, including in experimental state builds. They require further state-system integration and separate validation. |
| Offline achievements | Shared achievement code and data are retained, but achievement evaluation and the launcher integration are not enabled in the macOS build. |
| RetroAchievements account integration | Online login, achievement evaluation and submission are not integrated on macOS. |
| Discord rich presence | Not implemented on macOS and not currently planned. |
| Fullscreen resolution and refresh-rate selection | Uses the current desktop fullscreen mode. The Windows display-mode selection workflow is not implemented on macOS. |
| Input from multiple connected controllers | The macOS host reads one active controller; Windows can combine input from multiple connected controllers. This is an input difference, not a multiplayer mode. |
| Launcher-created desktop shortcut | The macOS adaptation reveals the app in Finder instead of creating a desktop shortcut. |
| EU and Japanese regional support | Build-script recognition exists, but regional builds and their language-specific behaviour remain unvalidated. Release packaging is US only. |
| Japanese HD text | Disabled on macOS because the bundled Latin fonts do not cover Japanese. Font coverage and rendering still need implementation and testing. |

Some other features need testing rather than a new implementation: physical controller input, audible audio and latency, Retina and external-display behaviour, launcher ROM selection and save-management workflows, real save-point/continue behaviour, and longer play sessions. Japanese voice replacement code is retained, but has not been validated with the required user-supplied Japanese ROM. See the pending list before treating these as verified features.

## Credits

- Original project and PC port: [MatyMeatBoy/ReKHCoM](https://github.com/MatyMeatBoy/ReKHCoM) and contributors (CC0).
- Decompilation: [Pheenoh/khcom](https://github.com/Pheenoh/khcom) and contributors (CC0).
- Third-party asset credits: [NOTICE.md](port/assets/fonts/NOTICE.md).

The original decompilation README is kept in [docs/README.decomp.md](docs/README.decomp.md).

This is an independent fan project, unaffiliated with Square Enix, Disney or
Nintendo. Their game content and trademarks are not covered by the source
code's [CC0 license](LICENSE.md). SDL3 and other bundled dependencies retain
their own licences and notices; see the macOS guide for packaging details.
