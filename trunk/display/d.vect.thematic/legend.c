/* Author: Adam Laza, GSoC 2016
 *
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/symbol.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "plot.h"
#include "local_proto.h"

void write_into_legend_file(const char *legfile, const char *icon, const char *title, double stats_min, double stats_max,
                            double *breakpoints, int nbreaks, int size, struct color_rgb bcolor,
                            struct color_rgb *colors, int default_width, int* frequencies, const char *topo)
{
    FILE *fd;
    int i;

    if (strcmp(legfile,"stdout") == 0)
        fd = stdout;
    else
        fd = fopen(legfile, "a");

    /* Title */
    fprintf(fd, "||||||%s\n", title);

    /* First line */
    if (stats_min > breakpoints[0]){
        fprintf(fd, "< %.2f|", breakpoints[0]);
    }
    else{
        fprintf(fd, "%.2f - %.2f|",stats_min, breakpoints[0]);
    }
    fprintf(fd, "%s|%d|ps|%d:%d:%d|%d:%d:%d|%d|%s|%d\n",
            icon, size, colors[0].r, colors[0].g, colors[0].b,
            bcolor.r, bcolor.g, bcolor.b, default_width,
            topo, frequencies[0]);
    /* Middle lines */
    for (i = 1; i < nbreaks; i++){
        fprintf(fd, "%.2f - %.2f|%s|%d|ps|%d:%d:%d|%d:%d:%d|%d|%s|%d\n",
                breakpoints[i-1],breakpoints[i],
                icon, size, colors[i].r, colors[i].g, colors[i].b,
                bcolor.r, bcolor.g, bcolor.b, default_width,
                topo, frequencies[i]);
    }
    /* Last one */
    if(stats_max < breakpoints[nbreaks-1]){
        fprintf(fd, ">%.2f|", breakpoints[nbreaks - 1]);
    }
    else {
        fprintf(fd, "%.2f - %.2f|", breakpoints[nbreaks - 1], stats_max);
    }
    fprintf(fd, "%s|%d|ps|%d:%d:%d|%d:%d:%d|%d|%s|%d\n",
            icon, size, colors[nbreaks].r, colors[nbreaks].g, colors[nbreaks].b,
            bcolor.r, bcolor.g, bcolor.b, default_width,
            topo, frequencies[nbreaks]);

    fclose(fd);
}
