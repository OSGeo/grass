#include <string.h>

/* location of graph origin in terms of % of screen */

#define ORIGIN_X        0.10
#define ORIGIN_Y        0.20

/* y-coordinate of end of y-axis */

#define YAXIS_END       0.86

/* x-coordinate of end of x-axis */

#define XAXIS_END       0.90

/* minimum distance between numbered tic-marks on x-axis */

#define XTIC_DIST	20

/* minimum distance between numbered tic-marks on y-axis */

#define YTIC_DIST	20

/* sizes of tic-marks */

#define BIG_TIC		0.025
#define SMALL_TIC	0.015

/* y-coordinate of the x axis label */

#define LABEL_1         0.07

/* x-coordinate of the y axis label */

#define LABEL_2         0.02

/* y-coordinate of x-axis tic-mark numbers */

#define XNUMS_Y		0.14

/* x-coordinate of y-axis tic-mark numbers */

#define YNUMS_X         0.05

/* text width and height */

#define TEXT_HEIGHT	0.04
#define TEXT_WIDTH	TEXT_HEIGHT*0.5

/* structures for determining tic-mark numbering scheme */
struct units
{
    char *name;                 /* name of unit (text) */
    int unit;                   /* tic-mark interval */
    int every;                  /* tic_mark number interval */
};


struct units tics[] = {
    {"", 1, 2},
    {"", 1, 5},
    {"in tens", 10, 10},
    {"in tens", 10, 20},
    {"in tens", 10, 50},
    {"in hundreds", 100, 100},
    {"in hundreds", 100, 500},
    {"in thousands", 1000, 1000},
    {"in thousands", 1000, 5000},
    {"in thousands", 1000, 10000},
    {"in thousands", 1000, 50000},
    {"in tens of thousands", 10000, 10000},
    {"in tens of thousands", 10000, 20000},
    {"in tens of thousands", 10000, 50000},
    {"in hundreds of thousands", 100000, 100000},
    {"in hundreds of thousands", 100000, 200000},
    {"in hundreds of thousands", 100000, 500000},
    {"in millions", 1000000, 1000000},
    {"in millions", 1000000, 2000000},
    {"in millions", 1000000, 5000000},
    {"in tens of millions", 10000000, 10000000},
    {"in tens of millions", 10000000, 20000000},
    {"in tens of millions", 10000000, 50000000},
    {"in hundreds of millions", 100000000, 100000000},
    {"in hundreds of millions", 100000000, 200000000},
    {"in hundreds of millions", 100000000, 500000000},
    {"in billions", 1000000000, 1000000000}
};
