/*!
   \file lib/nviz/render.c

   \brief Nviz library -- GLX context manipulation

   Based on visualization/nviz/src/togl.c
   
   (C) 2008, 2010 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
 */

#include <grass/glocale.h>
#include <grass/nviz.h>

/*!
   \brief Allocate memory for render window

   \return pointer to render_window struct
   \return NULL on failure
 */
struct render_window *Nviz_new_render_window(void)
{
    struct render_window *rwin;

    /* G_malloc() calls G_fatal_error() on failure */
    rwin = (struct render_window *)G_malloc(sizeof(struct render_window));

    return rwin;
}

/*!
   \brief Initialize render window

   \param win pointer to render_window struct
 */
void Nviz_init_render_window(struct render_window *rwin)
{
#if defined(OPENGL_X11)
    rwin->displayId = NULL;
    rwin->contextId = NULL;
    rwin->pixmap = 0;
    rwin->windowId = 0;
#elif defined(OPENGL_AQUA)
    rwin->pixelFmtId = NULL;
    rwin->contextId = NULL;
    rwin->windowId = NULL;
#elif defined(OPENGL_WINDOWS)
    rwin->displayId = NULL;
    rwin->contextId = NULL;
    rwin->bitmapId = NULL;
#endif
}

/*!
   \brief Free render window

   \param win pointer to render_window struct
 */
void Nviz_destroy_render_window(struct render_window *rwin)
{
#if defined(OPENGL_X11)
    glXDestroyGLXPixmap(rwin->displayId, rwin->windowId);
    XFreePixmap(rwin->displayId, rwin->pixmap);
    glXDestroyContext(rwin->displayId, rwin->contextId);
    XCloseDisplay(rwin->displayId);
#elif defined(OPENGL_AQUA)
    aglDestroyPixelFormat(rwin->pixelFmtId);
    aglDestroyContext(rwin->contextId);
    aglDestroyPBuffer(rwin->windowId);
    /* TODO FreePixMap */
#elif defined(OPENGL_WINDOWS)
    wglDeleteContext(rwin->contextId);
    DeleteDC(rwin->displayId);
    DeleteObject(rwin->bitmapId);
#endif

    G_free((void *)rwin);
}

/*!
   \brief Create render window

   \param rwin pointer to render_window struct
   \param display display instance (NULL for offscreen)
   \param width window width
   \param height window height

   \return 0 on success
   \return -1 on error
 */
int Nviz_create_render_window(struct render_window *rwin, void *display,
			      int width, int height)
{
#if defined(OPENGL_X11)
    int attributeList[] = { GLX_RGBA, GLX_RED_SIZE, 1,
	GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
	GLX_DEPTH_SIZE, 1, None
    };
    XVisualInfo *v;

    rwin->displayId = XOpenDisplay((char *)display);
    if (!rwin->displayId) {
	G_fatal_error(_("Bad server connection"));
    }

    v = glXChooseVisual(rwin->displayId,
			DefaultScreen(rwin->displayId), attributeList);
    if (!v) {
        G_warning(_("Unable to get visual info"));
        return -1;
    }
        
    rwin->contextId = glXCreateContext(rwin->displayId, v, NULL, GL_FALSE);

    if (!rwin->contextId) {
	G_warning(_("Unable to create rendering context"));
        return -1;
    }

    /* create win pixmap to render to (same depth as RootWindow) */
    rwin->pixmap = XCreatePixmap(rwin->displayId,
				 RootWindow(rwin->displayId, v->screen),
				 width, height, v->depth);

    /* create an off-screen GLX rendering area */
    rwin->windowId = glXCreateGLXPixmap(rwin->displayId, v, rwin->pixmap);

    XFree(v);
#elif defined(OPENGL_AQUA)
    int attributeList[] = { AGL_RGBA, AGL_RED_SIZE, 1,
	AGL_GREEN_SIZE, 1, AGL_BLUE_SIZE, 1,
	AGL_DEPTH_SIZE, 1, AGL_NONE
    };
    /* TODO: open mac display */

    /* TODO: dev = NULL, ndev = 0 ? */
    rwin->pixelFmtId = aglChoosePixelFormat(NULL, 0, attributeList);

    rwin->contextId = aglCreateContext(rwin->pixelFmtId, NULL);

    /* create an off-screen AGL rendering area */
    aglCreatePBuffer(width, height, GL_TEXTURE_2D, GL_RGBA, 0, &(rwin->windowId));
#elif defined(OPENGL_WINDOWS)
    PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	//  size of this pfd 
	1,			/* version number           */
	PFD_DRAW_TO_WINDOW |	/* support window           */
	    PFD_SUPPORT_OPENGL |	/* support OpenGL           */
	    PFD_DOUBLEBUFFER,	/* double buffered          */
	PFD_TYPE_RGBA,		/* RGBA type                */
	24,			/* 24-bit color depth       */
	0, 0, 0, 0, 0, 0,	/* color bits ignored       */
	0,			/* no alpha buffer          */
	0,			/* shift bit ignored        */
	0,			/* no accumulation buffer   */
	0, 0, 0, 0,		/* accum bits ignored       */
	32,			/* 32-bit z-buffer          */
	0,			/* no stencil buffer        */
	0,			/* no auxiliary buffer      */
	PFD_MAIN_PLANE,		/* main layer               */
	0,			/* reserved                 */
	0, 0, 0			/* layer masks ignored      */
    };
    int iPixelFormat;

    rwin->displayId = CreateCompatibleDC(NULL);
    iPixelFormat = ChoosePixelFormat(rwin->displayId, &pfd);
    SetPixelFormat(rwin->displayId, iPixelFormat, &pfd);
    rwin->bitmapId = CreateCompatibleBitmap(rwin->displayId, width, height);
    SelectObject(rwin->displayId, rwin->bitmapId);
    rwin->contextId = wglCreateContext(rwin->displayId);
    /* TODO */
#endif
    return 0;
}

/*!
   \brief Make window current for rendering

   \param win pointer to render_window struct

   \return 1 on success
   \return 0 on failure
 */
int Nviz_make_current_render_window(const struct render_window *rwin)
{
#if defined(OPENGL_X11)
    if (!rwin->displayId || !rwin->contextId)
	return 0;

    if (rwin->contextId == glXGetCurrentContext())
	return 1;

    glXMakeCurrent(rwin->displayId, rwin->windowId, rwin->contextId);
#elif defined(OPENGL_AQUA)
    if (!rwin->contextId)
	return 0;

    if (rwin->contextId == aglGetCurrentContext())
	return 1;

    aglSetCurrentContext(rwin->contextId);
    aglSetPBuffer(rwin->contextId, rwin->windowId, 0, 0, 0);
#elif defined(OPENGL_WINDOWS)
    if (!rwin->displayId || !rwin->contextId)
	return 0;

    wglMakeCurrent(rwin->displayId, rwin->contextId);
#endif

    return 1;
}
