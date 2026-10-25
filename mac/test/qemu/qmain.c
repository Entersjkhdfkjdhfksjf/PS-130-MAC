// QEMU (ARM946E-S) harness: runs the Mac blob with stub callbacks and a UART log. No ROM, no files.
#include "../../src/mac_chdk_abi.h"
#define UART (*(volatile unsigned *)0x16000000)
#define CLOCK (*(volatile unsigned *)0x05000000)   /* fake ms counter, outside the blob's BSS */
static void puts_(const char *s) { while (*s) { UART = (unsigned char)*s++; } }
static void putx(unsigned v) { int i; puts_("0x"); for (i = 28; i >= 0; i -= 4) { UART = "0123456789abcdef"[(v >> i) & 15]; } }
#define TMR_LOAD (*(volatile unsigned *)0x13000000)
#define TMR_VAL  (*(volatile unsigned *)0x13000004)
#define TMR_CTRL (*(volatile unsigned *)0x13000008)
#ifndef ICOUNT_SHIFT
#define ICOUNT_SHIFT 3
#endif
static unsigned ticks_per_ms = 1000;   /* calibrated in qmain */
static unsigned t_ms(void) { return (0xFFFFFFFFu - TMR_VAL) / ticks_per_ms; }
static void t_sleep(unsigned ms) { unsigned t0 = t_ms(); while (t_ms() - t0 < ms) { } }
extern const unsigned char rom_start[], rom_end[];
static unsigned rom_pos;
static int  f_open_(const char *p, int m) {
    const char *r = "MacII.ROM"; int i = 0;
    puts_("qemu: open "); puts_(p); puts_(" mode "); UART = (unsigned char)('0' + m); puts_("\n");
    if (p[0] == 'T' && p[1] == '-') return -1;
    while (r[i] && p[i] == r[i]) i++;
    if (!r[i] && !p[i]) { rom_pos = 0; return 7; }
    return -1;
}
static int  f_rw(int fd, void *b, int n) {
    unsigned char *d = b; unsigned len = (unsigned)(rom_end - rom_start); int k = 0;
    if (fd != 7) return -1;
    while (k < n && rom_pos < len) d[k++] = rom_start[rom_pos++];
    return k;
}
static int  f_seek(int fd, int o, int w) {
    unsigned len = (unsigned)(rom_end - rom_start);
    if (fd != 7) return -1;
    if (w == 0) rom_pos = (unsigned)o; else if (w == 2) rom_pos = len + o; else rom_pos += o;
    return (int)rom_pos;
}
static int  f_close_(int fd) { (void)fd; return 0; }
static void lg(const char *s) { puts_("[log] "); puts_(s); puts_("\n"); }
static unsigned quit_ms = 12000;
static int  quit_(void) { return t_ms() > quit_ms; }
static unsigned btn(void) { return 0; }
static int jog(void) { return 0; }
static unsigned char vpbuf[1080 * 240];
static unsigned char *vp(void) { return vpbuf; }
extern unsigned _blob_end;
extern void und_h(void), pabt_h(void), dabt_h(void);
void report_exc(unsigned pc, unsigned insn_unused, int kind) {
    static const char *n[] = { "?", "UNDEFINED INSTRUCTION", "PREFETCH ABORT", "DATA ABORT" };
    (void)insn_unused;
    puts_("\n*** "); puts_(n[kind]); puts_(" at pc="); putx(pc);
    if (kind != 2) { puts_(" insn="); putx(*(volatile unsigned *)pc); }
    puts_("\n");
}
static void install_vectors(void) {
    volatile unsigned *v = (volatile unsigned *)0x0;
    v[1] = 0xE59FF018; v[3] = 0xE59FF018; v[4] = 0xE59FF018;   /* ldr pc,[pc,#0x18] */
    v[9] = (unsigned)und_h; v[11] = (unsigned)pabt_h; v[12] = (unsigned)dabt_h;
}
int mac_main(mac_cb_t *cb);
void qmain(void) {
    mac_cb_t cb;
    unsigned *hdr = (unsigned *)0x01F40000;
    unsigned end = (unsigned)&_blob_end;
    char *p = (char *)&cb; int i; for (i = 0; i < (int)sizeof cb; i++) p[i] = 0;
    install_vectors();
    /* free-running 32-bit timer; calibrate it against a known number of instructions */
    TMR_LOAD = 0xFFFFFFFFu; TMR_CTRL = 0xC2;
    {
        unsigned a = TMR_VAL, n = 4000000;
        __asm__ volatile ("1: subs %0, %0, #1\n bne 1b" : "+r"(n));
        unsigned d = a - TMR_VAL;   /* timer ticks for ~8M instructions */
        puts_("qemu: timer ticks for 8M instr = "); putx(d); puts_("\n");
        ticks_per_ms = d / (8u << ICOUNT_SHIFT);   /* 8M instr = 8<<shift virtual ms */
        if (!ticks_per_ms) ticks_per_ms = 1;
    }
    cb.magic = MAC_ABI_MAGIC; cb.get_ticks_ms = t_ms; cb.sleep_ms = t_sleep; cb.log = lg; cb.should_quit = quit_;
    cb.f_open = f_open_; cb.f_read = f_rw; cb.f_write = (int (*)(int, const void *, int))f_rw; cb.f_lseek = f_seek; cb.f_close = f_close_;
    cb.get_buttons = btn; cb.get_jog = jog; cb.get_viewport = vp;
    cb.arena = (char *)end; cb.arena_size = 0x03105260 - end;
    {   /* disk images are opened "rb+": that must reach the launcher as mode 2, never 1 (create+truncate) */
        extern mac_cb_t *g_cb; extern int _open(const char *, int, int);
        g_cb = &cb;
        _open("T-rdonly", 0, 0);              /* fopen "rb"  */
        _open("T-rdwr(rb+)", 2, 0);           /* fopen "rb+" */
        _open("T-wronly+creat+trunc(wb)", 1 | 0x200 | 0x400, 0);   /* fopen "wb"  */
        _open("T-rdwr+creat+trunc(w+)", 2 | 0x200 | 0x400, 0);     /* fopen "w+"  */
    }
    puts_("qemu: blob_end="); putx(end); puts_(" arena="); putx(cb.arena_size); puts_("\n");
    (void)hdr;
    int r = mac_main(&cb);
    puts_("qemu: mac_main returned "); putx((unsigned)r); puts_("\n");
    puts_("QEMU-DONE\n");
}
