// PC test harness for the Mac blob: same OSGLUCHD.c + Mini vMac core, stdio-backed callbacks.
// Dumps the 720x240 viewport (converted from UYVYYY) as PPM.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "../src/mac_chdk_abi.h"
extern mac_cb_t *g_cb;
int mac_main(mac_cb_t *cb);
static unsigned int limit_ms = 4000, t0;
static unsigned int now_ms(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec*1000u + ts.tv_nsec/1000000u; }
static unsigned int c_ticks(void) { return now_ms() - t0; }
static unsigned char vp[1080*240];
static void c_log(const char *s) { fprintf(stderr, "[log] %s\n", s); }
static int c_quit(void) { return c_ticks() > limit_ms; }
static unsigned int c_btn(void) { return 0; }
static int c_jog(void) { return 0; }
static void c_sleep(unsigned ms) { (void)ms; }
static unsigned char *c_vp(void) { return vp; }
static int c_open(const char *p, int m) { return open(p, m == 0 ? O_RDONLY : m == 1 ? (O_WRONLY|O_CREAT|O_TRUNC) : O_RDWR, 0666); }
static int c_read(int fd, void *b, int n) { return read(fd, b, n); }
static int c_write(int fd, const void *b, int n) { return write(fd, b, n); }
static int c_seek(int fd, int o, int w) { return lseek(fd, o, w); }
static int c_close(int fd) { return close(fd); }
static void dump(const char *fn) {
    FILE *f = fopen(fn, "wb"); fprintf(f, "P6\n720 240\n255\n");
    for (int y = 0; y < 240; y++) for (int g = 0; g < 180; g++) {
        unsigned char *d = vp + y*1080 + g*6;
        int U = (signed char)d[0], V = (signed char)d[2]; int Y[4] = { d[1], d[3], d[4], d[5] };
        for (int k = 0; k < 4; k++) {
            int r = Y[k] + (V*1402)/1000, b = Y[k] + (U*1772)/1000, gg = Y[k] - (U*344 + V*714)/1000;
            unsigned char px[3] = { r<0?0:r>255?255:r, gg<0?0:gg>255?255:gg, b<0?0:b>255?255:b };
            fwrite(px, 1, 3, f);
        }
    }
    fclose(f);
}
int main(int argc, char **argv) {
    static mac_cb_t cb;
    if (argc > 1) limit_ms = atoi(argv[1]);
    cb.magic = MAC_ABI_MAGIC; cb.get_ticks_ms = c_ticks; cb.sleep_ms = c_sleep; cb.log = c_log; cb.should_quit = c_quit;
    cb.f_open = c_open; cb.f_read = c_read; cb.f_write = c_write; cb.f_lseek = c_seek; cb.f_close = c_close;
    cb.get_buttons = c_btn; cb.get_jog = c_jog; cb.get_viewport = c_vp;
    g_cb = &cb; t0 = now_ms();
    int r = mac_main(&cb);
    fprintf(stderr, "mac_main -> %d\n", r);
    dump("vp.ppm");
    return 0;
}
