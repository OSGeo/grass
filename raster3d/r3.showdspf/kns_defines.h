#ifndef MOTIFOGL
#define MOTIFOGL
#include <GL/glx.h>
typedef struct window_props
{
    GLXContext glx_context;
    Window window;
    Widget widget;
    int width;
    int height;
    int left_button_status;
    int middle_button_status;
    int right_button_hit;
    struct dspec *ptr_D_spec;
} OGLMotifWindowData;
#endif
