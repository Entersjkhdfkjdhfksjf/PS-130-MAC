/*
	OSGLUCHD.c

	Operating System GLUe for the Canon PowerShot SX130 IS (CHDK blob).

	Derived from OSGLUNDS.c (Nintendo DS) in Mini vMac, which is
	Copyright (C) 2006 Philip Cummins, Richard F. Bannister, Paul C. Pratt,
	GNU General Public License version 2. The same license applies here.

	Platform services come from the CHDK launcher through mac_cb_t (see
	mac_chdk_abi.h): ticks, buttons + wheel, card file I/O (via newlib),
	and direct access to the 720x240 YUV viewport. The emulated Mac II
	screen (512x384, 8 bit colour or 1 bit mono) is scaled down to fit it.
*/

#include "OSGCOMUI.h"
#include "OSGCOMUD.h"

#ifdef WantOSGLUNDS   /* the setup tool's Nintendo DS target selects this file */

#include "mac_chdk_abi.h"

mac_cb_t *g_cb = NULL;

#define CONSOLE_TRACE()

/* Progress log: each line is appended to MAC.LOG on the card (and closed again), so it survives a
   crash or power loss and can be read afterwards. */
LOCALPROC FLog(const char *msg)
{
	char line[96];
	int n = 0;
	int fd;
	unsigned int t = g_cb->get_ticks_ms();
	/* "[12345 ms] message\n" built by hand: no stdio here */
	{
		char num[12];
		int k = 0;
		unsigned int v = t;
		line[n++] = '[';
		if (v == 0) { num[k++] = '0'; }
		while (v > 0) { num[k++] = (char)('0' + v % 10); v /= 10; }
		while (k > 0) { line[n++] = num[--k]; }
		line[n++] = ' '; line[n++] = 'm'; line[n++] = 's'; line[n++] = ']'; line[n++] = ' ';
	}
	while (*msg && n < (int)sizeof(line) - 2) { line[n++] = *msg++; }
	line[n++] = '\n';
	fd = g_cb->f_open("MAC.LOG", 2);
	if (fd < 0) { fd = g_cb->f_open("MAC.LOG", 1); }
	if (fd >= 0) {
		(void) g_cb->f_lseek(fd, 0, 2);
		(void) g_cb->f_write(fd, line, n);
		(void) g_cb->f_close(fd);
	}
	line[n - 1] = 0;   /* drop the newline for the on-screen log */
	g_cb->log(line);
}

/* --- some simple utilities --- */

GLOBALOSGLUPROC MyMoveBytes(anyp srcPtr, anyp destPtr, si5b byteCount)
{
	(void) memcpy((char *)destPtr, (char *)srcPtr, byteCount);
}

/* --- control mode and internationalization --- */

#define NeedCell2PlainAsciiMap 1

#include "INTLCHAR.h"

/* --- information about the environment --- */

#define WantColorTransValid 0

#include "COMOSGLU.h"
#include "CONTROLM.h"

LOCALPROC NativeStrFromCStr(char *r, char *s)
{
	ui3b ps[ClStrMaxLength];
	int i;
	int L;

	ClStrFromSubstCStr(&L, ps, s);

	for (i = 0; i < L; ++i) {
		r[i] = Cell2PlainAsciiMap[ps[i]];
	}

	r[L] = 0;
}

/* --- drives --- */

#define NotAfileRef NULL

LOCALVAR FILE *Drives[NumDrives]; /* open disk image files */
#if IncludeSonyGetName || IncludeSonyNew
LOCALVAR char *DriveNames[NumDrives];
#endif

LOCALPROC InitDrives(void)
{
	/*
		This isn't really needed, Drives[i] and DriveNames[i]
		need not have valid values when not vSonyIsInserted[i].
	*/
	tDrive i;

	for (i = 0; i < NumDrives; ++i) {
		Drives[i] = NotAfileRef;
#if IncludeSonyGetName || IncludeSonyNew
		DriveNames[i] = NULL;
#endif
	}
}

GLOBALOSGLUFUNC tMacErr vSonyTransfer(blnr IsWrite, ui3p Buffer,
	tDrive Drive_No, ui5r Sony_Start, ui5r Sony_Count,
	ui5r *Sony_ActCount)
{
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	ui5r NewSony_Count = 0;

	if (0 == fseek(refnum, Sony_Start, SEEK_SET)) {
		if (IsWrite) {
			NewSony_Count = fwrite(Buffer, 1, Sony_Count, refnum);
		} else {
			NewSony_Count = fread(Buffer, 1, Sony_Count, refnum);
		}

		if (NewSony_Count == Sony_Count) {
			err = mnvm_noErr;
		}
	}

	if (nullpr != Sony_ActCount) {
		*Sony_ActCount = NewSony_Count;
	}

	return err; /*& figure out what really to return &*/
}

GLOBALOSGLUFUNC tMacErr vSonyGetSize(tDrive Drive_No, ui5r *Sony_Count)
{
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	long v;

	if (0 == fseek(refnum, 0, SEEK_END)) {
		v = ftell(refnum);
		if (v >= 0) {
			*Sony_Count = v;
			err = mnvm_noErr;
		}
	}

	return err; /*& figure out what really to return &*/
}

LOCALFUNC tMacErr vSonyEject0(tDrive Drive_No, blnr deleteit)
{
	FILE *refnum = Drives[Drive_No];

	DiskEjectedNotify(Drive_No);

	fclose(refnum);
	Drives[Drive_No] = NotAfileRef; /* not really needed */

#if IncludeSonyGetName || IncludeSonyNew
	{
		char *s = DriveNames[Drive_No];
		if (NULL != s) {
			if (deleteit) {
				remove(s);
			}
			free(s);
			DriveNames[Drive_No] = NULL; /* not really needed */
		}
	}
#endif

	return mnvm_noErr;
}

GLOBALOSGLUFUNC tMacErr vSonyEject(tDrive Drive_No)
{
	return vSonyEject0(Drive_No, falseblnr);
}

#if IncludeSonyNew
GLOBALOSGLUFUNC tMacErr vSonyEjectDelete(tDrive Drive_No)
{
	return vSonyEject0(Drive_No, trueblnr);
}
#endif

LOCALPROC UnInitDrives(void)
{
	tDrive i;

	for (i = 0; i < NumDrives; ++i) {
		if (vSonyIsInserted(i)) {
			(void) vSonyEject(i);
		}
	}
}

#if IncludeSonyGetName
GLOBALOSGLUFUNC tMacErr vSonyGetName(tDrive Drive_No, tPbuf *r)
{
	char *drivepath = DriveNames[Drive_No];
	if (NULL == drivepath) {
		return mnvm_miscErr;
	} else {
		char *s = strrchr(drivepath, '/');
		if (NULL == s) {
			s = drivepath;
		} else {
			++s;
		}
		return NativeTextToMacRomanPbuf(s, r);
	}
}
#endif

LOCALFUNC blnr Sony_Insert0(FILE *refnum, blnr locked,
	char *drivepath)
{
	tDrive Drive_No;
	blnr IsOk = falseblnr;

	if (! FirstFreeDisk(&Drive_No)) {
		MacMsg(kStrTooManyImagesTitle, kStrTooManyImagesMessage,
			falseblnr);
	} else {
		/* printf("Sony_Insert0 %d\n", (int)Drive_No); */

		{
			Drives[Drive_No] = refnum;
			DiskInsertNotify(Drive_No, locked);

#if IncludeSonyGetName || IncludeSonyNew
			{
				ui5b L = strlen(drivepath);
				char *p = malloc(L + 1);
				if (p != NULL) {
					(void) memcpy(p, drivepath, L + 1);
				}
				DriveNames[Drive_No] = p;
			}
#endif

			IsOk = trueblnr;
		}
	}

	if (! IsOk) {
		fclose(refnum);
	}

	return IsOk;
}

LOCALFUNC blnr Sony_Insert1(char *drivepath, blnr silentfail)
{
	blnr locked = falseblnr;
	/* printf("Sony_Insert1 %s\n", drivepath); */
	FILE *refnum = fopen(drivepath, "rb+");
	if (NULL == refnum) {
		locked = trueblnr;
		refnum = fopen(drivepath, "rb");
		CONSOLE_TRACE();
	}
	if (NULL == refnum) {
		if (! silentfail) {
			MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
			CONSOLE_TRACE();
		}
	} else {
		CONSOLE_TRACE();
		return Sony_Insert0(refnum, locked, drivepath);
	}

	return falseblnr;
}

#define Sony_Insert2(s) Sony_Insert1(s, trueblnr)

LOCALFUNC blnr Sony_InsertIth(int i)
{
	blnr v;

	if ((i > 9) || ! FirstFreeDisk(nullpr)) {
		v = falseblnr;
	} else {
		char s[] = "disk?.dsk";

		s[4] = '0' + i;

		v = Sony_Insert2(s);
	}

	return v;
}

LOCALFUNC blnr LoadInitialImages(void)
{
	int i;

	CONSOLE_TRACE();

	for (i = 1; Sony_InsertIth(i); ++i) {
		/* stop on first error (including file not found) */
	}

	return trueblnr;
}

#if IncludeSonyNew
LOCALFUNC blnr WriteZero(FILE *refnum, ui5b L)
{
#define ZeroBufferSize 2048
	ui5b i;
	ui3b buffer[ZeroBufferSize];

	memset(&buffer, 0, ZeroBufferSize);

	while (L > 0) {
		i = (L > ZeroBufferSize) ? ZeroBufferSize : L;
		if (fwrite(buffer, 1, i, refnum) != i) {
			return falseblnr;
		}
		L -= i;
	}
	return trueblnr;
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDisk(ui5b L, char *drivepath)
{
	blnr IsOk = falseblnr;
	FILE *refnum = fopen(drivepath, "wb+");
	if (NULL == refnum) {
		MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
	} else {
		if (WriteZero(refnum, L)) {
			IsOk = Sony_Insert0(refnum, falseblnr, drivepath);
			refnum = NULL;
		}
		if (refnum != NULL) {
			fclose(refnum);
		}
		if (! IsOk) {
			(void) remove(drivepath);
		}
	}
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDiskAtDefault(ui5b L)
{
	char s[ClStrMaxLength + 1];

	NativeStrFromCStr(s, "untitled.dsk");
	MakeNewDisk(L, s);
}
#endif

/* --- ROM --- */

LOCALFUNC tMacErr LoadMacRomFrom(char *path)
{
	tMacErr err;
	FILE *ROM_File;
	int File_Size;

	ROM_File = fopen(path, "rb");
	if (NULL == ROM_File) {
		err = mnvm_fnfErr;
	} else {
		File_Size = fread(ROM, 1, kROM_Size, ROM_File);
		if (kROM_Size != File_Size) {
			if (feof(ROM_File)) {
				MacMsgOverride(kStrShortROMTitle,
					kStrShortROMMessage);
				err = mnvm_eofErr;
			} else {
				MacMsgOverride(kStrNoReadROMTitle,
					kStrNoReadROMMessage);
				err = mnvm_miscErr;
			}
		} else {
			err = ROM_IsValid();
		}
		fclose(ROM_File);
	}

	return err;
}

LOCALFUNC blnr LoadMacRom(void)
{
	tMacErr err;

	if (mnvm_fnfErr == (err = LoadMacRomFrom(RomFileName)))
	{
	}

	return trueblnr; /* keep launching Mini vMac, regardless */
}


/* --- video out: scale the Mac screen to the camera's 720x240 viewport --- */

#define VP_W  720
#define VP_H  240
#define VP_BW 1080      /* UYVYYY: 6 bytes per 4 pixels */

LOCALVAR ui3b Ytab[256];
LOCALVAR si3b Utab[256];
LOCALVAR si3b Vtab[256];
LOCALVAR ui5b LastDrawMs = 0;

/* horizontal map: dst x -> two source columns and a weight (0..256) for the first */
LOCALVAR ui4b MapC0[VP_W];
LOCALVAR ui4b MapC1[VP_W];
LOCALVAR ui4b MapW0[VP_W];
LOCALVAR ui4b MapCenter[VP_W];   /* nearest source column (chroma) */
/* vertical map: dst y -> up to 3 source rows with weights summing to 256 */
LOCALVAR ui4b MapR0[VP_H];
LOCALVAR ui3b MapRN[VP_H];
LOCALVAR ui4b MapRW[VP_H][3];
LOCALVAR ui4b MapRC[VP_H];       /* nearest source row (chroma) */

LOCALVAR ui3b HRow[3][VP_W];
LOCALVAR si4b HRowId[3] = {-1, -1, -1};
LOCALVAR ui3b MonoRow8[vMacScreenWidth];

LOCALPROC BuildScaleMaps(void)
{
	int x;
	int y;
	/* 1/1024 source-pixel units */
	for (x = 0; x < VP_W; ++x) {
		long s0 = (long)x * vMacScreenWidth * 1024 / VP_W;
		long s1 = (long)(x + 1) * vMacScreenWidth * 1024 / VP_W;
		long c0 = s0 >> 10;
		long c1 = (s1 - 1) >> 10;
		long w0;
		if (c1 >= vMacScreenWidth) { c1 = vMacScreenWidth - 1; }
		if (c1 == c0) {
			w0 = 256;
		} else {
			w0 = (((c0 + 1) << 10) - s0) * 256 / (s1 - s0);
		}
		MapC0[x] = (ui4b)c0;
		MapC1[x] = (ui4b)c1;
		MapW0[x] = (ui4b)w0;
		MapCenter[x] = (ui4b)(((long)x * 2 + 1) * vMacScreenWidth / (VP_W * 2));
	}
	for (y = 0; y < VP_H; ++y) {
		long s0 = (long)y * vMacScreenHeight * 1024 / VP_H;
		long s1 = (long)(y + 1) * vMacScreenHeight * 1024 / VP_H;
		long r0 = s0 >> 10;
		long r1 = (s1 - 1) >> 10;
		long n;
		long i;
		long sum = 0;
		if (r1 >= vMacScreenHeight) { r1 = vMacScreenHeight - 1; }
		n = r1 - r0 + 1;
		if (n > 3) { n = 3; }
		for (i = 0; i < n; ++i) {
			long lo = (r0 + i) << 10;
			long hi = lo + 1024;
			long w;
			if (lo < s0) { lo = s0; }
			if (hi > s1) { hi = s1; }
			w = (hi - lo) * 256 / (s1 - s0);
			MapRW[y][i] = (ui4b)w;
			sum += w;
		}
		MapRW[y][0] += (ui4b)(256 - sum);   /* make the weights add up */
		MapR0[y] = (ui4b)r0;
		MapRN[y] = (ui3b)n;
		MapRC[y] = (ui4b)(((long)y * 2 + 1) * vMacScreenHeight / (VP_H * 2));
	}
}

LOCALPROC SetYUV(int i, int r, int g, int b)
{
	int y = (77 * r + 150 * g + 29 * b) >> 8;
	int u = ((b - y) * 144) >> 8;
	int v = ((r - y) * 183) >> 8;
	if (y < 0) { y = 0; } else if (y > 255) { y = 255; }
	if (u < -128) { u = -128; } else if (u > 127) { u = 127; }
	if (v < -128) { v = -128; } else if (v > 127) { v = 127; }
	Ytab[i] = (ui3b)y;
	Utab[i] = (si3b)u;
	Vtab[i] = (si3b)v;
}

LOCALPROC RebuildColorTables(void)
{
	int i;
#if 0 != vMacScreenDepth
	if (UseColorMode) {
		for (i = 0; i < CLUT_size; ++i) {
			SetYUV(i, CLUT_reds[i] >> 8, CLUT_greens[i] >> 8,
				CLUT_blues[i] >> 8);
		}
	} else
#endif
	{
		SetYUV(0, 255, 255, 255);   /* mono: 0 = white */
		SetYUV(1, 0, 0, 0);         /* mono: 1 = black */
		(void) i;
	}
}

/* pointer to one source row as 8 bit indices */
LOCALFUNC ui3p SourceRow(int r)
{
	ui3p buf = (ui3p)GetCurDrawBuff();
#if 0 != vMacScreenDepth
	if (UseColorMode) {
		return buf + (uimr)r * vMacScreenByteWidth;
	} else
#endif
	{
		ui3p s = buf + (uimr)r * vMacScreenMonoByteWidth;
		int x;
		for (x = 0; x < vMacScreenWidth; ++x) {
			MonoRow8[x] = (s[x >> 3] >> (7 - (x & 7))) & 1;
		}
		return MonoRow8;
	}
}

LOCALFUNC ui3p GetHRow(int r)
{
	int k;
	int x;
	ui3p dst;
	ui3p src;

	for (k = 0; k < 3; ++k) {
		if (HRowId[k] == r) {
			return HRow[k];
		}
	}
	/* evict the cached row furthest from r */
	k = 0;
	{
		int best = -1;
		int i;
		for (i = 0; i < 3; ++i) {
			int d = (HRowId[i] < 0) ? 100000
				: ((HRowId[i] > r) ? HRowId[i] - r : r - HRowId[i]);
			if (d > best) { best = d; k = i; }
		}
	}
	dst = HRow[k];
	src = SourceRow(r);
	for (x = 0; x < VP_W; ++x) {
		ui4r w0 = MapW0[x];
		dst[x] = (ui3b)((w0 * Ytab[src[MapC0[x]]]
			+ (256 - w0) * Ytab[src[MapC1[x]]]) >> 8);
	}
	HRowId[k] = r;
	return dst;
}

LOCALPROC DrawViewportRows(int y0, int y1)
{
	unsigned char *vp = g_cb->get_viewport();
	int y;
	if (NULL == vp) {
		return;
	}
	for (y = y0; y < y1; ++y) {
		unsigned char *d = vp + y * VP_BW;
		ui3b yrow[VP_W];
		ui3p crow;
		int x;
		int n = MapRN[y];

		{
			ui3p r0 = GetHRow(MapR0[y]);
			if (n == 1) {
				for (x = 0; x < VP_W; ++x) { yrow[x] = r0[x]; }
			} else {
				ui3p r1 = GetHRow(MapR0[y] + 1);
				if (n == 2) {
					for (x = 0; x < VP_W; ++x) {
						yrow[x] = (ui3b)((MapRW[y][0] * r0[x]
							+ MapRW[y][1] * r1[x]) >> 8);
					}
				} else {
					ui3p r2 = GetHRow(MapR0[y] + 2);
					for (x = 0; x < VP_W; ++x) {
						yrow[x] = (ui3b)((MapRW[y][0] * r0[x]
							+ MapRW[y][1] * r1[x]
							+ MapRW[y][2] * r2[x]) >> 8);
					}
				}
			}
		}
		/* chroma: nearest source row/columns (the eye mostly sees luma) */
		crow = SourceRow(MapRC[y]);
		for (x = 0; x < VP_W; x += 4) {
			ui3r q0 = crow[MapCenter[x]];
			ui3r q1 = crow[MapCenter[x + 1]];
			ui3r q2 = crow[MapCenter[x + 2]];
			ui3r q3 = crow[MapCenter[x + 3]];
			d[0] = (unsigned char)(((int)Utab[q0] + Utab[q1] + Utab[q2] + Utab[q3]) >> 2);
			d[1] = yrow[x];
			d[2] = (unsigned char)(((int)Vtab[q0] + Vtab[q1] + Vtab[q2] + Vtab[q3]) >> 2);
			d[3] = yrow[x + 1];
			d[4] = yrow[x + 2];
			d[5] = yrow[x + 3];
			d += 6;
		}
	}
}

LOCALPROC HaveChangedScreenBuff(ui4r top, ui4r left,
	ui4r bottom, ui4r right)
{
	int y0 = (int)((long)top * VP_H / vMacScreenHeight);
	int y1 = (int)(((long)bottom * VP_H + vMacScreenHeight - 1)
		/ vMacScreenHeight) + 1;
	int i;

	(void) left;
	(void) right;
	if (y1 > VP_H) { y1 = VP_H; }
	for (i = 0; i < 3; ++i) { HRowId[i] = -1; }
	RebuildColorTables();
	DrawViewportRows(y0, y1);
}

LOCALPROC MyDrawChangesAndClear(void)
{
	if (ScreenChangedBottom > ScreenChangedTop) {
		ui5b now = g_cb->get_ticks_ms();
		if ((now - LastDrawMs) >= 60) {   /* at most ~16 redraws per second */
			LastDrawMs = now;
			HaveChangedScreenBuff(ScreenChangedTop, ScreenChangedLeft,
				ScreenChangedBottom, ScreenChangedRight);
			ScreenClearChanges();
		}
	}
}

GLOBALOSGLUPROC DoneWithDrawingForTick(void)
{
	MyDrawChangesAndClear();
}

LOCALFUNC blnr Screen_Init(void)
{
	BuildScaleMaps();
	return trueblnr;
}

#if VarFullScreen
LOCALPROC ToggleWantFullScreen(void)
{
	/* the camera screen is always "full screen"; nothing to toggle */
}
#endif

/* --- mouse (d-pad + wheel + zoom lever) --- */

LOCALVAR ui4b HoldUp = 0;
LOCALVAR ui4b HoldDown = 0;
LOCALVAR ui4b HoldLeft = 0;
LOCALVAR ui4b HoldRight = 0;
LOCALVAR blnr LastCtrlClick = falseblnr;

LOCALFUNC si4b StepFor(ui4b hold)
{
	/* slow for precision, then speeds up while held */
	if (hold == 0) { return 0; }
	if (hold < 8) { return 1; }
	if (hold < 20) { return 3; }
	if (hold < 40) { return 6; }
	return 10;
}

LOCALPROC CheckMouseState(void)
{
	ui5b b = g_cb->get_buttons();
	si5b dx;
	si5b dy;
	int guard = 8;
	int d;
	blnr left;
	blnr right;

	HoldUp    = (b & BTN_UP)    ? HoldUp + 1    : 0;
	HoldDown  = (b & BTN_DOWN)  ? HoldDown + 1  : 0;
	HoldLeft  = (b & BTN_LEFT)  ? HoldLeft + 1  : 0;
	HoldRight = (b & BTN_RIGHT) ? HoldRight + 1 : 0;

	dx = StepFor(HoldRight) - StepFor(HoldLeft);
	dy = StepFor(HoldDown) - StepFor(HoldUp);

	/* wheel: fast horizontal motion */
	while (guard-- && (0 != (d = g_cb->get_jog()))) {
		dx += (d > 0) ? 12 : -12;
	}

	/* zoom out = left click, zoom in = right click (Control-click on a classic Mac) */
	left  = (0 != (b & (BTN_ZOOM_OUT | BTN_SET)));
	right = (0 != (b & BTN_ZOOM_IN));

	if (right != LastCtrlClick) {
		LastCtrlClick = right;
		Keyboard_UpdateKeyMap2(MKC_Control, right);
	}

	HaveMouseMotion = trueblnr;
	MyMousePositionSetDelta(dx, dy);
	MyMouseButtonSet(left || right);
}


/* --- time --- */

LOCALVAR ui5b TrueEmulatedTime = 0;
LOCALVAR ui5b BehindCount = 0;

#include "DATE2SEC.h"

#define TicksPerSecond 1000000

LOCALVAR blnr HaveTimeDelta = falseblnr;
LOCALVAR ui5b TimeDelta;

LOCALVAR ui5b NewMacDateInSeconds;

LOCALVAR ui5b LastTimeSec;
LOCALVAR ui5b LastTimeUsec;

LOCALPROC GetCurrentTicks(void)
{
	ui5b ms = g_cb->get_ticks_ms();
	ui5b sec = ms / 1000;

	if (! HaveTimeDelta) {
		/* the camera clock is not wired up yet: start at a fixed date */
		TimeDelta = Date2MacSeconds(0, 0, 12, 24, 10, 2026) - sec;
		HaveTimeDelta = trueblnr;
	}

	NewMacDateInSeconds = sec + TimeDelta;
	LastTimeSec = sec;
	LastTimeUsec = (ms % 1000) * 1000;
}

#define MyInvTimeStep 16626   /* TicksPerSecond / 60.14742 */

LOCALVAR ui5b NextTimeSec;
LOCALVAR ui5b NextTimeUsec;

LOCALPROC IncrNextTime(void)
{
	NextTimeUsec += MyInvTimeStep;
	if (NextTimeUsec >= TicksPerSecond) {
		NextTimeUsec -= TicksPerSecond;
		NextTimeSec += 1;
	}
}

LOCALPROC InitNextTime(void)
{
	NextTimeSec = LastTimeSec;
	NextTimeUsec = LastTimeUsec;
	IncrNextTime();
}

LOCALPROC StartUpTimeAdjust(void)
{
	GetCurrentTicks();
	InitNextTime();
}

LOCALFUNC si5b GetTimeDiff(void)
{
	return ((si5b)(LastTimeSec - NextTimeSec)) * TicksPerSecond
		+ ((si5b)(LastTimeUsec - NextTimeUsec));
}

LOCALPROC UpdateTrueEmulatedTime(void)
{
	si5b TimeDiff;

	GetCurrentTicks();

	TimeDiff = GetTimeDiff();
	if (TimeDiff >= 0) {
		if (TimeDiff > 4 * MyInvTimeStep) {
			/* emulation fell behind: run slower instead of racing to catch up */
			++TrueEmulatedTime;
			++BehindCount;
			InitNextTime();
		} else {
			do {
				++TrueEmulatedTime;
				IncrNextTime();
				TimeDiff -= TicksPerSecond;
			} while (TimeDiff >= 0);
		}
	} else if (TimeDiff < - 2 * MyInvTimeStep) {
		InitNextTime();
	}
}

LOCALFUNC blnr CheckDateTime(void)
{
	if (CurMacDateInSeconds != NewMacDateInSeconds) {
		CurMacDateInSeconds = NewMacDateInSeconds;
		return trueblnr;
	} else {
		return falseblnr;
	}
}

LOCALFUNC blnr InitLocationDat(void)
{
	GetCurrentTicks();
	CurMacDateInSeconds = NewMacDateInSeconds;

	return trueblnr;
}

LOCALPROC CheckSavedMacMsg(void)
{
	if (nullpr != SavedBriefMsg) {
		char briefMsg0[ClStrMaxLength + 1];
		char longMsg0[ClStrMaxLength + 1];

		NativeStrFromCStr(briefMsg0, SavedBriefMsg);
		NativeStrFromCStr(longMsg0, SavedLongMsg);

		g_cb->log(briefMsg0);
		g_cb->log(longMsg0);

		SavedBriefMsg = nullpr;
	}
}

/* --- SavedTasks --- */

LOCALVAR blnr CurSpeedStopped = trueblnr;

LOCALPROC LeaveSpeedStopped(void)
{
	StartUpTimeAdjust();
}

LOCALPROC EnterSpeedStopped(void)
{
}

LOCALPROC CheckForSavedTasks(void)
{
	if (MyEvtQNeedRecover) {
		MyEvtQNeedRecover = falseblnr;

		/* attempt cleanup, MyEvtQNeedRecover may get set again */
		MyEvtQTryRecoverFromFull();
	}

	if (RequestMacOff) {
		RequestMacOff = falseblnr;
		if (AnyDiskInserted()) {
			MacMsgOverride(kStrQuitWarningTitle,
				kStrQuitWarningMessage);
		} else {
			ForceMacOff = trueblnr;
		}
	}

	if (ForceMacOff) {
		return;
	}

	if (CurSpeedStopped != SpeedStopped) {
		CurSpeedStopped = ! CurSpeedStopped;
		if (CurSpeedStopped) {
			EnterSpeedStopped();
		} else {
			LeaveSpeedStopped();
		}
	}

#if IncludeSonyNew
	if (vSonyNewDiskWanted) {
#if IncludeSonyNameNew
		if (vSonyNewDiskName != NotAPbuf) {
			ui3p NewDiskNameDat;
			if (MacRomanTextToNativePtr(vSonyNewDiskName, trueblnr,
				&NewDiskNameDat))
			{
				MakeNewDisk(vSonyNewDiskSize, (char *)NewDiskNameDat);
				free(NewDiskNameDat);
			}
			PbufDispose(vSonyNewDiskName);
			vSonyNewDiskName = NotAPbuf;
		} else
#endif
		{
			MakeNewDiskAtDefault(vSonyNewDiskSize);
		}
		vSonyNewDiskWanted = falseblnr;
			/* must be done after may have gotten disk */
	}
#endif

	if ((nullpr != SavedBriefMsg) & ! MacMsgDisplayed) {
		MacMsgDisplayOn();
	}

	if (NeedWholeScreenDraw) {
		NeedWholeScreenDraw = falseblnr;
		ScreenChangedAll();
	}

#if NeedRequestIthDisk
	if (0 != RequestIthDisk) {
		Sony_InsertIth(RequestIthDisk);
		RequestIthDisk = 0;
	}
#endif
}


/* --- main program flow --- */

GLOBALOSGLUFUNC blnr ExtraTimeNotOver(void)
{
	UpdateTrueEmulatedTime();
	return TrueEmulatedTime == OnTrueTime;
}

LOCALPROC WaitForTheNextEvent(void)
{
}

LOCALPROC CheckForSystemEvents(void)
{
	if (g_cb->should_quit != NULL && g_cb->should_quit()) {
		ForceMacOff = trueblnr;   /* PRINT / face button: leave the emulator */
	}
}

LOCALVAR ui5b LastYieldMs = 0;
LOCALVAR ui5b LastBeatMs = 0;

LOCALPROC YieldToCamera(void)
{
	ui5b now = g_cb->get_ticks_ms();
	if ((now - LastYieldMs) >= 40) {
		g_cb->sleep_ms(10);      /* ~20% of the CPU goes back to Canon's tasks */
		LastYieldMs = g_cb->get_ticks_ms();
	}
	if ((now - LastBeatMs) >= 5000) {
		LastBeatMs = now;
		{
			char b[64];
			(void) sprintf(b, "alive ticks=%lu behind=%lu", (unsigned long)TrueEmulatedTime,
				(unsigned long)BehindCount);
			FLog(b);
		}
	}
}

GLOBALOSGLUPROC WaitForNextTick(void)
{
label_retry:
	YieldToCamera();
	CheckForSystemEvents();
	CheckForSavedTasks();
	if (ForceMacOff) {
		return;
	}

	if (CurSpeedStopped) {
		MyDrawChangesAndClear();
		WaitForTheNextEvent();
		goto label_retry;
	}

	if (ExtraTimeNotOver()) {
		si5b TimeDiff = GetTimeDiff();
		if (TimeDiff < - 3000) {
			g_cb->sleep_ms(1);   /* ahead of real time: let the camera rest */
		}
		goto label_retry;
	}

	if (CheckDateTime()) {
	}

	CheckMouseState();

	OnTrueTime = TrueEmulatedTime;
}

#include "PROGMAIN.h"

LOCALPROC ReserveAllocAll(void)
{
#if dbglog_HAVE
	dbglog_ReserveAlloc();
#endif
	ReserveAllocOneBlock(&ROM, kROM_Size, 5, falseblnr);

	ReserveAllocOneBlock(&screencomparebuff,
		vMacScreenNumBytes, 5, trueblnr);
#if UseControlKeys
	ReserveAllocOneBlock(&CntrlDisplayBuff,
		vMacScreenNumBytes, 5, falseblnr);
#endif

#if MySoundEnabled
	ReserveAllocOneBlock((ui3p *)&TheSoundBuffer,
		dbhBufferSize, 5, falseblnr);
#endif

	EmulationReserveAlloc();
}

LOCALFUNC blnr AllocMyMemory(void)
{
	uimr n;
	blnr IsOk = falseblnr;

	ReserveAllocOffset = 0;
	ReserveAllocBigBlock = nullpr;
	ReserveAllocAll();
	n = ReserveAllocOffset;
		ReserveAllocBigBlock = (ui3p)calloc(1, n);
	if (NULL == ReserveAllocBigBlock) {
		MacMsg(kStrOutOfMemTitle, kStrOutOfMemMessage, trueblnr);
	} else {
		ReserveAllocOffset = 0;
		ReserveAllocAll();
		if (n != ReserveAllocOffset) {
			/* oops, program error */
		} else {
			IsOk = trueblnr;
		}
	}

	return IsOk;
}

LOCALPROC UnallocMyMemory(void)
{
	if (nullpr != ReserveAllocBigBlock) {
		free((char *)ReserveAllocBigBlock);
	}
}


LOCALPROC ZapOSGLUVars(void)
{
	InitDrives();
}

LOCALFUNC blnr InitOSGLU(void)
{
	FLog("allocating memory");
	if (! AllocMyMemory()) { FLog("AllocMyMemory failed"); return falseblnr; }
	FLog("memory ok, loading ROM");
	if (! LoadMacRom()) { FLog("LoadMacRom failed"); return falseblnr; }
	FLog("ROM step done, loading disk images");
	if (! LoadInitialImages()) { FLog("LoadInitialImages failed"); return falseblnr; }
	FLog("disk images done");
	if (! InitLocationDat()) { return falseblnr; }
	if (! Screen_Init()) { return falseblnr; }
	FLog("screen init ok, WaitForRom");
	if (! WaitForRom()) { FLog("WaitForRom returned false"); return falseblnr; }
	return trueblnr;
}

LOCALPROC UnInitOSGLU(void)
{
	if (MacMsgDisplayed) {
		MacMsgDisplayOff();
	}

	UnInitDrives();

	UnallocMyMemory();
	CheckSavedMacMsg();
}

#ifndef MAC_HOST_TEST
extern char __bss_start[], __bss_end[];
#endif

int mac_main(mac_cb_t *cb)
{
	char *p;
#ifndef MAC_HOST_TEST
	for (p = __bss_start; p < __bss_end; p++) { *p = 0; }
#else
	(void) p;
#endif
	g_cb = cb;
	if (cb->magic != MAC_ABI_MAGIC) { return -1; }
	FLog("MAC BLOB ENTERED");

	ZapOSGLUVars();

	if (InitOSGLU()) {
		FLog("init ok, entering ProgramMain");
		ProgramMain();
		FLog("left ProgramMain");
	} else {
		FLog("emulator init failed (ROM missing or bad?)");
	}

	UnInitOSGLU();
	return 0;
}

#endif /* WantOSGLUNDS */
