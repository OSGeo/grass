/*!
  \file header.c
  
  \brief Vector library - header manipulation
  
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
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

static int lookup(char *file, char *key, char *value, size_t len);


/*!
  \brief Print vector map header

  \param Map vector map

  \return 0 on success
*/
int 
Vect_print_header (struct Map_info *Map)
{
  fprintf (stdout, "\nSelected information from dig header\n");
  fprintf (stdout, " Organization:  %s\n", Vect_get_organization(Map) );
  fprintf (stdout, " Map Name:      %s\n", Vect_get_map_name(Map) );
  fprintf (stdout, " Source Date:   %s\n", Vect_get_map_date(Map) );
  fprintf (stdout, " Orig. Scale:   %d\n", Vect_get_scale(Map)  );

  return 0;
}


/*!
  \brief Read vector map header from map head file

  \param Map vector map

  \return 0 on success
*/
int
Vect_read_header (struct Map_info *Map)
{
  Vect__read_head (Map);
  return 0;
}


/*!
  \brief Write vector map header to map head file

  \param Map vector map

  \return 0 on success
*/
int 
Vect_write_header (struct Map_info *Map)
{
  /* do some sanity checking here */
  Vect__write_head (Map);
  return 0;
}


/*! 
  \brief Writes head information to text file.

  \param Map vector map

  \return GRASS_OK - success
  \return GRASS_ERR - error
*/
int
Vect__write_head (struct Map_info *Map)
{
    char buf[200];	
    FILE *head_fp;

    sprintf (buf, "%s/%s", GRASS_VECT_DIRECTORY, Map->name);

    head_fp = G_fopen_new (buf, GRASS_VECT_HEAD_ELEMENT);
    if ( head_fp == NULL)
      {
        G_warning (_("Unable to open header file of vector <%s>"), Vect_get_full_name(Map));
        return (GRASS_ERR);
      }
	
    fprintf (head_fp, "ORGANIZATION: %s\n", Vect_get_organization(Map) );
    fprintf (head_fp, "DIGIT DATE:   %s\n", Vect_get_date(Map) );
    fprintf (head_fp, "DIGIT NAME:   %s\n", Vect_get_person(Map) );
    fprintf (head_fp, "MAP NAME:     %s\n", Vect_get_map_name(Map) );
    fprintf (head_fp, "MAP DATE:     %s\n", Vect_get_map_date(Map) );
    fprintf (head_fp, "MAP SCALE:    %d\n", Vect_get_scale(Map) );
    fprintf (head_fp, "OTHER INFO:   %s\n", Vect_get_comment(Map) );
    fprintf (head_fp, "ZONE:         %d\n", Vect_get_zone(Map) );
    fprintf (head_fp, "MAP THRESH:   %f\n", Vect_get_thresh(Map) );
    
    fclose (head_fp);
    return (GRASS_OK);
}

/*!
  \brief Reads head information from text file (GRASS_VECT_HEAD_ELEMENT).

  \param Map vector map
  
  \return GRASS_OK - success
  \return GRASS_ERR - error
*/
int
Vect__read_head (struct Map_info *Map)
{
    FILE *head_fp;
    char buff[2001];
    char *ptr;

    /* Reset / init */
    Vect_set_organization ( Map, "" );  
    Vect_set_date ( Map, "" );  
    Vect_set_person ( Map, "" );  
    Vect_set_map_name ( Map, "" );  
    Vect_set_map_date ( Map, "" );  
    Vect_set_scale ( Map, 1 );  
    Vect_set_comment ( Map, "" );  
    Vect_set_zone ( Map, 0 );  
    Vect_set_thresh ( Map, 0. );  

    G_debug (1, "Vect__read_head(): vector = %s@%s", Map->name, Map->mapset);
    sprintf (buff, "%s/%s", GRASS_VECT_DIRECTORY, Map->name);
    head_fp = G_fopen_old (buff, GRASS_VECT_HEAD_ELEMENT, Map->mapset); 
    if ( head_fp == NULL)
      {
        G_warning (_("Unable to open header file of vector <%s>"), Vect_get_full_name(Map));
        return (GRASS_ERR);
      }
   
    while ( G_getl2 (buff, 2000, head_fp) ) {

	if (!(ptr = G_index (buff, ':'))) {
	  G_warning (_("Corrupted row in head: %s"), buff );
 	  continue;
	}

	ptr++;			/* Search for the start of text */
	while (*ptr == ' ')
	ptr++;

	if (strncmp (buff, "ORGANIZATION:", sizeof(char)*12) == 0)
	  Vect_set_organization ( Map, ptr );  
	else if (strncmp (buff, "DIGIT DATE:", sizeof(char)*11) == 0)
	  Vect_set_date ( Map, ptr );  
	else if (strncmp (buff, "DIGIT NAME:", sizeof(char)*11) == 0)
	  Vect_set_person ( Map, ptr );  
	else if (strncmp (buff, "MAP NAME:", sizeof(char)*9) == 0)
	  Vect_set_map_name ( Map, ptr );  
	else if (strncmp (buff, "MAP DATE:", sizeof(char)*9) == 0)
	  Vect_set_map_date ( Map, ptr );  
	else if (strncmp (buff, "MAP SCALE:", sizeof(char)*10) == 0)
	  Vect_set_scale ( Map, atoi (ptr) );  
	else if (strncmp (buff, "OTHER INFO:", sizeof(char)*11) == 0)
	  Vect_set_comment ( Map, ptr );  
	else if (strncmp (buff, "ZONE:", sizeof(char)*5) == 0 || strncmp (buff, "UTM ZONE:", sizeof(char)*9) == 0)
	  Vect_set_zone ( Map, atoi (ptr) );  
	else if (strncmp (buff, "WEST EDGE:", sizeof(char)*10) == 0) {}
	else if (strncmp (buff, "EAST EDGE:", sizeof(char)*10) == 0) {}
	else if (strncmp (buff, "SOUTH EDGE:", sizeof(char)*11) == 0) {}
	else if (strncmp (buff, "NORTH EDGE:", sizeof(char)*11) == 0) {}
	else if (strncmp (buff, "MAP THRESH:", sizeof(char)*11) == 0)
	  Vect_set_thresh ( Map, atof (ptr) );  
	else 
	  G_warning(_("Unknown keyword %s in vector head"), buff);
    }
    
    fclose (head_fp);
    return (GRASS_OK);
}

/*!
  \brief Get map name

  \param Map vector map

  \return poiter to map name
*/
char *
Vect_get_name (struct Map_info *Map)
{
    return (Map->name);
}

/*!
  \brief Get mapset name

  \param Map vector map

  \return poiter to mapset name
*/
char *
Vect_get_mapset (struct Map_info *Map)
{
    return (Map->mapset);
}

/*!
  \brief Get full map name
  
  \param Map vector map

  \return poiter to map name (name@mapset)
*/
char *
Vect_get_full_name (struct Map_info *Map)
{
    char *ptr;

    ptr = (char *) G_malloc ( strlen(Map->name) +  strlen(Map->mapset) + 2 );
    sprintf (ptr, "%s@%s", Map->name, Map->mapset);
    return (ptr);
}

/*!
  \brief Check if vector map is 3D (with z)

  \param Map vector map

  \return 1 map is 3D
  \return 0 map is not 3D
*/
int
Vect_is_3d (struct Map_info *Map )
{
    return ( Map->head.with_z );
}

/*!
  \brief Set organization string in map header

  \param Map vector map
  \param str organization name

  \return 0
*/
int
Vect_set_organization (struct Map_info *Map, char *str )
{
    G_free ( Map->head.organization );
    Map->head.organization = G_store ( str );

    return 0;
}

/*!
  \brief Get organization string from map header

  \param Map vector map
  
  \return organization string
*/
char *
Vect_get_organization (struct Map_info *Map)
{
    return (Map->head.organization);
}

/*!
  \brief Set date of digitization string in map header

  SUGGESTION: this should be coupled to DateTime functions to support
  time series

  \param Map vector map
  \param str data string
  
  \return 0 on success
*/
int
Vect_set_date (struct Map_info *Map, char *str )
{
    G_free ( Map->head.date );
    Map->head.date = G_store ( str );
    return (0);
}

/*!
  \brief Get date of digitization string from map header

  SUGGESTION: this should be coupled to DateTime functions to support
  time series

  \param Map vector map

  \return date of digitization string
*/
char *
Vect_get_date (struct Map_info *Map)
{
    return (Map->head.date);
}

/*!
  \brief Set user name string who digitized the map in map header

  \param Map vector map
  \param str user name string

  \return 0 on success
*/
int
Vect_set_person (struct Map_info *Map, char *str )
{
    G_free ( Map->head.your_name );
    Map->head.your_name = G_store ( str );
    return (0);
}

/*!
  \brief Get user name string who digitized the map from map header

  \param Map vector map

  \return user name string
*/
char *
Vect_get_person (struct Map_info *Map)
{
    return (Map->head.your_name);
}

/*!
  \brief Set map name string in map header

  \param Map vector map
  \param str map name string

  \return 0 on success
*/
int
Vect_set_map_name (struct Map_info *Map, char *str )
{
    G_free ( Map->head.map_name );
    Map->head.map_name = G_store ( str );
    return (0);
}

/*!
  \brief Get map name string in map header

  \param Map vector map

  \return map name string
*/
char *
Vect_get_map_name (struct Map_info *Map)
{
    return (Map->head.map_name);
}

/*!
  \brief Set date string when the source map was originally produced in map header

  \param Map vector map
  \param str date when the source map was originally produced string

  \return 0 on success
*/
int
Vect_set_map_date (struct Map_info *Map, char *str )
{
    G_free ( Map->head.source_date );
    Map->head.source_date = G_store ( str );
    return (0);
}

/*!
  \brief Get date string when the source map was originally produced in map header

  \param Map vector map

  \return date when the source map was originally produced string
*/
char *
Vect_get_map_date (struct Map_info *Map)
{
    return (Map->head.source_date);
}

/*!
  \brief Set map scale in map header

  \param Map vector map
  \param map scale

  \return 0 on success
*/
int
Vect_set_scale (struct Map_info *Map, int scale )
{
    Map->head.orig_scale = scale;
    return (0);
}

/*!
  \brief Get map scale from map header
  
  \param Map vector map

  \return map scale
*/
int
Vect_get_scale (struct Map_info *Map)
{
    return ((int) Map->head.orig_scale);
}

/*!
  \brief Set comment or other info string in map header

  \param Map vector map
  \param str comment or other info string

  \return 0 on success
*/
int
Vect_set_comment (struct Map_info *Map, char *str )
{
    G_free ( Map->head.line_3 );
    Map->head.line_3 = G_store ( str );
    return (0);
}

/*!
  \brief Get comment or other info string from map header

  \param Map vector map

  \return comment or other info string
*/
char *
Vect_get_comment (struct Map_info *Map)
{
    return (Map->head.line_3);
}

/*!
  \brief Set projection zone in map header

  \param Map vector map
  \param zone projection zone

  \return 0 on success
*/
int
Vect_set_zone (struct Map_info *Map, int zone )
{
    Map->head.plani_zone = zone;
    return (0);
}


/*!
  \brief Get projection zone from map header

  \param Map vector map

  \return projection zone
*/
int
Vect_get_zone (struct Map_info *Map)
{
    return (Map->head.plani_zone);
}

/*!
  \brief Get projection from map header

  \param Map vector map

  \return PROJECTION_XY 0 - x,y (Raw imagery),
  \return PROJECTION_UTM 1 - UTM   Universal Transverse Mercator,
  \return PROJECTION_SP  2 - State Plane (in feet),
  \return PROJECTION_LL  3 - Latitude-Longitude
*/
int
Vect_get_proj (struct Map_info *Map)
{
    return (Map->proj);
}


/*!
  \brief Query cartographic projection name of vector map
 
  Returns a pointer to a string which is a printable name for
  projection code <b>proj</b> (as returned by <i>Vect_get_proj()</i>). Returns
  NULL if <b>proj</b> is not a valid projection.
 
  \param Map vector map

  \return poiter to projection name
*/

char *Vect_get_proj_name (struct Map_info *Map)
{
    int n;
    static char name[256];
    char *G__projection_name();

    switch(n=Vect_get_proj(Map))
    {
    case PROJECTION_XY:
    case PROJECTION_UTM:
    case PROJECTION_LL:
    case PROJECTION_SP:
	return G__projection_name(n);
    }
    if(!lookup (PROJECTION_FILE, "name", name, sizeof(name)))
	strcpy (name, _("Unknown projection"));
    return name;
}

/*!
  \brief Set threshold used for digitization in map header

  \param Map vector map
  \param thresh threshold used for digitization

  \return 0 on success
*/
int
Vect_set_thresh (struct Map_info *Map, double thresh )
{
    G_debug ( 1, "Vect_set_thresh(): thresh = %f", thresh );
    Map->head.digit_thresh = thresh;
    return (0);
}

/*!
  \brief Get threshold used for digitization from map header

  \param Map vector map

  \return threshold used for digitization
*/
double
Vect_get_thresh (struct Map_info *Map)
{
    G_debug ( 1, "Vect_get_thresh(): thresh = %f", Map->head.digit_thresh );
    return (Map->head.digit_thresh);
}


/* from lib/gis/proj3.c */
static int lookup(char *file, char *key, char *value, size_t len)
{
    char path[GPATH_MAX];

    G__file_name (path, "", file, "PERMANENT");
    return G_lookup_key_value_from_file(path, key, value, (int)len) == 1;
}
