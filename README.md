# Classic Mac (Mac Plus, System 6) on a Canon PowerShot SX130 IS

[Mini vMac](https://www.gryphel.com/c/minivmac/) running on the camera as a CHDK module. Default model: the original-style
**Mac Plus** (68000, 512x342 black and white, 4MB), which is what System 6 was made for. A colour **Mac II** build
(256 colours, 8MB, System 7) is also included, but it ran at roughly a quarter of real speed on the camera, so Plus is the default.

Same method as the sibling projects [PS130-doom](https://github.com/Entersjkhdfkjdhfksjf/PS130-doom) and
[PS130-ELKS](https://github.com/Entersjkhdfkjdhfksjf/PS130-ELKS): a small CHDK launcher loads a flat blob into the
idle raw buffer and jumps into it. Canon's firmware is never modified.

**Status: the Mac II build has run on the camera (it reached Mini vMac's own message screen). The Plus build has not been
tried on the camera yet, nor booted with a Plus ROM (none available to test with).**

## What you need to supply
I can't ship these:
* **`vMac.ROM`**: a 128KB **Mac Plus** ROM (version 3, checksum `4D1F8172`). Other models' ROMs don't work with this build
  (a Mac II ROM is for the Mac II build, which wants it named `MacII.ROM`).
* **`disk1.dsk`**: a bootable System 6 disk image (800KB floppy, raw sector image; System 6.0.8 is the classic choice).
  More disks can be `disk2.dsk` ... (up to six).

## Install
SD card root: `MAC.BIN` (from `mac/prebuilt/`), `vMac.ROM`, `disk1.dsk`.
`CHDK/MODULES/mac.flt` (from `mac/prebuilt/`).
For the colour Mac II instead: use `mac/prebuilt/MAC-II.BIN` renamed to `MAC.BIN`, plus `MacII.ROM`.
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
| DISP | Press the `C` key (dismisses Mini vMac's message boxes) |
| Shutter (full press) | Press Return |
| Face button | Quit back to CHDK |

No full keyboard yet (an on-screen keyboard is next), no sound.

## How the screen works
The emulated screen (512x342 black and white on the Plus) is scaled to fit the camera's 720x240 viewport (no scrolling):
about 1.4x wider and 0.7x as tall, with an area filter for brightness. It is redrawn at most about 16 times a second.

## Disk images
They are opened read/write and the Mac saves into them, so **keep a backup copy of every image**. Never pull the battery
while the Mac is running; quit with the face button. (Builds before commit be1753b had a bug that wiped any disk image
to 0 bytes and created empty `disk2.dsk`...`disk6.dsk`; if you ran one, restore your image from your original and delete those empty files.)

## If the camera hangs or turns off
The blob appends progress lines to **`MAC.LOG`** (including any message Mini vMac itself shows) in the card root (after each init step, then an "alive" line every
5 seconds). After a crash, read that file: the last line shows how far it got and how long it ran.
Also set CHDK's "Disable LCD off" option (CHDK menu > Miscellaneous; the exact label varies) to *Always* so Canon's
auto power-down and display-off don't fire while the emulator is running with no real key presses.
The emulator hands about 20% of the CPU back to the camera's own tasks every 40 ms for the same reason.

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
* **No FPU:** the camera's ARM946E-S has no floating-point unit, and the toolchain's ARM-mode libraries are built with VFP
  instructions (e.g. `sprintf` starts with `vpush`, an undefined instruction here). `build-mac.sh` therefore links the Thumb
  `nofp` libraries, and the finished blob has zero VFP instructions.
* ARM test: `mac/test/qemu/run-qemu.sh` runs the real ARM blob under QEMU's ARM946E-S (the camera's core) with stub callbacks. It caught a
  hang that the PC test could not: GCC turned `calloc` (malloc + zero loop) into a call to itself, an endless loop on the camera.
* PC test: `mac/test/run-host.sh` builds the same core + platform layer as a 32-bit PC program (needs `gcc -m32`) and
  writes `mac/test/vp.ppm`, the camera viewport.
* The blob is linked at `0x01F40000` (inside the SX130 IS raw buffer, cached alias); keep `BLOB_ADDR` in `gui_mac.c` and
  `mac/src/mac_blob.ld` in sync.

## License
GPL-2.0. Mini vMac (c) Paul C. Pratt and contributors, GPL-2; `OSGLUCHD.c` is derived from Mini vMac's Nintendo DS platform
layer (c) Philip Cummins, Richard F. Bannister, Paul C. Pratt, also GPL-2. CHDK: see its project.
The Mac ROM and Apple system software are not included and are not covered by this license.
