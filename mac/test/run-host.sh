#!/bin/bash
# Build + run the Mac core natively with the camera platform layer (needs ./build-mac.sh run once for the sources).
set -e
cd "$(dirname "$0")/.."
MVM=build/minivmac
SRCS="$MVM/src/OSGLUCHD.c"
for f in $(sed -n 's#^bld/\([A-Za-z0-9_]*\)\.o:.*#\1#p' $MVM/Makefile | grep -v '^OSGLUNDS$'); do SRCS="$SRCS $MVM/src/$f.c"; done
cp src/OSGLUCHD.c $MVM/src/OSGLUCHD.c
gcc -m32 -O2 -w -DMAC_HOST_TEST -Isrc -I$MVM/cfg -I$MVM/src -o test/host_mac test/test_host_mac.c $SRCS -lm
cd test && ./host_mac "${@:-4000}"
