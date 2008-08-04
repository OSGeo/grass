/* Architecture: i386-pc-mingw32msvc */

/* Native machine sizes */
#define NATIVE_DOUBLE 8
#define NATIVE_FLOAT  4
#define NATIVE_LONG   4
#define NATIVE_INT    4
#define NATIVE_SHORT  2
#define NATIVE_CHAR   1

/* Native machine byte orders */
#define DOUBLE_ORDER 0
#define FLOAT_ORDER  0
#define LONG_ORDER   0
#define INT_ORDER    0
#define SHORT_ORDER  0


/* Translation matrices from big endian to native */

/* Double format: */
static int dbl_cnvrt[] = { 7, 6, 5, 4, 3, 2, 1, 0 };

/* Float format : */
static int flt_cnvrt[] = { 3, 2, 1, 0 };

/* Long format  : */
static int lng_cnvrt[] = { 3, 2, 1, 0 };

/* Int format  : */
static int int_cnvrt[] = { 3, 2, 1, 0 };

/* Short format : */
static int shrt_cnvrt[] = { 1, 0 };
