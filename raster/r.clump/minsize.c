#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/rbtree.h>	/* Red Black Tree library functions */
#include <grass/glocale.h>
#include "rclist.h"
#include "local_proto.h"

struct nbr_cnt {
    int id;
    int row, col;
    int cnt;
};

static int cmp_nbrs(const void *a, const void *b)
{
    struct nbr_cnt *aa = (struct nbr_cnt *)a;
    struct nbr_cnt *bb = (struct nbr_cnt *)b;

    return (aa->id - bb->id);
}

static int cmp_ints(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

static int get_eight_neighbors(int row, int col, int nrows, int ncols,
			       int neighbors[8][2])
{
    int rown, coln, n;
    
    n = -1;
    /* previous row */
    rown = row - 1;
    if (rown >= 0) {
	coln = col - 1;
	if (coln >= 0) {
	    n++;
	    neighbors[n][0] = rown;
	    neighbors[n][1] = coln;
	}
	n++;
	neighbors[n][0] = rown;
	neighbors[n][1] = col;
	coln = col + 1;
	if (coln < ncols) {
	    n++;
	    neighbors[n][0] = rown;
	    neighbors[n][1] = coln;
	}
    }
    
    /* next row */
    rown = row + 1;
    if (rown < nrows) {
	coln = col - 1;
	if (coln >= 0) {
	    n++;
	    neighbors[n][0] = rown;
	    neighbors[n][1] = coln;
	}
	n++;
	neighbors[n][0] = rown;
	neighbors[n][1] = col;
	coln = col + 1;
	if (coln < ncols) {
	    n++;
	    neighbors[n][0] = rown;
	    neighbors[n][1] = coln;
	}
    }
    
    /* current row */
    coln = col - 1;
    if (coln >= 0) {
	n++;
	neighbors[n][0] = row;
	neighbors[n][1] = coln;
    }
    coln = col + 1;
    if (coln < ncols) {
	n++;
	neighbors[n][0] = row;
	neighbors[n][1] = coln;
    }
    
    return n;
}

static int get_four_neighbors(int row, int col, int nrows, int ncols,
			       int neighbors[8][2])
{
    int rown, coln, n;
    
    n = -1;
    /* previous row */
    rown = row - 1;
    if (rown >= 0) {
	n++;
	neighbors[n][0] = rown;
	neighbors[n][1] = col;
    }
    
    /* next row */
    rown = row + 1;
    if (rown < nrows) {
	n++;
	neighbors[n][0] = rown;
	neighbors[n][1] = col;
    }
    
    /* current row */
    coln = col - 1;
    if (coln >= 0) {
	n++;
	neighbors[n][0] = row;
	neighbors[n][1] = coln;
    }
    coln = col + 1;
    if (coln < ncols) {
	n++;
	neighbors[n][0] = row;
	neighbors[n][1] = coln;
    }
    
    return n;
}

static int (*get_neighbors)(int row, int col, int nrows, int ncols,
			       int neighbors[8][2]);

static int update_cid_box(int cfd, int rowmin, int rowmax, int colmin,
                          int colmax, int old_id, int new_id)
{
    int row, col;
    int csize, csizebox;
    CELL *cbuf, *cp;
    off_t coffset;
    
    csize = sizeof(CELL) * Rast_window_cols();
    csizebox = sizeof(CELL) * (colmax - colmin + 1);
    cbuf = G_malloc(csizebox);
    
    for (row = rowmin; row <= rowmax; row++) {
	coffset = (off_t) row * csize + colmin * sizeof(CELL);
	lseek(cfd, coffset, SEEK_SET);
	if (read(cfd, cbuf, csizebox) != csizebox)
	    G_fatal_error(_("Unable to read from temp file"));
	
	cp = cbuf;
	for (col = colmin; col <= colmax; col++) {
	    if (!Rast_is_c_null_value(cp) && *cp == old_id)
		*cp = new_id;
	    cp++;
	}

	lseek(cfd, coffset, SEEK_SET);
	if (write(cfd, cbuf, csizebox) != csizebox)
	    G_fatal_error(_("Unable to write to temp file"));
    }
    
    G_free(cbuf);

    return 1;
}

static double get_diff2(DCELL *a, DCELL *b, DCELL *rng, int n)
{
    int i;
    double diff, diff2;

    diff2 = 0;
    for (i = 0; i < n; i++) {
	if (Rast_is_d_null_value(&b[i]))
	    return 2;
	diff = a[i] - b[i];
	/* normalize with the band's range */
	if (rng[i])
	    diff /= rng[i];
	diff2 += diff * diff;
    }
    /* normalize difference to the range [0, 1] */
    diff2 /= n;
    
    return diff2;
}

static int find_best_neighbour(int bfd, int nin, DCELL *rng,
                               int cfd, int csize, int row, int col,
                               int this_id, struct RB_TREE *nbtree,
			       int *best_sim_id, int *best_cnt_id,
			       int *rowmin, int *rowmax, int *colmin, int *colmax)
{
    int rown, coln, n, count;
    int nrows, ncols;
    int neighbors[8][2];
    struct rc next;
    struct rclist rilist;
    CELL *cbuf;
    DCELL *val, *valn;
    int crow;
    int ngbr_id;
    struct RB_TREE *visited;
    int rc;
    struct nbr_cnt Rk, *Rfound, *Rbest;
    double sim, best_sim;
    int have_Ri;
    off_t coffset, boffset;
    int bsize;

    G_debug(3, "find_best_neighbour()");

    rbtree_clear(nbtree);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    cbuf = Rast_allocate_c_buf();

    bsize = sizeof(DCELL) * nin;
    val = (DCELL *) G_malloc(bsize);
    valn = (DCELL *) G_malloc(bsize);

    visited = rbtree_create(cmp_ints, sizeof(int));
    rc = row * ncols + col;
    rbtree_insert(visited, &rc);

    /* breadth-first search */
    next.row = row;
    next.col = col;
    rclist_init(&rilist);
    count = 1;
    *best_sim_id = 0;
    *best_cnt_id = 0;
    best_sim = 2;
    Rbest = NULL;
    crow = -1;
    
    do {
	have_Ri = 0;

	n = get_neighbors(next.row, next.col, nrows, ncols, neighbors);
	do {
	    rown = neighbors[n][0];
	    coln = neighbors[n][1];

	    if (crow != rown) {
		coffset = (off_t)rown * csize;
		lseek(cfd, coffset, SEEK_SET);
		if (read(cfd, cbuf, csize) != csize)
		    G_fatal_error(_("Unable to read from temp file"));
		crow = rown;
	    }

	    if (Rast_is_c_null_value(&cbuf[coln]) || cbuf[coln] < 1)
		continue;

	    rc = rown * ncols + coln;

	    if (!rbtree_find(visited, &rc)) {
		rbtree_insert(visited, &rc);

		/* get neighbor ID */
		ngbr_id = cbuf[coln];
		/* same neighbour */
		if (ngbr_id == this_id) {
		    count++;
		    rclist_add(&rilist, rown, coln);
		    if (*rowmin > rown)
			*rowmin = rown;
		    if (*rowmax < rown)
			*rowmax = rown;
		    if (*colmin > coln)
			*colmin = coln;
		    if (*colmax < coln)
			*colmax = coln;
		}
		else { /* different neighbour */
		    if (rng) {
			/* compare to this cell next.row, next.col */
			if (!have_Ri) {
			    boffset = (off_t) sizeof(DCELL) * (next.row * ncols + next.col);
			    lseek(bfd, boffset, SEEK_SET);
			    if (read(bfd, val, bsize) != bsize)
				G_fatal_error(_("Unable to read from temp file"));
			    have_Ri = 1;
			}
			boffset = (off_t) sizeof(DCELL) * (rown * ncols + coln);
			lseek(bfd, boffset, SEEK_SET);
			if (read(bfd, valn, bsize) != bsize)
			    G_fatal_error(_("Unable to read from temp file"));

			sim = get_diff2(val, valn, rng, nin);
			if (best_sim > sim) {
			    best_sim = sim;
			    *best_sim_id = ngbr_id;
			}
		    }

		    /* find in neighbor tree */
		    Rk.id = ngbr_id;
		    if ((Rfound = rbtree_find(nbtree, &Rk))) {
			Rfound->cnt++;
			if (Rbest->cnt < Rfound->cnt)
			    Rbest = Rfound;
		    }
		    else {
			Rk.cnt = 1;
			Rk.row = rown;
			Rk.col = coln;
			rbtree_insert(nbtree, &Rk);
			if (!Rbest)
			    Rbest = rbtree_find(nbtree, &Rk);
		    }
		}
	    }
	} while (n--);    /* end do loop - next neighbor */
    } while (rclist_drop(&rilist, &next));   /* while there are cells to check */

    if (Rbest)
	*best_cnt_id = Rbest->id;

    rclist_destroy(&rilist);
    rbtree_destroy(visited);

    G_free(val);
    G_free(valn);
    G_free(cbuf);

    return (*best_cnt_id > 0);
}

int merge_small_clumps(int *in_fd, int nin, DCELL *rng,
                        int diag, int minsize, CELL *n_clumps,
			int cfd, int out_fd)
{
    int row, col, nrows, ncols, i;
    int rowmin, rowmax, colmin, colmax;
    struct RB_TREE *nbtree;
    CELL this_id;
    CELL best_sim_id, best_cnt_id, best_id;
    int bfd;
    char *bname;
    int reg_size;
    CELL *clumpid, n_clumps_new;
    CELL *cbuf;
    off_t coffset;
    int csize, size1;

    /* two possible merge modes
     * best (most similar) neighbour
     * neighbour with longest boundary */

    if (minsize < 2)
	G_fatal_error(_("Minimum size must be larger than 1"));

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* load input bands to temp file */
    bfd = -1;
    bname = NULL;
    if (rng) {
	DCELL *bbuf, **inbuf;

	G_message(_("Loading input ..."));

	/* temp file for input map(s) */
	bname = G_tempfile();
	if ((bfd = open(bname, O_RDWR | O_CREAT | O_EXCL, 0600)) < 0)
	    G_fatal_error(_("Unable to open temp file"));

	inbuf = G_malloc(sizeof(DCELL *) * nin);
	for (i = 0; i < nin; i++)
	    inbuf[i] = Rast_allocate_d_buf();
	bbuf = G_malloc(sizeof(DCELL) * nin);

	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    for (i = 0; i < nin; i++) {
		Rast_get_d_row(in_fd[i], inbuf[i], row);
	    }
	    for (col = 0; col < ncols; col++) {
		for (i = 0; i < nin; i++)
		    bbuf[i] = inbuf[i][col];

		if (write(bfd, bbuf, sizeof(DCELL) * nin) != (int) (sizeof(DCELL) * nin))
		    G_fatal_error(_("Unable to write to temp file"));
	    }
	}
	G_percent(row, nrows, 2);
	for (i = 0; i < nin; i++)
	    G_free(inbuf[i]);
	G_free(inbuf);
	G_free(bbuf);
    }

    csize = ncols * sizeof(CELL);
    size1 = sizeof(CELL);

    if (diag)
	get_neighbors = get_eight_neighbors;
    else
	get_neighbors = get_four_neighbors;

    cbuf = Rast_allocate_c_buf();

    /* init clump id */
    clumpid = (CELL *) G_malloc(sizeof(CELL) * (*n_clumps + 1));
    for (i = 0; i <= *n_clumps; i++)
	clumpid[i] = 0;

    /* rewind temp file */
    lseek(cfd, 0, SEEK_SET);

    G_message(_("Merging clumps smaller than %d cells..."), minsize);

    /* get clump sizes */
    for (row = 0; row < nrows; row++) {
	/* read clumps */
	if (read(cfd, cbuf, csize) != csize)
	    G_fatal_error(_("Unable to read from temp file"));

	for (col = 0; col < ncols; col++) {
	    if (!Rast_is_c_null_value(&cbuf[col]) && cbuf[col] > 0) {
		if (clumpid[cbuf[col]] <= minsize)
		    clumpid[cbuf[col]]++;
	    }
	}
    }

    nbtree = rbtree_create(cmp_nbrs, sizeof(struct nbr_cnt));

    /* go through all cells */
    G_percent_reset();
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	for (col = 0; col < ncols; col++) {

	    /* read clump id */
	    coffset = (off_t)row * csize + col * size1;
	    lseek(cfd, coffset, SEEK_SET);
	    if (read(cfd, &this_id, size1) != size1)
		G_fatal_error(_("Unable to read from temp file"));

	    /* can not read a whole row in advance because values may change */
	    if (Rast_is_c_null_value(&this_id) || this_id < 1)
		continue;

	    reg_size = clumpid[this_id];
	    best_id = 1;
	    
	    rowmin = rowmax = row;
	    colmin = colmax = col;

	    while (reg_size < minsize && best_id > 0) {
		best_sim_id = 0;
		best_cnt_id = 0;

		find_best_neighbour(bfd, nin, rng, cfd, csize,
				    row, col, this_id, nbtree,
				    &best_sim_id, &best_cnt_id,
				    &rowmin, &rowmax, &colmin, &colmax);

		/* best_sim_*: most similar neighbour */
		/* best_cnt_*: most common neighbour */
		best_id = best_cnt_id;
		if (rng)
		    best_id = best_sim_id;

		if (best_id > 0) {
		    /* update cid */
		    /* update_cid_cell(cfd, csize, row, col, this_id, best_id); */
		    update_cid_box(cfd, rowmin, rowmax, colmin, colmax, this_id, best_id);
		    /* mark as merged */
		    clumpid[best_id] += clumpid[this_id];
		    reg_size = clumpid[best_id];
		    clumpid[this_id] = 0;
		    this_id = best_id;
		}
	    }
	}
    }
    G_percent(1, 1, 1);
    rbtree_destroy(nbtree);
    
    if (bfd > -1) {
	close(bfd);
	unlink(bname);
    }

    n_clumps_new = 0;
    /* clumpid becomes new region ID */
    for (i = 1; i <= *n_clumps; i++) {
	if (clumpid[i] > 0)
	    clumpid[i] = ++n_clumps_new;
    }
    *n_clumps = n_clumps_new;

    if (out_fd < 0) {
	fprintf(stdout, "clumps=%d\n", n_clumps_new);
	
	return 1;
    }

    G_message(_("Renumbering remaining %d clumps..."), n_clumps_new);

    /* rewind temp file */
    lseek(cfd, 0, SEEK_SET);

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 4);

	if (read(cfd, cbuf, csize) != csize)
	    G_fatal_error(_("Unable to read from temp file"));

	for (col = 0; col < ncols; col++) {

	    this_id = cbuf[col];
	    if (Rast_is_c_null_value(&this_id))
		continue;

	    if (this_id == 0) {
		Rast_set_c_null_value(&cbuf[col], 1);
	    }
	    else if (this_id != clumpid[this_id]) {
		cbuf[col] = clumpid[this_id];
	    }
	}
	Rast_put_row(out_fd, cbuf, CELL_TYPE);
    }
    G_percent(1, 1, 1);
    
    G_free(clumpid);
    G_free(cbuf);

    return 1;
}
