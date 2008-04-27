#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int nrows, ncols;
GLOBAL int buflen;
GLOBAL int direction;
GLOBAL int zero_only;
GLOBAL int preserve_edges;
