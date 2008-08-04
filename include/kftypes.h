#ifndef _KFTYPES_H
#define _KFTYPES_H

#ifndef _KEYFRAME_H
#include <grass/keyframe.h>
#endif

#define KF_FROMX 0
#define KF_FROMY 1
#define KF_FROMZ 2
#define KF_DIRX 3
#define KF_DIRY 4
#define KF_DIRZ 5
#define KF_FOV 6
#define KF_TWIST 7

#define FM_VECT 0x00000001
#define FM_SITE 0x00000002
#define FM_PATH 0x00000004
#define FM_VOL  0x00000008
#define FM_LABEL 0x00000010

typedef struct view_node
{
    float fields[KF_NUMFIELDS];
} Viewnode;

typedef struct key_node
{
    float pos, fields[KF_NUMFIELDS];
    int look_ahead;
    unsigned long fieldmask;
    struct key_node *next, *prior;
} Keylist;

#endif /* _KFTYPES_H */
