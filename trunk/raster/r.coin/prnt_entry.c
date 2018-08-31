
/****************************************************************************
 *
 * MODULE:       r.coin
 *
 * AUTHOR(S):    Michael O'Shea - CERL
 *               Michael Shapiro - CERL
 *
 * PURPOSE:      Calculates the coincidence of two raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include "coin.h"


#define F_CTOK(C)	((double)(C))/1000000.0
	/* sq km -> sq miles = 1000^2 / (0.0254 * 12 * 5280)^2 */
#define F_CTOM(C)	F_CTOK(C) *   0.386102158542446
	/* sq km -> acres = sq miles * 640 */
#define F_CTOA(C)	F_CTOK(C) * 247.105381467165
#define F_CTOH(C)	F_CTOK(C) * 100.0000

#define F_CTOP(C,R) ((int)R) ? (double)C / (double)R * 100.0 : 0.0
#define F_CTOX(C,R) ((int)R) ? (double)C / (double)R * 100.0 : 0.0
#define F_CTOY(C,R) ((int)R) ? (double)C / (double)R * 100.0 : 0.0

int print_entry(int Conformat, long count, double area)
{
    long total_count;
    double total_area;

    switch (Conformat) {
    case 'a':			/* acres */
	print_area(F_CTOA(area));
	break;
    case 'h':			/* hectares */
	print_area(F_CTOH(area));
	break;
    case 'k':			/* square km */
	print_area(F_CTOK(area));
	break;
    case 'm':			/* square miles */
	print_area(F_CTOM(area));
	break;
    case 'p':
	print_percent(F_CTOP(area, window_area));
	break;
    case 'x':
	col_total(Cndex, 1, &total_count, &total_area);
	print_percent(F_CTOX(area, total_area));
	break;
    case 'y':
	row_total(Rndex, 1, &total_count, &total_area);
	print_percent(F_CTOY(area, total_area));
	break;
    default:			/* case 'c' */
	fprintf(dumpfile, " %9ld |", count);
	break;
    }

    return 0;
}

int print_area(double value)
{
    char buf[20];

    format_double(value, buf, 9);
    fprintf(dumpfile, " %9s |", buf);

    return 0;
}

int print_percent(double value)
{
    fprintf(dumpfile, " %9.2f |", value);

    return 0;
}
