#ifndef _WHAT_H_
#define _WHAT_H_
#include <grass/gjson.h>

enum OutputFormat {
    PLAIN,
    SHELL,
    LEGACY_JSON /* For backward compatibility with the old JSON format */,
    JSON /* New JSON format */
};

/* what.c */
void what(struct Map_info *, int, char **, double, double, double, int, int,
          int, enum OutputFormat, int, int *, char *, G_JSON_Array *, int);

#endif
