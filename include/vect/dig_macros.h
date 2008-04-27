/* convert UTM to screen */
#define XADJ(x)		(int)((x - U_west ) * U_to_D_xconv + D_west)
#define YADJ(y)		(int)((y - U_south) * U_to_D_yconv + D_south)

#define _TOGGLE(x) ((x) ? 0 : 1)
#define TOGGLE(x)  ((x) = _TOGGLE (x))
#define BEEP	   putchar ('\007')
#define ON_OFF(x)  ((x) ? "        ON" : "       OFF")
/*
   #define ABS(x)          ((x) < 0 ? -(x) : (x))
 */

/* ALIVE MACROS take a pointer the the structure in question */
/*  and return 0 or non-zero */
#define LINE_ALIVE(p) ((p)->type<16)	/* assume DEAD are .GT. 1 << 3 */
#define NODE_ALIVE(p) ((p)->alive)	/* simple enuf                         */
#define AREA_LABELED(p) ((p)->alive && (p)->att)
#define LINE_LABELED(p) (LINE_ALIVE (p) && (p)->att)
#define AREA_ALIVE(p) ((p)->alive)
#define ISLE_ALIVE(p) ((p)->alive)
#define ATT_ALIVE(p)  ((p)->type<16)	/* see LINE_ALIVE */

#define LESSER(x,y)  ((x) < (y) ? (x) : (y))
#define GREATER(x,y) ((x) > (y) ? (x) : (y))


/* temporary till 3.0 */
/*#define R_standard_color(x) R_color (x) */
