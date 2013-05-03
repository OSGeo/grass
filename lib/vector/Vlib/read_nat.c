/*!
   \file lib/vector/Vlib/read_nat.c

   \brief Vector library - reading features (native format)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009, 2011-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 by Radim Blazek and David D. Gray.
   \author Update to GRASS 7 by Martin Landa <landa.martin gmail.com>
 */

#include <sys/types.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static int read_line_nat(struct Map_info *,
			 struct line_pnts *, struct line_cats *, off_t);

/*! \brief Read vector feature on non-topological level (level 1) -
  native format - internal use only

  This function implements random access for native format,
  constraints are ignored!
  
  \param Map pointer to Map_info struct 
  \param[out] Points container used to store line points within
  (pointer to line_pnts struct)
  \param[out] Cats container used to store line categories within
  (pointer to line_cats struct)
  \param offset given offset 
  
  \return feature type (GV_POINT, GV_LINE, ...)
  \return 0 dead line
  \return -2 nothing to read
  \return -1 on failure
*/
int V1_read_line_nat(struct Map_info *Map,
		     struct line_pnts *Points,
		     struct line_cats *Cats, off_t offset)
{
    return read_line_nat(Map, Points, Cats, offset);
}

/*! \brief Read next vector feature on non-topological level (level
  1) - native format - internal use only.

  This function implements sequential access, constraints are
  reflected, see Vect_set_constraint_region(),
  Vect_set_constraint_type(), or Vect_set_constraint_field().
  
  Dead features are skipped.
  
  Vect_rewind() can be used to reset reading.
   
  \param Map pointer to Map_info struct
  \param[out] line_p container used to store line points within
  (pointer to line_pnts struct)
  \param[out] line_c container used to store line categories within
  (pointer to line_cats struct)
  
  \return feature type (GV_POINT, GV_LINE, ...)
  \return 0 dead line
  \return -2 nothing to read
  \return -1 on failure
*/
int V1_read_next_line_nat(struct Map_info *Map,
			  struct line_pnts *line_p, struct line_cats *line_c)
{
    int itype;
    off_t offset;
    struct bound_box lbox, mbox;

    G_debug(3, "V1_read_next_line_nat()");

    if (Map->constraint.region_flag)
	Vect_get_constraint_box(Map, &mbox);

    while (TRUE) {
	offset = dig_ftell(&(Map->dig_fp));
	itype = read_line_nat(Map, line_p, line_c, offset);
	if (itype < 0)
	    return itype; /* nothing to read or failure */

	if (itype == 0)	  /* skip dead line */
	    continue;

	if (Map->constraint.type_flag) {
	    /* skip feature by type */
	    if (!(itype & Map->constraint.type))
		continue;
	}

	if (line_p && Map->constraint.region_flag) {
	    /* skip feature by region */
	    Vect_line_box(line_p, &lbox);
	    
	    if (!Vect_box_overlap(&lbox, &mbox))
		continue;
	}
	
	if (line_c && Map->constraint.field_flag) {
	    /* skip feature by field */
	    if (Vect_cat_get(line_c, Map->constraint.field, NULL) == 0)
		continue;
	}

	return itype;
    }

    return -1; /* NOTREACHED */
}

/*! \brief Read vector feature on topological level (level 2) -
  native format - internal use only

  This function implements random access for native format,
  constraints are ignored!
  
  Note: Topology must be built at level >= GV_BUILD_BASE
  
  \param Map pointer to Map_info struct 
  \param[out] Points container used to store line points within (pointer to line_pnts struct)
  \param[out] Cats container used to store line categories within (pointer to line_cats struct)
  \param line feature id to read (starts at 1)
  
  \return feature type (GV_POINT, GV_LINE, ...)
  \return -2 nothing to read
  \return -1 on failure
*/
int V2_read_line_nat(struct Map_info *Map,
		     struct line_pnts *line_p, struct line_cats *line_c, int line)
{
    struct P_line *Line;

    G_debug(3, "V2_read_line_nat(): line = %d", line);

    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }
    
    Line = Map->plus.Line[line];
    if (Line == NULL) {
	G_warning(_("Attempt to access dead feature %d"), line);
	return -1;
    }

    return read_line_nat(Map, line_p, line_c, Line->offset);
}

/*! \brief Read next vector feature on topological level (level 2) -
  native format - internal use only.

  This function implements sequential access, constraints are
  reflected, see Vect_set_constraint_region(),
  Vect_set_constraint_type(), or Vect_set_constraint_field().
  
  Use Vect_rewind() to reset reading.

  Dead feature are skipped.
   
  \param Map pointer to Map_info struct
  \param[out] line_p container used to store line points within
  (pointer to line_pnts struct)
  \param[out] line_c container used to store line categories within
  (pointer to line_cats struct)
  
  \return feature type (GV_POINT, GV_LINE, ...)
  \return -2 nothing to read
  \return -1 on error
*/
int V2_read_next_line_nat(struct Map_info *Map,
			  struct line_pnts *line_p, struct line_cats *line_c)
{
    int line, ret;
    struct P_line *Line;
    struct bound_box lbox, mbox;

    G_debug(3, "V2_read_next_line_nat()");

    if (Map->constraint.region_flag)
	Vect_get_constraint_box(Map, &mbox);
    
    while (TRUE) {
	line = Map->next_line;

	if (line > Map->plus.n_lines)
	    return -2; /* nothing to read */

	Line = Map->plus.Line[line];
	if (Line == NULL) {
	    /* skip dead line */
	    Map->next_line++;
	    continue;
	}

	if (Map->constraint.type_flag) {
	    /* skip feature by type */
	    if (!(Line->type & Map->constraint.type)) {
		Map->next_line++;
		continue;
	    }
	}

	Map->next_line++;
	ret = read_line_nat(Map, line_p, line_c, Line->offset);
	if (ret < 0)
	    return ret;
	
	if (line_p && Map->constraint.region_flag) {
	    /* skip feature by bbox */
	    Vect_line_box(line_p, &lbox);
	    
	    if (!Vect_box_overlap(&lbox, &mbox))
		continue;
	}

	if (line_c && Map->constraint.field_flag) {
	    /* skip feature by field */
	    if (Vect_cat_get(line_c, Map->constraint.field, NULL) == 0)
		continue;
	}
	
	return ret;
    }
    
    return -1; /* NOTREACHED */
}

/*!  
  \brief Read line from coor file 
  
  \param Map vector map layer
  \param[out] p container used to store line points within
  \param[out] c container used to store line categories within
  \param offset given offset
  
  \return line type ( > 0 )
  \return 0 dead line
  \return -1 out of memory
  \return -2 end of file
*/
int read_line_nat(struct Map_info *Map,
		  struct line_pnts *p, struct line_cats *c, off_t offset)
{
    register int i, dead = 0;
    int n_points;
    off_t size;
    int n_cats, do_cats;
    int type;
    char rhead, nc;
    short field;

    G_debug(3, "Vect__Read_line_nat: offset = %lu", (unsigned long) offset);

    Map->head.last_offset = offset;

    /* reads must set in_head, but writes use default */
    dig_set_cur_port(&(Map->head.port));

    dig_fseek(&(Map->dig_fp), offset, 0);

    if (0 >= dig__fread_port_C(&rhead, 1, &(Map->dig_fp)))
	return (-2);

    if (!(rhead & 0x01))	/* dead line */
	dead = 1;

    if (rhead & 0x02)		/* categories exists */
	do_cats = 1;		/* do not return here let file offset moves forward to next */
    else			/* line */
	do_cats = 0;

    rhead >>= 2;
    type = dig_type_from_store((int)rhead);

    G_debug(3, "    type = %d, do_cats = %d dead = %d", type, do_cats, dead);

    if (c != NULL)
	c->n_cats = 0;

    if (do_cats) {
	if (Map->plus.version.coor.minor == 1) {	/* coor format 5.1 */
	    if (0 >= dig__fread_port_I(&n_cats, 1, &(Map->dig_fp)))
		return (-2);
	}
	else {			/* coor format 5.0 */
	    if (0 >= dig__fread_port_C(&nc, 1, &(Map->dig_fp)))
		return (-2);
	    n_cats = (int)nc;
	}
	G_debug(3, "    n_cats = %d", n_cats);

	if (c != NULL) {
	    c->n_cats = n_cats;
	    if (n_cats > 0) {
		if (0 > dig_alloc_cats(c, (int)n_cats + 1))
		    return -1;

		if (Map->plus.version.coor.minor == 1) {	/* coor format 5.1 */
		    if (0 >=
			dig__fread_port_I(c->field, n_cats, &(Map->dig_fp)))
			return (-2);
		}
		else {		/* coor format 5.0 */
		    for (i = 0; i < n_cats; i++) {
			if (0 >= dig__fread_port_S(&field, 1, &(Map->dig_fp)))
			    return (-2);
			c->field[i] = (int)field;
		    }
		}
		if (0 >= dig__fread_port_I(c->cat, n_cats, &(Map->dig_fp)))
		    return (-2);

	    }
	}
	else {
	    if (Map->plus.version.coor.minor == 1) {	/* coor format 5.1 */
		size = (off_t) (2 * PORT_INT) * n_cats;
	    }
	    else {		/* coor format 5.0 */
		size = (off_t) (PORT_SHORT + PORT_INT) * n_cats;
	    }

	    dig_fseek(&(Map->dig_fp), size, SEEK_CUR);
	}
    }

    if (type & GV_POINTS) {
	n_points = 1;
    }
    else {
	if (0 >= dig__fread_port_I(&n_points, 1, &(Map->dig_fp)))
	    return (-2);
    }

    G_debug(3, "    n_points = %d", n_points);

    if (p != NULL) {
	if (0 > dig_alloc_points(p, n_points + 1))
	    return (-1);

	p->n_points = n_points;
	if (0 >= dig__fread_port_D(p->x, n_points, &(Map->dig_fp)))
	    return (-2);
	if (0 >= dig__fread_port_D(p->y, n_points, &(Map->dig_fp)))
	    return (-2);

	if (Map->head.with_z) {
	    if (0 >= dig__fread_port_D(p->z, n_points, &(Map->dig_fp)))
		return (-2);
	}
	else {
	    for (i = 0; i < n_points; i++)
		p->z[i] = 0.0;
	}
    }
    else {
	if (Map->head.with_z)
	    size = (off_t) n_points * 3 * PORT_DOUBLE;
	else
	    size = (off_t) n_points * 2 * PORT_DOUBLE;

	dig_fseek(&(Map->dig_fp), size, SEEK_CUR);
    }

    G_debug(3, "    off = %lu", (unsigned long) dig_ftell(&(Map->dig_fp)));

    if (dead)
	return 0;

    return type;
}
