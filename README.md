# Classic Mac (Mac II, colour) on a Canon PowerShot SX130 IS

[Mini vMac](https://www.gryphel.com/c/minivmac/)'s colour **Mac II** emulation (68020, 256 colours, 8MB RAM) running on
the camera as a CHDK module. Same method as the sibling projects
[PS130-doom](https://github.com/Entersjkhdfkjdhfksjf/PS130-doom) and
[PS130-ELKS](https://github.com/Entersjkhdfkjdhfksjf/PS130-ELKS): a small CHDK launcher loads a flat blob
into the idle raw buffer and jumps into it. Canon's firmware is never modified.

**Status: first milestone, NOT yet tested on the camera, and not yet booted with a real ROM** (see below).
What has been verified on a PC: the Mini vMac Mac II core builds for the camera's ARM9 and runs with the camera
platform layer, and the screen scaler/colour path works (Mini vMac's own "Unable to locate ROM image" screen
comes out readable on the 720x240 viewport).

## What you need to supply
I can't ship these:
* **`MacII.ROM`**: a 256KB Mac II (or IIx / SE/30) ROM image.
* **`disk1.dsk`**: a bootable System 7.x disk image (System 7.5.x is the practical ceiling for a Mac II class machine;
  Mac OS 8 needs a 68040 Mac and Mac OS 9 needs a PowerPC, neither is possible here). More disks can be `disk2.dsk` ...

## Install
SD card root: `MAC.BIN` (from `mac/prebuilt/`), `MacII.ROM`, `disk1.dsk`.
`CHDK/MODULES/mac.flt` (from `mac/prebuilt/`).
Needs CHDK 1.6.x for `sx130is` firmware `101f` (tested build: 1.6.1 rev 6315).
Boot the camera in PLAYBACK mode, enter ALT mode, then MENU > Games > MAC.

## Controls
| Control | Action |
|---|---|
| D-pad | Move the pointer (slow at first, speeds up while held) |
| Rear wheel | Move the pointer fast, left/right |
| Zoom lever, wide (out) | Left click (hold = drag) |
| Zoom lever, tele (in) | Right click (a Control-click on a classic Mac) |
| SET | Left click (same as zoom out) |
| Face button | Quit back to CHDK |

No keyboard yet (an on-screen keyboard is next), no sound.

## How the screen works
The emulated Mac II screen is 512x384 (256 colours, or 1-bit black and white when the Mac switches to it). It is
scaled to fit the camera's 720x240 viewport (no scrolling): about 1.4x wider and 0.625x as tall, with an area filter
for brightness and nearest-neighbour colour. The screen is redrawn at most about 16 times a second.

## Known limits / risks
* **Speed is unmeasured.** A Mac II class machine is slow, and System 7.5 is heavy for it.
* **Disk images are opened read/write**, so changes (including System preferences) are saved into the image on the card.
  Keep backups, and quit with the face button rather than pulling the battery.
* The Mac's clock starts at a fixed date (the camera clock isn't hooked up).
* This build uses the Nintendo DS target of the Mini vMac setup tool only to select our platform layer; the DS
  libraries are stubbed (`mac/src/nds.h` etc.).

## Build
Toolchain: [xPack arm-none-eabi-gcc 11.3.1](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/tag/v11.3.1-1.1).
* Blob: `XPACK=/path/to/xpack-arm-none-eabi-gcc-11.3.1-1.1 mac/build-mac.sh` (fetches Mini vMac at a pinned commit from
  [erichelgeson/minivmac](https://github.com/erichelgeson/minivmac), generates the Mac II configuration, builds `mac/MAC.BIN`).
* Launcher: inside a CHDK trunk checkout copy `mac/src/gui_mac.c` and `mac/src/mac_chdk_abi.h` to `modules/games/`, use
  `chdk-tree/makefile_usermodules.inc` as `modules/makefile_usermodules.inc`, then
  `make PLATFORM=sx130is PLATFORMSUB=101f fir` (serial) and take `modules/.o/mac.flt`.
* PC test: `mac/test/run-host.sh` builds the same core + platform layer as a 32-bit PC program (needs `gcc -m32`) and
  writes `mac/test/vp.ppm`, the camera viewport.
* The blob is linked at `0x01F40000` (inside the SX130 IS raw buffer, cached alias); keep `BLOB_ADDR` in `gui_mac.c` and
  `mac/src/mac_blob.ld` in sync.

## License
GPL-2.0. Mini vMac (c) Paul C. Pratt and contributors, GPL-2; `OSGLUCHD.c` is derived from Mini vMac's Nintendo DS platform
layer (c) Philip Cummins, Richard F. Bannister, Paul C. Pratt, also GPL-2. CHDK: see its project.
The Mac ROM and Apple system software are not included and are not covered by this license.
