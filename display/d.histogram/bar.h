/*
 * bar.h
 *
 */

/*
 * #include "strings.h"
 */
#include <grass/gis.h>
#include "options.h"
#include "dhist.h"

float rem();			/* remainder function */

/* normalized coordinates of bar-chart components */

/* origin */
#define ORIGIN_X        0.13
#define ORIGIN_Y        0.28

/* y-coordinate of end of y-axis */
#define YAXIS_END       0.85

/* x-coordinate of end of x-axis */
#define XAXIS_END       0.93

/* minimum distance between numbered tic-marks on x-axis */
#define XTIC_DIST	40

/* minimum distance between numbered tic-marks on y-axis */
#define YTIC_DIST	40

/* sizes of tic-marks */
#define BIG_TIC		0.025
#define SMALL_TIC	0.015

/* y-coordinates of the two text labels */
#define LABEL_1         0.10
#define LABEL_2         0.03

/* y-coordinate of x-axis tic-mark numbers */
#define XNUMS_Y		0.20

/* x-coordinate of y-axis tic-mark numbers */
#define YNUMS_X         0.05

/* text width and height */
#define TEXT_HEIGHT	0.05
#define TEXT_WIDTH	TEXT_HEIGHT*0.5

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
