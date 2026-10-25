#!/bin/bash
# Run the real ARM blob under QEMU's ARM946E-S (the camera's CPU core) with stub callbacks.
# Needs: qemu-system-arm, the xPack toolchain (XPACK=...), and ./build-mac.sh run once (for build/obj).
set -e
HERE="$(cd "$(dirname "$0")/../.." && pwd)"
if [ -n "$XPACK" ]; then export PATH="${XPACK%/}/bin:$PATH"; fi
TC="$(dirname "$(command -v arm-none-eabi-gcc)")/.."
cd "$HERE"
SHIFT="${ICOUNT:-3}"   # virtual ns per ARM instruction = 2^SHIFT (3 = 125 MIPS, 4 = 62 MIPS)
CF="-c -march=armv5te -mtune=arm946e-s -O2 -w -DICOUNT_SHIFT=$SHIFT"
arm-none-eabi-gcc $CF -o build/obj/q_start.o test/qemu/start.S
arm-none-eabi-gcc $CF -o build/obj/q_qmain.o test/qemu/qmain.c
LIBC="$TC/arm-none-eabi/lib/thumb/nofp/libc.a"
LIBGCC="$(ls "$TC"/lib/gcc/arm-none-eabi/*/thumb/nofp/libgcc.a | head -1)"
arm-none-eabi-gcc -nostartfiles -nostdlib -Wl,-T,src/mac_blob.ld -Wl,-e,_start -o build/q.elf \
  -Wl,--start-group build/obj/*.o "$LIBC" "$LIBGCC" -Wl,--end-group
[ -f test/MacII.ROM ] || { echo "put a Mac II ROM at mac/test/MacII.ROM first"; exit 1; }
timeout "${1:-60}" qemu-system-arm -M integratorcp -cpu arm946 -m 128 -nographic -icount shift=$SHIFT,sleep=off -kernel build/q.elf \
  -serial mon:stdio 2>&1 | grep --line-buffered -E "^(qemu|\[log\]|QEMU|\*\*\*)" || true
