/*
 **  Bitmap library
 **
 **  Written by David Gerdes     12 November 1992
 **  US Army Construction Engineering Research Laboratories
 **
 **
 **  This library provides basic support for the creation and manipulation
 **  of two dimensional bitmap arrays.
 **
 **   struct BM *
 **   BM_create (x, y)                  Create bitmap of specified dimensions
 **
 **   BM_set_mode (mode, size)          Specify Mode and data size in bits.
 **                                     Affects all further calls to BM_create()
 **                                     Mode can be BM_FLAT or BM_SPARSE
 **                                     Size can only be 1 currently.
 **
 **   BM_destroy (map)                  Destroy bitmap and free memory
 **
 **   BM_set (map, x, y, val)           Set array position to val [TRUE/FALSE]
 **
 **   BM_get (map, x, y)                        Return value at array position
 **
 **
 **   BM_file_write (fp, map)           Write bitmap to file
 **
 **   struct BM *
 **   BM_file_read (fp)                 Create bitmap and load from file 
 **
 **   BM_get_map_size (map)             returns size in bytes that bitmap is 
 **                                     taking up.  For diagnosis use.
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/linkm.h>
#include <grass/bitmap.h>


#define BM_col_to_byte(x)  ((x) >> 3)  /* x / 8 */
#define BM_col_to_bit(x)   ((x) & 7)   /* x % 8 */

static int Mode = BM_FLAT;
static int Size = 1;


/*!
 * \brief Create bitmap of dimension x/y and return structure token. 
 *
 * Bitmap is initialized to all zeros
 *
 *  \param x x dimension
 *  \param y y dimension
 *
 *  \return pointer to struct BM
 *  \return NULL on error
 */

struct BM *BM_create(int x, int y)
{
    struct BM *map;

    if (Mode == BM_SPARSE)
	return BM_create_sparse(x, y);

    if (NULL == (map = (struct BM *)malloc(sizeof(struct BM))))
	return (NULL);

    map->bytes = (x + 7) / 8;

    if (NULL ==
	(map->data = (unsigned char *)calloc(map->bytes * y, sizeof(char))))
	return (NULL);

    map->rows = y;
    map->cols = x;
    map->sparse = 0;

    return map;
}


/*!
 * \brief Destroy bitmap and free all associated memory
 *
 *  \param map
 *  \return int returns 0
 */
int BM_destroy(struct BM *map)
{
    if (map->sparse)
	return BM_destroy_sparse(map);

    free(map->data);
    free(map);

    return 0;
}


/*
 **  Caller can specify type of data structure to use for bitmap, as
 **   well as the size of the data values.  Currently since this is
 **   the 'bitmap' library, size can ONLY have value 1.
 **   Size is number of bits of storage per cell.
 **
 ** Mode:
 **  BM_FLAT    Your basic packed bitmap, eight values are stored per byte
 **             Thus you get a 1:8 compression over using char arrays
 **             and a 1:32 compression over using CELL arrays.
 **
 **
 **  BM_SPARSE  Linked array of values.  Much more efficient for large
 **             very sparse arrays.  Slower access, especially for writing,
 **             but can save several orders of magnitude of memory on large
 **             bitmaps since size of FLAT bitmap is O(M*N)
 **
 **  
 **  Returns 0  or negative on error;
 **   If error it will print a warning message to stderr and continue
 **   continue by running but will not change the option in error.
 */

/*!
 * \brief
 *
 * Specify the type of data structure to use for bitmap.
 * 'mode' can be either BM_FLAT or BM_SPARSE:
 *
 * BM_FLAT is a basic packed bitmap - eight values stored per byte
 * thus creating a 1:8 compression over using char arrays and a
 * 1:32 compression over using CELL arrays.
 *
 * BM_SPARSE is a linked array of values. This is much more efficient
 * for large, very sparse arrays.  It is slower to access, especially 
 * for writing, but can save several orders of magnitude of memory on
 * large bitmaps.
 *
 * NOTE: At this time 'size' must be passed a value of 1
 *
 * returns 0 on success or -1 on error
 *
 *  \param mode
 *  \param size
 *  \return int
 */

int BM_set_mode(int mode, int size)
{
    int ret = 0;

    switch (mode) {
    case BM_FLAT:
    case BM_SPARSE:
	Mode = mode;
    default:
	fprintf(stderr, "BM_set_mode:  Unknown mode: %d\n", mode);
	ret--;
    }

    if (size != 1) {
	fprintf(stderr, "BM_set_mode:  Bad size: %d\n", size);
	ret--;
    }
    else
	Size = size;

    return ret;
}


/*!
 * \brief
 *
 * Sets bitmap value to 'val' at location 'x' 'y'
 *
 * Returns 0 on success
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param val
 *  \return int
 */

int BM_set(struct BM *map, int x, int y, int val)
{
    unsigned char byte;

    if (x < 0 || x >= map->cols || y < 0 || y >= map->rows)
	return 0;

    if (map->sparse)
	return BM_set_sparse(map, x, y, val);

    byte = 0x01 << BM_col_to_bit(x);
    if (val)
	map->data[BM_col_to_byte(x) + y * map->bytes] |= byte;
    else
	map->data[BM_col_to_byte(x) + y * map->bytes] &= ~byte;

    return 0;
}


/*!
 * \brief
 *
 * Gets 'val' from the bitmap
 *
 * Returns 0 or 1 on success or -1 on error
 *
 *  \param map
 *  \param x
 *  \param y
 *  \return int
 */

int BM_get(struct BM *map, int x, int y)
{
    unsigned char byte;

    if (x < 0 || x >= map->cols || y < 0 || y >= map->rows)
	return -1;

    if (map->sparse)
	return BM_get_sparse(map, x, y);

    byte = map->data[BM_col_to_byte(x) + y * map->bytes];

    return byte >> BM_col_to_bit(x) & 0x01;
}


/*!
 * \brief
 *
 * Returns size in bytes that bitmap is taking up.
 *
 *  \param map
 *  \return int
 */

size_t BM_get_map_size(struct BM *map)
{
    if (map->sparse)
	return BM_get_map_size_sparse(map);

    return (size_t) map->bytes * map->rows;
}


/*!
 * \brief
 *
 * Write bitmap out to file
 *
 * Expects open file pointer 'fp' and existing map structure.
 * Caller is responsible to open and close 'fp'.
 *
 * Returns 0 or -1 on error
 *
 *  \param fp
 *  \param map
 *  \return int
 */

int BM_file_write(FILE * fp, struct BM *map)
{
    char c;
    int i;

    if (map->sparse)
	return BM_file_write_sparse(fp, map);

    c = BM_MAGIC;
    fwrite(&c, sizeof(char), sizeof(char), fp);

    fwrite(BM_TEXT, BM_TEXT_LEN, sizeof(char), fp);

    c = BM_FLAT;
    fwrite(&c, sizeof(char), sizeof(char), fp);

    fwrite(&(map->rows), sizeof(map->rows), sizeof(char), fp);

    fwrite(&(map->cols), sizeof(map->cols), sizeof(char), fp);

    for (i = 0; i < map->rows; i++)
	if (map->bytes !=
	    fwrite(&(map->data[i * map->bytes]), sizeof(char), map->bytes,
		   fp))
	    return -1;
    fflush(fp);

    return 0;
}


/*!
 * \brief
 *
 * Create map structure and load it from file
 *
 * 'fp' should previously been created by <b>BM_file_write()</b>
 *
 * Returns struct BM * or NULL on error
 *
 *  \param fp
 *  \return struct BM
 */

struct BM *BM_file_read(FILE * fp)
{
    struct BM *map;
    char c;
    char buf[BM_TEXT_LEN + 1];
    int i, y, n;
    struct BMlink *p = NULL, *p2;
    int cnt;

    if (NULL == (map = (struct BM *)malloc(sizeof(struct BM))))
	return (NULL);

    fread(&c, sizeof(char), sizeof(char), fp);
    if (c != BM_MAGIC)
	return NULL;

    fread(buf, BM_TEXT_LEN, sizeof(char), fp);

    fread(&c, sizeof(char), sizeof(char), fp);
    map->sparse = c;


    fread(&(map->rows), sizeof(map->rows), sizeof(char), fp);

    fread(&(map->cols), sizeof(map->cols), sizeof(char), fp);

    map->bytes = (map->cols + 7) / 8;

    if (map->sparse == BM_SPARSE)
	goto readsparse;

    if (NULL == (map->data = (unsigned char *)malloc(map->bytes * map->rows)))
	return (NULL);


    for (i = 0; i < map->rows; i++)
	if (map->bytes !=
	    fread(&(map->data[i * map->bytes]), sizeof(char), map->bytes, fp))
	    return NULL;


    return map;

  readsparse:

    link_set_chunk_size(500);
    map->token = link_init(sizeof(struct BMlink));


    if (NULL == (map->data = (unsigned char *)
		 malloc(sizeof(struct BMlink *) * map->rows)))
	return (NULL);

    for (y = 0; y < map->rows; y++) {
	/* first get number of links */
	fread(&i, sizeof(i), sizeof(char), fp);
	cnt = i;


	/* then read them in */
	for (i = 0; i < cnt; i++) {
	    p2 = (struct BMlink *)link_new(map->token);

	    if (i == 0) {
		((struct BMlink **)(map->data))[y] = p2;
		p = p2;
	    }
	    else {
		p->next = p2;
		p = p2;
	    }

	    fread(&n, sizeof(n), sizeof(char), fp);
	    p->count = n;

	    fread(&n, sizeof(n), sizeof(char), fp);
	    p->val = n;
	    p->next = NULL;
	}
    }

    return map;
}
