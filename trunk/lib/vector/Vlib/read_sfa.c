/*!
  \file lib/vector/Vlib/read_sfa.c
  
  \brief Vector library - reading features - simple feature access
  
  Higher level functions for reading/writing/manipulating vectors.
  
  See read_ogr.c (OGR interface) and read_pg.c (PostGIS interface)
  for imlementation issues.

  (C) 2011-2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/vector.h>
#include <grass/glocale.h>

/*!
  \brief Reads feature from OGR/PostGIS layer on topological level.
 
  This function implements random access on level 2.
  
  Note: Topology must be built at level >= GV_BUILD_BASE
  
  \param Map pointer to Map_info structure
  \param[out] line_p container used to store line points within
  (pointer to line_pnts struct)
  \param[out] line_c container used to store line categories within
  (pointer to line_cats struct)
  \param line feature id (starts at 1)
  
  \return feature type
  \return -2 no more features
  \return -1 on failure 
*/
int V2_read_line_sfa(struct Map_info *Map, struct line_pnts *line_p,
                     struct line_cats *line_c, int line)
{
#if defined HAVE_OGR || defined HAVE_POSTGRES
    int type;
    struct P_line *Line;
    
    G_debug(4, "V2_read_line_sfa() line = %d", line);
    
    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }

    Line = Map->plus.Line[line];
    if (Line == NULL) {
        G_warning(_("Attempt to access dead feature %d"), line);
        return -1;
    }
    
    if (Line->type == GV_CENTROID) {
        /* read centroid for topo */
        if (line_p != NULL) {
            int i, found;
            struct bound_box box;
            struct boxlist list;
            struct P_topo_c *topo = (struct P_topo_c *)Line->topo;

            G_debug(4, "Centroid: area = %d", topo->area);
            Vect_reset_line(line_p);
            
            if (topo->area > 0 && topo->area <= Map->plus.n_areas) {
                /* get area bbox */
                Vect_get_area_box(Map, topo->area, &box);
                /* search in spatial index for centroid with area bbox */
                dig_init_boxlist(&list, TRUE);
                Vect_select_lines_by_box(Map, &box, Line->type, &list);
                
                found = -1;
                for (i = 0; i < list.n_values; i++) {
                    if (list.id[i] == line) {
                        found = i;
                        break;
                    }
                }
                
                if (found > -1) {
                    Vect_append_point(line_p, list.box[found].E, list.box[found].N, 0.0);
                }
                else {
                    G_warning(_("Unable to construct centroid for area %d. Skipped."),
                              topo->area);
                }
            }
            else {
                G_warning(_("Centroid %d: invalid area %d"), line, topo->area);
            }
        }

        if (line_c != NULL) {
          /* cat = fid and offset = fid for centroid */
          Vect_reset_cats(line_c);
          Vect_cat_set(line_c, 1, (int) Line->offset);
        }
        
        return GV_CENTROID;
    }
    
    if (!line_p && !line_c)
        return Line->type;
    
    if (Map->format == GV_FORMAT_POSTGIS)
        type = V1_read_line_pg(Map, line_p, line_c, Line->offset);
    else
        type = V1_read_line_ogr(Map, line_p, line_c, Line->offset);

    if (type != Line->type) {
        G_warning(_("Unexpected feature type (%d) - should be (%d)"),
                  type, Line->type);
        return -1;
    }

    return type;
#else
    G_fatal_error(_("GRASS is not compiled with OGR/PostgreSQL support"));
    return -1;
#endif
}
