#ifndef KNS_GLOBALS_DEFINED
#define KNS_GLOBALS_DEFINED

#ifdef MAIN
#define DECLARATION 
#else
#define DECLARATION extern
#endif

DECLARATION GLuint Material_1_Dlist;
DECLARATION OGLMotifWindowData MainOGLWindow;
DECLARATION OGLMotifWindowData ColormapWindow;
DECLARATION GLuint MainDlist;
DECLARATION XtAppContext App_context;

#endif
