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

    if (NULL == getenv("NODUMP"))
	dump = 1;
    else
	dump = 0;

    map = BM_create_sparse(SIZE, SIZE);

    /* turn on bits in X pattern */
    for (i = 0; i < SIZE; i++) {
	BM_set(map, i, i, 1);
	BM_set(map, (SIZE - 1) - i, i, 1);
    }

    if (dump)
	dump_map(map);

    fprintf(stdout, "Size = %d\n", BM_get_map_size(map));

    /*
       BM_dump_map_sparse (map);
     */

    fprintf(stdout, "\n\n");
    /* now invert it */
    for (y = 0; y < SIZE; y++)
	for (x = 0; x < SIZE; x++) {
	    BM_set(map, x, y, !BM_get(map, x, y));
#ifdef FOO
	     /*DEBUG*/ if (y == 0)
		/*DEBUG*/ BM_dump_map_row_sparse(map, y);
#endif
	}

    if (dump)
	dump_map(map);

    fprintf(stdout, "Size = %d\n", BM_get_map_size(map));
    /*
       fprintf (stdout, "\n\n");
       BM_dump_map_sparse (map);
     */
    {
	fp = fopen("dumpfile", "w");
	if (0 > BM_file_write(fp, map)) {
	    fprintf(stderr, "File_write failed\n");
	    goto nowrite;
	}
	fclose(fp);

	fp = fopen("dumpfile", "r");
	map2 = BM_file_read(fp);
	fclose(fp);
	dump_map(map2);
    }

    BM_destroy(map2);
  nowrite:

    BM_destroy(map);

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
