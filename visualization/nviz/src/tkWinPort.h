/*
 * tkWinPort.h --
 *
 *	This header file handles porting issues that occur because of
 *	differences between Windows and Unix. It should be the only
 *	file that contains #ifdefs to handle different flavors of OS.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * Copyright (c) 1998 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#ifndef _WINPORT
#define _WINPORT

#if 0
#include <X11/Xlib.h>
#endif
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <malloc.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _MSC_VER
#    define hypot _hypot
#endif /* _MSC_VER */

#ifdef __CYGWIN32__
#define strnicmp strncasecmp
#define stricmp strcasecmp
#else
#define strncasecmp strnicmp
#define strcasecmp stricmp
#endif

#define NBBY 8

#ifndef __CYGWIN32__
#define OPEN_MAX 32
#endif

/*
 * The following define causes Tk to use its internal keysym hash table
 */

#define REDO_KEYSYM_LOOKUP

/*
 * The following macro checks to see whether there is buffered
 * input data available for a stdio FILE.
 */

#if defined (_MSC_VER) || defined (__MINGW32__)
#    define TK_READ_DATA_PENDING(f) ((f)->_cnt > 0)
#else /* _MSC_VER || __MINGW32__ */
#    define TK_READ_DATA_PENDING(f) ((f)->level > 0)
#endif /* _MSC_VER || __MINGW32__ */

/*
 * The following stubs implement various calls that don't do anything
 * under Windows.
 */

#define TkFreeWindowId(dispPtr,w)
#define TkInitXId(dispPtr)
#define TkpCmapStressed(tkwin,colormap) (0)
#define XFlush(display)
#define XGrabServer(display)
#define XUngrabServer(display)
#define TkpSync(display)

/*
 * The following functions are implemented as macros under Windows.
 */

#define XFree(data) {if ((data) != NULL) ckfree((char *) (data));}
#define XNoOp(display) {display->request++;}
#define XSynchronize(display, bool) {display->request++;}
#define XSync(display, bool) {display->request++;}
#define XVisualIDFromVisual(visual) (visual->visualid)

/*
 * The following Tk functions are implemented as macros under Windows.
 */

#define TkpGetPixel(p) (((((p)->red >> 8) & 0xff) \
	| ((p)->green & 0xff00) | (((p)->blue << 8) & 0xff0000)) | 0x20000000)

/*
 * These calls implement native bitmaps which are not currently 
 * supported under Windows.  The macros eliminate the calls.
 */

#define TkpDefineNativeBitmaps()
#define TkpCreateNativeBitmap(display, source) None
#define TkpGetNativeAppBitmap(display, name, w, h) None

/*
 * timezone et al are already defined in Windows32api headers used by
 * GNU mingw32 port.
 */

#ifndef __MINGW32__

/*
 * Define timezone for gettimeofday.
 */

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};


struct timeval;

extern int gettimeofday(struct timeval *, struct timezone *);

#endif /* ! __MINGW32__ */

EXTERN void		panic _ANSI_ARGS_(TCL_VARARGS(char *,format));

#endif /* _WINPORT */
