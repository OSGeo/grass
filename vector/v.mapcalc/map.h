#ifndef MAP_H
#define MAP_H

typedef struct Map_info Map_info;


typedef struct Map
{
    struct Map *next;
    char *name;
    Map_info *mapinfo;
    int refcnt;
} MAP;

extern void init_map(void);
extern void showmap(SYMBOL * map);
extern void setmap(SYMBOL * var, SYMBOL * map);
extern SYMBOL *mkmapvar(SYMBOL * var, SYMBOL * map);
extern SYMBOL *mapfunc(SYMBOL * func, SYMBOL * arglist);
extern SYMBOL *mapop(int op, SYMBOL * map1, SYMBOL * map2);

#endif
