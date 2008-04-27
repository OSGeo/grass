/*!
  \file build.c
  
  \brief Vector library - Building topology
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  Update to GRASS 5.7 Radim Blazek and David D. Gray.
  
  \date 2001-2008
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/Vect.h>


#ifndef HAVE_OGR
static int format () { G_fatal_error (_("Requested format is not compiled in this version")); return 0; }
#endif

static int (*Build_array[]) () =
{
      Vect_build_nat
#ifdef HAVE_OGR
    , Vect_build_ogr
#else
    , format
#endif
};

FILE *Msgout = NULL;


int prnmsg ( char *msg, ...) {
    char buffer[1000]; 
    va_list ap; 
    
    if ( Msgout != NULL ) {
        va_start(ap,msg);
	vsprintf(buffer,msg,ap);
	va_end(ap);
	fprintf (Msgout, "%s", buffer);
	fflush (Msgout);
    }
	
    return 1;	
}

/*!
  \brief Build topology for vector map

  \param Map vector map
  \param msgout file for message output (stdout/stderr for example) or NULL

  \return 1 on success
  \return 0 on error
*/
int
Vect_build ( struct Map_info *Map, FILE *msgout ) 
{
    return Vect_build_partial ( Map, GV_BUILD_ALL, msgout );
}


/*!
  \brief Return current highest built level (part)

  \param Map vector map

  \return current highest built level
*/
int
Vect_get_built ( struct Map_info *Map ) 
{
    return ( Map->plus.built );
}

/*!
  \brief Build partial topology for vector map.

  Should only be used in special cases of vector processing.

  This functions optionaly builds only some parts of topology. Highest level is specified by build
  parameter which may be:
   - GV_BUILD_NONE - nothing is build;
   - GV_BUILD_BASE - basic topology, nodes, spatial index;
   - GV_BUILD_AREAS - build areas and islands, but islands are not attached to areas;
   - GV_BUILD_ATTACH_ISLES - attach islands to areas;
   - GV_BUILD_CENTROIDS - assign centroids to areas;
   - GV_BUILD_ALL - top level, the same as GV_BUILD_CENTROIDS.

   If functions is called with build lower than current value of the Map, the level is downgraded to 
   requested value.

   All calls to Vect_write_line(), Vect_rewrite_line(), Vect_delete_line() respect the last value of 
   build used in this function.

   Values lower than GV_BUILD_ALL are supported only by GV_FORMAT_NATIVE,
   other formats ignore build and build always GV_BUILD_ALL
   
   Note that the functions has effect only if requested level is higher than current level, to rebuild
   part of topology, call first downgrade and then upgrade, for example:
   
   Vect_build()
   Vect_build_partial(,GV_BUILD_BASE,)
   Vect_build_partial(,GV_BUILD_AREAS,) 
 

   \param Map vector map
   \param build highest level of build
   \param msgout file pointer for message output (stdout/stderr for example) or NULL

   \return 1 on success, 0 on error
*/
int
Vect_build_partial ( struct Map_info *Map, int build, FILE *msgout ) 
{
    struct Plus_head *plus ;
    int    ret;
    
    G_debug (3, "Vect_build(): build = %d", build); 
    Msgout = msgout;

    /* If topology is already build (map on level2), set level to 1 so that lines will
    *  be read by V1_read_ (all lines) */
    Map->level = 1; /* may be not needed, because  V1_read is used directly by Vect_build_ */
    Map->support_updated = 1;

    Map->plus.Spidx_built = 1;
    
    plus = &(Map->plus);
    prnmsg ( _("Building topology for vector map <%s>...\n"), Vect_get_name(Map));
    plus->with_z = Map->head.with_z;
    plus->spidx_with_z = Map->head.with_z;

    if ( build == GV_BUILD_ALL ) {
	dig_cidx_free(plus); /* free old (if any) category index) */
	dig_cidx_init(plus);
    }
    
    ret = ( (*Build_array[Map->format]) (Map, build, msgout) );

    if ( ret == 0 ) { return 0; } 

    prnmsg (_("Topology was built\n"));
    
    Map->level = LEVEL_2;
    plus->mode = GV_MODE_WRITE;
    
    if ( build == GV_BUILD_ALL ) {
        plus->cidx_up_to_date = 1; /* category index was build */
	dig_cidx_sort ( plus );
    }
    
    /* prnmsg ("Topology was built.\n") ; */
   
    prnmsg (_("Number of nodes     :   %d\n"), plus->n_nodes);
    prnmsg (_("Number of primitives:   %d\n"), plus->n_lines);
    prnmsg (_("Number of points    :   %d\n"), plus->n_plines);
    prnmsg (_("Number of lines     :   %d\n"), plus->n_llines);
    prnmsg (_("Number of boundaries:   %d\n"), plus->n_blines);
    prnmsg (_("Number of centroids :   %d\n"), plus->n_clines);

    if ( plus->n_flines > 0 )
        prnmsg (_("Number of faces     :   %d\n"), plus->n_flines);

    if ( plus->n_klines > 0 )
        prnmsg (_("Number of kernels   :   %d\n"), plus->n_klines);

    if ( plus->built >= GV_BUILD_AREAS ) {
	int line, nlines, area, nareas, err_boundaries, err_centr_out, err_centr_dupl, err_nocentr;
	P_LINE *Line;
	struct Plus_head *Plus;
	
	/* Count errors (it does not take much time comparing to build process) */
	Plus = &(Map->plus);
	nlines = Vect_get_num_lines (Map);
	err_boundaries = err_centr_out = err_centr_dupl = 0;
	for ( line = 1; line <= nlines; line++ ){
	    Line = Plus->Line[line];
	    if ( !Line ) continue;
	    if ( Line->type == GV_BOUNDARY && ( Line->left == 0 || Line->right == 0 ) ) {
		G_debug ( 3, "line = %d left = %d right = %d", line,  Line->left, Line->right);
		err_boundaries++; 
	    }
	    if ( Line->type == GV_CENTROID ) {
		if ( Line->left == 0 ) 
		    err_centr_out++;
		else if ( Line->left < 0 )
		    err_centr_dupl++;
	    }
	}

	err_nocentr = 0;
	nareas = Vect_get_num_areas (Map);
	for ( area = 1; area <= nareas; area++ ){
	    if ( !Vect_area_alive(Map, area ) ) continue;
	    line = Vect_get_area_centroid ( Map, area );
	    if ( line == 0 ) 
		err_nocentr++;
	}
	    
	prnmsg (_("Number of areas     :   %d\n"), plus->n_areas);
	prnmsg (_("Number of isles     :   %d\n"), plus->n_isles);

	if ( err_boundaries )
	    prnmsg (_("Number of incorrect boundaries   :   %d\n"), err_boundaries);

	if ( err_centr_out )
	    prnmsg (_("Number of centroids outside area :   %d\n"), err_centr_out);
	
	if ( err_centr_dupl )
	    prnmsg (_("Number of duplicate centroids    :   %d\n"), err_centr_dupl);

	if ( err_nocentr )
	    prnmsg (_("Number of areas without centroid :   %d\n"), err_nocentr);
	    
    } else {
	prnmsg (_("Number of areas     :   -\n"));
	prnmsg (_("Number of isles     :   -\n"));
    }
    return 1;
}

/*!
  \brief Save topology file for vector map

  \param Map vector map

  \return 1 on success, 0 on error
*/
int
Vect_save_topo ( struct Map_info *Map )
{
    struct Plus_head *plus ;
    char   fname[GPATH_MAX], buf[GPATH_MAX];
    GVFILE  fp;
    
    G_debug (1, "Vect_save_topo()"); 

    plus = &(Map->plus);
    
    /*  write out all the accumulated info to the plus file  */
    sprintf (buf, "%s/%s", GRASS_VECT_DIRECTORY, Map->name);
    G__file_name (fname, buf, GV_TOPO_ELEMENT, Map->mapset);
    G_debug (1, "Open topo: %s", fname);
    dig_file_init ( &fp );
    fp.file = fopen( fname, "w");
    if ( fp.file ==  NULL) {
        G_warning(_("Unable to open topo file for write <%s>"), fname);
	return 0;
    }

    /* set portable info */
    dig_init_portable ( &(plus->port), dig__byte_order_out ());
    
    if ( 0 > dig_write_plus_file (&fp, plus) ) {
        G_warning (_("Error writing out topo file"));
	return 0;
    }
    
    fclose( fp.file );

    return 1;
}

/*!
  \brief Dump topology to file

  \param Map vector map
  \param out file for output (stdout/stderr for example)

  \return 1 on success
  \return 0 on error
*/
int
Vect_topo_dump ( struct Map_info *Map, FILE *out )
{
    int i, j, line, isle;
    P_NODE *Node;
    P_LINE *Line;
    P_AREA *Area;
    P_ISLE *Isle;
    BOUND_BOX box;
    struct Plus_head *plus;

    plus = &(Map->plus);

    fprintf (out, "---------- TOPOLOGY DUMP ----------\n" ); 
    
    /* box */
    Vect_box_copy ( &box, &(plus->box) );
    fprintf (out, "N,S,E,W,T,B: %f, %f, %f, %f, %f, %f\n", box.N, box.S, 
	                         box.E, box.W, box.T, box.B);
    
    /* nodes */
    fprintf (out, "Nodes (%d nodes, alive + dead ):\n", plus->n_nodes ); 
    for (i = 1; i <= plus->n_nodes; i++) {
	if ( plus->Node[i] == NULL ) { continue; }
	Node = plus->Node[i];
	fprintf (out, "node = %d, n_lines = %d, xy = %f, %f\n", i, Node->n_lines,
	                            Node->x, Node->y ); 
        for (j = 0; j < Node->n_lines; j++) {
	    line = Node->lines[j];
	    Line = plus->Line[abs(line)];
	    fprintf (out, "  line = %3d, type = %d, angle = %f\n", line, Line->type, Node->angles[j] ); 
	}
    }
    
    /* lines */
    fprintf (out, "Lines (%d lines, alive + dead ):\n", plus->n_lines ); 
    for (i = 1; i <= plus->n_lines; i++) {
	if ( plus->Line[i] == NULL ) { continue; }
	Line = plus->Line[i];
	fprintf (out, "line = %d, type = %d, offset = %ld n1 = %d, n2 = %d, "
	              "left/area = %d, right = %d\n",
		       i, Line->type, Line->offset, Line->N1, Line->N2,
	               Line->left, Line->right); 
	fprintf (out, "N,S,E,W,T,B: %f, %f, %f, %f, %f, %f\n", Line->N, Line->S,
	                       Line->E, Line->W, Line->T, Line->B);	
    }
    
    /* areas */
    fprintf (out, "Areas (%d areas, alive + dead ):\n", plus->n_areas ); 
    for (i = 1; i <= plus->n_areas; i++) {
	if ( plus->Area[i] == NULL ) { continue; }
	Area = plus->Area[i];
	
	fprintf (out, "area = %d, n_lines = %d, n_isles = %d centroid = %d\n", 
		 i, Area->n_lines, Area->n_isles, Area->centroid ); 
	
	fprintf (out, "N,S,E,W,T,B: %f, %f, %f, %f, %f, %f\n", Area->N, Area->S,
	                       Area->E, Area->W, Area->T, Area->B);	
		
        for (j = 0; j < Area->n_lines; j++) {
	    line = Area->lines[j];
	    Line = plus->Line[abs(line)];
	    fprintf (out, "  line = %3d\n", line ); 
	}
        for (j = 0; j < Area->n_isles; j++) {
	    isle = Area->isles[j];
	    fprintf (out, "  isle = %3d\n", isle ); 
	}
    }
    
    /* isles */
    fprintf (out, "Islands (%d islands, alive + dead ):\n", plus->n_isles ); 
    for (i = 1; i <= plus->n_isles; i++) {
	if ( plus->Isle[i] == NULL ) { continue; }
	Isle = plus->Isle[i];
	
	fprintf (out, "isle = %d, n_lines = %d area = %d\n", i, Isle->n_lines, 
		           Isle->area ); 
	
	fprintf (out, "N,S,E,W,T,B: %f, %f, %f, %f, %f, %f\n", Isle->N, Isle->S,
	                       Isle->E, Isle->W, Isle->T, Isle->B);	
	
        for (j = 0; j < Isle->n_lines; j++) {
	    line = Isle->lines[j];
	    Line = plus->Line[abs(line)];
	    fprintf (out, "  line = %3d\n", line ); 
	}
    }

    return 1;
}

/*!
  \brief Save spatial index file

  \param Map vector map

  \return 1 on success
  \return 0 on error
*/
int
Vect_save_spatial_index ( struct Map_info *Map )
{
    struct Plus_head *plus ;
    char   fname[GPATH_MAX], buf[GPATH_MAX];
    GVFILE   fp;
    
    G_debug (1, "Vect_save_spatial_index()"); 

    plus = &(Map->plus);
    
    /*  write out rtrees to the sidx file  */
    sprintf (buf, "%s/%s", GRASS_VECT_DIRECTORY, Map->name);
    G__file_name (fname, buf, GV_SIDX_ELEMENT, Map->mapset);
    G_debug (1, "Open sidx: %s", fname);
    dig_file_init ( &fp );
    fp.file = fopen( fname, "w");
    if ( fp.file ==  NULL) {
        G_warning(_("Unable open spatial index file for write <%s>"), fname);
	return 0;
    }

    /* set portable info */
    dig_init_portable ( &(plus->spidx_port), dig__byte_order_out ());
    
    if ( 0 > dig_write_spidx (&fp, plus) ) {
        G_warning (_("Error writing out spatial index file"));
	return 0;
    }
    
    fclose( fp.file );

    return 1;
}

/*!
  \brief Dump spatial index to file

  \param Map vector map
  \param out file for output (stdout/stderr for example)

  \return 1 on success
  \return 0 on error
*/
int
Vect_spatial_index_dump ( struct Map_info *Map, FILE *out ) 
{
    if ( !(Map->plus.Spidx_built) ) {
	Vect_build_sidx_from_topo ( Map, NULL );
    }

    fprintf (out, "---------- SPATIAL INDEX DUMP ----------\n" ); 
    
    dig_dump_spidx ( out, &(Map->plus) ); 

    return 1;
}
