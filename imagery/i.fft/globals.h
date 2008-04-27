#ifndef __L_GLOBALS_H__
#define __L_GLOBALS_H__

#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL char Cellmap_real[50], Cellmap_imag[50];

#endif /* __L_GLOBALS_H__ */
