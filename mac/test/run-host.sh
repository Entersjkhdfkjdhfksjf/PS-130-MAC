#!/bin/bash
# Build + run the Mac core natively with the camera platform layer (needs ./build-mac.sh run once for the sources).
set -e
cd "$(dirname "$0")/.."
MVM=build/minivmac
SRCS=""
for f in OSGLUCHD GLOBGLUE M68KITAB MINEM68K VIAEMDEV VIA2EMDV IWMEMDEV SCCEMDEV RTCEMDEV ROMEMDEV SCSIEMDV SONYEMDV SCRNEMDV VIDEMDEV MOUSEMDV ADBEMDEV ASCEMDEV PROGMAIN; do SRCS="$SRCS $MVM/src/$f.c"; done
cp src/OSGLUCHD.c $MVM/src/OSGLUCHD.c
gcc -m32 -O2 -w -DMAC_HOST_TEST -Isrc -I$MVM/cfg -I$MVM/src -o test/host_mac test/test_host_mac.c $SRCS -lm
cd test && ./host_mac "${@:-4000}"
