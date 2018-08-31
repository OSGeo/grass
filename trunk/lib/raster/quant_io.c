/*!
 * \file lib/raster/quant_io.c
 * 
 * \brief Raster Library - Quantization rules (input / output)
 *
 * (C) 1999-2010 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author USACERL and many others
 */

/**********************************************************************
 *
 **********************************************************************/

#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define QUANT_FILE_NAME "f_quant"

static int quant_parse_file(FILE *, struct Quant *);

#if 0
/* redundant: integer range doesn't exist now: it is defined by
   the quant rules */
static int quant_load_range(struct Quant *quant, const char *name, const char *mapset)
{
    struct FPRange fprange;
    struct Range range;
    char buf[300];
    DCELL dMin, dMax;
    CELL min, max;

    if (Rast_read_fp_range(name, mapset, &fprange) <= 0)
	return 0;
    Rast_get_fp_range_min_max(&fprange, &dMin, &dMax);
    if (Rast_is_d_null_value(&dMin) || Rast_is_d_null_value(&dMax)) {
	G_warning(_("Floating data range for raster map <%s> is empty"),
		  G_fully_qualified_name(name, mapset));
	return -3;
    }

    if (Rast_read_range(name, mapset, &range) < 0)
	return 0;
    Rast_get_range_min_max(&range, &min, &max);
    if (Rast_is_c_null_value(&min) && Rast_is_c_null_value(&max)) {
	G_warning(_("Integer data range for raster map <%s> is empty"),
		  G_fully_qualified_name(name, mapset));
	return -3;
    }

    Rast_quant_add_rule(quant, dMin, dMax, min, max);

    return 1;
}
#endif

/*!
  \brief Reads quantization rules (internal use only)

  Reads quantization rules for raster map <i>name</i> in <i>mapset</i>
  and stores them in the quantization structure "quant". If the map is
  in another mapset, first checks for quant2 table for this map in
  current mapset.
 
  Note: in the case of negative return value, the result of using the 
  quantization structure is not defined.
  in case of return value 0, calls to Rast_quant_perform_d() 
  and Rast_quant_perform_f() return NO_DATA (see description of 
  Rast_quant_perform_d() for more details). in case of
  return values 2 and 3, the explicit rule for quant is set:
  floating range is mapped to integer range.
  
  Note: use Rast_quant_init() to allocate and initialize the quantization
  staructure quant before the first usage of G_quant_import().

  Note: this function uses Rast_quant_free () to clear all previously
  stored rules in quant.

  \param name map name
  \param mapset mapset name
  \param[out] quant pointer to Quant structure
  
  \return -2 if raster map is of type integer.
  \return -1 if map name is fully qualified and mapset is not the current one
  \return 0 if quantization file does not exist, or the file is empty, 
  \return 1 if non-empty quantization file exists.
*/
int Rast__quant_import(const char *name, const char *mapset,
		       struct Quant *quant)
{
    char buf[1024];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX], element[GNAME_MAX + 7];
    int parsStat;
    FILE *fd;

    Rast_quant_free(quant);

    if (Rast_map_type(name, mapset) == CELL_TYPE) {
	G_warning(_("Attempt to open quantization"
		    " table for CELL raster map <%s>"),
		  G_fully_qualified_name(name, mapset));
	return -2;
    }

    if (G_name_is_fully_qualified(name, xname, xmapset)) {
	if (strlen(mapset) == 0)
	    mapset = xmapset;
	else if (strcmp(xmapset, mapset) != 0)
	    return -1;
	name = xname;
    }

    /* first check if quant2/mapset/name exists in the current mapset */
    sprintf(element, "quant2/%s", mapset);
    if ((fd = G_fopen_old(element, name, G_mapset()))) {
	parsStat = quant_parse_file(fd, quant);
	fclose(fd);
	if (parsStat)
	    return 1;
	sprintf(buf,
		"quantization file in quant2 for raster map <%s> is empty",
		G_fully_qualified_name(name, mapset));
    }

    /* now try reading regular : cell_misc/name/quant file */
    if (!(fd = G_fopen_old_misc("cell_misc", QUANT_FILE_NAME, name, mapset))) {

	/* int range doesn't exist anymore if (quant_load_range (quant, name, mapset)>0) return 3; */
	G_warning(_("Quantization file for raster map <%s> is missing"),
		  G_fully_qualified_name(name, mapset));
    }
    else {
	parsStat = quant_parse_file(fd, quant);
	fclose(fd);

	if (parsStat)
	    return 1;
	/* int range doesn't exist anymore if (quant_load_range (quant, name, mapset)>0) return 2; */
	G_warning(_("Quantization file for raster map <%s> is empty"),
		  G_fully_qualified_name(name, mapset));
    }
    
    return 0;
}

/*!
  \brief Parse input lines with the following formats
 
  \code
  d_high:d_low:c_high:c_low
  d_high:d_low:c_val          (i.e. c_high == c_low)
  *:d_val:c_val               (interval [inf, d_val])  (**)
  d_val:*:c_val               (interval [d_val, inf])  (**)
  \endcode
 
  All other lines are ignored
  
  (**) only the first appearances in the file are considered.
*/
static int quant_parse_file(FILE * fd, struct Quant *quant)
{
    CELL cLow, cHigh;
    DCELL dLow, dHigh;
    char buf[1024];
    int foundNegInf = 0, foundPosInf = 0;

    while (fgets(buf, sizeof(buf), fd)) {
	if (strncmp(buf, "truncate", 8) == 0) {
	    quant->truncate_only = 1;
	    return 1;
	}
	if (strncmp(buf, "round", 5) == 0) {
	    quant->round_only = 1;
	    return 1;
	}
	switch (sscanf(buf, "%lf:%lf:%d:%d", &dLow, &dHigh, &cLow, &cHigh)) {
	case 3:
	    Rast_quant_add_rule(quant, dLow, dHigh, cLow, cLow);
	    break;
	case 4:
	    Rast_quant_add_rule(quant, dLow, dHigh, cLow, cHigh);
	    break;
	default:
	    switch (sscanf(buf, "*:%lf:%d", &dLow, &cLow)) {
	    case 2:
		if (!foundNegInf) {
		    Rast_quant_set_neg_infinite_rule(quant, dLow, cLow);
		    foundNegInf = 1;
		}
		break;
	    default:
		switch (sscanf(buf, "%lf:*:%d", &dLow, &cLow)) {
		case 2:
		    if (!foundPosInf) {
			Rast_quant_set_pos_infinite_rule(quant, dLow, cLow);
			foundPosInf = 1;
		    }
		    break;
		default:
		    continue;	/* other lines are ignored */
		}
	    }
	}
    }

    if (Rast_quant_nof_rules(quant) > 0)
	Rast_quant_reverse_rule_order(quant);

    return ((Rast_quant_nof_rules(quant) > 0) ||
	    (Rast_quant_get_neg_infinite_rule(quant, &dLow, &cLow) > 0) ||
	    (Rast_quant_get_pos_infinite_rule(quant, &dLow, &cLow) > 0));
}

static void quant_write(FILE * fd, const struct Quant *quant)
{
    DCELL dLow, dHigh;
    CELL cLow, cHigh;
    int i;

    if (quant->truncate_only) {
	fprintf(fd, "truncate");
	return;
    }
    if (quant->round_only) {
	fprintf(fd, "round");
	return;
    }
    if (Rast_quant_get_neg_infinite_rule(quant, &dLow, &cLow) > 0)
	fprintf(fd, "*:%.20g:%d\n", dLow, cLow);

    if (Rast_quant_get_pos_infinite_rule(quant, &dLow, &cLow) > 0)
	fprintf(fd, "%.20g:*:%d\n", dLow, cLow);

    for (i = Rast_quant_nof_rules(quant) - 1; i >= 0; i--) {
	Rast_quant_get_ith_rule(quant, i, &dLow, &dHigh, &cLow, &cHigh);
	fprintf(fd, "%.20g:%.20g:%d", dLow, dHigh, cLow);
	if (cLow != cHigh)
	    fprintf(fd, ":%d", cHigh);
	fprintf(fd, "\n");
    }
}

/*!
  \brief Writes the quantization rules (internal use only)

  Writes the quantization rules stored in <i>quant</i> for <i>name</i>
  . If the mapset is the same as the current mapset, the quant file is
  created in 'cell_misc/name' directory, otherwise it is created in
  'quant2/mapset' directory, much like writing colors for map in
  another mapset.  The rules are written in decreasing order of
  priority (i.e. rules added earlier are written later).

  Note: if no rules are defined an empty file is created. 
  
  \param name map name
  \param mapset mapset name
  \param quant pointer to Quant structure

  \return -1 if map name is not fully qualified or file could not be opened.
  \return 1 otherwise.
*/
int Rast__quant_export(const char *name, const char *mapset,
		       const struct Quant *quant)
{
    char element[GNAME_MAX + 7];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    FILE *fd;

    if (G_name_is_fully_qualified(name, xname, xmapset)) {
	if (strcmp(xmapset, mapset) != 0)
	    return -1;
	name = xname;
    }

    if (strcmp(G_mapset(), mapset) == 0) {
	G_remove_misc("cell_misc", QUANT_FILE_NAME, name);
	G__make_mapset_element_misc("cell_misc", name);
	if (!(fd = G_fopen_new_misc("cell_misc", QUANT_FILE_NAME, name)))
	    return -1;
    }
    else {
	sprintf(element, "quant2/%s", mapset);
	G_remove(element, name);
	G_make_mapset_element(element);
	if (!(fd = G_fopen_new(element, name)))
	    return -1;
    }
    
    quant_write(fd, quant);
    fclose(fd);

    return 1;
}
