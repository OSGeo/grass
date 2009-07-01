#include <stdio.h>
#include <grass/raster.h>

#define RULE struct _rule_
RULE {
    CELL new;
    CELL lo;
    CELL hi;
    RULE *next;
};

/* add_rule.c */
int add_rule(RULE **, CELL, CELL, CELL);

/* parse.c */
int parse(const char *, RULE **, RULE **, struct Categories *);

/* reclass.c */
int reclass(const char *, const char *, const char *, RULE *, struct Categories *, const char *);

/* input.c */
int input(FILE *, int, const char *);

/* range.c */
int new_range(const char *, struct Reclass *);

/* stats.c */
void new_stats(const char *, struct Reclass *);
