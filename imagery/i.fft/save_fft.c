#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"


int save_fft (int total, double *data[2], double *maximum, double *minimum)
{
        int i;
        double max, min, *temp;
        FILE *fp;

        max = *maximum;
        min = *minimum;

        if ((fp = G_fopen_new_misc ("cell_misc", "fftreal", Cellmap_real)) == NULL)
                G_fatal_error(_("Unable to open file in the cell_misc directory."));
        fwrite((char *) data[0], sizeof(double), (size_t)total, fp);
        fclose(fp);

        if ((fp = G_fopen_new_misc ("cell_misc", "fftimag", Cellmap_imag)) == NULL)
                G_fatal_error(_("Unable to open file in the cell_misc directory."));
        fwrite((char *) data[1], sizeof(double), (size_t)total, fp);
        fclose(fp);

        temp = data[0] ;
        for (i=0; i<total; i++, temp++) {
                max = (max > *temp) ? max : *temp;
                min = (min < *temp) ? min : *temp;
        }

        temp = data[1] ;
        for (i=0; i<total; i++, temp++) {
                max = (max > *temp) ? max : *temp;
                min = (min < *temp) ? min : *temp;
        }

        *maximum = max;
        *minimum = min;

	return 0;
}
