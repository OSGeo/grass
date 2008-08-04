
/**********************************************************************
 *
 *  int
 *  G__quant_import (name, mapset, quant)
 *
 *      char *name;
 *      char *mapset;
 *      struct Quant *quant;
 * 
 *  reads quantization rules for "name" in "mapset" and stores them
 *  in the quantization structure "quant". If the map is in another
 *  mapset, first checks for quant2 table for this map in current
 *  mapset.
 *  
 *  returns: -2 if raster map is of type integer.
 *           -1 if (! G__name_is_fully_qualified ()).
 *            0 if quantization file does not exist, or the file is empty, 
 *            1 if non-empty quantization file exists.
 *                 read.
 *           
 *  note: in the case of negative return value, the result of using the 
 *        quantization structure is not defined.
 *        in case of return value 0, calls to G_quant_perform_d () 
 *        and G_quant_perform_f () return NO_DATA (see description of 
 *        G_quant_perform_d () for more details). in case of
 *        return values 2 and 3, the explicit rule for quant is set:
 *        floating range is mapped to integer range.
 *
 *  note: use G_quant_init () to allocate and initialize the quantization
 *        staructure quant before the first usage of G_quant_import ().
 *
 *  note: this function uses G_quant_free () to clear all previously
 *        stored rules in quant.
 * 
 **********************************************************************
 *
 *  int
 *  G__quant_export (name, mapset, quant)
 *
 *     char *name, *mapset;
 *     struct Quant *quant;
 *
 *  writes the quantization rules stored in "quant" for "name" . if the
 *  mapset is the same as the current mapset, the quant file is created
 *  in cell_misc/name directory, otherwise it is created in quant2/mapset
 *  directory, much like writing colors for map in another mapset.
 *  The rules are written in decreasing order
 *  of priority (i.e. rules added earlier are written later).
 *
 *  returns: -1 if (! G__name_is_fully_qualified) or file could not be
 *                 opened.
 *            1 otherwise.
 *
 *  note: if no rules are defined an empty file is created. 
 *
 **********************************************************************/

/*--------------------------------------------------------------------------*/

#include <grass/gis.h>
#include <grass/glocale.h>
#include <string.h>

/*--------------------------------------------------------------------------*/

#define QUANT_FILE_NAME "f_quant"

/*--------------------------------------------------------------------------*/

static int quant_parse_file(FILE *, struct Quant *);

#if 0
static int
/* redundant: integer range doesn't exist now: it is defined by
   the quant rules */
quant_load_range(struct Quant *quant, const char *name, const char *mapset)
{
    struct FPRange fprange;
    struct Range range;
    char buf[300];
    DCELL dMin, dMax;
    CELL min, max;

    if (G_read_fp_range(name, mapset, &fprange) <= 0)
	return 0;
    G_get_fp_range_min_max(&fprange, &dMin, &dMax);
    if (G_is_d_null_value(&dMin) || G_is_d_null_value(&dMax)) {
	sprintf(buf, _("The floating data range for %s@%s is empty"), name,
		mapset);
	G_warning(buf);
	return -3;
    }

    if (G_read_range(name, mapset, &range) < 0)
	return 0;
    G_get_range_min_max(&range, &min, &max);
    if (G_is_c_null_value(&min) && G_is_c_null_value(&max)) {
	sprintf(buf, _("The integer data range for %s@%s is empty"), name,
		mapset);
	G_warning(buf);
	return -3;
    }

    G_quant_add_rule(quant, dMin, dMax, min, max);

    return 1;
}
#endif

/*--------------------------------------------------------------------------*/

int G__quant_import(const char *name, const char *mapset, struct Quant *quant)
{
    char buf[1024];
    char *err;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX], element[GNAME_MAX + 7];
    int parsStat;
    FILE *fd;

    G_quant_free(quant);

    if (G_raster_map_type(name, mapset) == CELL_TYPE) {
	sprintf(buf,
		"G__quant_import: attempt to open quantization table for CELL_TYPE file [%s] in mapset {%s]",
		name, mapset);
	G_warning(buf);
	return -2;
    }

    if (G__name_is_fully_qualified(name, xname, xmapset)) {
	if (strcmp(xmapset, mapset) != 0)
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
		"quantization file in quant2 for [%s] in mapset [%s] is empty",
		name, mapset);
    }

    /* now try reading regular : cell_misc/name/quant file */
    if (!(fd = G_fopen_old_misc("cell_misc", QUANT_FILE_NAME, name, mapset))) {

	/* int range doesn't exist anymore if (quant_load_range (quant, name, mapset)>0) return 3; */
	err = "missing";

    }
    else {
	parsStat = quant_parse_file(fd, quant);
	fclose(fd);

	if (parsStat)
	    return 1;
	/* int range doesn't exist anymore if (quant_load_range (quant, name, mapset)>0) return 2; */

	err = "empty";
    }

    sprintf(buf,
	    _("quantization file [%s] in mapset [%s] %s"), name, mapset, err);
    G_warning(buf);

    return 0;
}

/*--------------------------------------------------------------------------*/

/* parse input lines with the following formats
 *
 *   d_high:d_low:c_high:c_low
 *   d_high:d_low:c_val          (i.e. c_high == c_low)
 *   *:d_val:c_val               (interval [inf, d_val])  (**)
 *   d_val:*:c_val               (interval [d_val, inf])  (**)
 *
 *   all other lines are ignored
 *
 *  (**) only the first appearances in the file are considered.
 *
 */

/*--------------------------------------------------------------------------*/

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
	    G_quant_add_rule(quant, dLow, dHigh, cLow, cLow);
	    break;
	case 4:
	    G_quant_add_rule(quant, dLow, dHigh, cLow, cHigh);
	    break;
	default:
	    switch (sscanf(buf, "*:%lf:%d", &dLow, &cLow)) {
	    case 2:
		if (!foundNegInf) {
		    G_quant_set_neg_infinite_rule(quant, dLow, cLow);
		    foundNegInf = 1;
		}
		break;
	    default:
		switch (sscanf(buf, "%lf:*:%d", &dLow, &cLow)) {
		case 2:
		    if (!foundPosInf) {
			G_quant_set_pos_infinite_rule(quant, dLow, cLow);
			foundPosInf = 1;
		    }
		    break;
		default:
		    continue;	/* other lines are ignored */
		}
	    }
	}
    }

    if (G_quant_nof_rules(quant) > 0)
	G_quant_reverse_rule_order(quant);

    return ((G_quant_nof_rules(quant) > 0) ||
	    (G_quant_get_neg_infinite_rule(quant, &dLow, &cLow) > 0) ||
	    (G_quant_get_pos_infinite_rule(quant, &dLow, &cLow) > 0));
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

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
    if (G_quant_get_neg_infinite_rule(quant, &dLow, &cLow) > 0)
	fprintf(fd, "*:%.20g:%d\n", dLow, cLow);

    if (G_quant_get_pos_infinite_rule(quant, &dLow, &cLow) > 0)
	fprintf(fd, "%.20g:*:%d\n", dLow, cLow);

    for (i = G_quant_nof_rules(quant) - 1; i >= 0; i--) {
	G_quant_get_ith_rule(quant, i, &dLow, &dHigh, &cLow, &cHigh);
	fprintf(fd, "%.20g:%.20g:%d", dLow, dHigh, cLow);
	if (cLow != cHigh)
	    fprintf(fd, ":%d", cHigh);
	fprintf(fd, "\n");
    }
}

/*--------------------------------------------------------------------------*/

int
G__quant_export(const char *name, const char *mapset,
		const struct Quant *quant)
{
    char element[GNAME_MAX + 7];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    FILE *fd;

    if (G__name_is_fully_qualified(name, xname, xmapset)) {
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
	G__make_mapset_element(element);
	if (!(fd = G_fopen_new(element, name)))
	    return -1;
    }



    quant_write(fd, quant);
    fclose(fd);

    return 1;
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
