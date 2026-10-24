// ABI between the CHDK launcher module (mac.flt) and the Mac II blob (MAC.BIN).
// The first fields are layout-compatible with the DOOM blob's callback table so the same newlib
// glue (elks_syscalls.c) works; new callbacks are appended at the end. Keep both sides in sync.
#ifndef MAC_CHDK_ABI_H
#define MAC_CHDK_ABI_H
#define MAC_ABI_MAGIC 0xE1C50002

// get_buttons() bitmask
#define BTN_UP        0x001
#define BTN_DOWN      0x002
#define BTN_LEFT      0x004
#define BTN_RIGHT     0x008
#define BTN_SET       0x010
#define BTN_MENU      0x020
#define BTN_DISP      0x040
#define BTN_SHOOT     0x080   // shutter, full press
#define BTN_HALF      0x100   // shutter, half press
#define BTN_ZOOM_IN   0x200
#define BTN_ZOOM_OUT  0x400

typedef struct {
    unsigned int  magic;
    unsigned int  (*get_ticks_ms)(void);
    void          (*sleep_ms)(unsigned int ms);
    int           (*get_key)(int *pressed, unsigned char *key);   // unused by ELKS (kept for layout)
    void          (*draw_frame)(const unsigned char *fb, int w, int h);   // 8-bit indexed, scaled to the viewport
    void          (*set_palette)(const unsigned char *rgb /*256*3*/);
    int           (*f_open)(const char *path, int mode);   // mode 0=read, 1=write(create/trunc), 2=read/write
    int           (*f_read)(int fd, void *buf, int n);
    int           (*f_write)(int fd, const void *buf, int n);
    int           (*f_lseek)(int fd, int off, int whence);
    int           (*f_close)(int fd);
    char         *arena;
    unsigned int  arena_size;
    void          (*log)(const char *s);
    int           (*should_quit)(void);
    // --- appended for the ELKS blob ---
    unsigned int  (*get_buttons)(void);   // currently held buttons (BTN_*)
    int           (*get_jog)(void);       // wheel: -1 / 0 / +1 detent since last call
    // Direct access to the camera's 720x240 YUV viewport (UYVYYY, 1080 bytes/row, U/V signed):
    // returns the buffer, or 0 if unavailable. The first call hides the UI overlay.
    unsigned char *(*get_viewport)(void);
} mac_cb_t;

int mac_main(mac_cb_t *cb);   // blob entry; returns when the user quits
#endif
