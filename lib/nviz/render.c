/*!
  \file render.c
 
  \brief Nviz library -- GLX context manipulation
  
  COPYRIGHT: (C) 2008 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.

  Based on visualization/nviz/src/togl.c etc.

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
    glXDestroyContext(rwin->displayId, rwin->contextId);
    XFreePixmap(rwin->displayId, rwin->pixmap);

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
    XVisualInfo  *v;

    int attributeList[] = { GLX_RGBA, GLX_RED_SIZE, 1,
			    GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
			    GLX_DEPTH_SIZE, 1, None };

    /* get the default display connection */
    rwin->displayId = XOpenDisplay((char *) display);
    if (!rwin->displayId) {
	G_fatal_error (_("Bad X server connection"));
    }

    /* get visual info and set up pixmap buffer */
    v = glXChooseVisual(rwin->displayId,
			DefaultScreen(rwin->displayId),
			attributeList);

    rwin->contextId = glXCreateContext(rwin->displayId,
				      v, NULL, GL_FALSE);
    if (!rwin->contextId) {
	G_fatal_error (_("Unable to create GLX rendering context"));
    }

    /* create win pixmap to render to (same depth as RootWindow) */
    rwin->pixmap = XCreatePixmap(rwin->displayId,
				RootWindow(rwin->displayId, v->screen),
				width,
				height,
				v->depth);

    /* create an off-screen GLX rendering area */
    rwin->windowId = glXCreateGLXPixmap(rwin->displayId,
					v, rwin->pixmap);

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

    if (rwin->contextId == glXGetCurrentContext())
	return 1;

    glXMakeCurrent(rwin->displayId, rwin->windowId,
		   rwin->contextId);

    /* TODO: AQUA */

    return 1;
}
