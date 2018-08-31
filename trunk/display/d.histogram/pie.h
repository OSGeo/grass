/*
 * pie.h
 *
 */

/* not available in SysV 
 * #include "strings.h"
 */
#include <grass/gis.h>
#include "options.h"
#include "dhist.h"

/*******************************************************************
 *
 *
 *  <cell-file name> in mapset <mapset name>     <---- the title
 *
 *                    ......
 *                 ............
 *               .................
 *             .....................
 *           ........................
 *           ........................
 *           ...........*............            <----- the pie
 *           ........................
 *           ........................
 *             ....................
 *               ................
 *                 ............
 *                   .......
 * 
 *   [][][][][][][][][][][][][][][][][][][][]    <---- the category  
 *   ---+----+----+----+----+----+----+----+-        number legend
 *      n1   n2   n3   n4                  nn
 *
 *   Category values in <tic-mark number units>  <--- legend label
 *
 *
 *******************************************************************
 */

float rem();			/* remainder function */

/* normalized coordinates & dimensions of pie-chart components */

/* origin */
#define ORIGIN_X	0.5
#define ORIGIN_Y	0.59

#define BAR_X1		0.13
#define BAR_X2		0.87
#define BAR_Y1		0.17
#define BAR_Y2		0.23

/* radius of pie */
#define RADIUS		0.25

/* height of legend "color-bar" */
#define BAR_HEIGHT	BAR_Y2-BAR_Y1

/* minimum distance between numbered tic-marks on legend */
#define XTIC_DIST	40

/* sizes of legend tic-marks */
#define BIG_TIC		0.025
#define SMALL_TIC	0.015

/* y-coordinate of legend label */
#define LABEL		0.03

/* y-coordinate of legend tic-mark numbers */
#define XNUMS_Y         0.09

/* text width and height */
#define TEXT_HEIGHT	0.05
#define TEXT_WIDTH	TEXT_HEIGHT*0.5

extern struct units tics[];
