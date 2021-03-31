/*!
  \file include/vect/dig_macros.h

  \brief Macros for diglib (part of vector library
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
