/*  
 ****************************************************************************
 *
 * MODULE:       g.proj 
 * AUTHOR(S):    Paul Kelly - paul-grass@stjohnspoint.co.uk
 * PURPOSE:      Provides a means of reporting the contents of GRASS
 *               projection information files and creating
 *               new projection information files.
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gprojects.h>

#include "local_proto.h"

/**
 * 
 * \brief Add or replace datum to the current co-ordinate system definition
 * 
 * \param datum       Use this datum (overrides any datum found in the
 *                    current co-ordinate system definition).
 * 
 * \return            1 if a change was made, 0 if not.
 **/

int set_datum(char *datum)
{
    struct gpj_datum dstruct;
    struct Key_Value *temp_projinfo;
    int i;

    if (cellhd.proj == PROJECTION_XY)
	return 0;

    if (!datum || GPJ_get_datum_by_name(datum, &dstruct) < 0)
    {
	G_fatal_error(_("Invalid datum code <%s>"), datum);
	return 0;
    }

    temp_projinfo = G_create_key_value();

    /* Copy old PROJ_INFO, skipping out any keys related
     * to datum or ellipsoid parameters */
    for (i = 0; i < projinfo->nitems; i++) {
        if (strcmp(projinfo->key[i], "datum") == 0
	    || strcmp(projinfo->key[i], "dx") == 0
	    || strcmp(projinfo->key[i], "dy") == 0
	    || strcmp(projinfo->key[i], "dz") == 0
	    || strcmp(projinfo->key[i], "datumparams") == 0
	    || strcmp(projinfo->key[i], "nadgrids") == 0
	    || strcmp(projinfo->key[i], "towgs84") == 0
	    || strcmp(projinfo->key[i], "ellps") == 0
	    || strcmp(projinfo->key[i], "a") == 0
	    || strcmp(projinfo->key[i], "b") == 0
	    || strcmp(projinfo->key[i], "es") == 0
	    || strcmp(projinfo->key[i], "f") == 0
	    || strcmp(projinfo->key[i], "rf") == 0)
	    continue;

	G_set_key_value(projinfo->key[i], projinfo->value[i],
			temp_projinfo);
    }

    /* Finally add datum and ellipsoid names */
    G_set_key_value("datum", dstruct.name, temp_projinfo);
    G_message(_("Datum set to <%s>"), dstruct.name);
    G_set_key_value("ellps", dstruct.ellps, temp_projinfo);
    G_message(_("Ellipsoid set to <%s>"), dstruct.ellps);

    /* Destroy original key/value structure and replace with new one */
    G_free_key_value(projinfo);
    projinfo = temp_projinfo;

    return 1;
}

/**
 * 
 * \brief Add or replace datum transformation parameters to the current
 *        co-ordinate system definition
 * 
 * \param datumtrans  Index number of parameter set to use, 0 to leave
 *                    unspecified (or remove specific parameters, leaving just
 *                    the datum name), -1 to list the available parameter
 *                    sets for this datum and exit.
 *
 * \param force       Force editing of parameters even if current co-ordinate
 *                    system already contains fully specified parameters.
 * 
 * \return            1 if a change was made, 0 if not.
 **/

int set_datumtrans(int datumtrans, int force)
{
    char *params, *datum = NULL;
    int paramsets, status;

    if (cellhd.proj == PROJECTION_XY)
	return 0;

    status = GPJ__get_datum_params(projinfo, &datum, &params);
    G_debug(3, "set_datumtrans(): GPJ__get_datum_params() status=%d", status);
    G_free(params);

    if (datum) {
	/* A datum name is specified; need to determine if
	 * there are parameters to choose from for this datum */
	struct gpj_datum dstruct;

	if (GPJ_get_datum_by_name(datum, &dstruct) > 0) {
	    char *defparams;

	    paramsets =
		GPJ_get_default_datum_params_by_name(dstruct.name,
						     &defparams);
	    G_free(defparams);
	    GPJ_free_datum(&dstruct);

	    G_debug(3, "set_datumtrans(): datum transform terms found "
		    "with %d options", paramsets);

	    if (paramsets > 1 && (status == 1 || datumtrans != 0))
		/* Parameters are missing and there is a choice to be
                   made / or / user asked to print datum
                   transformation parameters */
		force = 1;

	}
	else {
	    /* Datum name not found in table; can't do anything. */
	    G_debug(3, "set_datumtrans(): Datum name not found in table.");
	    force = 0;
	}
    }
    else {
	/* No datum name; can't do anything. */
	G_debug(3, "set_datumtrans(): Datum name either invalid or not supplied.");
	force = 0;
    }

    if (force) {
	char *chosenparams = NULL;
	char *paramkey, *paramvalue;
	struct Key_Value *temp_projinfo;
	int i;

	/* First of all obtain the new parameters 
	 * through the supplied transform number index */
	{
	    struct gpj_datum_transform_list *list;

	    if (datumtrans > paramsets)
		G_fatal_error
		    ("Invalid transformation number %d; valid range is 1 to %d",
		     datumtrans, paramsets);

	    G_debug(3, "set_datumtrans(): looking up available datum "
		    "transforms for <%s>", datum);

	    list = GPJ_get_datum_transform_by_name(datum);

	    if (list != NULL) {
		if (datumtrans == -1) {
		    do {
			struct gpj_datum_transform_list *old = list;
			fprintf(stdout, "---\n%d\nUsed in %s\n%s\n%s\n",
				list->count, list->where_used,
				list->params, list->comment);
			list = list->next;
		        GPJ_free_datum_transform(old);
		    } while (list != NULL);

		    exit(EXIT_SUCCESS);
		}
		else {
		    do {
			struct gpj_datum_transform_list *old = list;
			if (list->count == datumtrans)
			    chosenparams = G_store(list->params);
			list = list->next;
		        GPJ_free_datum_transform(old);
		    } while (list != NULL);
		}
	    }

	}

	temp_projinfo = G_create_key_value();

	/* Copy old PROJ_INFO, skipping out any keys related
	 * to datum parameters */
	for (i = 0; i < projinfo->nitems; i++) {
	    if (strcmp(projinfo->key[i], "dx") == 0
		|| strcmp(projinfo->key[i], "dy") == 0
		|| strcmp(projinfo->key[i], "dz") == 0
		|| strcmp(projinfo->key[i], "datumparams") == 0
		|| strcmp(projinfo->key[i], "nadgrids") == 0
		|| strcmp(projinfo->key[i], "towgs84") == 0)
		continue;

	    G_set_key_value(projinfo->key[i], projinfo->value[i],
			    temp_projinfo);
	}

	/* Finally add new parameters (if we have them) */
	if (chosenparams != NULL) {
	    /* Now split 'chosenparams' into key/value format */
	    paramkey = strtok(chosenparams, "=");
	    paramvalue = chosenparams + strlen(paramkey) + 1;
	    G_set_key_value(paramkey, paramvalue, temp_projinfo);
	    G_free(chosenparams);
	}

	/* Destroy original key/value structure and replace with new one */
	G_free_key_value(projinfo);
	projinfo = temp_projinfo;

    }
    G_free(datum);

    return force;
}
