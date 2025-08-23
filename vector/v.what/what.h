#ifndef _WHAT_H_
#define _WHAT_H_
#include <grass/gjson.h>

enum OutputFormat { PLAIN, SHELL, JSON };

/* what.c */
void what(struct Map_info *, int, char **, double, double, double, int, int,
          int, enum OutputFormat, int, int *, char *, JSON_Array *);

#endif
