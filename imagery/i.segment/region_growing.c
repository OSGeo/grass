/* PURPOSE:      Develop the image segments */

/* Currently only region growing is implemented */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/segment.h> /* segmentation library */
#include "pavl.h"
#include "iseg.h"

#define EPSILON 1.0e-8

struct idlist {
    int *ids;
    int nids, nalloc;
    CELL cellmax;
};

static struct idlist idlist;

/* internal functions */
static int merge_regions(struct ngbr_stats *, struct reg_stats *, /* Ri */
                         struct ngbr_stats *, struct reg_stats *, /* Rk */
                         int, struct globals *);
static int search_neighbors(struct ngbr_stats *, /* Ri */
                            struct reg_stats *,
                            struct NB_TREE *,    /* Ri's neighbors */
                            double *,            /* highest similarity */
                            struct ngbr_stats *, /* Ri's best neighbor */
                            struct reg_stats *, struct globals *);
static int set_candidate_flag(struct ngbr_stats *, int, struct globals *);
static int find_best_neighbor(struct ngbr_stats *, struct reg_stats *,
                              struct NB_TREE *, struct ngbr_stats *,
                              struct reg_stats *, double *, int,
                              struct globals *);
static int calculate_reg_stats(int, int, struct reg_stats *, struct globals *);

void init_free_ids(void)
{
    idlist.nalloc = 10;
    idlist.nids = 0;

    idlist.ids = G_malloc(idlist.nalloc * sizeof(int));

    idlist.cellmax = ((CELL)1 << (sizeof(CELL) * 8 - 2)) - 1;
    idlist.cellmax += ((CELL)1 << (sizeof(CELL) * 8 - 2));

    return;
}

void add_free_id(int id)
{
    if (id <= 0)
        return;

    if (idlist.nalloc <= idlist.nids) {
        idlist.nalloc = idlist.nids + 10;
        idlist.ids = G_realloc(idlist.ids, idlist.nalloc * sizeof(int));
    }
    idlist.ids[idlist.nids++] = id;

    return;
}

int get_free_id(struct globals *globals)
{
    if (idlist.nids > 0) {
        idlist.nids--;

        return idlist.ids[idlist.nids];
    }

    if (globals->max_rid == idlist.cellmax)
        G_fatal_error(_("Too many objects: integer overflow"));

    globals->max_rid++;

    return globals->max_rid;
}

void free_free_ids(void)
{
    if (idlist.nalloc) {
        G_free(idlist.ids);
        idlist.ids = NULL;
        idlist.nalloc = 0;
        idlist.nids = 0;
    }

    return;
}

/* function used by binary tree to compare items */
static int compare_rc(const void *first, const void *second)
{
    struct rc *a = (struct rc *)first, *b = (struct rc *)second;

    if (a->row == b->row)
        return (a->col - b->col);
    return (a->row - b->row);

    if (a->row < b->row)
        return -1;
    if (a->row > b->row)
        return 1;

    /* same row */
    if (a->col < b->col)
        return -1;
    if (a->col > b->col)
        return 1;
    /* same row and col */
    return 0;
}

static int compare_double(double first, double second)
{
    if (first < second)
        return -1;
    return (first > second);
}

static int compare_sim_ngbrs(double simi, double simk, int candi, int candk,
                             struct ngbr_stats *Ri, struct ngbr_stats *Rk)
{
    if (simi < simk)
        return -1;

    if (simi > simk)
        return 1;

    if (Rk->count == 0 || Ri->count < Rk->count)
        return -1;
    if (Ri->count > Rk->count)
        return 1;

    if (candi && !candk)
        return -1;

    if (candk && !candi)
        return 1;

    if (Ri->row < Rk->row)
        return -1;
    if (Ri->row > Rk->row)
        return 1;

    if (Ri->col < Rk->col)
        return -1;
    return (Ri->col > Rk->col);
}

#if 0  /* unused */
static int dump_Ri(struct ngbr_stats *Ri, struct reg_stats *Ri_rs,
                   double *Ri_sim, double *Rk_sim, int *Ri_nn, int *Rk_nn,
                   struct globals *globals)
{
    int i;

    G_debug(0, "Ri, Ri_rs ID: %d, %d", Ri->id, Ri_rs->id);
    G_debug(0, "Ri, Ri_rs count: %d, %d", Ri->count, Ri_rs->count);

    for (i = 0; i < globals->nbands; i++) {
        G_debug(0, "Ri, Ri_rs mean %d: %g, %g", i, Ri->mean[i],
                Ri_rs->mean[i]);
        G_debug(0, "Ri_rs sum %d: %g", i, Ri_rs->sum[i]);
    }
    G_debug(0, "Ri nn: %d", *Ri_nn);
    if (Rk_nn)
        G_debug(0, "Rk nn: %d", *Rk_nn);
    G_debug(0, "Ri similarity: %g", *Ri_sim);
    if (Rk_sim)
        G_debug(0, "Rk similarity: %g", *Rk_sim);

    return 1;
}
#endif /* unused */

int region_growing(struct globals *globals)
{
    int row, col, t;
    double threshold, adjthresh, Ri_similarity, Rk_similarity;
    double alpha2, divisor; /* threshold parameters */
    int n_merges, do_merge; /* number of merges on that iteration */
    int pathflag; /* =1 if we didn't find mutually best neighbors, continue with
                     Rk */
    int candidates_only;
    struct ngbr_stats Ri = {0}, Rk = {0},
                      Rk_bestn = {0}, /* Rk's best neighbor */
        *next;
    int Ri_nn, Rk_nn; /* number of neighbors for Ri/Rk */
    struct NB_TREE *Ri_ngbrs, *Rk_ngbrs;
    struct NB_TRAV travngbr;

    /* not all regions are in the tree, but we always need reg_stats for Ri and
     * Rk */
    struct reg_stats Ri_rs, Rk_rs, Rk_bestn_rs;
    double *dp;
    struct NB_TREE *tmpnbtree;

    /* CELL cellmax; */
    struct Cell_head cellhd;

    G_verbose_message("Running region growing algorithm");

    /* cellmax = ((CELL) 1 << (sizeof(CELL) * 8 - 2)) - 1;
       cellmax += ((CELL) 1 << (sizeof(CELL) * 8 - 2)); */

    init_free_ids();

    /* init neighbor stats */
    Ri.mean = G_malloc(globals->datasize);
    Rk.mean = G_malloc(globals->datasize);
    Rk_bestn.mean = G_malloc(globals->datasize);

    Ri_ngbrs = nbtree_create(globals->nbands, globals->datasize);
    Rk_ngbrs = nbtree_create(globals->nbands, globals->datasize);

    /* init region stats */
    Ri_rs.mean = G_malloc(globals->datasize);
    Ri_rs.sum = G_malloc(globals->datasize);
    Rk_rs.mean = G_malloc(globals->datasize);
    Rk_rs.sum = G_malloc(globals->datasize);
    Rk_bestn_rs.mean = G_malloc(globals->datasize);
    Rk_bestn_rs.sum = G_malloc(globals->datasize);

    t = 0;
    n_merges = 2;

    /* threshold calculation */
    alpha2 = globals->alpha * globals->alpha;
    threshold = alpha2;
    G_debug(1, "Squared threshold: %g", threshold);

    Rast_get_cellhd(globals->Ref.file[0].name, globals->Ref.file[0].mapset,
                    &cellhd);
    divisor = cellhd.rows + cellhd.cols;

    /* TODO: renumber seeds */

    while (t < globals->end_t && n_merges > 1) {

        G_message(_("Processing pass %d..."), ++t);

        n_merges = 0;
        globals->candidate_count = 0;
        flag_clear_all(globals->candidate_flag);

        /* Set candidate flag to true/1 for all non-NULL cells */
        for (row = globals->row_min; row < globals->row_max; row++) {
            for (col = globals->col_min; col < globals->col_max; col++) {
                if (!(FLAG_GET(globals->null_flag, row, col))) {

                    FLAG_SET(globals->candidate_flag, row, col);
                    globals->candidate_count++;
                }
            }
        }

        G_debug(4, "Starting to process %" PRI_LONG " candidate cells",
                globals->candidate_count);

        /*process candidate cells */
        G_percent_reset();
        for (row = globals->row_min; row < globals->row_max; row++) {
            G_percent(row - globals->row_min,
                      globals->row_max - globals->row_min, 4);
            for (col = globals->col_min; col < globals->col_max; col++) {
                if (!(FLAG_GET(globals->candidate_flag, row, col)))
                    continue;

                pathflag = TRUE;
                candidates_only = TRUE;

                nbtree_clear(Ri_ngbrs);
                nbtree_clear(Rk_ngbrs);

                G_debug(4, "Next starting cell: row, %d, col, %d", row, col);

                /* First cell in Ri is current row/col */
                Ri.row = row;
                Ri.col = col;

                /* get Ri's segment ID */
                Segment_get(&globals->rid_seg, (void *)&Ri.id, Ri.row, Ri.col);

                /* find segment neighbors */
                /* find Ri's best neighbor, clear candidate flag */
                Ri_similarity = 2;

                Ri_rs.id = Ri.id;
                fetch_reg_stats(Ri.row, Ri.col, &Ri_rs, globals);
                memcpy(Ri.mean, Ri_rs.mean, globals->datasize);
                Ri.count = Ri_rs.count;

                /* Ri is now complete */
                G_debug(4, "Ri is now complete");

                /* find best neighbor, get region stats for Rk */
                Ri_nn = find_best_neighbor(&Ri, &Ri_rs, Ri_ngbrs, &Rk, &Rk_rs,
                                           &Ri_similarity, 1, globals);

                /* Rk is now complete */
                G_debug(4, "Rk is now complete");

                if (Rk.id < 0) {
                    /* this can only happen if the segment is surrounded by NULL
                     * data */
                    G_debug(4, "Segment had no valid neighbors");
                    continue;
                }

                if (compare_double(Ri_similarity, threshold) >= 0) {
                    G_debug(4, "Best neighbor is not similar enough");
                    continue;
                }

                if (/* !(t & 1) && */ Ri_nn == 1 &&
                    !(FLAG_GET(globals->candidate_flag, Rk.row, Rk.col)) &&
                    compare_double(Ri_similarity, threshold) == -1) {
                    /* this is slow ??? */
                    int smaller = Rk.count;

                    if (Ri.count < Rk.count)
                        smaller = Ri.count;

                    /* TODO: better */
                    adjthresh = pow(alpha2, 1. + (double)smaller / divisor);

                    if (compare_double(Ri_similarity, adjthresh) == -1) {
                        G_debug(4, "Ri nn == 1");
                        if (Rk.count < 2)
                            G_fatal_error("Rk count too low");
                        if (Rk.count < Ri.count)
                            G_debug(4, "Rk count lower than Ri count");

                        merge_regions(&Ri, &Ri_rs, &Rk, &Rk_rs, 1, globals);
                        n_merges++;
                    }

                    pathflag = FALSE;
                }

                while (pathflag) {
                    pathflag = FALSE;

                    if (Rk.count <= globals->nn ||
                        Rk.count <= globals->min_segment_size)
                        candidates_only = FALSE;

                    /* optional check if Rk is candidate
                     * to prevent backwards merging */
                    if (candidates_only &&
                        !(FLAG_GET(globals->candidate_flag, Rk.row, Rk.col))) {

                        Ri_similarity = 2;
                    }

                    candidates_only = TRUE;

                    if (compare_double(Ri_similarity, threshold) == -1) {
                        do_merge = 1;

                        /* we'll have the neighbor pixel to start with. */
                        G_debug(4, "Working with Rk");

                        /* find Rk's best neighbor, do not clear candidate flag
                         */
                        Rk_similarity = Ri_similarity;
                        Rk_bestn_rs.count = 0;
                        /* Rk_rs is already complete */
                        Rk_nn = find_best_neighbor(&Rk, &Rk_rs, Rk_ngbrs,
                                                   &Rk_bestn, &Rk_bestn_rs,
                                                   &Rk_similarity, 0, globals);

                        /* not mutually best neighbors */
                        if (Rk_similarity != Ri_similarity) {
                            do_merge = 0;
                        }
                        /* Ri has only one neighbor, merge */
                        if (Ri_nn == 1 && Rk_nn > 1)
                            do_merge = 1;

                        /* adjust threshold */
                        if (do_merge) {
                            int smaller = Rk.count;

                            if (Ri.count < Rk.count)
                                smaller = Ri.count;

                            /* TODO: better */
                            adjthresh =
                                pow(alpha2, 1. + (double)smaller / divisor);

                            do_merge = 0;
                            if (compare_double(Ri_similarity, adjthresh) ==
                                -1) {
                                do_merge = 1;
                            }
                        }

                        if (do_merge) {

                            G_debug(4, "merge neighbor trees");

                            Ri_nn -= Ri_ngbrs->count;
                            Ri_nn += (Rk_nn - Rk_ngbrs->count);
                            globals->ns.id = Rk.id;
                            globals->ns.row = Rk.row;
                            globals->ns.col = Rk.col;
                            nbtree_remove(Ri_ngbrs, &(globals->ns));

                            nbtree_init_trav(&travngbr, Rk_ngbrs);
                            while ((next = nbtree_traverse(&travngbr))) {
                                if (!nbtree_find(Ri_ngbrs, next) &&
                                    cmp_ngbr(next, &Ri) != 0)
                                    nbtree_insert(Ri_ngbrs, next);
                            }
                            nbtree_clear(Rk_ngbrs);
                            Ri_nn += Ri_ngbrs->count;

                            merge_regions(&Ri, &Ri_rs, &Rk, &Rk_rs, 1, globals);
                            /* Ri is now updated, Rk no longer usable */

                            /* made a merge, need another iteration */
                            n_merges++;

                            Ri_similarity = 2;
                            Rk_similarity = 2;

                            /* we have checked the neighbors of Ri, Rk already
                             * use faster version of finding the best neighbor
                             */

                            /* use neighbor tree to find Ri's new best neighbor
                             */
                            search_neighbors(&Ri, &Ri_rs, Ri_ngbrs,
                                             &Ri_similarity, &Rk, &Rk_rs,
                                             globals);

                            if (Rk.id >= 0 && Ri_nn > 0 &&
                                compare_double(Ri_similarity, threshold) ==
                                    -1) {

                                pathflag = TRUE;
                                /* candidates_only:
                                 * FALSE: less passes, slower, but less memory
                                 * TRUE: more passes but faster */
                                candidates_only = TRUE;
                            }
                            /* else end of Ri -> Rk chain since we merged Ri and
                             * Rk go to next row, col */
                        }
                        else {
                            if (compare_double(Rk_similarity, threshold) ==
                                -1) {
                                pathflag = TRUE;
                            }
                            /* test this: can it cause an infinite loop ? */
                            if (!(FLAG_GET(globals->candidate_flag, Rk.row,
                                           Rk.col)))
                                pathflag = FALSE;

                            if (Rk_nn < 2)
                                pathflag = FALSE;

                            if (Rk.id < 0)
                                pathflag = FALSE;

                            if (Rk_bestn.id < 0) {
                                G_debug(4, "Rk's best neighbour is negative");
                                pathflag = FALSE;
                            }

                            if (pathflag) {

                                /* clear candidate flag for Rk */
                                if (FLAG_GET(globals->candidate_flag, Rk.row,
                                             Rk.col)) {
                                    set_candidate_flag(&Rk, FALSE, globals);
                                }

                                /* Use Rk as next Ri:
                                 * this is the eCognition technique. */
                                G_debug(4, "do ecog");
                                Ri_nn = Rk_nn;
                                Ri_similarity = Rk_similarity;

                                dp = Ri.mean;
                                Ri = Rk;
                                Rk = Rk_bestn;
                                Rk_bestn.mean = dp;

                                Ri_rs.id = Rk_rs.id;
                                Rk_rs.id = Rk_bestn_rs.id;
                                Rk_bestn_rs.id = -1;
                                Ri_rs.count = Rk_rs.count;
                                Rk_rs.count = Rk_bestn_rs.count;
                                Rk_bestn_rs.count = 0;
                                dp = Ri_rs.mean;
                                Ri_rs.mean = Rk_rs.mean;
                                Rk_rs.mean = Rk_bestn_rs.mean;
                                Rk_bestn_rs.mean = dp;
                                dp = Ri_rs.sum;
                                Ri_rs.sum = Rk_rs.sum;
                                Rk_rs.sum = Rk_bestn_rs.sum;
                                Rk_bestn_rs.sum = dp;

                                tmpnbtree = Ri_ngbrs;
                                Ri_ngbrs = Rk_ngbrs;
                                Rk_ngbrs = tmpnbtree;
                                nbtree_clear(Rk_ngbrs);
                            }
                        }
                    } /* end if < threshold */
                } /* end pathflag */
            } /* next col */
        } /* next row */
        G_percent(1, 1, 1);

        /* finished one pass for processing candidate pixels */
        G_verbose_message("%d merges", n_merges);

        G_debug(4, "Finished pass %d", t);
    }

    /*end t loop */ /*TODO, should there be a max t that it can iterate for?
                       Include t in G_message? */
    if (n_merges > 1)
        G_message(_("Segmentation processes stopped at %d due to reaching max "
                    "iteration limit, more merges may be possible"),
                  t);
    else
        G_message(_("Segmentation converged after %d iterations"), t);

    /* assign region IDs to remaining 0 IDs */
    G_message(_("Assigning region IDs to remaining single-cell regions..."));
    for (row = globals->row_min; row < globals->row_max; row++) {
        G_percent(row - globals->row_min, globals->row_max - globals->row_min,
                  4);
        for (col = globals->col_min; col < globals->col_max; col++) {
            if (!(FLAG_GET(globals->null_flag, row, col))) {
                /* get segment id */
                Segment_get(&globals->rid_seg, (void *)&Ri.id, row, col);
                if (Ri.id == 0) {
                    Ri.id = get_free_id(globals);
                    Segment_put(&globals->rid_seg, (void *)&Ri.id, row, col);
                }
            }
        }
    }
    G_percent(1, 1, 1);

    free_free_ids();

    /* ******************************************************************************************
     */
    /* final pass, ignore threshold and force a merge for small segments with
     * their best neighbor */
    /* ******************************************************************************************
     */

    if (globals->min_segment_size > 1) {
        G_message(_("Merging segments smaller than %d cells..."),
                  globals->min_segment_size);

        threshold = globals->alpha * globals->alpha;

        flag_clear_all(globals->candidate_flag);

        n_merges = 0;
        globals->candidate_count = 0;

        /* Set candidate flag to true/1 for all non-NULL cells */
        for (row = globals->row_min; row < globals->row_max; row++) {
            for (col = globals->col_min; col < globals->col_max; col++) {
                if (!(FLAG_GET(globals->null_flag, row, col))) {
                    FLAG_SET(globals->candidate_flag, row, col);

                    globals->candidate_count++;
                }
            }
        }

        G_debug(4, "Starting to process %" PRI_LONG " candidate cells",
                globals->candidate_count);

        /* process candidate cells */
        G_percent_reset();
        for (row = globals->row_min; row < globals->row_max; row++) {
            G_percent(row - globals->row_min,
                      globals->row_max - globals->row_min, 4);
            for (col = globals->col_min; col < globals->col_max; col++) {
                do_merge = 1;

                if (!(FLAG_GET(globals->candidate_flag, row, col)))
                    continue;

                Ri.row = row;
                Ri.col = col;

                /* get segment id */
                Segment_get(&globals->rid_seg, (void *)&Ri.id, row, col);

                if (Ri.id < 0)
                    continue;

                Ri_rs.id = Ri.id;

                /* get segment size */

                fetch_reg_stats(Ri.row, Ri.col, &Ri_rs, globals);
                memcpy(Ri.mean, Ri_rs.mean, globals->datasize);
                Ri.count = Ri_rs.count;

                if (Ri.count >= globals->min_segment_size) {
                    set_candidate_flag(&Ri, FALSE, globals);
                    do_merge = 0;
                }

                while (do_merge) {
                    do_merge = 0;

                    /* merge all smaller than min size */
                    if (Ri.count < globals->min_segment_size)
                        do_merge = 1;

                    Ri_nn = 0;
                    Ri_similarity = 2;

                    Rk.id = -1;

                    if (do_merge) {

                        /* find Ri's best neighbor, clear candidate flag */
                        Ri_nn = find_best_neighbor(&Ri, &Ri_rs, Ri_ngbrs, &Rk,
                                                   &Rk_rs, &Ri_similarity, 1,
                                                   globals);
                    }
                    do_merge = 0;

                    if (Rk.id >= 0) {
                        /* merge Ri with Rk */
                        /* do not clear candidate flag for Rk */
                        merge_regions(&Ri, &Ri_rs, &Rk, &Rk_rs, 0, globals);
                        n_merges++;

                        if (Ri.count < globals->min_segment_size)
                            do_merge = 1;
                    }
                }
            }
        }
        G_percent(1, 1, 1);

        /* finished one pass for processing candidate pixels */
        G_verbose_message("%d merges", n_merges);
    }

    /* free neighbor stats */
    G_free(Ri.mean);
    G_free(Rk.mean);
    G_free(Rk_bestn.mean);

    nbtree_clear(Ri_ngbrs);
    nbtree_clear(Rk_ngbrs);
    free(Ri_ngbrs);
    free(Rk_ngbrs);

    /* free region stats */
    G_free(Ri_rs.mean);
    G_free(Ri_rs.sum);
    G_free(Rk_rs.mean);
    G_free(Rk_rs.sum);
    G_free(Rk_bestn_rs.mean);
    G_free(Rk_bestn_rs.sum);

    return TRUE;
}

static void free_item(void *p)
{
    G_free(p);
}

static int find_best_neighbor(struct ngbr_stats *Ri, struct reg_stats *Ri_rs,
                              struct NB_TREE *Ri_ngbrs, struct ngbr_stats *Rk,
                              struct reg_stats *Rk_rs, double *sim,
                              int clear_cand, struct globals *globals)
{
    int n, n_ngbrs, no_check, cmp;
    struct rc ngbr_rc = {0}, next = {0}, *pngbr_rc = NULL;
    struct rclist rilist;
    double tempsim;
    int neighbors[8][2];
    struct pavl_table *no_check_tree; /* cells already checked */
    struct reg_stats *rs_found;
    int candk, candtmp;

    G_debug(4, "find_best_neighbor()");

    if (Ri->id != Ri_rs->id)
        G_fatal_error("Ri = %d but Ri_rs = %d", Ri->id, Ri_rs->id);
    if (Ri->id < 0)
        G_fatal_error("Ri is %d", Ri->id);

    /* dynamics of the region growing algorithm
     * some regions are growing fast, often surrounded by many small regions
     * not all regions are equally growing, some will only grow at a later stage
     * ? */

    /* *** initialize data *** */

    ngbr_rc.row = Ri->row;
    ngbr_rc.col = Ri->col;
    no_check_tree = pavl_create(compare_rc, NULL);
    pngbr_rc = G_malloc(sizeof(struct rc));
    *pngbr_rc = ngbr_rc;
    pavl_insert(no_check_tree, pngbr_rc);
    pngbr_rc = NULL;

    nbtree_clear(Ri_ngbrs);
    n_ngbrs = 0;
    /* TODO: add size of largest region to reg_tree, use this as min */
    Rk->count = Rk_rs->count = 0;
    Rk->id = Rk_rs->id = -1;
    candk = 0;

    /* go through segment, spreading outwards from head */
    rclist_init(&rilist);

    /* check neighbors of start cell */
    next.row = Ri->row;
    next.col = Ri->col;
    do {
        /* remove from candidates */
        if (clear_cand)
            FLAG_UNSET(globals->candidate_flag, next.row, next.col);

        G_debug(5, "find_pixel_neighbors for row: %d , col %d", next.row,
                next.col);

        globals->find_neighbors(next.row, next.col, neighbors);

        n = globals->nn - 1;
        do {

            globals->ns.row = ngbr_rc.row = neighbors[n][0];
            globals->ns.col = ngbr_rc.col = neighbors[n][1];

            no_check = (ngbr_rc.row < globals->row_min ||
                        ngbr_rc.row >= globals->row_max ||
                        ngbr_rc.col < globals->col_min ||
                        ngbr_rc.col >= globals->col_max);

            n_ngbrs += no_check;

            if (!no_check) {

                no_check = ((FLAG_GET(globals->null_flag, ngbr_rc.row,
                                      ngbr_rc.col)) != 0);
                n_ngbrs += no_check;

                if (!no_check) {

                    if (pngbr_rc == NULL)
                        pngbr_rc = G_malloc(sizeof(struct rc));
                    *pngbr_rc = ngbr_rc;

                    if (pavl_insert(no_check_tree, pngbr_rc) == NULL) {
                        pngbr_rc = NULL;

                        /* get neighbor ID */
                        Segment_get(&globals->rid_seg,
                                    (void *)&(globals->ns.id), ngbr_rc.row,
                                    ngbr_rc.col);

                        if (Ri->id > 0 && globals->ns.id == Ri->id) {

                            /* want to check this neighbor's neighbors */
                            rclist_add(&rilist, ngbr_rc.row, ngbr_rc.col);
                        }
                        else {

                            /* new neighbor ? */
                            if (nbtree_find(Ri_ngbrs, &globals->ns) == NULL) {

                                /* get values for Rk */
                                globals->rs.id = globals->ns.id;
                                rs_found = rgtree_find(globals->reg_tree,
                                                       &(globals->rs));
                                if (!rs_found) {
                                    /* region stats are not in search tree */
                                    rs_found = &(globals->rs);
                                    calculate_reg_stats(ngbr_rc.row,
                                                        ngbr_rc.col, rs_found,
                                                        globals);
                                }
                                globals->ns.mean = rs_found->mean;
                                globals->ns.count = rs_found->count;
                                /* globals->ns is now complete */

                                tempsim = (globals->calculate_similarity)(
                                    Ri, &globals->ns, globals);
                                candtmp =
                                    (FLAG_GET(globals->candidate_flag,
                                              ngbr_rc.row, ngbr_rc.col)) != 0;

                                cmp =
                                    compare_sim_ngbrs(tempsim, *sim, candtmp,
                                                      candk, &globals->ns, Rk);

                                if (cmp == -1) {
                                    *sim = tempsim;
                                    candk = candtmp;
                                    /* copy temp Rk to Rk */
                                    Rk->row = ngbr_rc.row;
                                    Rk->col = ngbr_rc.col;

                                    Rk->id = rs_found->id;
                                    Rk->count = rs_found->count;
                                    memcpy(Rk->mean, rs_found->mean,
                                           globals->datasize);

                                    Rk_rs->id = Rk->id;
                                    Rk_rs->count = Rk->count;
                                    memcpy(Rk_rs->mean, rs_found->mean,
                                           globals->datasize);
                                    memcpy(Rk_rs->sum, rs_found->sum,
                                           globals->datasize);
                                }

                                n_ngbrs++;
                                nbtree_insert(Ri_ngbrs, &globals->ns);
                            }
                        }
                    }
                }
            }
        } while (n--); /* end do loop - next neighbor */
    } while (rclist_drop(&rilist, &next)); /* while there are cells to check */

    /* clean up */
    if (pngbr_rc)
        G_free(pngbr_rc);
    pavl_destroy(no_check_tree, free_item);
    rclist_destroy(&rilist);

    return n_ngbrs;
}

#ifdef _OR_SHAPE_
double calculate_shape(struct reg_stats *rsi, struct reg_stats *rsk,
                       int nshared, struct globals *globals)
{
    /*
       In the eCognition literature, we find that the key factor in the
       multi-scale segmentation algorithm used by Definiens is the scale
       factor f:

       f = W.Hcolor + (1 - W).Hshape
       Hcolor = sum(b = 1:nbands)(Wb.SigmaB)
       Hshape = Ws.Hcompact + (1 - Ws).Hsmooth
       Hcompact = PL/sqrt(Npx)
       Hsmooth = PL/Pbbox

       Where
       W is a user-defined weight of importance of object radiometry vs
       shape (usually .9 vs .1)
       Wb is the weigh given to band B
       SigmaB is the std dev of the object for band b
       Ws is a user-defined weight giving the importance of compactedness vs
       smoothness
       PL is the perimeter length of the object
       Npx the number of pixels within the object
       Pbbox the perimeter of the bounding box of the
       object.
     */

    /* here we calculate a shape index for the new object to be created
     * the radiometric index ranges from 0 to 1, 0 = identical
     * with the shape index we want to favour compact and smooth objects
     * thus the shape index should range from 0 to 1,
     * 0 = maximum compactness and smoothness */

    double smooth, compact;
    int pl, pbbox, count;
    double bboxdiag;
    int pl1 = 0, pl2 = 0, count1 = 0, count2 = 0;
    int e1, n1, s1, w1, e2, n2, s2, w2, ns_extent, ew_extent;

    pl = pl1 + pl2 - nshared;

    ns_extent = MAX(n1, n2) - MIN(s1, s2);
    ew_extent = MAX(e1, e2) - MIN(w1, w2);

    pbbox = 2 * (ns_extent + ew_extent);

    /* Smoothness Hsmooth = PL / Pbbox
     * the smallest possible value would be
     * the diagonal divided by the bbox perimeter
     * invert such that the largest possible value would be
     * the bbox perimeter divided by the diagonal
     */

    bboxdiag = sqrt(ns_extent * ns_extent + ew_extent * ew_extent);
    smooth = 1. - (double)bboxdiag / pl; /* smaller -> smoother */

    count = count1 + count2;

    /* compactness Hcompact = PL / sqrt(Npx)
     * a circle is the most compact form
     * Npx = M_PI * r * r;
     * r = sqrt(Npx / M_pi)
     * pl_circle = 2 * sqrt(count * M_PI);
     * compact = 1 - pl_circle / (pl * sqrt(count);
     */
    /* compact = 1 - 2 * sqrt(M_PI) / pl; */

    /* PL max = Npx */
    /* Hcompact max = Npx / sqrt(Npx) = sqrt(Npx)
     * Hcompact / Hcompact max = (PL / sqrt(Npx)) / sqrt(Npx)
     *                         = PL / Npx
     */
    compact = (double)pl / count; /* smaller -> more compact */

    return globals->smooth_weight * smooth +
           (1 - globals->smooth_weight) * compact;
}
#endif

static int search_neighbors(struct ngbr_stats *Ri, struct reg_stats *Ri_rs,
                            struct NB_TREE *Ri_ngbrs, double *sim,
                            struct ngbr_stats *Rk, struct reg_stats *Rk_rs,
                            struct globals *globals)
{
    double tempsim, *dp;
    struct NB_TRAV travngbr;
    struct ngbr_stats *next;
    int cmp, candk, candtmp;

    G_debug(4, "search_neighbors");

    if (Ri->id != Ri_rs->id)
        G_fatal_error("Ri = %d but Ri_rs = %d", Ri->id, Ri_rs->id);
    if (Ri->id <= 0)
        G_fatal_error("Ri is %d", Ri->id);
    if (Ri_rs->id <= 0)
        G_fatal_error("Ri_rs is %d", Ri_rs->id);

    nbtree_init_trav(&travngbr, Ri_ngbrs);
    Rk->count = 0;
    Rk->id = Rk_rs->id = -1;
    candk = 0;

    while ((next = nbtree_traverse(&travngbr))) {
        tempsim = (globals->calculate_similarity)(Ri, next, globals);
        candtmp =
            (FLAG_GET(globals->candidate_flag, next->row, next->col)) != 0;

        cmp = compare_sim_ngbrs(tempsim, *sim, candtmp, candk, next, Rk);

        if (cmp == -1) {
            *sim = tempsim;
            candk = candtmp;

            dp = Rk->mean;
            *Rk = *next;
            Rk->mean = dp;
            memcpy(Rk->mean, next->mean, globals->datasize);
        }
    }
    Rk_rs->id = Rk->id;

    if (Rk->id >= 0) {
        fetch_reg_stats(Rk->row, Rk->col, Rk_rs, globals);
    }

    return 1;
}

int update_band_vals(int row, int col, struct reg_stats *rs,
                     struct globals *globals)
{
    struct rc next = {0}, ngbr_rc = {0};
    int neighbors[8][2];
    int rid, count, n;

    /* update band values with sum */
    /* rs->id must be set */
    G_debug(4, "update_band_vals()");

    if (rs->count >= globals->min_reg_size) {
        char buf[100];
        snprintf(buf, sizeof(buf), "%" PRI_LONG, globals->min_reg_size);
        G_fatal_error(_("Region stats should go in tree, %d >= %s"), rs->count,
                      buf);
    }

    Segment_get(&globals->rid_seg, (void *)&rid, row, col);

    if (rid != rs->id) {
        G_fatal_error(_("Region ids are different"));
    }

    if (rs->id < 1) {
        G_fatal_error(_("Region id %d is invalid"), rs->id);
    }

    if (rs->count == 1) {
        G_warning(_("Region consists of only one cell, nothing to update"));
        return rs->count;
    }

    /* update region stats */
    Segment_put(&globals->bands_seg, (void *)rs->sum, row, col);
    count = 1;

    /* fast version for rs->count == 2 */
    if (rs->count == 2) {
        globals->find_neighbors(row, col, neighbors);

        n = globals->nn - 1;
        do {

            ngbr_rc.row = neighbors[n][0];
            ngbr_rc.col = neighbors[n][1];

            if (ngbr_rc.row < globals->row_min ||
                ngbr_rc.row >= globals->row_max ||
                ngbr_rc.col < globals->col_min ||
                ngbr_rc.col >= globals->col_max) {
                continue;
            }

            if ((FLAG_GET(globals->null_flag, ngbr_rc.row, ngbr_rc.col)) == 0) {

                Segment_get(&globals->rid_seg, (void *)&rid, ngbr_rc.row,
                            ngbr_rc.col);

                if (rid == rs->id) {

                    /* update region stats */
                    Segment_put(&globals->bands_seg, (void *)rs->sum,
                                ngbr_rc.row, ngbr_rc.col);

                    count++;

                    /* only one other neighbor can have the same ID
                     * deactivate for debugging */
                    break;
                }
            }
        } while (n--);
        if (count > 2)
            G_fatal_error(_("Region size is larger than 2: %d"), count);
    }
    else if (rs->count > 2) {
        struct pavl_table *rc_check_tree; /* cells already checked */
        struct rclist rlist;
        struct rc *pngbr_rc;
        int no_check;

        /* go through region, spreading outwards from head */
        rclist_init(&rlist);

        ngbr_rc.row = row;
        ngbr_rc.col = col;
        pngbr_rc = G_malloc(sizeof(struct rc));
        *pngbr_rc = ngbr_rc;
        rc_check_tree = pavl_create(compare_rc, NULL);
        pavl_insert(rc_check_tree, pngbr_rc);
        pngbr_rc = NULL;

        next.row = row;
        next.col = col;
        do {
            G_debug(5, "find_pixel_neighbors for row: %d , col %d", next.row,
                    next.col);

            globals->find_neighbors(next.row, next.col, neighbors);

            n = globals->nn - 1;
            do {

                ngbr_rc.row = neighbors[n][0];
                ngbr_rc.col = neighbors[n][1];

                no_check = (ngbr_rc.row < 0 || ngbr_rc.row >= globals->nrows ||
                            ngbr_rc.col < 0 || ngbr_rc.col >= globals->ncols);

                if (!no_check) {
                    if ((FLAG_GET(globals->null_flag, ngbr_rc.row,
                                  ngbr_rc.col)) == 0) {

                        if (pngbr_rc == NULL)
                            pngbr_rc = G_malloc(sizeof(struct rc));
                        *pngbr_rc = ngbr_rc;

                        /* already checked ? */
                        if (pavl_insert(rc_check_tree, pngbr_rc) == NULL) {
                            pngbr_rc = NULL;

                            Segment_get(&globals->rid_seg, (void *)&rid,
                                        ngbr_rc.row, ngbr_rc.col);

                            if (rid == rs->id) {

                                /* want to check this neighbor's neighbors */
                                rclist_add(&rlist, ngbr_rc.row, ngbr_rc.col);

                                /* update region stats */
                                Segment_put(&globals->bands_seg,
                                            (void *)rs->sum, ngbr_rc.row,
                                            ngbr_rc.col);
                                count++;
                            }
                        }
                    }
                }
            } while (n--);
        } while (rclist_drop(&rlist, &next));

        /* clean up */
        if (pngbr_rc)
            G_free(pngbr_rc);
        pavl_destroy(rc_check_tree, free_item);
        rclist_destroy(&rlist);
    }

    if (count != rs->count) {
        G_fatal_error(_("Region size is %d, should be %d"), count, rs->count);
    }

    return count;
}

static int merge_regions(struct ngbr_stats *Ri, struct reg_stats *Ri_rs,
                         struct ngbr_stats *Rk, struct reg_stats *Rk_rs,
                         int do_cand, struct globals *globals)
{
    int n;
    int R_id;
    struct rc next, ngbr_rc;
    struct rclist rlist;
    int neighbors[8][2];
    struct reg_stats *new_rs;

    G_debug(4, "merge_regions");

    /* Ri ID must always be positive */
    if (Ri_rs->id < 1 && Ri_rs->count > 1)
        G_fatal_error("Ri id is not positive: %d, but count is > 1: %d",
                      Ri_rs->id, Ri_rs->count);
    /* if Rk ID is zero (no seed), Rk count must be 1  */
    if (Rk_rs->id < 1 && Rk_rs->count > 1)
        G_fatal_error("Rk id is not positive: %d, but count is > 1: %d",
                      Rk_rs->id, Rk_rs->count);

    /* update segment id and clear candidate flag */

    /* cases
     * Ri, Rk are not in the tree
     * Ri, Rk are both in the tree
     * Ri is in the tree, Rk is not
     * Rk is in the tree, Ri is not
     */

    /* Ri_rs, Rk_rs must always be set */
    /* add Rk */
    Ri_rs->count += Rk_rs->count;
    n = globals->nbands - 1;
    do {
        Ri_rs->sum[n] += Rk_rs->sum[n];
        Ri_rs->mean[n] = Ri_rs->sum[n] / Ri_rs->count;
    } while (n--);

    if (Ri->count >= Rk->count) {

        if (Ri->id == 0) {
            Ri->id = get_free_id(globals);
            Ri_rs->id = Ri->id;
            Segment_put(&globals->rid_seg, (void *)&Ri->id, Ri->row, Ri->col);
        }

        if (Rk->count >= globals->min_reg_size) {
            if (rgtree_find(globals->reg_tree, Rk_rs) == NULL)
                G_fatal_error("merge regions: Rk should be in tree");
            /* remove from tree */
            rgtree_remove(globals->reg_tree, Rk_rs);
        }
        add_free_id(Rk->id);
    }
    else {

        if (Ri->count >= globals->min_reg_size) {
            if (rgtree_find(globals->reg_tree, Ri_rs) == NULL)
                G_fatal_error("merge regions: Ri should be in tree");
            /* remove from tree */
            rgtree_remove(globals->reg_tree, Ri_rs);
        }
        add_free_id(Ri->id);

        /* magic switch */
        Ri_rs->id = Rk->id;
    }

    if ((new_rs = rgtree_find(globals->reg_tree, Ri_rs)) != NULL) {
        /* update stats for tree item */
        new_rs->count = Ri_rs->count;
        memcpy(new_rs->mean, Ri_rs->mean, globals->datasize);
        memcpy(new_rs->sum, Ri_rs->sum, globals->datasize);
    }
    else if (Ri_rs->count >= globals->min_reg_size) {
        /* add to tree */
        rgtree_insert(globals->reg_tree, Ri_rs);
    }

    Ri->count = Ri_rs->count;
    memcpy(Ri->mean, Ri_rs->mean, globals->datasize);

    if (Rk->id == 0) {
        /* the actual merge: change region id */
        Segment_put(&globals->rid_seg, (void *)&Ri->id, Rk->row, Rk->col);

        if (do_cand) {
            if (FLAG_GET(globals->candidate_flag, Rk->row, Rk->col)) {
                /* clear candidate flag */
                FLAG_UNSET(globals->candidate_flag, Rk->row, Rk->col);
                globals->candidate_count--;
            }
        }
    }
    else if (Ri->id == Ri_rs->id) {
        /* Ri is already updated, including candidate flags
         * need to clear candidate flag for Rk and set new id */

        /* the actual merge: change region id */
        Segment_put(&globals->rid_seg, (void *)&Ri->id, Rk->row, Rk->col);

        if (do_cand) {
            do_cand = 0;
            if (FLAG_GET(globals->candidate_flag, Rk->row, Rk->col)) {
                /* clear candidate flag */
                FLAG_UNSET(globals->candidate_flag, Rk->row, Rk->col);
                globals->candidate_count--;
                do_cand = 1;
            }
        }

        rclist_init(&rlist);
        if (Rk->count > 1)
            rclist_add(&rlist, Rk->row, Rk->col);

        while (rclist_drop(&rlist, &next)) {

            if (do_cand) {
                /* clear candidate flag */
                FLAG_UNSET(globals->candidate_flag, next.row, next.col);
                globals->candidate_count--;
            }

            globals->find_neighbors(next.row, next.col, neighbors);

            n = globals->nn - 1;
            do {

                ngbr_rc.row = neighbors[n][0];
                ngbr_rc.col = neighbors[n][1];

                if (ngbr_rc.row >= globals->row_min &&
                    ngbr_rc.row < globals->row_max &&
                    ngbr_rc.col >= globals->col_min &&
                    ngbr_rc.col < globals->col_max) {

                    if (!(FLAG_GET(globals->null_flag, ngbr_rc.row,
                                   ngbr_rc.col))) {

                        Segment_get(&globals->rid_seg, (void *)&R_id,
                                    ngbr_rc.row, ngbr_rc.col);

                        if (Rk->id > 0 && R_id == Rk->id) {
                            /* the actual merge: change region id */
                            Segment_put(&globals->rid_seg, (void *)&Ri->id,
                                        ngbr_rc.row, ngbr_rc.col);

                            /* want to check this neighbor's neighbors */
                            rclist_add(&rlist, ngbr_rc.row, ngbr_rc.col);
                        }
                    }
                }
            } while (n--);
        }
        rclist_destroy(&rlist);
    }
    else {
        /* Rk was larger than Ri */

        /* clear candidate flag for Rk */
        if (do_cand && FLAG_GET(globals->candidate_flag, Rk->row, Rk->col)) {
            set_candidate_flag(Rk, FALSE, globals);
        }

        /* update region id for Ri */

        /* the actual merge: change region id */
        Segment_put(&globals->rid_seg, (void *)&Rk->id, Ri->row, Ri->col);

        rclist_init(&rlist);
        if (Ri->count > 1)
            rclist_add(&rlist, Ri->row, Ri->col);

        while (rclist_drop(&rlist, &next)) {

            globals->find_neighbors(next.row, next.col, neighbors);

            n = globals->nn - 1;
            do {

                ngbr_rc.row = neighbors[n][0];
                ngbr_rc.col = neighbors[n][1];

                if (ngbr_rc.row >= globals->row_min &&
                    ngbr_rc.row < globals->row_max &&
                    ngbr_rc.col >= globals->col_min &&
                    ngbr_rc.col < globals->col_max) {

                    if (!(FLAG_GET(globals->null_flag, ngbr_rc.row,
                                   ngbr_rc.col))) {

                        Segment_get(&globals->rid_seg, (void *)&R_id,
                                    ngbr_rc.row, ngbr_rc.col);

                        if (Ri->id > 0 && R_id == Ri->id) {
                            /* the actual merge: change region id */
                            Segment_put(&globals->rid_seg, (void *)&Rk->id,
                                        ngbr_rc.row, ngbr_rc.col);

                            /* want to check this neighbor's neighbors */
                            rclist_add(&rlist, ngbr_rc.row, ngbr_rc.col);
                        }
                    }
                }
            } while (n--);
        }
        rclist_destroy(&rlist);

        Ri->id = Ri_rs->id; /* == Rk->id */
        if (Ri->id != Rk->id)
            G_fatal_error("Ri ID should be set to Rk ID");
    }

    /* disable Rk */
    Rk->id = Rk_rs->id = -1;
    Rk->count = Rk_rs->count = 0;

    /* update Ri */
    Ri->id = Ri_rs->id;

    if (Ri_rs->count < globals->min_reg_size) {
        update_band_vals(Ri->row, Ri->col, Ri_rs, globals);
    }

    return TRUE;
}

static int set_candidate_flag(struct ngbr_stats *head, int value,
                              struct globals *globals)
{
    int n, R_id;
    struct rc next, ngbr_rc;
    struct rclist rlist;
    int neighbors[8][2];

    G_debug(4, "set_candidate_flag");

    if ((!(FLAG_GET(globals->candidate_flag, head->row, head->col))) != value) {
        G_warning(_("Candidate flag is already %s"),
                  value ? _("set") : _("unset"));
        return FALSE;
    }

    /* (un)set candidate flag */
    if (value == TRUE) {
        FLAG_SET(globals->candidate_flag, head->row, head->col);
        globals->candidate_count++;
    }
    else {
        FLAG_UNSET(globals->candidate_flag, head->row, head->col);
        globals->candidate_count--;
    }

    if (head->id == 0)
        return TRUE;

    rclist_init(&rlist);
    rclist_add(&rlist, head->row, head->col);

    while (rclist_drop(&rlist, &next)) {

        globals->find_neighbors(next.row, next.col, neighbors);

        n = globals->nn - 1;
        do {

            ngbr_rc.row = neighbors[n][0];
            ngbr_rc.col = neighbors[n][1];

            if (ngbr_rc.row >= globals->row_min &&
                ngbr_rc.row < globals->row_max &&
                ngbr_rc.col >= globals->col_min &&
                ngbr_rc.col < globals->col_max) {

                if (!(FLAG_GET(globals->null_flag, ngbr_rc.row, ngbr_rc.col))) {

                    if ((!(FLAG_GET(globals->candidate_flag, ngbr_rc.row,
                                    ngbr_rc.col))) == value) {

                        Segment_get(&globals->rid_seg, (void *)&R_id,
                                    ngbr_rc.row, ngbr_rc.col);

                        if (R_id == head->id) {
                            /* want to check this neighbor's neighbors */
                            rclist_add(&rlist, ngbr_rc.row, ngbr_rc.col);

                            /* (un)set candidate flag */
                            if (value == TRUE) {
                                FLAG_SET(globals->candidate_flag, ngbr_rc.row,
                                         ngbr_rc.col);
                                globals->candidate_count++;
                            }
                            else {
                                FLAG_UNSET(globals->candidate_flag, ngbr_rc.row,
                                           ngbr_rc.col);
                                globals->candidate_count--;
                            }
                        }
                    }
                }
            }
        } while (n--);
    }
    rclist_destroy(&rlist);

    return TRUE;
}

int fetch_reg_stats(int row, int col, struct reg_stats *rs,
                    struct globals *globals)
{
    struct reg_stats *rs_found;

    if (rs->id < 0)
        G_fatal_error("fetch_reg_stats(): invalid region id %d", rs->id);

    if (rs->id > 0 && (rs_found = rgtree_find(globals->reg_tree, rs)) != NULL) {

        memcpy(rs->mean, rs_found->mean, globals->datasize);
        memcpy(rs->sum, rs_found->sum, globals->datasize);
        rs->count = rs_found->count;

        return 1;
    }

    calculate_reg_stats(row, col, rs, globals);

    return 2;
}

static int calculate_reg_stats(int row, int col, struct reg_stats *rs,
                               struct globals *globals)
{
    int ret = 0;

    G_debug(4, "calculate_reg_stats()");

    if (rs->id < 0)
        G_fatal_error("Invalid region id %d", rs->id);

    Segment_get(&globals->bands_seg, (void *)globals->bands_val, row, col);
    rs->count = 1;
    memcpy(rs->sum, globals->bands_val, globals->datasize);

    if (rs->id == 0) {
        memcpy(rs->mean, rs->sum, globals->datasize);

        return 1;
    }

    if (globals->min_reg_size < 3)
        ret = 1;
    else if (globals->min_reg_size == 3) {
        int n, rid;
        struct rc ngbr_rc = {0};
        int neighbors[8][2];

        globals->find_neighbors(row, col, neighbors);

        n = globals->nn - 1;
        do {

            ngbr_rc.row = neighbors[n][0];
            ngbr_rc.col = neighbors[n][1];

            if (ngbr_rc.row < globals->row_min ||
                ngbr_rc.row >= globals->row_max ||
                ngbr_rc.col < globals->col_min ||
                ngbr_rc.col >= globals->col_max) {
                continue;
            }

            if ((FLAG_GET(globals->null_flag, ngbr_rc.row, ngbr_rc.col)) == 0) {

                Segment_get(&globals->rid_seg, (void *)&rid, ngbr_rc.row,
                            ngbr_rc.col);

                if (rid == rs->id) {

                    /* update region stats */
                    rs->count++;

                    /* only one other neighbor can have the same ID
                     * deactivate for debugging */
                    break;
                }
            }
        } while (n--);
        if (rs->count > 2)
            G_fatal_error(_("Region size is larger than 2: %d"), rs->count);

        ret = 2;
    }
    else if (globals->min_reg_size > 3) {
        /* rs->id must be set */
        struct pavl_table *rc_check_tree; /* cells already checked */
        int n, rid;
        struct rc ngbr_rc = {0}, *pngbr_rc = NULL, next = {0};
        struct rclist rilist;
        int neighbors[8][2];
        int no_check;

        /* go through region, spreading outwards from head */
        rclist_init(&rilist);

        ngbr_rc.row = row;
        ngbr_rc.col = col;
        pngbr_rc = G_malloc(sizeof(struct rc));
        *pngbr_rc = ngbr_rc;
        rc_check_tree = pavl_create(compare_rc, NULL);
        pavl_insert(rc_check_tree, pngbr_rc);
        pngbr_rc = NULL;

        next.row = row;
        next.col = col;
        do {
            G_debug(5, "find_pixel_neighbors for row: %d , col %d", next.row,
                    next.col);

            globals->find_neighbors(next.row, next.col, neighbors);

            n = globals->nn - 1;
            do {

                ngbr_rc.row = neighbors[n][0];
                ngbr_rc.col = neighbors[n][1];

                no_check = (ngbr_rc.row < globals->row_min ||
                            ngbr_rc.row >= globals->row_max ||
                            ngbr_rc.col < globals->col_min ||
                            ngbr_rc.col >= globals->col_max);

                if (!no_check) {
                    if ((FLAG_GET(globals->null_flag, ngbr_rc.row,
                                  ngbr_rc.col)) == 0) {

                        if (pngbr_rc == NULL)
                            pngbr_rc = G_malloc(sizeof(struct rc));
                        *pngbr_rc = ngbr_rc;

                        /* already checked ? */
                        if (pavl_insert(rc_check_tree, pngbr_rc) == NULL) {
                            pngbr_rc = NULL;

                            Segment_get(&globals->rid_seg, (void *)&rid,
                                        ngbr_rc.row, ngbr_rc.col);

                            if (rid == rs->id) {

                                /* want to check this neighbor's neighbors */
                                rclist_add(&rilist, ngbr_rc.row, ngbr_rc.col);

                                /* update region stats */
                                rs->count++;
                            }
                        }
                    }
                }
            } while (n--);
        } while (rclist_drop(&rilist, &next));

        /* clean up */
        if (pngbr_rc)
            G_free(pngbr_rc);
        pavl_destroy(rc_check_tree, free_item);
        rclist_destroy(&rilist);

        ret = 3;
    }

    /* band mean */
    if (rs->count == 1)
        memcpy(rs->mean, rs->sum, globals->datasize);
    else {
        int i = globals->nbands - 1;

        do {
            rs->mean[i] = rs->sum[i] / rs->count;
        } while (i--);
    }

    if (rs->count >= globals->min_reg_size)
        G_fatal_error(_("Region of size %d should be in search tree"),
                      rs->count);

    return ret;
}
