#ifndef __LOCAL_PROTO__
#define __LOCAL_PROTO__

/* formatting macros for compilation date */
#define YEAR ((((__DATE__ [7]-'0')*10+(__DATE__[8]-'0'))*10+\
	       (__DATE__ [9]-'0'))*10+(__DATE__ [10]-'0'))

/* month: 0 - 11 */
#define MONTH (__DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 0 : 5) \
              : __DATE__ [2] == 'b' ? 1 \
              : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 2 : 3) \
              : __DATE__ [2] == 'y' ? 4 \
              : __DATE__ [2] == 'l' ? 6 \
              : __DATE__ [2] == 'g' ? 7 \
              : __DATE__ [2] == 'p' ? 8 \
              : __DATE__ [2] == 't' ? 9 \
              : __DATE__ [2] == 'v' ? 10 : 11)

#define DAY ((__DATE__ [4]==' ' ? 0 : __DATE__[4]-'0')*10+(__DATE__[5]-'0'))

#endif
