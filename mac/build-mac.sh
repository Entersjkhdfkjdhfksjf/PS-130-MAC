#!/bin/bash
# Build MAC.BIN: Mini vMac (Mac II, 256 colours) as a blob for the Canon PowerShot SX130 IS (fw 101f).
# Needs: arm-none-eabi-gcc (xPack 11.3.1: set XPACK=...), gcc, git, bash.
# Mini vMac is fetched at a pinned commit; our platform layer (src/OSGLUCHD.c) replaces the
# Nintendo DS one, which the setup tool's "-t ndsa" target selects.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -n "$XPACK" ]; then export PATH="${XPACK%/}/bin:$PATH"; fi
command -v arm-none-eabi-gcc >/dev/null || { echo "arm-none-eabi-gcc not found (set XPACK= or add to PATH)"; exit 1; }
TC="$(dirname "$(command -v arm-none-eabi-gcc)")/.."
OUT="$HERE/build"; mkdir -p "$OUT"
MVM="$OUT/minivmac"
PIN=f403ffba150ace0b0ba4d713b65f5e6625b88252
if [ ! -d "$MVM/.git" ]; then git clone -q https://github.com/erichelgeson/minivmac.git "$MVM"; fi
(cd "$MVM" && git checkout -q "$PIN" 2>/dev/null || true)

# 1. generate the configuration (Mac II, 512x384, 256 colours, 8MB, no sound/FPU tweaks)
gcc -O1 -w -o "$OUT/mvsetup" "$MVM/setup/tool.c"
( cd "$MVM" && "$OUT/mvsetup" -t ndsa -m II -hres 512 -vres 384 -depth 3 -mf 2 -magnify 0 \
    -em-cpu 2 -mem 8M -sound 0 -n mvchd > "$OUT/gen.sh" && bash "$OUT/gen.sh" > "$OUT/gen.log" 2>&1 )

# 2. our platform layer replaces the DS one
cp "$HERE/src/OSGLUCHD.c" "$MVM/src/OSGLUCHD.c"

CF="-c -march=armv5te -mtune=arm946e-s -O2 -ffunction-sections -w -I$MVM/cfg -I$MVM/src -I$HERE/src"
OBJ="$OUT/obj"; rm -rf "$OBJ"; mkdir -p "$OBJ"
for f in OSGLUCHD GLOBGLUE M68KITAB MINEM68K VIAEMDEV VIA2EMDV IWMEMDEV SCCEMDEV RTCEMDEV ROMEMDEV \
         SCSIEMDV SONYEMDV SCRNEMDV VIDEMDEV MOUSEMDV ADBEMDEV ASCEMDEV PROGMAIN; do
  arm-none-eabi-gcc $CF -o "$OBJ/$f.o" "$MVM/src/$f.c"
done
arm-none-eabi-gcc $CF -o "$OBJ/mac_syscalls.o" "$HERE/src/mac_syscalls.c"

LIBC="$TC/arm-none-eabi/lib/arm/v5te/softfp/libc.a"
LIBGCC="$(ls "$TC"/lib/gcc/arm-none-eabi/*/arm/v5te/softfp/libgcc.a | head -1)"
arm-none-eabi-gcc -nostartfiles -nostdlib -Wl,-T,"$HERE/src/mac_blob.ld" -Wl,-Map,"$OUT/mac_blob.map" \
  -o "$OUT/mac_blob.elf" -Wl,--start-group "$OBJ"/*.o "$LIBC" "$LIBGCC" -Wl,--end-group
arm-none-eabi-objcopy -O binary "$OUT/mac_blob.elf" "$HERE/MAC.BIN"
echo "built MAC.BIN:"; ls -la "$HERE/MAC.BIN"
arm-none-eabi-nm "$OUT/mac_blob.elf" | grep -iE " mac_main$|__bss_start|__bss_end|_blob_end"
