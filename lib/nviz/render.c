/*!
   \file lib/nviz/render.c

   \brief Nviz library -- GLX context manipulation

   Based on visualization/nviz/src/togl.c

   (C) 2008, 2010, 2018 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
   \author Support for framebuffer objects by Huidae Cho <grass4u gmail.com> (July 2018)
 */

#include <grass/glocale.h>
#include <grass/nviz.h>

#if defined(OPENGL_WINDOWS) && defined(OPENGL_FBO)
static int gl_funcs_found = 0;
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
static PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
static PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
static PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;

/* https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions */
static void *GetAnyGLFuncAddress(const char *name)
{
    void *p = (void *)wglGetProcAddress(name);
    if (p == 0 || p == (void*)0x1 || p == (void*)0x2 || p == (void*)0x3 ||
	    p == (void*)-1) {
	HMODULE module = LoadLibraryA("opengl32.dll");
	p = (void *)GetProcAddress(module, name);
    }
    if (!p)
	G_fatal_error(_("Unable to get function address for %s"), name);
    return p;
}

static void find_gl_funcs()
{
    if (gl_funcs_found)
	return;

    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GetAnyGLFuncAddress("glGenFramebuffers");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GetAnyGLFuncAddress("glBindFramebuffer");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)GetAnyGLFuncAddress("glGenRenderbuffers");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)GetAnyGLFuncAddress("glBindRenderbuffer");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)GetAnyGLFuncAddress("glRenderbufferStorage");
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)GetAnyGLFuncAddress("glFramebufferRenderbuffer");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)GetAnyGLFuncAddress("glCheckFramebufferStatus");

    gl_funcs_found = 1;
}
#endif

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
#if defined(OPENGL_AGL)
    rwin->pixelFmtId = NULL;
    rwin->contextId = NULL;
    rwin->windowId = NULL;
#else
    rwin->contextId = NULL;
#endif
#elif defined(OPENGL_WINDOWS)
    rwin->displayId = NULL;
    rwin->contextId = NULL;
#endif

    rwin->width = 0;
    rwin->height = 0;
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
#if defined(OPENGL_AGL)
    aglDestroyPixelFormat(rwin->pixelFmtId);
    aglDestroyContext(rwin->contextId);
    aglDestroyPBuffer(rwin->windowId);
#else
    CGLDestroyContext(rwin->contextId);
#endif
#elif defined(OPENGL_WINDOWS)
    wglDeleteContext(rwin->contextId);
    DeleteDC(rwin->displayId);
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
    int attributeList[] = {
	GLX_RGBA,
	GLX_RED_SIZE, 1,
	GLX_GREEN_SIZE, 1,
	GLX_BLUE_SIZE, 1,
	GLX_DEPTH_SIZE, 1,
#if !defined(OPENGL_FBO)
	GLX_DOUBLEBUFFER,
#endif
	None
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

    rwin->contextId = glXCreateContext(rwin->displayId, v, NULL, GL_TRUE);

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
#if defined(OPENGL_AGL)
    int attributeList[] = {
	AGL_RGBA,
	AGL_RED_SIZE, 1,
	AGL_GREEN_SIZE, 1,
	AGL_BLUE_SIZE, 1,
	AGL_DEPTH_SIZE, 1,
#if !defined(OPENGL_FBO)
	AGL_DOUBLEBUFFER,
#endif
	AGL_NONE
    };

    /* TODO: open mac display */

    /* TODO: dev = NULL, ndev = 0 ? */
    rwin->pixelFmtId = aglChoosePixelFormat(NULL, 0, attributeList);

    rwin->contextId = aglCreateContext(rwin->pixelFmtId, NULL);

    /* create an off-screen AGL rendering area */
    aglCreatePBuffer(width, height, GL_TEXTURE_2D, GL_RGBA, 0, &(rwin->windowId));
    aglSetPBuffer(rwin->contextId, rwin->windowId, 0, 0, 0);
#else
    CGLPixelFormatAttribute attributeList[] = {
	kCGLPFAColorSize, 24,
	kCGLPFADepthSize, 32,
	(CGLPixelFormatAttribute) 0
    };
    CGLPixelFormatObj pix;
    GLint nvirt;
    CGLError error;

    error = CGLChoosePixelFormat(attributeList, &pix, &nvirt);
    if (error) {
	G_warning(_("Unable to choose pixel format (CGL error = %d)"), error);
	return -1;
    }

    error = CGLCreateContext(pix, NULL, &rwin->contextId);
    if (error) {
	G_warning(_("Unable to create context (CGL error = %d)"), error);
	return -1;
    }

    CGLDestroyPixelFormat(pix);
#endif
#elif defined(OPENGL_WINDOWS)
    WNDCLASS wc = {0};
    HWND hWnd;
    PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	/* size of this pfd	    */
	1,				/* version number	    */
	PFD_DRAW_TO_WINDOW |		/* support window	    */
	PFD_SUPPORT_OPENGL |		/* support OpenGL	    */
	PFD_DOUBLEBUFFER,		/* double buffered	    */
	PFD_TYPE_RGBA,			/* RGBA type		    */
	24,				/* 24-bit color depth	    */
	0, 0, 0, 0, 0, 0,		/* color bits ignored	    */
	0,				/* no alpha buffer	    */
	0,				/* shift bit ignored	    */
	0,				/* no accumulation buffer   */
	0, 0, 0, 0,			/* accum bits ignored	    */
	32,				/* 32-bit z-buffer	    */
	0,				/* no stencil buffer	    */
	0,				/* no auxiliary buffer	    */
	PFD_MAIN_PLANE,			/* main layer		    */
	0,				/* reserved		    */
	0, 0, 0				/* layer masks ignored	    */
    };
    int iPixelFormat;

    wc.lpfnWndProc = DefWindowProc;
    wc.lpszClassName = "nviz";

    if (!RegisterClass(&wc)) {
	G_warning(_("Unable to register window class"));
	return -1;
    }

    hWnd = CreateWindow(wc.lpszClassName, wc.lpszClassName, WS_POPUP,
	    CW_USEDEFAULT, CW_USEDEFAULT, width, height,
	    NULL, NULL, wc.hInstance, NULL);

    if (!hWnd) {
	G_warning(_("Unable to create window"));
	return -1;
    }

    rwin->displayId = GetDC(hWnd);
    iPixelFormat = ChoosePixelFormat(rwin->displayId, &pfd);
    SetPixelFormat(rwin->displayId, iPixelFormat, &pfd);
    rwin->contextId = wglCreateContext(rwin->displayId);
#endif

    rwin->width = width;
    rwin->height = height;

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
#if defined(OPENGL_AGL)
    if (!rwin->contextId)
	return 0;

    if (rwin->contextId == aglGetCurrentContext())
	return 1;

    aglSetCurrentContext(rwin->contextId);
#else
    CGLError error;

    error = CGLSetCurrentContext(rwin->contextId);
    if (error) {
	G_warning(_("Unable to set current context (CGL error = %d)"), error);
	return 0;
    }
#endif
#elif defined(OPENGL_WINDOWS)
    if (!rwin->displayId || !rwin->contextId)
	return 0;

    wglMakeCurrent(rwin->displayId, rwin->contextId);
#endif

#if defined(OPENGL_FBO)
#if defined(OPENGL_WINDOWS)
    find_gl_funcs();
#endif

    GLuint framebuf, renderbuf, depthbuf;
    GLenum status;

    glGenFramebuffers(1, &framebuf);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuf);

    glGenRenderbuffers(1, &renderbuf);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8,
	    rwin->width, rwin->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	    GL_RENDERBUFFER, renderbuf);

    glGenRenderbuffers(1, &depthbuf);
    glBindRenderbuffer(GL_RENDERBUFFER, depthbuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
	    rwin->width, rwin->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
	    GL_RENDERBUFFER, depthbuf);

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
	G_warning(_("Incomplete framebuffer status (status = %d)"), status);
	return 0;
    }
#endif

    glViewport(0, 0, rwin->width, rwin->height);

    return 1;
}
