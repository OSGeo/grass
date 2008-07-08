/*!
  \file render.c
 
  \brief Nviz library -- GLX context manipulation
  
  COPYRIGHT: (C) 2008 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.

  Based on visualization/nviz/src/togl.c

  \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008)

  \date 2008
*/

#include <grass/glocale.h>
#include <grass/nviz.h>

/*!
  \brief Allocate memory for render window

  \return pointer to render_window struct
  \return NULL on failure
*/
struct render_window *Nviz_new_render_window()
{
  struct render_window *rwin;

  /* G_malloc() calls G_fatal_error() on failure */
  rwin = (struct render_window *) G_malloc (sizeof (struct render_window));
  
  return rwin;
}
					    
/*!
  \brief Initialize render window

  \param win pointer to render_window struct
*/
void Nviz_init_render_window(struct render_window *rwin)
{
    rwin->displayId = NULL;
    rwin->contextId = NULL;
    rwin->pixmap = 0;
    rwin->windowId = 0;

    return;
}

/*!
  \brief Free render window

  \param win pointer to render_window struct
*/
void Nviz_destroy_render_window(struct render_window *rwin)
{
#if defined(OPENGL_X11)
    glXDestroyContext(rwin->displayId, rwin->contextId);
    glXDestroyGLXPixmap(rwin->displayId, rwin->windowId); 
    XFreePixmap(rwin->displayId, rwin->pixmap);
#elif defined(OPENGL_AQUA)
    aglDestroyContext(rwin->contextId);
    aglDestroyAGLPixmap(rwin->windowId); 
    /* TODO FreePixMap */
#elif defined(OPENGL_WGL)
    /* TODO: wglDeleteContext( HRC hrc ) */
#endif

    G_free ((void *) rwin);

    return;
}

/*!
  \brief Create render window

  \param rwin pointer to render_window struct
  \param display display instance (NULL for offscreen)
  \param width window width
  \param height window height

  \return 1
*/
int Nviz_create_render_window(struct render_window *rwin, void *display,
			      int width, int height)
{
#if defined(OPENGL_X11)
    XVisualInfo  *v;
#elif defined(OPENGL_AQUA)
    AGLPixelFmtID v;
#elif defined(OPENGL_WGL)
    int v;
#endif

    int attributeList[] = { GLX_RGBA, GLX_RED_SIZE, 1,
			    GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
			    GLX_DEPTH_SIZE, 1, None };

    /* get the default display connection */
#if defined(OPENGL_X11)
    rwin->displayId = XOpenDisplay((char *) display);
#elif defined(OPENGL_AQUA)
    /* TODO */
#elif defined(OPENGL_WGL)
    /* TODO */
#endif

    if (!rwin->displayId) {
	G_fatal_error (_("Bad server connection"));
    }

    /* get visual info and set up pixmap buffer */
#if defined(OPENGL_X11)
    v = glXChooseVisual(rwin->displayId,
			DefaultScreen(rwin->displayId),
			attributeList);

    rwin->contextId = glXCreateContext(rwin->displayId,
				       v, NULL, GL_FALSE);
#elif defined(OPENGL_AQUA)
    /* TODO */
    rwin->displayId = aglChoosePixelFmt(GDHandle *dev, int ndev, attributeList);
    
    rwin->contextId = aglCreateContext(rwin->display, NULL); 
#elif defined(OPENGL_WGL)
    /* TODO int ChoosePixelFormat( HDC hdc, PIXELFORMATDESCRIPTOR *pfd ) */
#endif

    if (!rwin->contextId) {
	G_fatal_error (_("Unable to create rendering context"));
    }


#if defined(OPENGL_X11)
    /* create win pixmap to render to (same depth as RootWindow) */
    rwin->pixmap = XCreatePixmap(rwin->displayId,
				RootWindow(rwin->displayId, v->screen),
				width,
				height,
				v->depth);

    /* create an off-screen GLX rendering area */
    rwin->windowId = glXCreateGLXPixmap(rwin->displayId,
					v, rwin->pixmap);
#elif defined(OPENGL_AQUA)
    /* create win pixmap to render to (same depth as RootWindow) */
    rwin->pixmap = NULL; /* TODO */
    /* create an off-screen AGL rendering area */
    rwin->windowId = aglCreateAGLPixmap(rwin->displayId,
					rwin->pixmap); 
#elif defined(OPENGL_WGL)
    /* TODO */
#endif

    if (v) {
	XFree(v);
    }

    return 1;
}

/*!
  \brief Make window current for rendering

  \param win pointer to render_window struct

  \return 1 on success
  \return 0 on failure
*/
int Nviz_make_current_render_window(const struct render_window *rwin)
{
    if (!rwin->displayId || !rwin->contextId)
	return 0;

#if defined(OPENGL_X11)
    if (rwin->contextId == glXGetCurrentContext())
	return 1;

    glXMakeCurrent(rwin->displayId, rwin->windowId,
		   rwin->contextId);
#elif defined(OPENGL_AQUA)
    if (rwin->contextId == aglGetCurrentContext())
	return 1;

    aglMakeCurrent(rwin->windowId, rwin->contextId);
#elif defined(OPENGL_WGL)
    /* TODO wglMakeCurrent( HDC hdc, HGLRC hrc ) */
#endif

    return 1;
}
