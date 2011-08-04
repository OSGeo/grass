#include <stdio.h>
#include <grass/gis.h>

enum state
{
    S_TOPLEVEL,
    S_MODULE,
    S_FLAG,
    S_OPTION
};

struct context
{
    struct GModule *module;
    struct Option *option;
    struct Flag *flag;
    struct Option *first_option;
    struct Flag *first_flag;
    int state;
    FILE *fp;
    int line;
};

extern int translate_output;
