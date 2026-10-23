# ReKHCoM for PS Vita (alpha)

**Alpha, never run on a console yet.** A VPK for a Vita with HENkaku/Ensō, installed with VitaShell.

## How it works

The Vita's ARM CPU runs the game natively, but user memory starts at `0x81000000`: the GBA's addresses cannot be
mapped. The Switch build (`port/switch`) has the same problem and the Vita reuses its solution:

- the GBA space is relocatable (`PORT_GBA_RELOC`, `include/gba/gba_space.h`): every region is an array of the game,
  and the ROM's pointer words (`port/data/rom_pointers_*.inc`) are moved into them when the ROM is loaded;
- the game and the port layer are compiled by clang for the Cortex-A9 (ARMv7, Thumb-2, hard float) and keep their
  own small libc (`port/ps5/guest`); in the VM each region becomes one object with every symbol local except
  `guest_main` and `gHostTable`, linked with the host;
- the host is `port/switch/host/switch_host.c` built with `__vita__`: the same software rasterizer into a 960x544
  display buffer, sound through a `sceAudioOut` port, controls through `sceCtrl`. The game calls it through the PS5
  bridge (`port/ps5/shared/bridge_list.h`), straight jumps on the Vita (`guest/stubs_arm.c`).

There is one executable per region: `eboot.bin` (Europe), `eboot_us.self` and `eboot_jp.self`. The one that starts
hands over to the one that matches the cartridge.

ggdrn's [KH_COM_VITA](https://github.com/ggdrn/KH_COM_VITA) (MIT) solved the same problem differently (the ROM data
linked into the executable, pointers translated where they are used); its documentation guided this build.

## Install

1. Install `ReKHCoM.vpk` with VitaShell.
2. Copy your own cartridge dump (EU, US or JP, any name ending in `.gba`) to `ux0:data/ReKHCoM/`.
3. Start ReKHCoM. The log is `ux0:data/ReKHCoM/logs/ReKHCoM.log`; saves and settings sit next to the ROM.

Controls: Cross/Circle/Square/Triangle, L/R, D-pad or left stick, Start, Select;
Select + L saves a state, Select + R loads it.

## Build

```
python port/vita/build_vita.py                  # guests (eu us jp) + package: build/vita/ReKHCoM.vpk
python port/vita/build_vita.py guest            # only the guest archives
python port/vita/build_vita.py test --regions eu --env KHCOM_BATTLE=10:4 --env KHCOM_KEYS=60-63:A --shots 400
```

The package is built in the consoles VM (`port/progress/TOOLKIT_CONSOLAS.md`) with VitaSDK in
`/root/consoles/vitasdk`. `test` links the same guest with the host's Linux mode for 32-bit ARM and runs it in the
ARM64 VM (screenshots in `build/vita/test-out/shots.tar`).
