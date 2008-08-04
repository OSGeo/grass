/* -*-c-basic-offset: 4;-*-
 *  Chained memory allocator 
 *  memory.h
 *
 *  Pierre de Mouveaux (pmx)  
 *  pmx@audiovu.com  - 10 april 2000.  
 *
 *  Used in GRASS 5.0 r.cost module
 *
 *  Released under GPL
 */

#ifndef  __R_COST_MEMORY__
#define  __R_COST_MEMORY__

#include "cost.h"

int allocate(void);
int release(void);
struct cost *get(void);
int give(struct cost *p);

#endif /* __R_COST_MEMORY__ */
