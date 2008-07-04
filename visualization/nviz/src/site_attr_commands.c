/***************************************************************
 *
 * MODULE:       site_attr_commands.c 1.0
 *
 * AUTHOR(S):    ACS - Massimo Cuomo - m.cuomo at acsys.it
 *
 * PURPOSE:      To manage color and site attributes for each
 *                 point in a single site.
 *
 * REQUIREMENTS: Many modifications in many files:
 *                 see following README section
 *
 * COPYRIGHT:    (C) 2005 by the ACS / GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/

#if 0
*********************************************************************************
********************* FILE MAP **************************************************
*********************************************************************************

/* init function called by nviz_init.c *****************************************/
site_attr_init_tcl


/*******************************************************************************/
/*** Tcl commands definition ***************************************************/
/*******************************************************************************/

/* Map Tcl commands: related to a specific map, but not to a specific field ****/
Nsite_attr_get_fields_name_cmd
Nsite_attr_get_fields_type_cmd
Nsite_attr_get_fields_name_and_type_cmd

/* Field Tcl commands: related to a specific map *AND* to a specific field (by its index) */
Nsite_attr_get_field_values_cmd
Nsite_attr_get_field_not_emtpy_cats_cmd

/* Field Tcl commands: related to a specific map *AND* to a specific record (by its category) */
Nsite_attr_get_record_values_cmd

/* General Tcl commands: not related to any specific map ***********************/
Nsite_attr_get_GPT_MAX_ATTR_cmd



/*******************************************************************************/
/* Functions used by set_att in map_obj.c to set the geosite attributes from DB*/
/*******************************************************************************/

/******************* Open / Close Map structures *******************************/
site_attr_open_map
site_attr_close_map


site_attr_init
site_attr_set
site_attr_unset
site_attr_set_color
site_attr_set_size
site_attr_set_fixed_color
site_attr_set_fixed_size
site_attr_set_fixed_marker



/*******************************************************************************/
/* Interpolation and evaluation functions definitions **************************/
/*******************************************************************************/

/* Interpolation Tcl command ***************************************************/
Nsite_attr_get_interpolated_values_cmd

/* Color conversion functions **************************************************/
attr_get_int_BBGGRR
attr_get_RRGGBB
attr_get_char_RRGGBB

/************************* interp **********************************************/
attr_interp_entries
attr_interp_entries_string
attr_interp_colors
attr_interp_colors_string

/************************* eval ************************************************/
attr_eval_entry
attr_eval_entry_string
attr_eval_color
attr_eval_color_string




*******************************************************************************
******************** site_attr "README" ***************************************
*******************************************************************************

Files involved:

- src
(mod) visualization/nviz/src/Makefile
(new) visualization/nviz/src/site_attr_commands.c
(mod) visualization/nviz/src/nviz_init.c
(mod) visualization/nviz/src/map_obj.c

- ogsf
(mod) lib/ogsf/gstypes.h
(mod) lib/ogsf/Gp3.c
(mod) lib/ogsf/gpd.c
(mod) lib/ogsf/gsd_objs.c

- sites
(mod) lib/sites/sites.c

- script
(mod) visualization/nviz/scripts/panel_site.tcl
(mod) visualization/nviz/scripts/colorPopup.tcl
(new) visualization/nviz/scripts/site_attr.tcl
(mod) visualization/nviz/scripts/ACS_utils.tcl



- src

(mod) visualization/nviz/src/Makefile
	added compilation for site_attr_commands.c

(new) visualization/nviz/src/site_attr_commands.c
	this file that contains:
	- all c functions for getting fields values and setting attributes
	- interpolation/evaluation functions used for color and entries
	  that could be put in a separate file if used also by others

(mod) visualization/nviz/src/nviz_init.c
	added call to function: site_attr_init_tcl(interp, &data);
	to intialize all the Tcl commands

(mod) visualization/nviz/src/map_obj.c
	added function: site_attr_init for each created site
	added set/unset useatt functions



- ogsf

(mod) lib/ogsf/gstypes.h
	added:

	#define GPT_MAX_ATTR 8

	in typedef struct g_point{
	...
		int cat;
		int color[GPT_MAX_ATTR];
		float size[GPT_MAX_ATTR];
		int marker[GPT_MAX_ATTR];
	...
	} geopoint;

	in typedef struct g_site{
	...
		int use_attr[GPT_MAX_ATTR]; /* ST_ATT_COLOR, ST_ATT_MARKER, ST_ATT_SIZE, ST_ATT_NONE, for multiple attr's */
	...
	} geosite;



(mod) lib/ogsf/Gp3.c
	added line:
		gpt->cat = nextsite->ccat;
	in order to get the link at the site cat


(mod) lib/ogsf/gpd.c
	added gpd_obj_site_attr in place of gpd_obj to manage multiple attributes

(mod) lib/ogsf/gsd_objs.c
	added the "box" drawing for histograms even if it should better stayed
	in file gsd_objs.c


- sites

(mod) lib/sites/sites.c
	- managing Map_info structure for sites


- script
(mod) visualization/nviz/scripts/panel_site.tcl
	added site_attr.tcl calls


(new) visualization/nviz/scripts/site_attr.tcl
	all the GUI and calls to site_attr_commands.c functions


(mod) visualization/nviz/scripts/colorPopup.tcl
	added line
		setScales $w.top.left $color
	to solve that the scales and $w.top.color are set to white next time
	after a color button (not sliders) has been used, regardless $color


(mod) visualization/nviz/scripts/ACS_utils.tcl
	added function:
		modal_edit_list_plain use in site_attr.tcl

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <grass/Vect.h>
#include <grass/site.h>

#include "interface.h"

#define	R_G_B_2_RGB(r,g,b) ((b) & 0xff) | (((g) & 0xff) << 8) | (((r) & 0xff) << 16)


/*******************************************************************************/
/* Tcl commands declaration and initialization *********************************/
/*******************************************************************************/
int Nsite_attr_get_value_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);

int Nsite_attr_get_fields_name_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);
int Nsite_attr_get_fields_type_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);
int Nsite_attr_get_fields_name_and_type_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);

int Nsite_attr_get_field_values_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);
int Nsite_attr_get_field_not_emtpy_cats_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);

int Nsite_attr_get_record_values_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);




int Nget_interpolated_values_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv);



/* init function called by nviz_init.c *****************************************/

void site_attr_init_tcl(Tcl_Interp * interp, Nv_data * data)
{
	Tcl_CreateCommand(interp, "Nsite_attr_get_value", (Tcl_CmdProc*)Nsite_attr_get_value_cmd, data, NULL);

	Tcl_CreateCommand(interp, "Nsite_attr_get_fields_name", (Tcl_CmdProc*)Nsite_attr_get_fields_name_cmd, data, NULL);
	Tcl_CreateCommand(interp, "Nsite_attr_get_fields_type", (Tcl_CmdProc*)Nsite_attr_get_fields_type_cmd, data, NULL);
	Tcl_CreateCommand(interp, "Nsite_attr_get_fields_name_and_type", (Tcl_CmdProc*)Nsite_attr_get_fields_name_and_type_cmd, data, NULL);

	Tcl_CreateCommand(interp, "Nsite_attr_get_field_values", (Tcl_CmdProc*)Nsite_attr_get_field_values_cmd, data, NULL);
	Tcl_CreateCommand(interp, "Nsite_attr_get_field_not_emtpy_cats", (Tcl_CmdProc*)Nsite_attr_get_field_not_emtpy_cats_cmd, data, NULL);

	Tcl_CreateCommand(interp, "Nsite_attr_get_record_values", (Tcl_CmdProc*)Nsite_attr_get_record_values_cmd, data, NULL);



	Tcl_CreateCommand(interp, "Nget_interpolated_values", (Tcl_CmdProc*)Nget_interpolated_values_cmd, data, NULL);
}





/*******************************************************************************/
/* Internal functions declaration **********************************************/
/*******************************************************************************/

/*
  These following functions will better stay in appropriate .h files if
    these functions will be put in separate files
*/
int attr_interp_entries(int n, const char** argvX, const char** argvY, float **x, float **y, float **m);
int attr_interp_colors(int n, const char** argvX, const char** argvY, float **x,
							float **yr, float **yg, float **yb,
							float **mr, float **mg, float **mb);

float attr_eval_entry(float xvalue, int n, float *x, float *y, float *m);
int attr_eval_color(float xvalue, int n, float *x,
							float *yr, float *yg, float *yb,
							float *mr, float *mg, float *mb);

/* better move to ../../../include/P_site.h */
SITE_ATT * G_sites_get_atts (FILE * ptr, int* cat);

/*******************************************************************************/
/*******************************************************************************/
/*** Tcl commands definition ***************************************************/
/*******************************************************************************/
/*******************************************************************************/


/********************************************************************************
	Map Tcl commands: related to a specific map, but not to a specific field
********************************************************************************/

/*
	argv[1] map filename

	returns: list of fields name
*/
int Nsite_attr_get_fields_name_cmd(data, interp, argc, argv)
     Nv_data *data;
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
    struct Map_info *Map;
	int *ctypes;
	char **cnames;
	int *ndx;
	int ncols, i;
	char buf[1024];

    if (argc != 2) return (TCL_ERROR);

	Map = (struct Map_info *)G_sites_open_old(argv[1], G_find_vector2(argv[1], ""));
	ncols=G_sites_get_fields((FILE *)Map, &cnames, &ctypes, &ndx);

	for(i=0; i<ncols; i++) {
		sprintf(buf, "%s", cnames[i]);
		Tcl_AppendElement(interp, buf);
	}

	if (ncols>0) G_sites_free_fields(ncols, cnames, ctypes, ndx);
	G_sites_close(Map);
	return (TCL_OK);
}

/*
	argv[1] map filename

	returns: list of fields type
*/
int Nsite_attr_get_fields_type_cmd(data, interp, argc, argv)
     Nv_data *data;
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
    struct Map_info *Map;
	int *ctypes;
	char **cnames;
	int *ndx;
	int ncols, i;
	char buf[1024];

    if (argc != 2) return (TCL_ERROR);

	Map = (struct Map_info *)G_sites_open_old(argv[1], G_find_vector2(argv[1], ""));
	ncols=G_sites_get_fields((FILE *)Map, &cnames, &ctypes, &ndx);

	for(i=0; i<ncols; i++) {
		sprintf(buf, "%c", ctypes[i]);
		Tcl_AppendElement(interp, buf);
	}

	if (ncols>0) G_sites_free_fields(ncols, cnames, ctypes, ndx);
	G_sites_close(Map);
	return (TCL_OK);
}

/*
	argv[1] map filename

	returns: list of fields name and type
*/
int Nsite_attr_get_fields_name_and_type_cmd(data, interp, argc, argv)
     Nv_data *data;
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
    struct Map_info *Map;
	int *ctypes;
	char **cnames;
	int *ndx;
	int ncols, i;
	char buf[2048];

    if (argc != 2) return (TCL_ERROR);

	Map = (struct Map_info *)G_sites_open_old(argv[1], G_find_vector2(argv[1], ""));
	ncols=G_sites_get_fields((FILE *)Map, &cnames, &ctypes, &ndx);

	for(i=0; i<ncols; i++) {
		sprintf(buf, "%s", cnames[i]);
		Tcl_AppendElement(interp, buf);
		sprintf(buf, "%c", ctypes[i]);
		Tcl_AppendElement(interp, buf);
	}

	if (ncols>0) G_sites_free_fields(ncols, cnames, ctypes, ndx);
	G_sites_close(Map);
	return (TCL_OK);
}



/********************************************************************************
	Field Tcl commands: related to a specific map *AND* to a specific
			field (by its index)
********************************************************************************/

/*
	argv[1] map filename
	argv[2] field index

	returns: list of values of the given field index for all records of the whole site
*/
int Nsite_attr_get_field_values_cmd(data, interp, argc, argv)
     Nv_data *data;
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
    struct Map_info *Map;
	int *ctypes;
	char **cnames;
	int *ndx;
	int ncols, i, index;
	char buf[9182];
	SITE_ATT * sa;

    if (argc != 3) return (TCL_ERROR);

	index=atoi(argv[2]);

	Map = (struct Map_info *)G_sites_open_old(argv[1], G_find_vector2(argv[1], ""));
	ncols=G_sites_get_fields((FILE *)Map, &cnames, &ctypes, &ndx);

	for (i=0; i<Map->n_site_att; i++) {
		sa = &(Map->site_att[i]);
		switch(ctypes[index]) {
			case 'c': /* category */
				sprintf(buf, "%d", sa->cat);
			break;
			case 'd': /* double */
				sprintf(buf, "%f", sa->dbl[ndx[index]]);
			break;
			case 's': /* string */
				sprintf(buf, "%s", sa->str[ndx[index]]);
			break;
		}
		Tcl_AppendElement(interp, buf);
	}

	if (ncols>0) G_sites_free_fields(ncols, cnames, ctypes, ndx);
	G_sites_close(Map);

    return (TCL_OK);
}


/*
	argv[1] map filename
	argv[2] field index

	returns: list of cats, if the given field index is not empty, for all records of the whole site
*/
int Nsite_attr_get_field_not_emtpy_cats_cmd(data, interp, argc, argv)
     Nv_data *data;
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
    struct Map_info *Map;
	int *ctypes;
	char **cnames;
	int *ndx;
	int ncols, i, index;
	char buf[9182];
	SITE_ATT * sa;

    if (argc != 3) return (TCL_ERROR);

	index=atoi(argv[2]);

	Map = (struct Map_info *)G_sites_open_old(argv[1], G_find_vector2(argv[1], ""));
	ncols=G_sites_get_fields((FILE *)Map, &cnames, &ctypes, &ndx);

	for (i=0; i<Map->n_site_att; i++) {
		sa = &(Map->site_att[i]);
/*		printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ i=%d cat=%d field=%s\n", i, sa->cat, sa->str[ndx[index]]);*/
		switch(ctypes[index]) {
			case 's': /* string */
				if ( strlen(sa->str[ndx[index]]) > 0 ) {
					sprintf(buf, "%d", sa->cat);
					Tcl_AppendElement(interp, buf);
				}
			break;
		}
	}

	if (ncols>0) G_sites_free_fields(ncols, cnames, ctypes, ndx);
	G_sites_close(Map);

    return (TCL_OK);
}


/********************************************************************************
	Field Tcl commands: related to a specific map *AND* to a specific
			record (by its category)
********************************************************************************/
/*
	argv[1] map filename
	argv[2] cat

	returns: list of record values with category "cat"
*/
int Nsite_attr_get_record_values_cmd(data, interp, argc, argv)
     Nv_data *data;
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
    struct Map_info *Map;
	int *ctypes;
	char **cnames;
	int *ndx;
	int ncols, i, cat;
	char buf[9182];
	SITE_ATT * sa;

    if (argc != 3) return (TCL_ERROR);

	cat=atoi(argv[2]);

	Map = (struct Map_info *)G_sites_open_old(argv[1], G_find_vector2(argv[1], ""));
	ncols=G_sites_get_fields((FILE *)Map, &cnames, &ctypes, &ndx);


	if ( (sa = (SITE_ATT *)G_sites_get_atts((FILE *)Map, &cat)) == NULL ) return (TCL_ERROR);

	for (i=0; i<ncols; i++) {
		switch(ctypes[i]) {
			case 'c': /* category */
				sprintf(buf, "%d", sa->cat);
			break;
			case 'd': /* double */
				sprintf(buf, "%f", sa->dbl[ndx[i]]);
			break;
			case 's': /* string */
				sprintf(buf, "%s", sa->str[ndx[i]]);
			break;
		}
		Tcl_AppendElement(interp, buf);
	}

	if (ncols>0) G_sites_free_fields(ncols, cnames, ctypes, ndx);
	G_sites_close(Map);

    return (TCL_OK);
}


/*******************************************************************************/
/* General Tcl commands: not related to any specific map **************************/
/*******************************************************************************/

/********************************************************************************
	Get GPT_MAX_ATTR, ST_ATT_COLOR, ST_ATT_MARKER, ST_ATT_SIZE values Tcl command


	no input argument

	returns the value of GPT_MAX_ATTR defined in gstypes.h
********************************************************************************/
int Nsite_attr_get_value_cmd(data, interp, argc, argv)
     Nv_data *data;
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
	char buf[1024];

	if (!strcmp(argv[1], "GPT_MAX_ATTR")) sprintf(buf, "%d", GPT_MAX_ATTR);
	else if (!strcmp(argv[1], "ST_ATT_COLOR")) sprintf(buf, "%d", ST_ATT_COLOR);
	else if (!strcmp(argv[1], "ST_ATT_MARKER")) sprintf(buf, "%d", ST_ATT_MARKER);
	else if (!strcmp(argv[1], "ST_ATT_SIZE")) sprintf(buf, "%d", ST_ATT_SIZE);
	else return (TCL_ERROR);

	Tcl_AppendElement(interp, buf);
	return (TCL_OK);
}

/*******************************************************************************/
/*
	Functions used by set_att in map_obj.c to set the geosite attributes from DB

		site_attr_init, site_attr_set, site_attr_unset
*/
/*******************************************************************************/

/******************* Open / Close Map structures *******************************/
int site_attr_open_map(geosite * gp, int index,
					struct Map_info **Map, int *ncols, char ***cnames, int **ctypes, int **ndx)
{
	const char *function_name="site_attr_open_map";

	*Map = (struct Map_info *)G_sites_open_old(gp->filename, G_find_vector2(gp->filename, ""));

	/* this is needed to find association between names and proper indexes/types */
	*ncols=G_sites_get_fields((FILE *)*Map, cnames, ctypes, ndx);

	if (*ncols <= 0) {
		printf("WARNING / \"%s()\": site \"%s\" database error\n", function_name, gp->filename);
		return(-1);
	}
	return(0);
}


void site_attr_close_map(struct Map_info *Map, int ncols, char **cnames, int *ctypes, int *ndx)
{
	if (ncols>0) G_sites_free_fields(ncols, cnames, ctypes, ndx);
	G_sites_close(Map);
}


/* declarations of following functions */
int site_attr_set_color(geosite * gp, int nattr, int index, int n, const char** argvX, const char** argvY);
int site_attr_set_size(geosite * gp, int nattr, int index, int n, const char** argvX, const char** argvY);
int site_attr_set_fixed_color(geosite * gp, int nattr, unsigned int color);
int site_attr_set_fixed_size(geosite * gp, int nattr, float size);
int site_attr_set_fixed_marker(geosite * gp, int nattr, int marker);



void site_attr_init(int id)
{
	geosite * gp;
	int i;

	gp = gp_get_site(id);
	for (i=0; i<GPT_MAX_ATTR; i++) gp->use_attr[i]=ST_ATT_NONE;
}


int site_attr_set(Tcl_Interp *interp, geosite * gp, int nattr, char * attr, int index, char *xlist, char *ylist)
{
	int argcPtrX, argcPtrY;
	const char **argvPtrX, **argvPtrY;

	if (nattr >= GPT_MAX_ATTR) {return(TCL_ERROR);} /* too many */

	argvPtrX = NULL; argvPtrY = NULL;

	if (index >= 0) {
		/* if index < 0, then fixed values */
		if (TCL_OK != Tcl_SplitList(interp, xlist, &argcPtrX, &argvPtrX)) goto error;
		if (TCL_OK != Tcl_SplitList(interp, ylist, &argcPtrY, &argvPtrY)) goto error;
		if (argcPtrX != argcPtrY) {
			printf("WARNING: elements in lists are different\n");
			goto error;
		}
		if (argcPtrX < 2) {
			printf("WARNING: elements in list are too few\n");
			goto error;
		}
	}

	if (!strcmp(attr, "size")) {
		if (index < 0) {
			site_attr_set_fixed_size(gp, nattr, atof(xlist));
		} else {
			if ( 0 > site_attr_set_size(gp, nattr, index, argcPtrX, argvPtrX, argvPtrY)) goto error;
		}
		gp->use_attr[nattr] |= ST_ATT_SIZE;
	}

	else if (!strcmp(attr, "color")) {
		if (index < 0) {
			site_attr_set_fixed_color(gp, nattr, attr_get_int_BBGGRR(xlist));
		} else {
			if ( 0 > site_attr_set_color(gp, nattr, index, argcPtrX, argvPtrX, argvPtrY)) goto error;
		}
		gp->use_attr[nattr] |= ST_ATT_COLOR;
	}
	else if (!strcmp(attr, "marker")) {
		site_attr_set_fixed_marker(gp, nattr, atoi(xlist));
		gp->use_attr[nattr] |= ST_ATT_MARKER;
	}

	if (argvPtrX) Tcl_Free((char *) argvPtrX);
	if (argvPtrY) Tcl_Free((char *) argvPtrY);
	return(TCL_OK);

error:
	if (argvPtrX) Tcl_Free((char *) argvPtrX);
	if (argvPtrY) Tcl_Free((char *) argvPtrY);
	return(TCL_ERROR);
}

int site_attr_unset(Tcl_Interp *interp, geosite * gp, int nattr, char * attr)
{
/* See if we have to empty the vectors or not */
	if (!strcmp(attr, "size")) {
		gp->use_attr[nattr] &= ~ST_ATT_SIZE;
	}
	else if (!strcmp(attr, "color")) {
		gp->use_attr[nattr] &= ~ST_ATT_COLOR;
	}
	else if (!strcmp(attr, "marker")) {
		gp->use_attr[nattr] &= ~ST_ATT_MARKER;
	}

	return(TCL_OK);
}

int site_attr_get(Tcl_Interp *interp, geosite * gp, int nattr)
{
	char buf[1024];

	sprintf(buf, "%d", gp->use_attr[nattr]);

	Tcl_AppendElement(interp, buf);
	return(TCL_OK);
}



int site_attr_set_color(geosite * gp, int nattr, int index, int n, const char** argvX, const char** argvY)
{
	struct Map_info *Map; geopoint *gpt; SITE_ATT *sa;
	int *ctypes; char **cnames; int *ndx;
	int ncols;
	float *x, *yr, *yg, *yb, *mr, *mg, *mb;

	if (0 > site_attr_open_map(gp, index, &Map, &ncols, &cnames, &ctypes, &ndx)) return(-1);

	if (ctypes[index] == 's') {
		if (0 > attr_interp_colors_string(n, argvY, &yr, &yg, &yb)) return(-1);
	} else {
		if (0 > attr_interp_colors(n, argvX, argvY, &x, &yr, &yg, &yb, &mr, &mg, &mb)) return(-1);
	}

	for (gpt = gp->points; gpt; gpt = gpt->next) {
		if (ctypes[index] == 'c') {
			gpt->color[nattr] = attr_eval_color(gpt->cat, n, x, yr, yg, yb, mr, mg, mb);
		}
		else if (ctypes[index] == 'd') {
			if ( (sa = (SITE_ATT *)G_sites_get_atts((FILE *)Map, &(gpt->cat))) == NULL ) continue;
			else gpt->color[nattr] = attr_eval_color(sa->dbl[ndx[index]], n, x, yr, yg, yb, mr, mg, mb);
		} else {
			if ( (sa = (SITE_ATT *)G_sites_get_atts((FILE *)Map, &(gpt->cat))) == NULL ) continue;
			else gpt->color[nattr] = attr_eval_color_string(sa->str[ndx[index]], n, argvX, yr, yg, yb);
		}
	}

	if (ctypes[index] == 's') {free(yr); free(yg); free(yb);}
	else {free(x); free(yr); free(yg); free(yb); free(mr); free(mg); free(mb);}

	site_attr_close_map(Map, ncols, cnames, ctypes, ndx);
	return(0);
}

int site_attr_set_size(geosite * gp, int nattr, int index, int n, const char** argvX, const char** argvY)
{
	struct Map_info *Map; geopoint *gpt; SITE_ATT *sa;
	int *ctypes; char **cnames; int *ndx;
	int ncols;
	float *x, *y, *m;

	if (0 > site_attr_open_map(gp, index, &Map, &ncols, &cnames, &ctypes, &ndx)) return(-1);

	if (ctypes[index] == 's') {
		if (0 > attr_interp_entries_string(n, argvY, &y)) return(-1);
	} else {
		if (0 > attr_interp_entries(n, argvX, argvY, &x, &y, &m)) return(-1);
	}

	for (gpt = gp->points; gpt; gpt = gpt->next) {
		if (ctypes[index] == 'c') {
			gpt->size[nattr] = attr_eval_entry(gpt->cat, n, x, y, m);
		}
		else if (ctypes[index] == 'd') {
			if ( (sa = (SITE_ATT *)G_sites_get_atts((FILE *)Map, &(gpt->cat))) == NULL ) continue;
			else gpt->size[nattr] = attr_eval_entry(sa->dbl[ndx[index]], n, x, y, m);
		}
		else {
			if ( (sa = (SITE_ATT *)G_sites_get_atts((FILE *)Map, &(gpt->cat))) == NULL ) continue;
			else gpt->size[nattr] = attr_eval_entry_string(sa->str[ndx[index]], n, argvX, y);
		}
	}

	if (ctypes[index] == 's') {free(y);}
	else {free(x); free(y); free(m);}

	site_attr_close_map(Map, ncols, cnames, ctypes, ndx);
	return(0);
}

int site_attr_set_fixed_color(geosite * gp, int nattr, unsigned int color)
{
	geopoint *gpt;
	for (gpt = gp->points; gpt; gpt = gpt->next) {gpt->color[nattr] = color;}
	return(0);
}

int site_attr_set_fixed_size(geosite * gp, int nattr, float size)
{
	geopoint *gpt;
	for (gpt = gp->points; gpt; gpt = gpt->next) {gpt->size[nattr] = size;}
	return(0);
}

int site_attr_set_fixed_marker(geosite * gp, int nattr, int marker)
{
	geopoint *gpt;
	for (gpt = gp->points; gpt; gpt = gpt->next) {gpt->marker[nattr] = marker;}
	return(0);
}



/*
 The following section doesn't depends from sites.
 It is only color conversion, interpolation and evaluation of color/entries.
 Hence they are not called site_attr_XXX, but only attr_XXX.
 They can be put in a separate file, but are kept here to not increase too much the
 total file number.
*/

/*******************************************************************************/
/* Color conversion functions **************************************************/
/*******************************************************************************/

int attr_get_int_BBGGRR(const char* rrggbb)
{
/* rrggbb is in the form of #RRGGBB (first char is skipped) */
	char strbuf[16];
	memcpy(strbuf + 0, rrggbb + 5, 2);
	memcpy(strbuf + 2, rrggbb + 3, 2);
	memcpy(strbuf + 4, rrggbb + 1, 2);
	memset(strbuf + 6, 0, 1);
	return(strtol(strbuf, NULL, 16));
}


float attr_get_RRGGBB(const char* rrggbb, float *r, float *g, float *b)
{
/* rrggbb is in the form of #RRGGBB (first char is skipped) */
	char strbuf[16];

	memcpy(strbuf, rrggbb + 1, 2);
	memset(strbuf+2, 0, 1);
	*r=(float)strtol(strbuf, NULL, 16);

	memcpy(strbuf, rrggbb + 3, 2);
	memset(strbuf+2, 0, 1);
	*g=(float)strtol(strbuf, NULL, 16);

	memcpy(strbuf, rrggbb + 5, 2);
	memset(strbuf+2, 0, 1);
	*b=(float)strtol(strbuf, NULL, 16);

	return(*r * 65536.0f + *g * 256.0f + *b);
}


void attr_get_char_RRGGBB(char* rrggbb, float bbggrr)
{
	char strbuf[16];
	sprintf(strbuf, "%06x", (int)bbggrr);

	memset(rrggbb + 0, '#', 1);
	memcpy(rrggbb + 1, strbuf + 4, 2);
	memcpy(rrggbb + 3, strbuf + 2, 2);
	memcpy(rrggbb + 5, strbuf + 0, 2);
	memset(rrggbb + 7, 0, 1);
}



/********************************************************************************
	Interpolation Tcl command


	argv[1] attr
	argv[2] list of X values
	argv[3] list of X interpolation points
	argv[4] list of Y interpolation points

	returns the list of Y interpolated values
********************************************************************************/
int Nget_interpolated_values_cmd(data, interp, argc, argv)
     Nv_data *data;
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
	int i, n;
	char buf[1024];

	int argcPtrX, argcPtrY;
	const char **argvPtrX, **argvPtrY;
	float *x, *y, *m, *yr, *yg, *yb, *mr, *mg, *mb, xvalue, yvalue;

    if (argc != 5)
	return (TCL_ERROR);


	if (TCL_OK != Tcl_SplitList(interp, argv[3], &argcPtrX, &argvPtrX)) return (TCL_ERROR);
	if (TCL_OK != Tcl_SplitList(interp, argv[4], &argcPtrY, &argvPtrY)) return (TCL_ERROR);
	if(argcPtrX != argcPtrY) return (TCL_ERROR);
	n = argcPtrX;

	if (!strcmp(argv[1], "color")) {
		if (0 < attr_interp_colors(n, argvPtrX, argvPtrY, &x, &yr, &yg, &yb, &mr, &mg, &mb)) return(TCL_ERROR);
	}
	else if (!strcmp(argv[1], "size")) {
			if (0 < attr_interp_entries(n, argvPtrX, argvPtrY, &x, &y, &m)) return(TCL_ERROR);
	}

	Tcl_Free((char *) argvPtrX);
	Tcl_Free((char *) argvPtrY);

	if (TCL_OK != Tcl_SplitList(interp, argv[2], &argcPtrX, &argvPtrX)) return (TCL_ERROR);

	if (!strcmp(argv[1], "color")) {
		for(i=0; i<argcPtrX; i++) {
			xvalue=atof(argvPtrX[i]);

			yvalue = attr_eval_color(xvalue, n, x, yr, yg, yb, mr, mg, mb);
			attr_get_char_RRGGBB(buf, yvalue);

			Tcl_AppendElement(interp, buf);
		}
		free(x); free(yr); free(yg); free(yb); free(mr); free(mg); free(mb);
	}
	else if (!strcmp(argv[1], "size")) {

		for(i=0; i<argcPtrX; i++) {
			xvalue=atof(argvPtrX[i]);

			yvalue = attr_eval_entry(xvalue, n, x, y, m);
			sprintf(buf, "%f", yvalue);

			Tcl_AppendElement(interp, buf);
		}
		free(x); free(y); free(m);
	}

	Tcl_Free((char *) argvPtrX);
	return (TCL_OK);
}




/*******************************************************************************/
/* Interpolation and evaluation functions definitions **************************/
/*******************************************************************************/


/************************* interp **********************************************/

/********************************************************************************
	- argvX and argvY are X and Y of the points used to buil the intepolation
	- argvX and argvY must have the same number (n) of values
	- argvX elements must be ordered
	- n must be more than 2 (at least 2 points for interpolation)
	- the interpolation is piecewise linear
	- values outside the first/last X interval will be kept constant at
		corresponding Y values
	- caller must free x, y, m

	There is difference between numerics and strings:
	 - numerics really interpolate
	 - string do not: values are kept constant and change when there is a new one

********************************************************************************/
int attr_interp_entries(int n, const char** argvX, const char** argvY, float **x, float **y, float **m)
{
	const char *function_name="attr_interp_entries";

	int i;

	*x = (float*) malloc(n*sizeof(float));
	*y = (float*) malloc(n*sizeof(float));
	*m = (float*) malloc((n-1)*sizeof(float));

	(*x)[0] = atof(argvX[0]);
	(*y)[0] = atof(argvY[0]);

	for(i=1; i<n; i++) {
		(*x)[i] = atof(argvX[i]);
		(*y)[i] = atof(argvY[i]);

		if ((*x)[i] == (*x)[i-1]) {
			printf("WARNING / \"%s()\": x elements in list cannot be equal\n", function_name);
			return (-4);
		}
		(*m)[i-1] = ((*y)[i] - (*y)[i-1]) / ((*x)[i] - (*x)[i-1]);
	}
	return(0);
}

int attr_interp_entries_string(int n, const char** argvY, float **y)
{
	int i;
	*y = (float*) malloc(n*sizeof(float));
	for(i=0; i<n; i++) (*y)[i] = atof(argvY[i]);
	return(0);
}

int attr_interp_colors(int n, const char** argvX, const char** argvY, float **x,
		       float **yr, float **yg, float **yb,
		       float **mr, float **mg, float **mb)
{
	const char *function_name="attr_interp_colors";

	int i;
	float dx;

	*x = (float*) malloc(n*sizeof(float));
	*yr = (float*) malloc(n*sizeof(float));
	*mr = (float*) malloc((n-1)*sizeof(float));
	*yg = (float*) malloc(n*sizeof(float));
	*mg = (float*) malloc((n-1)*sizeof(float));
	*yb = (float*) malloc(n*sizeof(float));
	*mb = (float*) malloc((n-1)*sizeof(float));

	(*x)[0] = atof(argvX[0]);

	/* argv[0] is in #BBGGRR*/
	attr_get_RRGGBB(argvY[0], &((*yb)[0]), &((*yg)[0]), &((*yr)[0]));

	for(i=1; i<n; i++) {
		(*x)[i] = atof(argvX[i]);

		/* argvY[i] is in #BBGGRR*/
		attr_get_RRGGBB(argvY[i], &((*yb)[i]), &((*yg)[i]), &((*yr)[i]));

		if ((*x)[i] == (*x)[i-1]) {
			printf("WARNING / \"%s()\": x elements in list cannot be equal\n", function_name);
			return (-4);
		}
		dx = (*x)[i] - (*x)[i-1];
		(*mr)[i-1] = ((*yr)[i] - (*yr)[i-1]) / dx;
		(*mg)[i-1] = ((*yg)[i] - (*yg)[i-1]) / dx;
		(*mb)[i-1] = ((*yb)[i] - (*yb)[i-1]) / dx;
	}
	return(0);

}

int attr_interp_colors_string(int n, const char** argvY,
			      float **yr, float **yg, float **yb)
{
	int i;

	*yr = (float*) malloc(n*sizeof(float));
	*yg = (float*) malloc(n*sizeof(float));
	*yb = (float*) malloc(n*sizeof(float));

	for(i=0; i<n; i++) {
		/* argvY[i] is in #BBGGRR*/
		attr_get_RRGGBB(argvY[i], &((*yb)[i]), &((*yg)[i]), &((*yr)[i]));
	}
	return(0);
}

/************************* eval ************************************************/

float attr_eval_entry(float xvalue, int n, float *x, float *y, float *m)
{
	int i;
	if (xvalue <= x[0]) return(y[0]);
	else if (xvalue >= x[n-1]) return(y[n-1]);
	else {
		for(i=1; i<n && xvalue > x[i]; i++);
		/* now: x[i-1] < value <= x[i] && i >= 1 */
		return(m[i-1] * (xvalue - x[i-1]) + y[i-1]);
	}
}

float attr_eval_entry_string(const char* xvalue, int n, const char** x, float* y)
{
	int i;
	if (strcmp(xvalue, x[0]) <= 0) return(y[0]);
	else if (strcmp(xvalue, x[n-1]) >= 0) return(y[n-1]);
	else {
		for(i=1; i<n && (strcmp(xvalue, x[i]) > 0); i++);
		/* now: x[i-1] < value <= x[i] && i >= 1 */
		return(y[i-1]);
	}
}

int attr_eval_color(float xvalue, int n, float *x,
		    float *yr, float *yg, float *yb,
		    float *mr, float *mg, float *mb)
{
	int i;
	float r, g, b, dx;

	if (xvalue <= x[0]) return(R_G_B_2_RGB((int)(yr[0]), (int)(yg[0]), (int)(yb[0])));
	else if (xvalue >= x[n-1]) return(R_G_B_2_RGB((int)(yr[n-1]), (int)(yg[n-1]), (int)(yb[n-1])));
	else {
		for(i=1; i<n && xvalue > x[i]; i++);
		/* now: x[i-1] < value <= x[i] && i >= 1 */
		dx = xvalue - x[i-1];

		r = dx * mr[i-1] + yr[i-1];
		g = dx * mg[i-1] + yg[i-1];
		b = dx * mb[i-1] + yb[i-1];

		return(R_G_B_2_RGB((int)(r), (int)(g), (int)(b)));
	}
}

int attr_eval_color_string(const char* xvalue, int n, const char** x,
			   float *yr, float *yg, float *yb)
{
	int i;

	if (strcmp(xvalue, x[0]) <= 0) return(R_G_B_2_RGB((int)(yr[0]), (int)(yg[0]), (int)(yb[0])));
	else if (strcmp(xvalue, x[n-1]) >= 0) return(R_G_B_2_RGB((int)(yr[n-1]), (int)(yg[n-1]), (int)(yb[n-1])));
	else {
		for(i=1; i<n && (strcmp(xvalue, x[i]) > 0); i++);
		/* now: x[i-1] < value <= x[i] && i >= 1 */
		return(R_G_B_2_RGB((int)(yr[i-1]), (int)(yg[i-1]), (int)(yb[i-1])));
	}
}


