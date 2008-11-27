
/*************************************************
 *
 *   costHa.h - Data structure for costHa (spread)
 *
 *   It is used for (heap) sorting the
 *   cumulative time, it's a contiguous structure.
 *
 *************************************************/
#ifndef COSTHA_H
#define COSTHA_H 1
struct costHa
{
    float min_cost, angle;
    int row, col;
};
#endif
