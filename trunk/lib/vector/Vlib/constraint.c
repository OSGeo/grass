/*!
   \file lib/vector/Vlib/constraint.c

   \brief Vector library - constraints for reading features

   Higher level functions for reading/writing/manipulating vectors.

   These routines can affect the Vect_read_next_line() functions by
   restricting what they return. They are applied on a per map basis.

   These do not affect the lower level direct read functions.

   Normally, all 'Alive' lines will be returned unless overridden by
   this function. You can specified all the types you are interested
   in (by oring their types together). You can use this to say exclude
   'boundary' type features.

   By default all DEAD lines are ignored by the Vect_read_next_line()
   functions. This too can be overridden by including their types.

   (C) 2001-2009, 2011-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Set constraint region

   Vect_read_next_line() will read only features inside of given
   region or features with overlapping bounding box. 

   \note Constraint is ignored for random access - Vect_read_line().

   \param Map pointer to Map_info struct
   \param n,s,e,w,t,b bbox definition (north, south, east, west, top, and bottom coordinates)
   
   \return 0 on success
   \return -1 on error (invalid region)
 */
int Vect_set_constraint_region(struct Map_info *Map,
			       double n, double s, double e, double w,
			       double t, double b)
{
    if (n <= s)
	return -1;
    if (e <= w)
	return -1;

    Map->constraint.region_flag = TRUE;
    Map->constraint.box.N = n;
    Map->constraint.box.S = s;
    Map->constraint.box.E = e;
    Map->constraint.box.W = w;
    Map->constraint.box.T = t;
    Map->constraint.box.B = b;
    Map->head.proj = G_projection();

    return 0;
}

/*!
   \brief Get constraint box 

   Constraint box can be defined by Vect_set_constraint_region().

   \param Map vector map
   \param[out] Box bounding box

   \return 0 on success
   \return -1 no region constraint defined
 */
int Vect_get_constraint_box(const struct Map_info *Map, struct bound_box * Box)
{
    if (!Map->constraint.region_flag)
	return -1;
	    
    Box->N = Map->constraint.box.N;
    Box->S = Map->constraint.box.S;
    Box->E = Map->constraint.box.E;
    Box->W = Map->constraint.box.W;
    Box->T = Map->constraint.box.T;
    Box->B = Map->constraint.box.B;

    return 0;
}

/*!
   \brief Set constraint type

   Vect_read_next_line() will read only features of given
   type. Constraint is ignored for random access - Vect_read_line().

   \param Map pointer to Map_info struct
   \param type constraint feature type (GV_POINT, GV_LINE, ...)

   \return 0 on success
   \return -1 invalid feature type
*/
int Vect_set_constraint_type(struct Map_info *Map, int type)
{
    if (!(type & (GV_POINTS | GV_LINES | GV_FACE | GV_KERNEL)))
	return -1;
    
    Map->constraint.type = type;
    Map->constraint.type_flag = TRUE;

    return 0;
}

/*!
   \brief Remove all constraints

   \param Map pointer to Map_info struct
 */
void Vect_remove_constraints(struct Map_info *Map)
{
    Map->constraint.region_flag = FALSE;
    Map->constraint.type_flag   = FALSE;
    Map->constraint.field_flag  = FALSE;
}

/*!
   \brief Set constraint field

   Vect_read_next_line() will read only features of given type. Note
   that categories must be read otherwise this constraint is
   ignored. Constraint is ignored for random access -
   Vect_read_line().

   Ignored for non-native vector formats.
   
   Note: Field is called layer on user level.

   \param Map pointer to Map_info struct
   \param field field number (-1 for all fields)

   \return 0 on success
   \return -1 invalid field
*/
int Vect_set_constraint_field(struct Map_info *Map, int field)
{
    if (Map->format != GV_FORMAT_NATIVE) {
	G_warning(_("Layer constraint ignored for non-native vector formats"));
	return -1;
    }

    if (field == -1) {
	Map->constraint.field_flag = FALSE;
	return 0;
    }
    if (field < 1) {
	return -1;
    }
    Map->constraint.field = field;
    Map->constraint.field_flag = TRUE;
    
    return 0;
}
