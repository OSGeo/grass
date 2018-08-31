#include <grass/imagery.h>

struct PS_group
{
    struct Ref ref;
    struct Colors colors[3];
    char *name[3];
    char *mapset[3];
    char *group_name;
    int fd[3];
    int do_group;
};

extern struct PS_group grp;
