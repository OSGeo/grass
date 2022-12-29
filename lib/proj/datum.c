
/**
   \file lib/proj/datum.c

   \brief GProj library - Functions for reading datum parameters from the location database

   \author Andreas Lange <andreas.lange rhein-main.de>, Paul Kelly <paul-grass stjohnspoint.co.uk>

   (C) 2003-2008 by the GRASS Development Team
 
   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
**/

#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gprojects.h>
#include "local_proto.h"

/**
 * \brief Look up a string in datum.table file to see if it is a valid datum 
 *        name and if so place its information into a gpj_datum struct
 * 
 * \param name String containing datum name to look up
 * \param dstruct gpj_datum struct into which datum parameters will be placed
 *        if found
 * 
 * \return 1 if datum found, -1 if not
 **/

int GPJ_get_datum_by_name(const char *name, struct gpj_datum *dstruct)
{
    struct datum_list *list, *listhead;

    list = listhead = read_datum_table();

    while (list != NULL) {
	if (G_strcasecmp(name, list->name) == 0) {
	    dstruct->name = G_store(list->name);
	    dstruct->longname = G_store(list->longname);
	    dstruct->ellps = G_store(list->ellps);
	    dstruct->dx = list->dx;
	    dstruct->dy = list->dy;
	    dstruct->dz = list->dz;
	    free_datum_list(listhead);
	    return 1;
	}
	list = list->next;
    }
    free_datum_list(listhead);
    return -1;
}

/**
 * \brief "Last resort" function to retrieve a "default" set of datum
 * parameters for a datum (N.B. there really is no such thing as a
 * catch-all default!)
 * 
 * Kind of a "last resort" function as there really is no such thing
 * as a default set of datum transformation parameters. Only should 
 * really be used where user interaction to choose a set of parameters
 * is not desirable. Use of this function is not likely to result in
 * selection of the optimum set of datum transformation parameters
 * for the location
 *
 * \param name      String containing GRASS datum name for which default
 *                  parameters are to be retrieved
 * 
 * \param params    Pointer to a pointer which will have memory
 *                  allocated and into which a string containing
 *                  the datum parameters (if present) will
 *                  be placed
 * 
 * \return          The number of possible parameter sets GRASS knows
 *                  about for this datum
 * 
 **/

int GPJ_get_default_datum_params_by_name(const char *name, char **params)
{
    struct gpj_datum_transform_list *list, *old;
    int count = 0;

    list = GPJ_get_datum_transform_by_name(name);

    if (list == NULL) {
	*params = NULL;
	return -1;
    }

    /* Take the first parameter set in the list as the default
     * (will normally be a 3-parameter transformation)        */
    *params = G_store(list->params);

    while (list != NULL) {
	count++;
	old = list;
	list = list->next;
	GPJ_free_datum_transform(old);
    }

    return count;

}

/**
 *  
 * \brief Extract the datum transformation-related parameters for
 *  the current location.
 * 
 *  This function can be used to test if a location's co-ordinate
 *  system set-up supports datum transformation.
 *  
 * \param name      Pointer to a pointer which will have memory 
 *                  allocated and into which a string containing the 
 *                  datum name (if present) will be placed. Otherwise 
 *                  set to NULL.
 * 
 * \param params    Pointer to a pointer which will have memory
 *                  allocated and into which a string containing
 *                  the datum parameters (if present) will
 *                  be placed. Otherwise set to NULL.
 *
 * \return  -1 error or no datum information found, 
 *           1 only datum name found, 2 params found
 * 
 **/

int GPJ_get_datum_params(char **name, char **params)
{
    int ret;
    struct Key_Value *proj_keys = G_get_projinfo();

    ret = GPJ__get_datum_params(proj_keys, name, params);
    G_free_key_value(proj_keys);

    return ret;
}

/**
 *  
 * \brief Extract the datum transformation-related parameters from a 
 *  set of general PROJ_INFO parameters.
 * 
 *  This function can be used to test if a location's co-ordinate
 *  system set-up supports datum transformation.
 *  
 * \param projinfo  Set of key_value pairs containing
 *                  projection information in PROJ_INFO file
 *                  format
 * 
 * \param datumname Pointer to a pointer which will have memory 
 *                  allocated and into which a string containing the 
 *                  datum name (if present) will be placed. Otherwise 
 *                  set to NULL.
 * 
 * \param params    Pointer to a pointer which will have memory
 *                  allocated and into which a string containing
 *                  the datum parameters (if present) will
 *                  be placed. Otherwise set to NULL.
 *
 * \return  -1 error or no datum information found, 
 *           1 only datum name found, 2 params found
 * 
 **/

int GPJ__get_datum_params(const struct Key_Value *projinfo,
			  char **datumname, char **params)
{
    int returnval = -1;

    if (NULL != G_find_key_value("datum", projinfo)) {
	*datumname = G_store(G_find_key_value("datum", projinfo));
	G_debug(3, "GPJ__get_datum_params: datumname: <%s>", G_find_key_value("datum", projinfo));
	returnval = 1;
    }
    else
	*datumname = NULL;

    if (G_find_key_value("datumparams", projinfo) != NULL) {
	*params = G_store(G_find_key_value("datumparams", projinfo));
	G_debug(3, "GPJ__get_datum_params: datumparams: <%s>", G_find_key_value("datumparams", projinfo));
	returnval = 2;
    }
    else if (G_find_key_value("nadgrids", projinfo) != NULL) {
	/* 1. beware of '@', do not create something like
	 *    /usr/share/proj/@null, correct is @null or
	 *    @/usr/share/proj/null
	 * 2. do not add path to the grid, there might already be a
	 *    path, and it is safer to use pj_set_finder with PROJ.4 in
	 *    datum.c */

	G_asprintf(params, "nadgrids=%s", G_find_key_value("nadgrids", projinfo));

	returnval = 2;
    }
    else if (G_find_key_value("towgs84", projinfo) != NULL) {
	G_asprintf(params, "towgs84=%s",
		   G_find_key_value("towgs84", projinfo));
	returnval = 2;
    }
    else if (G_find_key_value("dx", projinfo) != NULL
	     && G_find_key_value("dy", projinfo) != NULL
	     && G_find_key_value("dz", projinfo) != NULL) {
	G_asprintf(params, "towgs84=%s,%s,%s",
		   G_find_key_value("dx", projinfo),
		   G_find_key_value("dy", projinfo),
		   G_find_key_value("dz", projinfo));
	returnval = 2;
    }
    else
	*params = NULL;

    return returnval;

}

/**
 * \brief Internal function to find all possible sets of 
 *        transformation parameters for a particular datum
 *  
 * \param inputname   String containing the datum name we
 *                    are going to look up parameters for
 * 
 * \return   Pointer to struct gpj_datum_transform_list (a linked
 *           list containing transformation parameters),
 *           or NULL if no suitable parameters were found.
 **/

struct gpj_datum_transform_list *GPJ_get_datum_transform_by_name(const char
								 *inputname)
{
    FILE *fd;
    char file[GPATH_MAX];
    char buf[1024];
    int line;
    struct gpj_datum_transform_list *current = NULL, *outputlist = NULL;
    struct gpj_datum dstruct;
    int count = 0;

    GPJ_get_datum_by_name(inputname, &dstruct);
    if (dstruct.dx < 99999 && dstruct.dy < 99999 && dstruct.dz < 99999) {
	/* Include the old-style dx dy dz parameters from datum.table at the 
	 * start of the list, unless these have been set to all 99999 to 
	 * indicate only entries in datumtransform.table should be used */
	if (current == NULL)
	    current = outputlist =
		G_malloc(sizeof(struct gpj_datum_transform_list));
	else
	    current = current->next =
		G_malloc(sizeof(struct gpj_datum_transform_list));
	G_asprintf(&(current->params), "towgs84=%.3f,%.3f,%.3f", dstruct.dx,
		   dstruct.dy, dstruct.dz);
	G_asprintf(&(current->where_used), "whole %s region", inputname);
	G_asprintf(&(current->comment),
		   "Default 3-Parameter Transformation (May not be optimum for "
		   "older datums; use this only if no more appropriate options "
		   "are available.)");
	count++;
	current->count = count;
	current->next = NULL;
    }
    GPJ_free_datum(&dstruct);

    /* Now check for additional parameters in datumtransform.table */

    sprintf(file, "%s%s", G_gisbase(), DATUMTRANSFORMTABLE);

    fd = fopen(file, "r");
    if (!fd) {
	G_warning(_("Unable to open datum table file <%s>"), file);
	return outputlist;
    }

    for (line = 1; G_getl2(buf, sizeof(buf), fd); line++) {
	char name[100], params[1024], where_used[1024], comment[1024];

	G_strip(buf);
	if (*buf == '\0' || *buf == '#')
	    continue;

	if (sscanf(buf, "%99s \"%1023[^\"]\" \"%1023[^\"]\" \"%1023[^\"]\"",
		   name, params, where_used, comment) != 4) {
	    G_warning(_("Error in datum table file <%s>, line %d"), file,
		      line);
	    continue;
	}

	if (G_strcasecmp(inputname, name) == 0) {
	    /* If the datum name in this line matches the one we are 
	     * looking for, add an entry to the linked list */
	    if (current == NULL)
		current = outputlist =
		    G_malloc(sizeof(struct gpj_datum_transform_list));
	    else
		current = current->next =
		    G_malloc(sizeof(struct gpj_datum_transform_list));
	    current->params = G_store(params);
	    current->where_used = G_store(where_used);
	    current->comment = G_store(comment);
	    count++;
	    current->count = count;
	    current->next = NULL;
	}
    }

    fclose(fd);

    return outputlist;

}

/**
 * \brief Free the memory used by a gpj_datum_transform_list struct
 *
 * \param item gpj_datum_transform_list struct to be freed
 **/

void GPJ_free_datum_transform(struct gpj_datum_transform_list *item)
{
    G_free(item->params);
    G_free(item->where_used);
    G_free(item->comment);
    G_free(item);
    return;
}

/**
 * \brief Read the current GRASS datum.table from disk and store in
 *        memory
 * 
 * The datum information is stored in a datum_list linked list structure.
 * 
 * \return Pointer to first datum_list element in linked list, or NULL
 *         if unable to open datum.table file
 **/

struct datum_list *read_datum_table(void)
{
    FILE *fd;
    char file[GPATH_MAX];
    char buf[4096];
    int line;
    struct datum_list *current = NULL, *outputlist = NULL;
    int count = 0;

    sprintf(file, "%s%s", G_gisbase(), DATUMTABLE);

    fd = fopen(file, "r");
    if (!fd) {
	G_warning(_("Unable to open datum table file <%s>"), file);
	return NULL;
    }

    for (line = 1; G_getl2(buf, sizeof(buf), fd); line++) {
	char name[100], descr[1024], ellps[100];
	double dx, dy, dz;

	G_strip(buf);
	if (*buf == '\0' || *buf == '#')
	    continue;

	if (sscanf(buf, "%s \"%1023[^\"]\" %s dx=%lf dy=%lf dz=%lf",
		   name, descr, ellps, &dx, &dy, &dz) != 6) {
	    G_warning(_("Error in datum table file <%s>, line %d"), file,
		      line);
	    continue;
	}

	if (current == NULL)
	    current = outputlist = G_malloc(sizeof(struct datum_list));
	else
	    current = current->next = G_malloc(sizeof(struct datum_list));
	current->name = G_store(name);
	current->longname = G_store(descr);
	current->ellps = G_store(ellps);
	current->dx = dx;
	current->dy = dy;
	current->dz = dz;
	current->next = NULL;

	count++;
    }

    fclose(fd);

    return outputlist;
}

/**
 * \brief Free the memory used for the strings in a gpj_datum struct
 * 
 * \param dstruct gpj_datum struct to be freed
 **/

void GPJ_free_datum(struct gpj_datum *dstruct)
{
    G_free(dstruct->name);
    G_free(dstruct->longname);
    G_free(dstruct->ellps);
    return;
}

/**
 * \brief Free the memory used by a datum_list linked list structure
 * 
 * \param dstruct datum_list struct to be freed
 **/

void free_datum_list(struct datum_list *dstruct)
{
    struct datum_list *old;

    while (dstruct != NULL) {
	G_free(dstruct->name);
	G_free(dstruct->longname);
	G_free(dstruct->ellps);
	old = dstruct;
	dstruct = old->next;
	G_free(old);
    }

    return;
}
