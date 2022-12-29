
#ifndef DISPLAYLIB_CLIP_H
#define DISPLAYLIB_CLIP_H

#include "path.h"

struct plane
{
    double x, y, k;
};

struct clip
{
    struct plane left, rite, bot, top;
};

struct rectangle
{
    double left, rite, bot, top;
};

void D__set_clip_planes(struct clip *, const struct rectangle *);
void D__cull_path(struct path *, const struct path *, const struct clip *);
void D__clip_path(struct path *, const struct path *, const struct clip *);

#endif

