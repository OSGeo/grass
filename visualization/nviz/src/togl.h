/* vi:set sw=4: */

/* 
 * Togl - a Tk OpenGL widget
 *
 * Copyright (C) 1996-1998  Brian Paul and Ben Bederson
 * See the LICENSE file for copyright details.
 */


#ifndef TOGL_H
#  define TOGL_H

#  include "togl_ws.h"

#  ifdef TOGL_WGL
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    undef WIN32_LEAN_AND_MEAN
#    if defined(_MSC_VER)
#      	define DllEntryPoint DllMain
#    endif
#  endif

#  ifdef _WIN32
#    define TOGL_EXTERN __declspec(dllexport) extern
#  else
#    define TOGL_EXTERN extern
#  endif /* _WIN32 */

#  ifdef TOGL_AGL_CLASSIC
#    ifndef MAC_TCL
#      define MAC_TCL 1
#    endif
#  endif

#  ifdef TOGL_AGL
#    ifndef MAC_OSX_TCL
#      define MAC_OSX_TCL 1
#    endif
#    ifndef MAC_OSX_TK
#      define MAC_OSX_TK 1
#    endif
#  endif

#  include <tcl.h>
#  include <tk.h>
#  if defined(TOGL_AGL) || defined(TOGL_AGL_CLASSIC)
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif

#  ifdef __sgi
#    include <GL/glx.h>
#    include <X11/extensions/SGIStereo.h>
#  endif

#  ifndef CONST84
#    define CONST84
#  endif

#  ifndef NULL
#    define NULL 0
#  endif

#  ifndef TOGL_USE_FONTS
#    define TOGL_USE_FONTS 1    /* needed for demos */
#  endif

#  ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#  endif

#  define TOGL_VERSION "1.7"
#  define TOGL_MAJOR_VERSION 1
#  define TOGL_MINOR_VERSION 7

/* 
 * "Standard" fonts which can be specified to Togl_LoadBitmapFont()
 */
#  define TOGL_BITMAP_8_BY_13		((char *) 1)
#  define TOGL_BITMAP_9_BY_15		((char *) 2)
#  define TOGL_BITMAP_TIMES_ROMAN_10	((char *) 3)
#  define TOGL_BITMAP_TIMES_ROMAN_24	((char *) 4)
#  define TOGL_BITMAP_HELVETICA_10	((char *) 5)
#  define TOGL_BITMAP_HELVETICA_12	((char *) 6)
#  define TOGL_BITMAP_HELVETICA_18	((char *) 7)

/* 
 * Normal and overlay plane constants
 */
#  define TOGL_NORMAL	1
#  define TOGL_OVERLAY	2

struct Togl;
typedef struct Togl Togl;

typedef void (Togl_Callback) (Togl *togl);
typedef int (Togl_CmdProc) (Togl *togl, int argc, CONST84 char *argv[]);

TOGL_EXTERN int Togl_Init(Tcl_Interp *interp);

/* 
 * Default/initial callback setup functions
 */

TOGL_EXTERN void Togl_CreateFunc(Togl_Callback *proc);
TOGL_EXTERN void Togl_DisplayFunc(Togl_Callback *proc);
TOGL_EXTERN void Togl_ReshapeFunc(Togl_Callback *proc);
TOGL_EXTERN void Togl_DestroyFunc(Togl_Callback *proc);
TOGL_EXTERN void Togl_TimerFunc(Togl_Callback *proc);
TOGL_EXTERN void Togl_ResetDefaultCallbacks(void);

/* 
 * Change callbacks for existing widget
 */

TOGL_EXTERN void Togl_SetCreateFunc(Togl *togl, Togl_Callback *proc);
TOGL_EXTERN void Togl_SetDisplayFunc(Togl *togl, Togl_Callback *proc);
TOGL_EXTERN void Togl_SetReshapeFunc(Togl *togl, Togl_Callback *proc);
TOGL_EXTERN void Togl_SetDestroyFunc(Togl *togl, Togl_Callback *proc);
TOGL_EXTERN void Togl_SetTimerFunc(Togl *togl, Togl_Callback *proc);

/* 
 * Miscellaneous
 */

TOGL_EXTERN int Togl_Configure(Tcl_Interp *interp, Togl *togl,
        int argc, const char *argv[], int flags);
TOGL_EXTERN void Togl_MakeCurrent(const Togl *togl);
TOGL_EXTERN void Togl_CreateCommand(char *cmd_name, Togl_CmdProc *cmd_proc);
TOGL_EXTERN void Togl_PostRedisplay(Togl *togl);
TOGL_EXTERN void Togl_SwapBuffers(const Togl *togl);

/* 
 * Query functions
 */

TOGL_EXTERN const char *Togl_Ident(const Togl *togl);
TOGL_EXTERN int Togl_Width(const Togl *togl);
TOGL_EXTERN int Togl_Height(const Togl *togl);
TOGL_EXTERN Tcl_Interp *Togl_Interp(const Togl *togl);
TOGL_EXTERN Tk_Window Togl_TkWin(const Togl *togl);

/* 
 * Color Index mode
 */

TOGL_EXTERN unsigned long Togl_AllocColor(const Togl *togl, float red,
        float green, float blue);
TOGL_EXTERN void Togl_FreeColor(const Togl *togl, unsigned long index);
TOGL_EXTERN void Togl_SetColor(const Togl *togl, unsigned long index,
        float red, float green, float blue);

#  if TOGL_USE_FONTS == 1
/* 
 * Bitmap fonts
 */

TOGL_EXTERN GLuint Togl_LoadBitmapFont(const Togl *togl, const char *fontname);
TOGL_EXTERN void Togl_UnloadBitmapFont(const Togl *togl, GLuint fontbase);

#  endif
/* 
 * Overlay functions
 */

TOGL_EXTERN void Togl_UseLayer(Togl *togl, int layer);
TOGL_EXTERN void Togl_ShowOverlay(Togl *togl);
TOGL_EXTERN void Togl_HideOverlay(Togl *togl);
TOGL_EXTERN void Togl_PostOverlayRedisplay(Togl *togl);
TOGL_EXTERN void Togl_OverlayDisplayFunc(Togl_Callback *proc);
TOGL_EXTERN int Togl_ExistsOverlay(const Togl *togl);
TOGL_EXTERN int Togl_GetOverlayTransparentValue(const Togl *togl);
TOGL_EXTERN int Togl_IsMappedOverlay(const Togl *togl);
TOGL_EXTERN unsigned long Togl_AllocColorOverlay(const Togl *togl,
        float red, float green, float blue);
TOGL_EXTERN void Togl_FreeColorOverlay(const Togl *togl, unsigned long index);

/* 
 * User client data
 */

TOGL_EXTERN void Togl_ClientData(ClientData clientData);
TOGL_EXTERN ClientData Togl_GetClientData(const Togl *togl);
TOGL_EXTERN void Togl_SetClientData(Togl *togl, ClientData clientData);

#  ifdef TOGL_X11
/* 
 * X11-only commands.
 * Contributed by Miguel A. De Riera Pasenau (miguel@DALILA.UPC.ES)
 */

TOGL_EXTERN Display *Togl_Display(const Togl *togl);
TOGL_EXTERN Screen *Togl_Screen(const Togl *togl);
TOGL_EXTERN int Togl_ScreenNumber(const Togl *togl);
TOGL_EXTERN Colormap Togl_Colormap(const Togl *togl);

#  endif
#  ifdef __sgi
/* 
 * SGI stereo-only commands.
 * Contributed by Ben Evans (Ben.Evans@anusf.anu.edu.au)
 */

TOGL_EXTERN void Togl_OldStereoDrawBuffer(GLenum mode);
TOGL_EXTERN void Togl_OldStereoClear(GLbitfield mask);
#  endif

TOGL_EXTERN void Togl_StereoFrustum(GLfloat left, GLfloat right, GLfloat bottom,
        GLfloat top, GLfloat near, GLfloat far, GLfloat eyeDist,
        GLfloat eyeOffset);

/* 
 * Generate EPS file.
 * Contributed by Miguel A. De Riera Pasenau (miguel@DALILA.UPC.ES)
 */

TOGL_EXTERN int Togl_DumpToEpsFile(const Togl *togl, const char *filename,
        int inColor, void (*user_redraw) (const Togl *));

#  ifdef TOGL_AGL_CLASSIC
/* 
 * Mac-specific setup functions
 */
extern int Togl_MacInit(void);
extern int Togl_MacSetupMainInterp(Tcl_Interp *interp);
#  endif

#  ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#  endif


#endif
