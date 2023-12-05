#include <assert.h>
/* Override any gcc2 internal prototype to avoid an error.  */
/* We use char because int might match the return type of a gcc2
    builtin and then its argument prototype would still apply.  */
char glXCreatePbuffer();

int main()
{

/* The GNU C library defines this for functions which it implements
    to always fail with ENOSYS.  Some functions are actually named
    something starting with __ and the normal name is an alias.  */
#if defined(__stub_glXCreatePbuffer) || defined(__stub___glXCreatePbuffer)
    choke me
#else
    glXCreatePbuffer();
#endif
        ;
    return 0;
}
