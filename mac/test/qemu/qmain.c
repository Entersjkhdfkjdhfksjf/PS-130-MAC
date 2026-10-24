// QEMU (ARM946E-S) harness: runs the Mac blob with stub callbacks and a UART log. No ROM, no files.
#include "../../src/mac_chdk_abi.h"
#define UART (*(volatile unsigned *)0x16000000)
#define CLOCK (*(volatile unsigned *)0x05000000)   /* fake ms counter, outside the blob's BSS */
static void puts_(const char *s) { while (*s) { UART = (unsigned char)*s++; } }
static void putx(unsigned v) { int i; puts_("0x"); for (i = 28; i >= 0; i -= 4) { UART = "0123456789abcdef"[(v >> i) & 15]; } }
static unsigned t_ms(void) { CLOCK += 1; return CLOCK / 4; }   /* time advances with every query */
static void t_sleep(unsigned ms) { CLOCK += ms * 4; }
static int  f_open_(const char *p, int m) { puts_("[open "); puts_(p); puts_("] -> -1\n"); (void)m; return -1; }
static int  f_rw(int fd, void *b, int n) { (void)fd; (void)b; (void)n; return -1; }
static int  f_seek(int fd, int o, int w) { (void)fd; (void)o; (void)w; return -1; }
static int  f_close_(int fd) { (void)fd; return 0; }
static void lg(const char *s) { puts_("[log] "); puts_(s); puts_("\n"); }
static int  quit_(void) { return CLOCK > 4 * 3000; }   /* ~3 s of fake time */
static unsigned btn(void) { return 0; }
static int jog(void) { return 0; }
static unsigned char vpbuf[1080 * 240];
static unsigned char *vp(void) { return vpbuf; }
extern unsigned _blob_end;
int mac_main(mac_cb_t *cb);
void qmain(void) {
    mac_cb_t cb;
    unsigned *hdr = (unsigned *)0x01F40000;
    unsigned end = (unsigned)&_blob_end;
    char *p = (char *)&cb; int i; for (i = 0; i < (int)sizeof cb; i++) p[i] = 0;
    CLOCK = 0;
    cb.magic = MAC_ABI_MAGIC; cb.get_ticks_ms = t_ms; cb.sleep_ms = t_sleep; cb.log = lg; cb.should_quit = quit_;
    cb.f_open = f_open_; cb.f_read = f_rw; cb.f_write = (int (*)(int, const void *, int))f_rw; cb.f_lseek = f_seek; cb.f_close = f_close_;
    cb.get_buttons = btn; cb.get_jog = jog; cb.get_viewport = vp;
    cb.arena = (char *)end; cb.arena_size = 0x03105260 - end;
    puts_("qemu: blob_end="); putx(end); puts_(" arena="); putx(cb.arena_size); puts_("\n");
    (void)hdr;
    int r = mac_main(&cb);
    puts_("qemu: mac_main returned "); putx((unsigned)r); puts_("\n");
    puts_("QEMU-DONE\n");
}
