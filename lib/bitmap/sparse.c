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
 **   BM_create (x, y)                  Create bitmap of specified dimensions
 **
 **   BM_destroy (map)                  Destroy bitmap and free memory
 **
 **   BM_set (map, x, y, val)           Set array position to val [TRUE/FALSE]
 **
 **   BM_get (map, x, y)                        Return value at array position
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/linkm.h>
#include <grass/bitmap.h>


#define BM_col_to_byte(x)  ((x) >> 3)  /* x / 8 */
#define BM_col_to_bit(x)   ((x) & 7)   /* x % 8 */

static int depth;


/*!
 * \brief
 *
 * Create a sparse bitmap of dimension 'x'/'y'
 *
 * Returns bitmap structure or NULL on error
 *
 *  \param x
 *  \param y
 *  \return struct BM 
 */

struct BM *BM_create_sparse(int x, int y)
{
    struct BM *map;
    int i;

    if (NULL == (map = (struct BM *)malloc(sizeof(struct BM))))
	return (NULL);
    map->bytes = (x + 7) / 8;

    if (NULL == (map->data = (unsigned char *)
		 malloc(sizeof(struct BMlink *) * y)))
	return (NULL);

    map->rows = y;
    map->cols = x;
    map->sparse = 1;

    link_set_chunk_size(500);
    map->token = link_init(sizeof(struct BMlink));

    for (i = 0; i < map->rows; i++) {
	((struct BMlink **)(map->data))[i] =
	    (struct BMlink *)link_new(map->token);
	((struct BMlink **)(map->data))[i]->count = x;
	((struct BMlink **)(map->data))[i]->val = 0;
	((struct BMlink **)(map->data))[i]->next = NULL;
    }


    depth++;

    return map;
}


/*!
 * \brief
 *
 * Destroy sparse bitmap and free all associated memory
 *
 * Returns 0
 *
 *  \param map
 *  \return int
 */

int BM_destroy_sparse(struct BM *map)
{
    int i;
    struct BMlink *p, *tmp;

    for (i = 0; i < map->rows; i++) {
	p = ((struct BMlink **)(map->data))[i];
	while (p != NULL) {
	    tmp = p->next;
	    link_dispose(map->token, (VOID_T *) p);
	    p = tmp;
	}
    }

    if (--depth == 0)
	link_cleanup(map->token);

    free(map->data);
    free(map);

    return 0;
}


/*!
 * \brief
 *
 * Set sparse bitmap value to 'val' at location 'x'/'y'
 *
 * Returns 0
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param val
 *  \return int
 */

int BM_set_sparse(struct BM *map, int x, int y, int val)
{
    struct BMlink *p, *p2, *prev;
    int cur_x = 0;
    int Tcount, Tval;
    int dist_a, dist_b;

    val = !(!val);		/* set val == 1 or 0 */

    p = ((struct BMlink **)(map->data))[y];
    prev = NULL;
    while (p != NULL) {
	if (cur_x + p->count > x) {
	    if (p->val == val)	/* no change */
		return 0;

	    Tcount = p->count;	/* save current state */
	    Tval = p->val;

	    /* if x is on edge, then we probably want to merge it with 
	     ** its neighbor for efficiency 
	     */

	    /* dist_a is how far x is from Left  edge of group */
	    /* dist_b is how far x is from right edge of group */

	    dist_a = x - cur_x;
	    dist_b = (cur_x + p->count - 1) - x;

	    /* if on both edges, then we should be able to  merge 3 into one */
	    if (dist_b == 0 && p->next && p->next->val == val) {
		if (dist_a == 0 && x > 0) {
		    if (prev != NULL && prev->val == val) {
			prev->count += p->next->count + 1;
			prev->next = p->next->next;
			link_dispose(map->token, (VOID_T *) p->next);
			link_dispose(map->token, (VOID_T *) p);
			return 0;
		    }
		}
	    }

	    /* handle right edge merge */
	    if (dist_b == 0 && p->next && p->next->val == val) {
		p->count--;
		p->next->count++;
		if (p->count == 0) {
		    if (prev) {
			prev->next = p->next;
		    }
		    else {
			((struct BMlink **)(map->data))[y] = p->next;
		    }
		    link_dispose(map->token, (VOID_T *) p);
		}
		return 0;
	    }

	    /* handle left edge merge */
	    if (dist_a == 0 && x > 0) {

		if (prev != NULL && prev->val == val) {
		    prev->count++;
		    p->count--;
		    if (p->count == 0) {
			prev->next = p->next;
			link_dispose(map->token, (VOID_T *) p);
		    }
		    return 0;
		}
	    }

	    /* if not on edge, have to add link for each side */
	    if (dist_a > 0) {
		p->count = dist_a;
		p->val = Tval;
		p2 = (struct BMlink *)link_new(map->token);
		p2->next = p->next;
		p->next = p2;
		p = p2;
	    }
	    p->count = 1;
	    p->val = val;

	    if (dist_b > 0) {
		p2 = (struct BMlink *)link_new(map->token);
		p2->count = dist_b;
		p2->val = Tval;

		p2->next = p->next;
		p->next = p2;
	    }

	    return 0;

	}
	cur_x += p->count;
	prev = p;
	p = p->next;
    }

    return 0;
}


/*!
 * \brief
 *
 * Returns sparse bitmap value at location 'x'/'y'
 *
 * Returns value or -1 on error
 *
 *  \param map
 *  \param x
 *  \param y
 *  \return int
 */

int BM_get_sparse(struct BM *map, int x, int y)
{
    struct BMlink *p;
    int cur_x = 0;

    p = ((struct BMlink **)(map->data))[y];
    while (p != NULL) {
	if (cur_x + p->count > x)
	    return (p->val);
	cur_x += p->count;
	p = p->next;
    }

    return -1;
}


/*!
 * \brief
 *
 * Returns size of sparse bitmap in bytes
 *
 *  \param map
 *  \return int
 */

size_t BM_get_map_size_sparse(struct BM *map)
{
    int i;
    size_t size;
    struct BMlink *p;

    size = (size_t) map->rows * sizeof(struct BMlink *);
    for (i = 0; i < map->rows; i++) {
	p = ((struct BMlink **)(map->data))[i];
	while (p != NULL) {
	    size += sizeof(struct BMlink);
	    p = p->next;
	}
    }

    return size;
}


/*!
 * \brief
 *
 * Debugging code to dump out structure of links
 *
 * Returns 0
 *
 *  \param map
 *  \return int
 */

int BM_dump_map_sparse(struct BM *map)
{
    int i;
    struct BMlink *p;

    for (i = 0; i < map->rows; i++) {
	p = ((struct BMlink **)(map->data))[i];
	while (p != NULL) {
	    fprintf(stdout, "(%2d %2d)  ", p->count, p->val);
	    p = p->next;
	}
	fprintf(stdout, "\n");
    }

    return 0;
}


/*!
 * \brief
 *
 * Debugging code to dump out structure of links for single row
 *
 * Returns 0
 *
 *  \param map
 *  \param y
 *  \return int
 */

int BM_dump_map_row_sparse(struct BM *map, int y)
{
    int i;
    struct BMlink *p;

    i = y;
    {
	p = ((struct BMlink **)(map->data))[i];
	while (p != NULL) {
	    fprintf(stdout, "(%2d %2d)  ", p->count, p->val);
	    p = p->next;
	}
	fprintf(stdout, "\n");
    }

    return 0;
}


/*!
 * \brief
 *
 * Write sparse bitmap matrix out to disk file 'fp'.
 * NOTE: 'fp' must already be opened and later closed by user
 *
 * Returns 0 on success or -1 on error
 *
 *  \param fp
 *  \param map
 *  \return int
 */

int BM_file_write_sparse(FILE * fp, struct BM *map)
{
    char c;
    int i, y;
    struct BMlink *p;
    int cnt;

    c = BM_MAGIC;
    fwrite(&c, sizeof(char), sizeof(char), fp);

    fwrite(BM_TEXT, BM_TEXT_LEN, sizeof(char), fp);

    c = BM_SPARSE;
    fwrite(&c, sizeof(char), sizeof(char), fp);

    fwrite(&(map->rows), sizeof(map->rows), sizeof(char), fp);

    fwrite(&(map->cols), sizeof(map->cols), sizeof(char), fp);

    for (y = 0; y < map->rows; y++) {
	/* first count number of links */
	p = ((struct BMlink **)(map->data))[y];
	cnt = 0;
	while (p != NULL) {
	    cnt++;
	    p = p->next;
	}

	i = cnt;
	fwrite(&i, sizeof(i), sizeof(char), fp);


	/* then write them out */
	p = ((struct BMlink **)(map->data))[y];
	while (p != NULL) {
	    i = p->count;
	    fwrite(&i, sizeof(i), sizeof(char), fp);

	    i = p->val;
	    fwrite(&i, sizeof(i), sizeof(char), fp);

	    p = p->next;
	}
    }
    fflush(fp);

    return 0;
}
