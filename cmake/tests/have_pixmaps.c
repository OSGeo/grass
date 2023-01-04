/* System header to define __stub macros and hopefully few prototypes,
    which can conflict with char glXCreateGLXPixmap(); below.  */
#include <assert.h>
/* Override any gcc2 internal prototype to avoid an error.  */
/* We use char because int might match the return type of a gcc2
    builtin and then its argument prototype would still apply.  */
char glXCreateGLXPixmap();

int main()
{

/* The GNU C library defines this for functions which it implements
    to always fail with ENOSYS.  Some functions are actually named
    something starting with __ and the normal name is an alias.  */
#if defined(__stub_glXCreateGLXPixmap) || defined(__stub___glXCreateGLXPixmap)
    choke me
#else
    glXCreateGLXPixmap();
#endif

        ;
    return 0;
}
