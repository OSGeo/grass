
/****************************************************************************
 *
 * MODULE:       bitmap
 * AUTHOR(S):    David Gerdes (CERL) (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      provides basic support for the creation and manipulation of
 *               two dimensional bitmap arrays
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <grass/bitmap.h>


static int dump_map(struct BM *map);


int main(int argc, char *argv[])
{
    int SIZE;
    struct BM *map, *map2;
    int i, x, y;
    int dump;
    FILE *fp;

    if (argc < 2)
	SIZE = 11;
    else
	SIZE = atoi(argv[1]);

    if (NULL != getenv("NODUMP"))
	dump = 0;
    else
	dump = 1;

    map = BM_create(SIZE, SIZE);

    /* turn on bits in X pattern */
    for (i = 0; i < SIZE; i++) {
	BM_set(map, i, i, 1);
	BM_set(map, (SIZE - 1) - i, i, 1);
    }


    if (dump)
	dump_map(map);
    fprintf(stdout, "Size = %d\n", BM_get_map_size(map));

    fprintf(stdout, "\n\n");

    /* now invert it */
    for (y = 0; y < SIZE; y++)
	for (x = 0; x < SIZE; x++)
	    BM_set(map, x, y, !BM_get(map, x, y));

    if (dump)
	dump_map(map);

    fprintf(stdout, "Size = %d\n", BM_get_map_size(map));

    {
	fp = fopen("dumpfile", "w");
	BM_file_write(fp, map);
	fclose(fp);

	fp = fopen("dumpfile", "r");
	map2 = BM_file_read(fp);
	fclose(fp);
	dump_map(map2);
    }

    BM_destroy(map);
    BM_destroy(map2);

    return 0;
}


static int dump_map(struct BM *map)
{
    int x, y;

    for (y = 0; y < map->rows; y++) {
	for (x = 0; x < map->cols; x++) {
	    fprintf(stdout, "%c", BM_get(map, x, y) ? '#' : '.');

	}
	fprintf(stdout, "\n");
    }
}
