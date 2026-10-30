#ifndef __GLOBALS_H_
#define __GLOBALS_H_

#include <stdint.h>

extern int overwrite_flag;
extern int64_t seed_value;
extern long seeded;
extern int rand_calls;
extern int region_approach;

extern int current_depth;
extern int *current_row;
extern int depths, rows;

#endif /* __GLOBALS_H_ */
