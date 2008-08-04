#ifndef __RENDER_H__
#define __RENDER_H__

#include <grass/gsurf.h>
#include <grass/gstypes.h>

/*** Windows headers ***/
#if defined(OPENGL_WINDOWS)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  include <winnt.h>

/*** X Window System headers ***/
#elif defined(OPENGL_X11)
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>	/* for XA_RGB_DEFAULT_MAP atom */
#  if defined(__vms)
#    include <X11/StdCmap.h>	/* for XmuLookupStandardColormap */
#  else
#    include <X11/Xmu/StdCmap.h>	/* for XmuLookupStandardColormap */
#  endif
#  include <GL/glx.h>

/*** Mac headers ***/
#elif defined(OPENGL_AQUA)
#  define Cursor QDCursor
#  include <AGL/agl.h>
#  undef Cursor
#  include <ApplicationServices/ApplicationServices.h>

#else /* make sure only one platform defined */
#  error Unsupported platform, or confused platform defines...
#endif

typedef struct
{
    Display *displayId;		/* display connection */
    GLXContext contextId;	/* GLX rendering context */
    Pixmap pixmap;
    GLXPixmap windowId;
} render_window;

#endif /* __RENDER_H__ */
