#include "png.h"		/* includes zlib.h and setjmp.h */


typedef struct _jmpbuf_wrapper
{
    jmp_buf jmpbuf;
} jmpbuf_wrapper;

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#ifndef NONE
#  define NONE 0
#endif
#define MAXCOLORS 256
#define MAXCOMMENTS 256

/* function prototypes */
static void pnmtopng_error_handler(png_structp png_ptr, png_const_charp msg);

#if 0
/* unused */
static int filter = -1;
#endif
static jmpbuf_wrapper pnmtopng_jmpbuf_struct;

typedef unsigned char xelval;
