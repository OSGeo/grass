#include <stdio.h>
#include <grass/gis.h>

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
int parse(char *, RULE **, RULE **, struct Categories *);

/* reclass.c */
int reclass(char *, char *, char *, RULE *, struct Categories *, char *);

/* input.c */
int input(FILE *, int, char *);

/* range.c */
int new_range(char *, struct Reclass *);

/* stats.c */
void new_stats(char *, struct Reclass *);
