/*!
   \file lib/vector/Vlib/build.c

   \brief Vector library - Building topology

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2010 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */
#include <grass/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/vector.h>


#ifndef HAVE_OGR
static int format()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}
#endif

static int (*Build_array[]) () = {
    Vect_build_nat
#ifdef HAVE_OGR
	, Vect_build_ogr
	, Vect_build_ogr
#else
	, format
        , format
#endif
};

/*!
   \brief Build topology for vector map

   \param Map vector map

   \return 1 on success
   \return 0 on error
 */
int Vect_build(struct Map_info *Map)
{
    return Vect_build_partial(Map, GV_BUILD_ALL);
}

/*!
   \brief Return current highest built level (part)

   \param Map vector map

   \return current highest built level
 */
int Vect_get_built(const struct Map_info *Map)
{
    return (Map->plus.built);
}

/*!
   \brief Build partial topology for vector map.

   Should only be used in special cases of vector processing.

   This functions optionally builds only some parts of topology. Highest level is specified by build
   parameter which may be:
   - GV_BUILD_NONE - nothing is build;
   - GV_BUILD_BASE - basic topology, nodes, spatial index;
   - GV_BUILD_AREAS - build areas and islands, but islands are not attached to areas;
   - GV_BUILD_ATTACH_ISLES - attach islands to areas;
   - GV_BUILD_CENTROIDS - assign centroids to areas;
   - GV_BUILD_ALL - top level, the same as GV_BUILD_CENTROIDS.

   If functions is called with build lower than current value of the
   Map, the level is downgraded to requested value.

   All calls to Vect_write_line(), Vect_rewrite_line(),
   Vect_delete_line() respect the last value of build used in this
   function.

   Values lower than GV_BUILD_ALL are supported only by
   GV_FORMAT_NATIVE, other formats ignore build and build always
   GV_BUILD_ALL

   Note that the functions has effect only if requested level is
   higher than current level, to rebuild part of topology, call first
   downgrade and then upgrade, for example:

   - Vect_build()
   - Vect_build_partial(,GV_BUILD_BASE,)
   - Vect_build_partial(,GV_BUILD_AREAS,) 

   \param Map vector map
   \param build highest level of build

   \return 1 on success
   \return 0 on error
 */
int Vect_build_partial(struct Map_info *Map, int build)
{
    struct Plus_head *plus;
    int ret;

    G_debug(3, "Vect_build(): build = %d", build);

    /* If topology is already build (map on level2), set level to 1 so that lines will
     *  be read by V1_read_ (all lines) */
    Map->level = 1;		/* may be not needed, because  V1_read is used directly by Vect_build_ */
    if (Map->format != GV_FORMAT_OGR_DIRECT)
	Map->support_updated = 1;

    if (Map->plus.Spidx_built == 0)
	Vect_open_sidx(Map, 2);

    plus = &(Map->plus);
    if (build > GV_BUILD_NONE) {
	G_message(_("Building topology for vector map <%s>..."),
		  Vect_get_full_name(Map));
    }
    plus->with_z = Map->head.with_z;
    plus->spidx_with_z = Map->head.with_z;

    if (build == GV_BUILD_ALL) {
	dig_cidx_free(plus);	/* free old (if any) category index) */
	dig_cidx_init(plus);
    }

    ret = ((*Build_array[Map->format]) (Map, build));

    if (ret == 0) {
	return 0;
    }

    if (build > GV_BUILD_NONE) {
	G_verbose_message(_("Topology was built"));
    }

    Map->level = LEVEL_2;
    plus->mode = GV_MODE_WRITE;

    if (build == GV_BUILD_ALL) {
	plus->cidx_up_to_date = 1;	/* category index was build */
	dig_cidx_sort(plus);
    }

    if (build > GV_BUILD_NONE) {
	G_message(_("Number of nodes: %d"), plus->n_nodes);
	G_message(_("Number of primitives: %d"), plus->n_lines);
	G_message(_("Number of points: %d"), plus->n_plines);
	G_message(_("Number of lines: %d"), plus->n_llines);
	G_message(_("Number of boundaries: %d"), plus->n_blines);
	G_message(_("Number of centroids: %d"), plus->n_clines);

	if (plus->n_flines > 0)
	    G_message(_("Number of faces: %d"), plus->n_flines);

	if (plus->n_klines > 0)
	    G_message(_("Number of kernels: %d"), plus->n_klines);
    }

    if (plus->built >= GV_BUILD_AREAS) {
	int line, nlines, area, nareas, err_boundaries, err_centr_out,
	    err_centr_dupl, err_nocentr;
	struct P_line *Line;
	struct Plus_head *Plus;

	/* Count errors (it does not take much time comparing to build process) */
	Plus = &(Map->plus);
	nlines = Vect_get_num_lines(Map);
	err_boundaries = err_centr_out = err_centr_dupl = 0;
	for (line = 1; line <= nlines; line++) {
	    Line = Plus->Line[line];
	    if (!Line)
		continue;
	    if (Line->type == GV_BOUNDARY &&
		(Line->left == 0 || Line->right == 0)) {
		G_debug(3, "line = %d left = %d right = %d", line, Line->left,
			Line->right);
		err_boundaries++;
	    }
	    if (Line->type == GV_CENTROID) {
		if (Line->left == 0)
		    err_centr_out++;
		else if (Line->left < 0)
		    err_centr_dupl++;
	    }
	}

	err_nocentr = 0;
	nareas = Vect_get_num_areas(Map);
	for (area = 1; area <= nareas; area++) {
	    if (!Vect_area_alive(Map, area))
		continue;
	    line = Vect_get_area_centroid(Map, area);
	    if (line == 0)
		err_nocentr++;
	}

	G_message(_("Number of areas: %d"), plus->n_areas);
	G_message(_("Number of isles: %d"), plus->n_isles);

	if (err_boundaries)
	    G_message(_("Number of incorrect boundaries: %d"),
		      err_boundaries);

	if (err_centr_out)
	    G_message(_("Number of centroids outside area: %d"),
		      err_centr_out);

	if (err_centr_dupl)
	    G_message(_("Number of duplicate centroids: %d"), err_centr_dupl);

	if (err_nocentr)
	    G_message(_("Number of areas without centroid: %d"), err_nocentr);

    }
    else if (build > GV_BUILD_NONE) {
	G_message(_("Number of areas: -"));
	G_message(_("Number of isles: -"));
    }
    return 1;
}

/*!
   \brief Save topology file for vector map

   \param Map vector map

   \return 1 on success
   \return 0 on error
 */
int Vect_save_topo(struct Map_info *Map)
{
    struct Plus_head *plus;
    char fname[GPATH_MAX], buf[GPATH_MAX];
    struct gvfile fp;

    G_debug(1, "Vect_save_topo()");

    plus = &(Map->plus);

    /*  write out all the accumulated info to the plus file  */
    sprintf(buf, "%s/%s", GV_DIRECTORY, Map->name);
    G_file_name(fname, buf, GV_TOPO_ELEMENT, Map->mapset);
    G_debug(1, "Open topo: %s", fname);
    dig_file_init(&fp);
    fp.file = fopen(fname, "w");
    if (fp.file == NULL) {
	G_warning(_("Unable to open topo file for write <%s>"), fname);
	return 0;
    }

    /* set portable info */
    dig_init_portable(&(plus->port), dig__byte_order_out());

    if (0 > dig_write_plus_file(&fp, plus)) {
	G_warning(_("Error writing out topo file"));
	return 0;
    }

    fclose(fp.file);

    return 1;
}

/*!
   \brief Dump topology to file

   \param Map vector map
   \param out file for output (stdout/stderr for example)

   \return 1 on success
   \return 0 on error
 */
int Vect_topo_dump(const struct Map_info *Map, FILE *out)
{
    int i, j, line, isle;
    struct P_node *Node;
    struct P_line *Line;
    struct P_area *Area;
    struct P_isle *Isle;
    struct bound_box box;
    const struct Plus_head *plus;

    plus = &(Map->plus);
    
    fprintf(out, "---------- TOPOLOGY DUMP ----------\n");

    /* box */
    Vect_box_copy(&box, &(plus->box));
    fprintf(out, "N,S,E,W,T,B: %f, %f, %f, %f, %f, %f\n", box.N, box.S,
	    box.E, box.W, box.T, box.B);

    /* nodes */
    fprintf(out, "Nodes (%d nodes, alive + dead ):\n", plus->n_nodes);
    for (i = 1; i <= plus->n_nodes; i++) {
	if (plus->Node[i] == NULL) {
	    continue;
	}
	Node = plus->Node[i];
	fprintf(out, "node = %d, n_lines = %d, xyz = %f, %f, %f\n", i,
		Node->n_lines, Node->x, Node->y, Node->z);
	for (j = 0; j < Node->n_lines; j++) {
	    line = Node->lines[j];
	    Line = plus->Line[abs(line)];
	    fprintf(out, "  line = %3d, type = %d, angle = %f\n", line,
		    Line->type, Node->angles[j]);
	}
    }

    /* lines */
    fprintf(out, "Lines (%d lines, alive + dead ):\n", plus->n_lines);
    for (i = 1; i <= plus->n_lines; i++) {
	if (plus->Line[i] == NULL) {
	    continue;
	}
	Line = plus->Line[i];
	fprintf(out, "line = %d, type = %d, offset = %lu n1 = %d, n2 = %d, "
		"left/area = %d, right = %d\n",
		i, Line->type, (unsigned long)Line->offset, Line->N1,
		Line->N2, Line->left, Line->right);
	fprintf(out, "N,S,E,W,T,B: %f, %f, %f, %f, %f, %f\n", Line->N,
		Line->S, Line->E, Line->W, Line->T, Line->B);
    }

    /* areas */
    fprintf(out, "Areas (%d areas, alive + dead ):\n", plus->n_areas);
    for (i = 1; i <= plus->n_areas; i++) {
	if (plus->Area[i] == NULL) {
	    continue;
	}
	Area = plus->Area[i];

	fprintf(out, "area = %d, n_lines = %d, n_isles = %d centroid = %d\n",
		i, Area->n_lines, Area->n_isles, Area->centroid);

	fprintf(out, "N,S,E,W,T,B: %f, %f, %f, %f, %f, %f\n", Area->N,
		Area->S, Area->E, Area->W, Area->T, Area->B);

	for (j = 0; j < Area->n_lines; j++) {
	    line = Area->lines[j];
	    Line = plus->Line[abs(line)];
	    fprintf(out, "  line = %3d\n", line);
	}
	for (j = 0; j < Area->n_isles; j++) {
	    isle = Area->isles[j];
	    fprintf(out, "  isle = %3d\n", isle);
	}
    }

    /* isles */
    fprintf(out, "Islands (%d islands, alive + dead ):\n", plus->n_isles);
    for (i = 1; i <= plus->n_isles; i++) {
	if (plus->Isle[i] == NULL) {
	    continue;
	}
	Isle = plus->Isle[i];

	fprintf(out, "isle = %d, n_lines = %d area = %d\n", i, Isle->n_lines,
		Isle->area);

	fprintf(out, "N,S,E,W,T,B: %f, %f, %f, %f, %f, %f\n", Isle->N,
		Isle->S, Isle->E, Isle->W, Isle->T, Isle->B);

	for (j = 0; j < Isle->n_lines; j++) {
	    line = Isle->lines[j];
	    Line = plus->Line[abs(line)];
	    fprintf(out, "  line = %3d\n", line);
	}
    }

    return 1;
}

/*!
   \brief Create spatial index if necessary.

   To be used in modules.
   Map must be opened on level 2.

   \param[in,out] Map pointer to vector map

   \return 0 OK
   \return 1 error
 */
int Vect_build_sidx(struct Map_info *Map)
{
    if (Map->level < 2) {
	G_fatal_error(_("Unable to build spatial index from topology, "
			"vector map is not opened at topology level 2"));
    }
    if (!(Map->plus.Spidx_built)) {
	return (Vect_build_sidx_from_topo(Map));
    }
    return 0;
}

/*!
   \brief Create spatial index from topology if necessary

   \param Map pointer to vector map

   \return 0 OK
   \return 1 error
 */
int Vect_build_sidx_from_topo(struct Map_info *Map)
{
    int i, total, done;
    struct Plus_head *plus;
    struct bound_box box;
    struct P_line *Line;
    struct P_node *Node;
    struct P_area *Area;
    struct P_isle *Isle;

    G_debug(3, "Vect_build_sidx_from_topo()");

    plus = &(Map->plus);

    Vect_open_sidx(Map, 2);

    total = plus->n_nodes + plus->n_lines + plus->n_areas + plus->n_isles;

    /* Nodes */
    for (i = 1; i <= plus->n_nodes; i++) {
	G_percent(i, total, 3);

	Node = plus->Node[i];
	if (!Node)
	    G_fatal_error(_("BUG (Vect_build_sidx_from_topo): node does not exist"));

	dig_spidx_add_node(plus, i, Node->x, Node->y, Node->z);
    }

    /* Lines */
    done = plus->n_nodes;
    for (i = 1; i <= plus->n_lines; i++) {
	G_percent(done + i, total, 3);

	Line = plus->Line[i];
	if (!Line)
	    G_fatal_error(_("BUG (Vect_build_sidx_from_topo): line does not exist"));

	box.N = Line->N;
	box.S = Line->S;
	box.E = Line->E;
	box.W = Line->W;
	box.T = Line->T;
	box.B = Line->B;

	dig_spidx_add_line(plus, i, &box);
    }

    /* Areas */
    done += plus->n_lines;
    for (i = 1; i <= plus->n_areas; i++) {
	G_percent(done + i, total, 3);

	Area = plus->Area[i];
	if (!Area)
	    G_fatal_error(_("BUG (Vect_build_sidx_from_topo): area does not exist"));

	box.N = Area->N;
	box.S = Area->S;
	box.E = Area->E;
	box.W = Area->W;
	box.T = Area->T;
	box.B = Area->B;

	dig_spidx_add_area(plus, i, &box);
    }

    /* Isles */
    done += plus->n_areas;
    for (i = 1; i <= plus->n_isles; i++) {
	G_percent(done + i, total, 3);

	Isle = plus->Isle[i];
	if (!Isle)
	    G_fatal_error(_("BUG (Vect_build_sidx_from_topo): isle does not exist"));

	box.N = Isle->N;
	box.S = Isle->S;
	box.E = Isle->E;
	box.W = Isle->W;
	box.T = Isle->T;
	box.B = Isle->B;

	dig_spidx_add_isle(plus, i, &box);
    }

    Map->plus.Spidx_built = 1;

    G_debug(3, "Spatial index was built");

    return 0;
}

/*!
   \brief Save spatial index file for vector map

   \param Map vector map

   \return 1 on success
   \return 0 on error
 */
int Vect_save_sidx(struct Map_info *Map)
{
    struct Plus_head *plus;
    char fname[GPATH_MAX], buf[GPATH_MAX];

    G_debug(1, "Vect_save_spatial_index()");

    plus = &(Map->plus);

    if (plus->Spidx_built == 0) {
	G_warning("Spatial index not available, can not be saved");
	return 0;
    }

    /* new or update mode ? */
    if (plus->Spidx_new == 1) {

	/*  write out rtrees to sidx file  */
	sprintf(buf, "%s/%s", GV_DIRECTORY, Map->name);
	G_file_name(fname, buf, GV_SIDX_ELEMENT, Map->mapset);
	G_debug(1, "Open sidx: %s", fname);
	dig_file_init(&(plus->spidx_fp));
	plus->spidx_fp.file = fopen(fname, "w+");
	if (plus->spidx_fp.file == NULL) {
	    G_warning(_("Unable open spatial index file for write <%s>"),
		      fname);
	    return 0;
	}

	/* set portable info */
	dig_init_portable(&(plus->spidx_port), dig__byte_order_out());

	if (0 > dig_Wr_spidx(&(plus->spidx_fp), plus)) {
	    G_warning(_("Error writing out spatial index file"));
	    return 0;
	}
	dig_spidx_free(plus);
	Map->plus.Spidx_new = 0;
    }

    fclose(Map->plus.spidx_fp.file);

    Map->plus.Spidx_built = 0;

    return 1;
}

/*!
   \brief Dump spatial index to file

   \param Map vector map
   \param out file for output (stdout/stderr for example)

   \return 1 on success
   \return 0 on error
 */
int Vect_sidx_dump(struct Map_info *Map, FILE * out)
{
    if (!(Map->plus.Spidx_built)) {
	Vect_build_sidx_from_topo(Map);
    }

    fprintf(out, "---------- SPATIAL INDEX DUMP ----------\n");

    dig_dump_spidx(out, &(Map->plus));

    return 1;
}
