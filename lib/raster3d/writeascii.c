#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/G3d.h>

/*!
 * \brief 
 *
 *  Writes the cell-values of <em>map</em> in ascii format to file 
 *  <em>fname</em>. The values are organized by horizontal slices.
 *
 *  \param map
 *  \param fname
 *  \return void
 */

void G3d_writeAscii(void *map, const char *fname)
{
    FILE *fp;
    DCELL d1 = 0;
    DCELL *d1p;
    FCELL *f1p;
    int x, y, z;
    int rows, cols, depths, typeIntern;

    G3d_getCoordsMap(map, &rows, &cols, &depths);
    typeIntern = G3d_tileTypeMap(map);

    d1p = &d1;
    f1p = (FCELL *) &d1;

    if (fname == NULL)
        fp = stdout;
    else if ((fp = fopen(fname, "w")) == NULL)
        G3d_fatalError("G3d_writeAscii: can't open file to write\n");

    for (z = 0; z < depths; z++) {
        for (y = 0; y < rows; y++) {
            fprintf(fp, "z y x %d %d (%d - %d)\n", z, y, 0, cols - 1);
            for (x = 0; x < cols; x++) {
                G3d_getValueRegion(map, x, y, z, d1p, typeIntern);

                if (typeIntern == FCELL_TYPE)
                    fprintf(fp, "%.18f ", *f1p);
                else
                    fprintf(fp, "%.50f ", d1);
            }
            fprintf(fp, "\n");
        }
    }

    if (fp != stdout)
        fclose(fp);
}
