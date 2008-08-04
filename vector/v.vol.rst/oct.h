#ifndef __OCT_H__
#define __OCT_H__

#define VOID_T char


struct octfunc
{
    int (*compare) ();
    VOID_T **(*divide_data) ();
    int (*add_data) ();
    int (*intersect) ();
    int (*division_check) ();
    int (*get_points) ();
};

struct octtree
{
    VOID_T *data;
    struct octtree **leafs;
    struct octtree *parent;
    struct octfunc *functions;
    int octant;
};

struct octfunc *OT_functions_new();
struct octtree *OT_tree_new();
int OT_insert_oct();
int OT_region_data();

#endif
