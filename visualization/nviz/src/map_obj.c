/*
 * map_obj.c --
 *
 * Define creation and interface functions for map objects.
 * Map objects are considered to be surfaces, vector plots,
 * or site files.
 */

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <grass/gis.h>
#include "interface.h"

/* Extern declarations */
extern char *int_to_tcl_color();

/* Prototypes */
int Nnew_map_obj_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		     char *argv[]);
int Nmap_obj_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char *argv[]);
int Nget_id_from_name_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
			  char *argv[]);
int set_drawmode(Tcl_Interp * interp, int type, int id, Nv_data * data,
		 int argc, char *argv[]);
int get_drawmode(int type, int id, Nv_data * data, Tcl_Interp * interp);
int draw_obj(Tcl_Interp * interp, int id, int type);
int delete_obj(int id, int type, Nv_data * data, Tcl_Interp * interp);
int get_res(int id, int type, Tcl_Interp * interp, int argc, char *argv[]);
int set_res(Tcl_Interp * interp, int id, int type, int argc, char *argv[]);
int set_wirecolor(Tcl_Interp * interp, int id, int type, Nv_data * data,
		  int argc, char *argv[]);
int get_wirecolor(int id, int type, Nv_data * data, Tcl_Interp * interp);
int get_trans(int id, int type, Tcl_Interp * interp);
int set_trans(Tcl_Interp * interp, int id, int type, int argc, char *argv[]);
int set_nozero(Tcl_Interp * interp, int id, int type, Nv_data * data,
	       int argc, char *argv[]);
int get_nozero(int id, int type, Nv_data * data, Tcl_Interp * interp,
	       int argc, char *argv[]);
int select_surf(Tcl_Interp * interp, int id, int type, int argc,
		char *argv[]);
int unselect_surf(int id, int type, int argc, char *argv[],
		  Tcl_Interp * interp);
int surf_is_selected(int id, int type, Tcl_Interp * interp, int argc,
		     char *argv[]);
int set_exag_obj(int id, int type, int argc, char *argv[],
		 Tcl_Interp * interp);
int get_exag_guess(int id, int type, Tcl_Interp * interp);
int load_obj(int id, int type, Nv_data * data, int argc, char *argv[],
	     Tcl_Interp * interp);
int get_att(int id, int type, Nv_data * data, Tcl_Interp * interp, int argc,
	    char *argv[]);
int get_mask_mode(int id, int type, Nv_data * data, Tcl_Interp * interp);
int set_mask_mode(int id, int type, Nv_data * data, Tcl_Interp * interp,
		  int argc, char *argv[]);
int set_att(int id, int type, Nv_data * data, Tcl_Interp * interp, int argc,
	    char *argv[]);
int unset_att(int id, int type, Tcl_Interp * interp, int argc, char *argv[]);
int get_idnum(char *name);
int get_type(char *name);
int att_atoi(char *attname);
int sv_att_atoi(char *attname);
int get_char_marker(int m, char *marker);
int get_int_marker(char *marker);
int Nget_surf_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		       char *argv[]);
int Nget_vect_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		       char *argv[]);
int Nget_site_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		       char *argv[]);
int Nget_vol_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		      char *argv[]);
int set_logical_name(int id, Tcl_Interp * interp, int argc, char *argv[]);
int get_logical_name(int id, Tcl_Interp * interp, int argc, char *argv[]);
int Nliteral_from_logical_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
			      char *argv[]);
int Nlogical_from_literal_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
			      char *argv[]);

/*
 * Nliteral_from_logical_cmd --
 *
 * Map a logical name to its literal map object.
 */
int Nliteral_from_logical_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
			      char *argv[])
{
    int *surf_list, *vect_list, *site_list, *vol_list, num_surfs, num_vects,
	num_sites, num_vols, i, s1_length;
    char err_string[200];
    Nv_clientData *cdata;

    /* Verify number of args */
    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nliteral_from_logical logical_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    s1_length = strlen(argv[1]);

    /* Scan through the list of all surfaces, sites and vectors looking for the given logical
       name.  If found, return the corresponding map object, otherwise return an error. */
    surf_list = GS_get_surf_list(&num_surfs);
    if (num_surfs) {
	for (i = 0; i < num_surfs; i++) {
	    cdata = (Nv_clientData *) GS_Get_ClientData(surf_list[i]);
	    if (cdata == NULL) {
		sprintf(err_string,
			"Internal Error: Can't find client data for surf map %d",
			surf_list[i]);
		Tcl_SetResult(interp, err_string, TCL_VOLATILE);
		return (TCL_ERROR);
	    }
	    if (!strncmp(cdata->logical_name, argv[1], s1_length)) {
		/* found it */
		sprintf(err_string, "Nsurf%d", surf_list[i]);
		Tcl_SetResult(interp, err_string, TCL_VOLATILE);
		return (TCL_OK);
	    }
	}
    }

    vect_list = GV_get_vect_list(&num_vects);
    if (num_vects) {
	for (i = 0; i < num_vects; i++) {
	    cdata = (Nv_clientData *) GV_Get_ClientData(vect_list[i]);
	    if (cdata == NULL) {
		sprintf(err_string,
			"Internal Error: Can't find client data for vect map %d",
			vect_list[i]);
		Tcl_SetResult(interp, err_string, TCL_VOLATILE);
		return (TCL_ERROR);
	    }
	    if (!strncmp(cdata->logical_name, argv[1], s1_length)) {
		/* found it */
		sprintf(err_string, "Nvect%d", vect_list[i]);
		Tcl_SetResult(interp, err_string, TCL_VOLATILE);
		return (TCL_OK);
	    }
	}
    }

    site_list = GP_get_site_list(&num_sites);
    if (num_sites) {
	for (i = 0; i < num_sites; i++) {
	    cdata = (Nv_clientData *) GP_Get_ClientData(site_list[i]);
	    if (cdata == NULL) {
		sprintf(err_string,
			"Internal Error: Can't find client data for site map %d",
			site_list[i]);
		Tcl_SetResult(interp, err_string, TCL_VOLATILE);
		return (TCL_ERROR);
	    }
	    if (!strncmp(cdata->logical_name, argv[1], s1_length)) {
		/* found it */
		sprintf(err_string, "Nsite%d", site_list[i]);
		Tcl_SetResult(interp, err_string, TCL_VOLATILE);
		return (TCL_OK);
	    }
	}
    }

    vol_list = GVL_get_vol_list(&num_vols);
    if (num_vols) {
	for (i = 0; i < num_vols; i++) {
	    cdata = (Nv_clientData *) GVL_Get_ClientData(vol_list[i]);
	    if (cdata == NULL) {
		sprintf(err_string,
			"Internal Error: Can't find client data for volume map %d",
			vect_list[i]);
		Tcl_SetResult(interp, err_string, TCL_VOLATILE);
		return (TCL_ERROR);
	    }
	    if (!strncmp(cdata->logical_name, argv[1], s1_length)) {
		/* found it */
		sprintf(err_string, "Nvol%d", vol_list[i]);
		Tcl_SetResult(interp, err_string, TCL_VOLATILE);
		return (TCL_OK);
	    }
	}
    }

    sprintf(err_string, "Error: can't find map object for logical name %s",
	    argv[1]);
    Tcl_SetResult(interp, err_string, TCL_VOLATILE);
    return (TCL_ERROR);
}

/*
 * Nlogical_from_literal_cmd --
 *
 * Map a literal name of the form N<map type><map id> to a logical name.
 */
int Nlogical_from_literal_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
			      char *argv[])
{

    /* Check correct syntax */
    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nlogical_from_literal <map_obj>",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Otherwise return the name */
    return Tcl_VarEval(interp, argv[1], " get_logical_name", NULL);
}

/*
 * set_logical_name --
 *
 *  Set the logical name of the given map object
 */
int set_logical_name(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    Nv_clientData *data = NULL;
    char err_string[80];

    /* Check correct syntax */
    if (argc != 3) {
	Tcl_SetResult(interp, "Usage: <map> set_logical_name <string>",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Try to get the client data structure for the current map */
    switch (get_type(argv[0])) {
    case SURF:
	data = (Nv_clientData *) GS_Get_ClientData(id);
	break;
    case SITE:
	data = (Nv_clientData *) GP_Get_ClientData(id);
	break;
    case VECT:
	data = (Nv_clientData *) GV_Get_ClientData(id);
	break;
    case VOL:
	data = (Nv_clientData *) GVL_Get_ClientData(id);
	break;
    }

    if (data == NULL) {
	sprintf(err_string,
		"Internal Error: Can't find client data for map %d", id);
	Tcl_SetResult(interp, err_string, TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Free any olf space for logical name */
    if (data->logical_name != NULL)
	G_free(data->logical_name);

    /* Allocate new space and copy name */
    data->logical_name =
	(char *)G_malloc(sizeof(char) * (strlen(argv[2]) + 1));
    if (data->logical_name == NULL) {
	Tcl_SetResult(interp,
		      "Error: out of memory in set_logical_name - allocating new space",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    strcpy(data->logical_name, argv[2]);

    /* Clean exit */
    return (TCL_OK);
}

/*
 * get_logical_name --
 *
 *  Get the logical name of the given map object
 */
int get_logical_name(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    Nv_clientData *data = NULL;
    char err_string[80];

    /* Check for correct syntax */
    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: <map> get_logical_name", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Try to get the client data for this map */
    switch (get_type(argv[0])) {
    case SURF:
	data = (Nv_clientData *) GS_Get_ClientData(id);
	break;
    case SITE:
	data = (Nv_clientData *) GP_Get_ClientData(id);
	break;
    case VECT:
	data = (Nv_clientData *) GV_Get_ClientData(id);
	break;
    case VOL:
	data = (Nv_clientData *) GVL_Get_ClientData(id);
	break;
    }

    if (data == NULL) {
	sprintf(err_string,
		"Internal Error: Can't find client data for map %d", id);
	Tcl_SetResult(interp, err_string, TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Now copy the name to the return value */
    Tcl_SetResult(interp, data->logical_name, TCL_VOLATILE);

    /* Clean exit */
    return (TCL_OK);
}

/*
 * Nget_surf_list_cmd --
 *
 *   Return a list of id numbers of all surfaces currently in the system.
 */
int Nget_surf_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		       char *argv[])
{
    int *surf_list, num_surfs, i;
    char temp[100];

    surf_list = GS_get_surf_list(&num_surfs);

    if (num_surfs) {
	for (i = 0; i < num_surfs; i++) {
	    sprintf(temp, "%d", surf_list[i]);
	    Tcl_AppendElement(interp, temp);
	}

	G_free(surf_list);
    }

    return (TCL_OK);
}

/*
 * Nget_vect_list_cmd --
 *
 *   Return a list of id numbers of all vector maps currently in the system.
 */
int Nget_vect_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		       char *argv[])
{
    int *vect_list, num_vects, i;
    char temp[100];

    vect_list = GV_get_vect_list(&num_vects);

    if (num_vects) {
	for (i = 0; i < num_vects; i++) {
	    sprintf(temp, "%d", vect_list[i]);
	    Tcl_AppendElement(interp, temp);
	}

	G_free(vect_list);
    }

    return (TCL_OK);
}

/*
 * Nget_site_list_cmd --
 *
 *   Return a list of id numbers of all site maps currently in the system.
 */
int Nget_site_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		       char *argv[])
{
    int *site_list, num_sites, i;
    char temp[100];

    site_list = GP_get_site_list(&num_sites);

    if (num_sites) {
	for (i = 0; i < num_sites; i++) {
	    sprintf(temp, "%d", site_list[i]);
	    Tcl_AppendElement(interp, temp);
	}

	G_free(site_list);
    }

    return (TCL_OK);
}

/*
 * Nget_vol_list_cmd --
 *
 *   Return a list of id numbers of all volume maps currently in the system.
 */
int Nget_vol_list_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		      char *argv[])
{
    int *vol_list, num_vols, i;
    char temp[100];

    vol_list = GVL_get_vol_list(&num_vols);

    if (num_vols) {
	for (i = 0; i < num_vols; i++) {
	    sprintf(temp, "%d", vol_list[i]);
	    Tcl_AppendElement(interp, temp);
	}

	G_free(vol_list);
    }

    return (TCL_OK);
}

/*
 * Nnew_map_obj_cmd --
 *   Create a new map object which can be one of
 *   surf, vect, vol or site.  This routine creates the object
 *   internally in the gsf library and links a new tcl/tk
 *   command to the general object command parser below.
 *   Optionally, a logical name may be specified for the new map
 *   object.  If no name is specified, a logical name is assigned to
 *   the new object automatically.  Note that maintaining unique
 *   logical names is not the responsibility of the library (currently).
 *
 *   Initially map objects contain no data, use the
 *   attribute commands to set attributes such as topology,
 *   color, etc.
 */
int Nnew_map_obj_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		     char *argv[])
{
    char id[128], *arglist[5], *log_name = NULL;
    char topo_string[] = "topo";
    char const_string[] = "constant";
    char zero_string[] = "0";
    int new_id;
    int rows, cols, depths, max;
    int *surf_list, num_surfs, i;
    Nv_clientData *new_data;
    int file_used = 0;

    /* Check for correct syntax */
    if (argc < 2) {
	Tcl_SetResult(interp,
		      "Usage: Nnew_map_obj <type> {default file} {name=logical_name}",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /*
     * For each type of map obj do the following --
     *   1) Verify we havn't maxed out the number of
     *      allowed objects.
     *   2) Call the internal library to generate a new
     *      map object of the specified type.
     *   3) Create a new tcl/tk command with the new object
     *      id number and link it to the Nmap_obj_cmd routine
     *      below.
     */
    if (!strcmp(argv[1], "surf")) {
	if (GS_num_surfs() >= MAX_SURFS) {
	    Tcl_SetResult(interp, "Maximum surfaces loaded!", TCL_VOLATILE);
	    return (TCL_ERROR);
	}
	new_id = GS_new_surface();

	/* See if there is a default file name specified */
	arglist[2] = topo_string;
	if ((argc >= 3) && (strncmp(argv[2], "name=", 5))) {
	    arglist[3] = argv[2];
	    if (set_att(new_id, SURF, data, interp, 3, arglist) != TCL_OK) {
		GS_delete_surface(new_id);
		return (TCL_ERROR);
	    }
	    file_used = 1;
	}
	else {
	    arglist[3] = const_string;
	    arglist[4] = zero_string;
	    set_att(new_id, SURF, data, interp, 4, arglist);
	}

	sprintf(id, "Nsurf%d", new_id);

    }

    /***** SITES *****/
    else if (!strcmp(argv[1], "site")) {
	if (GP_num_sites() >= MAX_SITES) {
	    Tcl_SetResult(interp, "Maximum sites loaded!", TCL_VOLATILE);
	    return (TCL_ERROR);
	}
	new_id = GP_new_site();

	/* See if there is a default file name specified */
	if ((argc >= 3) && (strncmp(argv[2], "name=", 5))) {
	    arglist[2] = argv[2];
	    if (load_obj(new_id, SITE, data, 3, arglist, interp) == TCL_ERROR) {
		GP_delete_site(new_id);
		Tcl_SetResult(interp, "Error loading vector points",
			      TCL_VOLATILE);
		return (TCL_ERROR);

	    }
	    file_used = 1;
	}

	/* Initialize display parameters */
	GP_set_style(new_id, 0xFF0000, 2, 100, ST_X);
	surf_list = GS_get_surf_list(&num_surfs);
	if (num_surfs) {
	    for (i = 0; i < num_surfs; i++) {
		GP_select_surf(new_id, surf_list[i]);
	    }
	}
	G_free(surf_list);

	sprintf(id, "Nsite%d", new_id);

    }

    /********* VECTOR *******/
    else if (!strcmp(argv[1], "vect")) {
	if (GV_num_vects() >= MAX_VECTS) {
	    Tcl_SetResult(interp, "Maximum vectors loaded!", TCL_VOLATILE);
	    return (TCL_ERROR);
	}
	new_id = GV_new_vector();

	/* See if there is a default file name specified */
	if ((argc >= 3) && (strncmp(argv[2], "name=", 5))) {
	    arglist[2] = argv[2];
	    /*      load_obj(new_id, VECT, data, 3, arglist, interp); */
	    if (load_obj(new_id, VECT, data, 3, arglist, interp) == TCL_ERROR) {
		GV_delete_vector(new_id);
		Tcl_SetResult(interp, "Error loading vector", TCL_VOLATILE);
		return (TCL_ERROR);
	    }
	    file_used = 1;
	}

	/* Initialize display parameters */
	/* automatically select all surfaces to draw vector */
	GV_set_style(new_id, 1, 0xFF0000, 2, 0);
	surf_list = GS_get_surf_list(&num_surfs);
	if (num_surfs) {
	    for (i = 0; i < num_surfs; i++) {
		GV_select_surf(new_id, surf_list[i]);
	    }
	}
	G_free(surf_list);

	sprintf(id, "Nvect%d", new_id);

    }

    /****** VOLUME *****/
    else if (!strcmp(argv[1], "vol")) {
	if (GVL_num_vols() >= MAX_VOLS) {
	    Tcl_SetResult(interp, "Maximum volumes loaded!", TCL_VOLATILE);
	    return (TCL_ERROR);
	}
	new_id = GVL_new_vol();

	/* See if there is a default file name specified */
	if ((argc >= 3) && (strncmp(argv[2], "name=", 5))) {
	    arglist[2] = argv[2];
	    if (load_obj(new_id, VOL, data, 3, arglist, interp) == TCL_ERROR) {
		GVL_delete_vol(new_id);
		Tcl_SetResult(interp, "Error loading volume", TCL_VOLATILE);
		return (TCL_ERROR);
	    }
	    file_used = 1;
	}

	/** Initilaze defaults for GUI **/

	/** should proably be called seperately with set_att **/
	GVL_get_dims(new_id, &rows, &cols, &depths);
	max = (rows > cols) ? rows : cols;
	max = (depths > max) ? depths : max;
	max = max / 35;
	if (max < 1)
	    max = 1;

	if (max > cols)
	    max = cols / 2;
	if (max > rows)
	    max = rows / 2;
	if (max > depths)
	    max = depths / 2;

	/* set default drawres and drawmode for isosurfaces */
	GVL_isosurf_set_drawres(new_id, max, max, max);
	GVL_isosurf_set_drawmode(new_id, DM_GOURAUD);

	/* set default drawres and drawmode for slices */
	GVL_slice_set_drawres(new_id, 1., 1., 1.);
	GVL_slice_set_drawmode(new_id, DM_GOURAUD | DM_POLY);

	sprintf(id, "Nvol%d", new_id);

    }
    else {
	Tcl_SetResult(interp,
		      "Error: type must be one of surf, site, vect or vol",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Initialize the client data filed for the new map object */
    new_data = (Nv_clientData *) G_malloc(sizeof(Nv_clientData));
    if (new_data == NULL) {
	Tcl_SetResult(interp,
		      "Error: out of memory in Nnew_map_object - creating client data",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (argc >= 3) {
	if (file_used & (argc == 4)) {
	    log_name = argv[3];
	}
	else if (!file_used) {
	    log_name = argv[2];
	}

	if (log_name != NULL) {
	    if (strncmp(log_name, "name=", 5)) {
		Tcl_SetResult(interp,
			      "Error: logical name syntax should be name=<logical name>",
			      TCL_VOLATILE);
		return (TCL_ERROR);
	    }

	    log_name += 5;
	    new_data->logical_name =
		(char *)G_malloc(sizeof(char) * (strlen(log_name) + 1));
	    if (new_data->logical_name == NULL) {
		Tcl_SetResult(interp,
			      "Error: out of memory in Nnew_map_object - copying logical name",
			      TCL_VOLATILE);
		return (TCL_ERROR);
	    }

	    strcpy(new_data->logical_name, log_name);
	}

    }

    if (log_name == NULL) {
	char temp_space[80];
	time_t tp;

	/* Need to generate a random id */
	time(&tp);
	sprintf(temp_space, "%s*%ld", argv[1], tp);

	new_data->logical_name =
	    (char *)G_malloc(sizeof(char) * (strlen(temp_space) + 1));
	if (new_data->logical_name == NULL) {
	    Tcl_SetResult(interp,
			  "Error: out of memory in Nnew_map_object - creating logical name",
			  TCL_VOLATILE);
	    return (TCL_ERROR);
	}

	strcpy(new_data->logical_name, temp_space);
    }

    G_debug(3, "Logical name set to %s\n", new_data->logical_name);

    /* Set client data based on type of map object created */
    if (!strcmp(argv[1], "surf")) {
	GS_Set_ClientData(new_id, (void *)new_data);
    }
    else if (!strcmp(argv[1], "vect")) {
	GV_Set_ClientData(new_id, (void *)new_data);
    }
    else if (!strcmp(argv[1], "site")) {
	GP_Set_ClientData(new_id, (void *)new_data);
    }
    else if (!strcmp(argv[1], "vol")) {
	GVL_Set_ClientData(new_id, (void *)new_data);
    }

    Tcl_CreateCommand(interp, id, (Tcl_CmdProc *) Nmap_obj_cmd,
		      (ClientData *) data, (Tcl_CmdDeleteProc *) NULL);
    Tcl_SetResult(interp, id, TCL_VOLATILE);

    return (TCL_OK);
}

/*
 * Nmap_obj_cmd --
 *     Parse commands sent to a map object.  These commands
 *     are used to change attributes of map objects as well
 *     as return information necessary to form the Nviz
 *     user interface.  Number of arguments depends on the
 *     command.
 */
int Nmap_obj_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char *argv[])
{
    int id, type;

    /* Parse out id and map object type */
    id = get_idnum(argv[0]);
    type = get_type(argv[0]);

    if (!strcmp(argv[1], "draw"))
	return (draw_obj(interp, id, type));
    else if (!strcmp(argv[1], "delete"))
	return (delete_obj(id, type, data, interp));
    else if (!strcmp(argv[1], "set_drawmode"))
	return (set_drawmode(interp, type, id, data, argc, argv));
    else if (!strcmp(argv[1], "get_drawmode"))
	return (get_drawmode(type, id, data, interp));
    else if (!strcmp(argv[1], "get_res"))
	return (get_res(id, type, interp, argc, argv));
    else if (!strcmp(argv[1], "set_res"))
	return (set_res(interp, id, type, argc, argv));
    else if (!strcmp(argv[1], "set_wirecolor"))
	return (set_wirecolor(interp, id, type, data, argc, argv));
    else if (!strcmp(argv[1], "get_wirecolor"))
	return (get_wirecolor(id, type, data, interp));
    else if (!strcmp(argv[1], "set_nozero"))
	return (set_nozero(interp, id, type, data, argc, argv));
    else if (!strcmp(argv[1], "get_nozero"))
	return (get_nozero(id, type, data, interp, argc, argv));
    else if (!strcmp(argv[1], "get_trans"))
	return (get_trans(id, type, interp));
    else if (!strcmp(argv[1], "set_trans"))
	return (set_trans(interp, id, type, argc, argv));
    else if (!strcmp(argv[1], "select_surf"))
	return (select_surf(interp, id, type, argc, argv));
    else if (!strcmp(argv[1], "unselect_surf"))
	return (unselect_surf(id, type, argc, argv, interp));
    else if (!strcmp(argv[1], "surf_is_selected"))
	return (surf_is_selected(id, type, interp, argc, argv));
    else if (!strcmp(argv[1], "get_exag_guess"))
	return (get_exag_guess(id, type, interp));
    else if (!strcmp(argv[1], "set_exag"))
	return (set_exag_obj(id, type, argc, argv, interp));
    else if (!strcmp(argv[1], "load"))
	return (load_obj(id, type, data, argc, argv, interp));
    else if (!strcmp(argv[1], "get_att"))
	return (get_att(id, type, data, interp, argc, argv));
    else if (!strcmp(argv[1], "set_att"))
	return (set_att(id, type, data, interp, argc, argv));
    else if (!strcmp(argv[1], "unset_att"))
	return (unset_att(id, type, interp, argc, argv));
    else if (!strcmp(argv[1], "set_mask_mode"))
	return (set_mask_mode(id, type, data, interp, argc, argv));
    else if (!strcmp(argv[1], "get_mask_mode"))
	return (get_mask_mode(id, type, data, interp));
    else if (!strcmp(argv[1], "set_logical_name"))
	return (set_logical_name(id, interp, argc, argv));
    else if (!strcmp(argv[1], "get_logical_name"))
	return (get_logical_name(id, interp, argc, argv));
    else if (!strcmp(argv[1], "isosurf"))
	return (isosurf(id, type, interp, argc, argv));
    else if (!strcmp(argv[1], "slice"))
	return (slice(id, type, interp, argc, argv));
    else
	return (TCL_ERROR);
}

/*
 * set_drawmode --
 *     Syntax: <map_obj> set_drawmode SurfStyle ShadeStyle
 *     Set the draw mode of the given map object.
 *     Object must be a surface.
 */
int set_drawmode(Tcl_Interp * interp, int type, int id, Nv_data * data,
		 int argc, char *argv[])
{
    int mode;

    /* Make sure this command is being used with a surface */
    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use set_drawmode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Set the mode flags by parsing the surface style and shading arguments */
    mode = 0;

    if (!strcmp(argv[4], "gouraud"))
	mode |= DM_GOURAUD;
    else if (!strcmp(argv[4], "flat"))
	mode |= DM_FLAT;
    else {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> set_drawmode [ poly | wire_poly | wire | col_wire] [grid_wire | grid_surf] [ gouraud | flat ]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (!strcmp(argv[3], "grid_wire"))
	mode |= DM_GRID_WIRE;
    else if (!strcmp(argv[3], "grid_surf"))
	mode |= DM_GRID_SURF;
    else {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> set_drawmode [ poly | wire_poly | wire | col_wire] [grid_wire | grid_surf] [ gouraud | flat ]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (!strcmp(argv[2], "poly"))
	mode |= DM_POLY;
    else if (!strcmp(argv[2], "wire_poly"))
	mode |= DM_WIRE_POLY;
    else if (!strcmp(argv[2], "wire"))
	mode |= DM_WIRE;
    else if (!strcmp(argv[2], "col_wire"))
	mode |= DM_COL_WIRE;
    else {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> set_drawmode [ poly | wire_poly | wire | col_wire] [grid_wire | grid_surf] [ gouraud | flat ]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Set the appropriate mode in the gsf library */
    GS_set_drawmode(id, mode);

    return (TCL_OK);
}

/*
 * get_drawmode --
 *     Syntax: <map_obj> get_drawmode
 *     Get the surface style and rendering style of the given surface object
 */
int get_drawmode(int type, int id, Nv_data * data, Tcl_Interp * interp)
{
    int mode;
    char surf[32], wire[32], shade[32];
    char *list[4];

    /* Make sure we are using a surface */
    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use get_drawmode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (GS_get_drawmode(id, &mode) == -1) {
	Tcl_SetResult(interp, "Error: id in get_drawmode is invalid.",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Parse mode returned for shade style and surface style */
    G_debug(3, "drawmode: %d", mode);
    if (mode & DM_GOURAUD || mode == 2308)	/* DM_GRID_SURF|DM_GOURAUD|DM_POLY */
	strcpy(shade, "gouraud");
    else if (mode & DM_FLAT)
	strcpy(shade, "flat");
    else {
	Tcl_SetResult(interp,
		      "Internal Error map_obj.c: unknown shade style returned in get_drawmode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (mode & DM_POLY)
	strcpy(surf, "poly");
    else if (mode & DM_WIRE)
	strcpy(surf, "wire");
    else if (mode & DM_WIRE_POLY)
	strcpy(surf, "wire_poly");
    else if (mode & DM_COL_WIRE)
	strcpy(surf, "col_wire");
    else {
	Tcl_SetResult(interp,
		      "Internal Error: unknown surface style returned in get_drawmode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (mode & DM_GRID_WIRE)
	strcpy(wire, "grid_wire");
    else if (mode & DM_GRID_SURF)
	strcpy(wire, "grid_surf");
    else {
	Tcl_SetResult(interp,
		      "Internal Error: unknown surface style returned in get_drawmode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    list[0] = shade;
    list[1] = surf;
    list[2] = wire;
    list[3] = NULL;

    Tcl_SetResult(interp, Tcl_Merge(3, list), TCL_DYNAMIC);
    return (TCL_OK);

}

/*
 * draw_obj --
 *     Syntax: <map_obj> draw_obj
 *     Pretty simple, based on the type of object, call the appropriate
 *     gsf library draw routine.
 */
int draw_obj(Tcl_Interp * interp, int id, int type)
{
    switch (type) {
    case SURF:
	GS_draw_surf(id);
	break;
    case SITE:
	GP_draw_site(id);
	break;
    case VECT:
	GV_draw_vect(id);
	break;
    case VOL:
	GVL_draw_vol(id);
	break;
    default:
	Tcl_SetResult(interp,
		      "Internal Error: map_obj type unknown in draw_mode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
	break;
    }

    return (TCL_OK);
}

/*
 * delete_obj --
 *     Syntax: <map_obj> delete
 *     Tell a map object to delete itself.
 */
int delete_obj(int id, int type, Nv_data * data, Tcl_Interp * interp)
{
    switch (type) {
    case SURF:
	GS_delete_surface(id);
	break;
    case SITE:
	GP_delete_site(id);
	break;
    case VECT:
	GV_delete_vector(id);
	break;
    case VOL:
	GVL_delete_vol(id);
	break;
    default:
	Tcl_SetResult(interp,
		      "Internal Error: unknown map object type in delete",
		      TCL_VOLATILE);
	return (TCL_ERROR);
	break;
    }
    return (TCL_OK);

}

/*
 * get_res --
 *     Syntax: <map_obj> get_res wire | poly | both
 *     Get the wire draw and/or polygon resolution
 *     of the given map object (must be a surface.
 */
int get_res(int id, int type, Tcl_Interp * interp, int argc, char *argv[])
{
    int n;
    int xres, yres, xwire, ywire;
    char x[32], y[32], xw[32], yw[32];
    char *list[5];

    /* Verify that this map object is a surface */
    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use get_res",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    GS_get_drawres(id, &xres, &yres, &xwire, &ywire);
    sprintf(x, "%d", xres);
    sprintf(y, "%d", yres);
    sprintf(xw, "%d", xwire);
    sprintf(yw, "%d", ywire);

    if (!strcmp(argv[2], "wire")) {
	list[0] = xw;
	list[1] = yw;
	list[2] = NULL;
	n = 2;
    }
    else if (!strcmp(argv[2], "poly")) {
	list[0] = x;
	list[1] = y;
	list[2] = NULL;
	n = 2;
    }
    else if (!strcmp(argv[2], "both")) {
	list[0] = x;
	list[1] = y;
	list[2] = xw;
	list[3] = yw;
	list[4] = NULL;
	n = 4;
    }
    else {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> get_res [ wire | poly | both]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    Tcl_SetResult(interp, Tcl_Merge(n, list), TCL_DYNAMIC);

    return (TCL_OK);
}

/*
 * set_res --
 *     Syntax: <map_obj> set_res wire | poly | both
 *     Set the wire and/or polygon rendering resolutions for a given surface
 */
int set_res(Tcl_Interp * interp, int id, int type, int argc, char *argv[])
{
    int xres, yres, xwire, ywire;

    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use set_res",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (argc < 5) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> set_res [ [wire xwire ywire] |\n\t\t[poly xres yres] |\n\t\t[both xres yres xwire ywire] ]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    GS_get_drawres(id, &xres, &yres, &xwire, &ywire);
    if (!strcmp(argv[2], "poly")) {
	xres = atoi(argv[3]);
	yres = atoi(argv[4]);
    }
    else if (!strcmp(argv[2], "wire")) {
	xwire = atoi(argv[3]);
	ywire = atoi(argv[4]);
    }
    else if (!strcmp(argv[2], "both") && argc == 7) {
	xres = atoi(argv[3]);
	yres = atoi(argv[4]);
	xwire = atoi(argv[5]);
	ywire = atoi(argv[6]);
    }
    else {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> set_res [ [wire xwire ywire] |\n\t\t[poly xres yres] |\n\t\t[both xres yres xwire ywire] ]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    GS_set_drawres(id, xres, yres, xwire, ywire);

    return (TCL_OK);
}

/*
 * set_wirecolor --
 *     Syntax: <map_obj> set_wirecolor [color | "UseMap"]
 *     Set the wire color for the current surface object
 */
int set_wirecolor(Tcl_Interp * interp, int id, int type,
		  Nv_data * data, int argc, char *argv[])
{
    int col;

    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use set_wirecolor",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (argc < 3) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> set_wirecolor [color / \"UseMap\"]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (!strncmp(argv[2], "UseMap", 6))
	col = WC_COLOR_ATT;
    else
	col = tcl_color_to_int(argv[2]);

    GS_set_wire_color(id, col);

    return (TCL_OK);
}

/*
 * get_wirecolor --
 *     Syntax: <map_obj> get_wirecolor
 *     Returns the wire color for the surface map object.
 *     Returns the string "UseMap" if the surface color map is being used.
 */
int get_wirecolor(int id, int type, Nv_data * data, Tcl_Interp * interp)
{
    int colr;
    char *col;
    char err[255];

    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use get_wirecolor",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (GS_get_wire_color(id, &colr) == -1) {
	sprintf(err, "Error: id (%d) in get_wirecolor is invalid", id);
	Tcl_SetResult(interp, err, TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (colr == WC_COLOR_ATT) {
	Tcl_SetResult(interp, "UseMap", TCL_VOLATILE);
    }
    else {
	col = int_to_tcl_color(colr);
	Tcl_SetResult(interp, col, TCL_VOLATILE);
    }

    return (TCL_OK);
}

/*
 * get_trans --
 *     Syntax: <map_obj> get_trans
 *     Get the x, y and z coordinates of the given map object.
 */
int get_trans(int id, int type, Tcl_Interp * interp)
{
    float x, y, z;
    char xtrans[32], ytrans[32], ztrans[32];
    char *list[4];

    switch (type) {
    case SURF:
	GS_get_trans(id, &x, &y, &z);
	break;
    case VECT:
	GV_get_trans(id, &x, &y, &z);
	break;
    case SITE:
	GP_get_trans(id, &x, &y, &z);
	break;
    case VOL:
	GVL_get_trans(id, &x, &y, &z);
	break;
    default:
	Tcl_SetResult(interp,
		      "Internal Error: unknown map object type in get_trans",
		      TCL_VOLATILE);
	return (TCL_ERROR);
	break;
    }
    sprintf(xtrans, "%f", x);
    sprintf(ytrans, "%f", y);
    sprintf(ztrans, "%f", z);
    list[0] = xtrans;
    list[1] = ytrans;
    list[2] = ztrans;
    list[3] = NULL;

    Tcl_SetResult(interp, Tcl_Merge(3, list), TCL_DYNAMIC);

    return (TCL_OK);
}

/*
 * set_trans --
 *     Syntax: <map_obj> set_trans x y z
 *     Set the x, y and z coordinates of the given map object
 */
int set_trans(Tcl_Interp * interp, int id, int type, int argc, char *argv[])
{
    float x, y, z;

    x = (float)atof(argv[2]);
    y = (float)atof(argv[3]);
    z = (float)atof(argv[4]);

    switch (type) {
    case SURF:
	GS_set_trans(id, x, y, z);
	break;
    case VECT:
	GV_set_trans(id, x, y, z);
	break;
    case SITE:
	GP_set_trans(id, x, y, z);
	break;
    case VOL:
	GVL_set_trans(id, x, y, z);
	break;
    default:
	Tcl_SetResult(interp,
		      "Internal Error: unknown map object type in set_trans",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }
    return (TCL_OK);
}

/*
 * set_nozero --
 *     Syntax: <map_obj> set_nozero [topo | color] mode
 *     Set to ignore zeros for either the topo or color map
 */
int set_nozero(Tcl_Interp * interp, int id, int type, Nv_data * data,
	       int argc, char *argv[])
{
    int att, mode;

    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use set_nozero",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (argc < 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> set_nozero [topo | color] [0 | 1]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    mode = atoi(argv[3]);
    if (!strcmp(argv[2], "topo")) {
	att = ATT_TOPO;
    }
    else if (!strcmp(argv[2], "color")) {
	att = ATT_COLOR;
    }
    else {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> set_nozero [topo | color] [0 | 1]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    GS_set_nozero(id, att, mode);

    return (TCL_OK);
}

/*
 * get_nozero --
 *     Syntax: <map_obj> get_nozero [topo | color]
 *     Get whether or not we are using zeros for the given surface map object
 */
int get_nozero(int id, int type, Nv_data * data, Tcl_Interp * interp,
	       int argc, char *argv[])
{
    int mode;
    char ret[32];

    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use get_nozero",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (argc < 3) {
	Tcl_SetResult(interp, "Usage: <map_obj> get_nozero [topo | color]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (!strcmp(argv[2], "topo")) {
	GS_get_nozero(id, ATT_TOPO, &mode);
    }
    else if (!strcmp(argv[2], "color")) {
	GS_get_nozero(id, ATT_COLOR, &mode);
    }
    else {
	Tcl_SetResult(interp, "Usage: <map_obj> get_nozero [topo | color]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    sprintf(ret, "%d", mode);
    Tcl_SetResult(interp, ret, TCL_VOLATILE);
    return (TCL_OK);
}

/*
 * select_surf --
 *     Syntax: <map_obj> select_surf <map_obj>
 *     Select the given surface as the drape surface for
 *     either a site or vector map.  The surface to drape over
 *     is passed as the second argument.
 */
int select_surf(Tcl_Interp * interp, int id, int type, int argc, char *argv[])
{
    int surfid;
    int ret = 1;

    surfid = get_idnum(argv[2]);
    if (get_type(argv[2]) != SURF) {
	Tcl_SetResult(interp,
		      "Error: argument to select_surf must be a surface map object",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (type == SITE) {
	ret = GP_select_surf(id, surfid);
    }
    else if (type == VECT) {
	ret = GV_select_surf(id, surfid);
    }
    else {
	Tcl_SetResult(interp,
		      "Error: expected vect or site map_obj in select_surf",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return ((0 > ret) ? TCL_ERROR : TCL_OK);
}

/*
 * unselect_surf --
 *     Syntax: <map_obj> unselect_surf <map_obj>
 *     Unselect the given surface as the drape surface for
 *     either a site or vector map.  The surface to undrape over
 *     is passed as the second argument.
 */
int unselect_surf(int id, int type, int argc, char *argv[],
		  Tcl_Interp * interp)
{
    int surfid;
    int ret = 1;

    surfid = get_idnum(argv[2]);
    if (get_type(argv[2]) != SURF) {
	Tcl_SetResult(interp,
		      "Error: argument to unselect_surf must be a surface map object",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (type == SITE) {
	ret = GP_unselect_surf(id, surfid);
    }
    else if (type == VECT) {
	ret = GV_unselect_surf(id, surfid);
    }
    else {
	Tcl_SetResult(interp,
		      "Error: expected vect or site map_obj in unselect_surf",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return ((0 > ret) ? TCL_ERROR : TCL_OK);
}

/*
 * surf_is_selected --
 *     Syntax: <map_obj> surf_is_selected <map_obj>
 *     Return whether or not the surface argument is selected
 *     as a drape surface for a vect or site file.
 */
int surf_is_selected(int id, int type, Tcl_Interp * interp, int argc,
		     char *argv[])
{
    int surfid;
    int ret = 1;
    char selected[128];

    surfid = get_idnum(argv[2]);
    if (get_type(argv[2]) != SURF) {
	Tcl_SetResult(interp,
		      "Error: argument to surf_is_selected must be a surface map object",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (type == SITE) {
	ret = GP_surf_is_selected(id, surfid);
    }
    else if (type == VECT) {
	ret = GV_surf_is_selected(id, surfid);
    }
    else {
	Tcl_SetResult(interp,
		      "Error: expected vect or site map_obj in surf_is_selected",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    sprintf(selected, "%d", ret);
    Tcl_SetResult(interp, selected, TCL_VOLATILE);

    return ((0 > ret) ? TCL_ERROR : TCL_OK);
}

/*
 * set_exag_obj --
 *     Syntax: <map_obj> set_exag value
 *     Set the exaggeration value for the given surface.
 *     Note that the map object must be a surface or volume
 */
int set_exag_obj(int id, int type, int argc, char *argv[],
		 Tcl_Interp * interp)
{
    float exag;
    double atof();

    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use set_exag",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (argc < 3) {
	Tcl_SetResult(interp, "Usage: <map_obj> set_exag value",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    exag = (float)atof(argv[2]);
    GS_set_exag(id, exag);

    return (TCL_OK);
}

/*
 * get_exag_guess --
 *     Syntax: <map_obj> get_exag_guess
 *     Get the gsf library exaggeration guess for the given surface
 */
int get_exag_guess(int id, int type, Tcl_Interp * interp)
{
    char exag[256];
    float guess;

    if (type != SURF) {
	Tcl_SetResult(interp,
		      "Error: map object must be a surface in order to use get_exag_guess",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (0 > GS_get_exag_guess(id, &guess))
	return (TCL_ERROR);
    sprintf(exag, "%f", guess);

    Tcl_SetResult(interp, exag, TCL_VOLATILE);

    return (TCL_OK);
}

/*
 * load_obj --
 *     Syntax: <map_obj> load file_name
 *     General load function used to load either a vector, site or
 *     volumefile.  The type loaded depends on the
 *     type of the map object.
 */
int load_obj(int id, int type, Nv_data * data, int argc, char *argv[],
	     Tcl_Interp * interp)
{
    switch (type) {
    case SITE:
	if (0 > GP_load_site(id, argv[2])) {
	    GP_delete_site(id);
	    return (TCL_ERROR);
	}
	else {
	    return (TCL_OK);
	}
	break;
    case VECT:
	if (0 > GV_load_vector(id, argv[2])) {
	    GV_delete_vector(id);
	    return (TCL_ERROR);
	}
	else {
	    return (TCL_OK);
	}
    case VOL:
	if (0 > GVL_load_vol(id, argv[2])) {
	    GVL_delete_vol(id);
	    return (TCL_ERROR);
	}
	break;
    default:
	Tcl_SetResult(interp,
		      "Error: Map object must be of type vector or site in load_obj",
		      TCL_VOLATILE);
	return (TCL_ERROR);
	break;
    }

    return (TCL_OK);
}

/*
 * get_att --
 *      for SURF Syntax: <map_obj> get_att [ topo | color | mask | transp | shin | emi ]
 *      for VECT Syntax: <map_obj> get_att [ color | width | map]
 *      for SITE Syntax: <map_obj> get_att [ color | width | marker | size | useatt | display | map]
 *      for VOL Syntax: <map_obj> get_att [ map ]
 *      Return the current value of the given attribute
 *      This routine has changes significantly.  The syntax now depends on
 *      what type of map_obj we are calling.  Different commands are offered for
 *      different types of objects.  This was done so that querying map object types
 *      appears uniform in the Tcl/Tk script code.
 *
 *      For surfaces, returns a two element list, the first element
 *      indicating the type of data value (i.e. map, const, or unset) and the
 *      second element indicating the actual value.
 */
int get_att(int id, int type, Nv_data * data, Tcl_Interp * interp, int argc,
	    char *argv[])
{

    /* Switch on the type of map object */
    switch (type) {
    case SURF:{
	    int set;
	    float c;
	    char mapname[100], temp[100];

	    GS_get_att(id, att_atoi(argv[2]), &set, &c, mapname);

	    switch (set) {
	    case NOTSET_ATT:
		Tcl_AppendElement(interp, "unset");
		break;
	    case MAP_ATT:
		Tcl_AppendElement(interp, "map");
		Tcl_AppendElement(interp, mapname);
		break;
	    case CONST_ATT:
		Tcl_AppendElement(interp, "const");
		sprintf(temp, "%f", c);
		Tcl_AppendElement(interp, temp);
		break;
	    case FUNC_ATT:
		/* not implemented yet */
		break;
	    }
	}
	break;

    case VECT:{
	    int mem, color, width, flat;
	    char temp[128], *tempname;

	    GV_get_style(id, &mem, &color, &width, &flat);
	    switch (sv_att_atoi(argv[2])) {
	    case ATT_COLOR:
		Tcl_SetResult(interp, int_to_tcl_color(color), TCL_VOLATILE);
		break;
	    case SV_ATT_WIDTH:
		sprintf(temp, "%d", width);
		Tcl_SetResult(interp, temp, TCL_VOLATILE);
		break;
	    case SV_ATT_FLAT:
		sprintf(temp, "%d", flat);
		Tcl_SetResult(interp, temp, TCL_VOLATILE);
		break;
	    case SV_ATT_MAP:
		GV_get_vectname(id, &tempname);
		Tcl_SetResult(interp, tempname, TCL_VOLATILE);
		break;
	    }
	}
	break;

    case SITE:{
	    int atmod, color, width, marker, use_z;
	    float size;
	    char temp[128], *tempname;

	    GP_get_style(id, &color, &width, &size, &marker);
	    GP_get_zmode(id, &use_z);
	    switch (sv_att_atoi(argv[2])) {
	    case ATT_COLOR:
		Tcl_SetResult(interp, int_to_tcl_color(color), TCL_VOLATILE);
		break;
	    case SV_ATT_WIDTH:
		sprintf(temp, "%d", width);
		Tcl_SetResult(interp, temp, TCL_VOLATILE);
		break;
	    case SV_ATT_MARKER:
		get_char_marker(marker, temp);
		Tcl_SetResult(interp, temp, TCL_VOLATILE);
		break;
	    case SV_ATT_SIZE:
		sprintf(temp, "%f", size);
		Tcl_SetResult(interp, temp, TCL_VOLATILE);
		break;
	    case SV_ATT_USEATT:

/*** ACS_MODIFY BEGIN - site_attr management ***********************************/
		/* BEGIN original code
		   switch (atmod) {
		   case ST_ATT_SIZE:
		   sprintf(temp, "z");
		   break;
		   case ST_ATT_COLOR:
		   sprintf(temp, "color");
		   break;
		   case ST_ATT_NONE:
		   sprintf(temp, "none");
		   }
		   Tcl_SetResult(interp, temp, TCL_VOLATILE);
		   END original code */
		{
			    return (TCL_ERROR);
		    /* result should be in interp */
		}

/*** ACS_MODIFY END - site_attr management *************************************/

		break;
	    case SV_ATT_DISPLAY:
		if (use_z == 1)
		    sprintf(temp, "3d");
		else
		    sprintf(temp, "surfdisp");
		Tcl_SetResult(interp, temp, TCL_VOLATILE);

		break;
	    case SV_ATT_MAP:
		GP_get_sitename(id, &tempname);
		Tcl_SetResult(interp, tempname, TCL_VOLATILE);
		break;
	    }
	}
	break;

    case VOL:{
	    char temp[128];

	    switch (sv_att_atoi(argv[2])) {
	    case SV_ATT_MAP:
		GVL_get_volname(id, temp);
		Tcl_SetResult(interp, temp, TCL_VOLATILE);
		break;
	    }
	}
	break;

    default:
	Tcl_SetResult(interp,
		      "Internal Error: unknown map_obj type in get_att",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/*
 * set_att -
 *     for SURF Syntax: <map_obj> set_att [topo | color | mask | transp | shin | emi]
 *                  [file_name | constant value]
 *     for VECT Syntax: <map_obj> set_att [ color | width | map]
 *     for SITE Syntax: <map_obj> set_att [ color | width | marker | size | useatt | display | map]
 *     for VOL Syntax: <map_obj> set_att [ map ]
 *     Set the current value of the given attribute to the given file name.
 *     Causes the file to be loaded if it hasn't been already.
 */
int set_att(int id, int type, Nv_data * data, Tcl_Interp * interp, int argc,
	    char *argv[])
{
    int att, ret, width, marker, useatt, mem;
    int max2;
    float size;
    float temp;
    double atof();
    int col, flat;
    char errStr[255];

    /* Switch based on the type of map object we are using */
    switch (type) {
    case SURF:
	/* Decode the attribute we are setting */
	att = att_atoi(argv[2]);
	if (att < 0) {
	    sprintf(errStr,
		    "Internal Error: unknown attribute name '%s' in set_att",
		    argv[2]);
	    Tcl_SetResult(interp, errStr, TCL_VOLATILE);
	    return (TCL_ERROR);
	}

	/* Basically two cases, either we are setting to a constant field, or
	 * we are loading an actual file.  Setting a constant is the easy part
	 * so we try and do that first.
	 */
	if (!strcmp(argv[3], "constant")) {

	    /* Get the value for the constant
	     * Note that we require the constant to be an integer
	     */
	    temp = (float)atof(argv[4]);

	    /* Only special case is setting constant color.
	     * In this case we have to decode the constant Tcl
	     * returns so that the gsf library understands it.
	     */
	    if (att == ATT_COLOR) {
		/* TODO check this - sometimes gets reversed when save state
		   saves a surface with constant color */
		/* Tcl code and C code reverse RGB to BGR (sigh) */
		int r, g, b;

		r = (((int)temp) & 0xff0000) >> 16;
		g = (((int)temp) & 0x00ff00) >> 8;
		b = (((int)temp) & 0x0000ff);
		temp = r + (g << 8) + (b << 16);
	    }

	    /* Once the value is parsed, set it */
	    ret = GS_set_att_const(id, att, temp);

	}
	else {

	    G_debug(3, "Loading attribute map %s\n", argv[3]);
	    ret = GS_load_att_map(id, argv[3], att);
	    if (ret < 0) {
		return TCL_ERROR;
	    }
	}

	/* After we've loaded a constant map or a file,
	 * may need to adjust resolution if we are resetting
	 * topology (for example)
	 */
	if (0 <= ret) {

	    if (att == ATT_TOPO) {
		int rows, cols, max;

		/* If topology attribute is being set then need to set
		 * resolution of incoming map to some sensible value so we
		 * don't wait all day for drawing.
		 */
		GS_get_dims(id, &rows, &cols);
		max = (rows > cols) ? rows : cols;
		max = max / 50;
		if (max < 1)
		    max = 1;
		max2 = max / 5;
		if (max2 < 1)
		    max2 = 1;
		/* reset max to finer for coarse surf drawing */
		max = max2 + max2 / 2;
		if (max < 1)
		    max = 1;

		GS_set_drawres(id, max2, max2, max, max);
		GS_set_drawmode(id, DM_GOURAUD | DM_POLY | DM_GRID_SURF);
	    }

	    /* Not sure about this next line, should probably just
	     * create separate routines to figure the Z range as well
	     * as the XYrange
	     */
	    G_debug(3, "Calling update_ranges\n");
	    update_ranges(data);
	}
	break;

    case VECT:

	/* First check if the user is trying to load a map */
	if (!strncmp(argv[2], "map", 3)) {
	    char *args[3];

	    args[2] = argv[3];
	    load_obj(id, VECT, data, 2, args, interp);

	}
	else {

	    /* Get the old values of color and width and optionally change one of them */
	    GV_get_style(id, &mem, &col, &width, &flat);
	    col =
		(strncmp(argv[2], "color", 5)) ? col :
		tcl_color_to_int(argv[3]);
	    width = (strncmp(argv[2], "width", 5)) ? width : atoi(argv[3]);
	    flat = (strncmp(argv[2], "flat", 5)) ? flat : atoi(argv[3]);

	    GV_set_style(id, 1, col, width, flat);

	}
	break;

    case SITE:

	/* First check if the user is trying to load a map */
	if (!strncmp(argv[2], "map", 3)) {
	    char *args[3];

	    args[2] = argv[3];
	    load_obj(id, SITE, data, 2, args, interp);
	}
	else {
	    /* Reset display type? */
	    if (!strncmp(argv[2], "display", 7)) {
		if (!strncmp(argv[3], "3d", 2)) {
		    if (0 < GP_set_zmode(id, 1)) {
			Tcl_SetResult(interp, "1", TCL_VOLATILE);
		    }
		    else {
			Tcl_SetResult(interp, "0", TCL_VOLATILE);
		    }
		}
		else if (!strncmp(argv[3], "surfdisp", 8)) {
		    GP_set_zmode(id, 0);
		    Tcl_SetResult(interp, "1", TCL_VOLATILE);
		}
		else {
		    Tcl_SetResult(interp,
				  "Error in set_att display, must be 3d or surfdisp",
				  TCL_VOLATILE);
		    return (TCL_ERROR);
		}
	    }
	    GP_get_style(id, &col, &width, &size, &marker);
	    col =
		(strcmp(argv[2], "color")) ? col : tcl_color_to_int(argv[3]);
	    width = (strcmp(argv[2], "width")) ? width : atoi(argv[3]);
	    size = (strcmp(argv[2], "size")) ? size : (float)atof(argv[3]);
	    marker =
		(strcmp(argv[2], "marker")) ? marker :
		get_int_marker(argv[3]);

/*** ACS_MODIFY BEGIN - site_attr management ***********************************/
	    /* This let the points of the sites be related to associated (if any)
	       database fields */

	    /* Original code: this lacks of or-ing more than an att and we need to call
	       a function that prepares the parameters
	       useatt =
	       (strcmp(argv[2], "useatt")) ? useatt : (!strcmp(argv[3], "z"))
	       ? ST_ATT_SIZE : ((!strcmp(argv[3], "color")) ? ST_ATT_COLOR :
	       ST_ATT_NONE);
	     */


	    /*if (!strcmp(argv[2], "useatt")) {
		geosite *gp;

		if ((gp = gp_get_site(id))) {

		    if (TCL_OK !=
			site_attr_set(interp, gp, atoi(argv[3]), argv[4],
				      atoi(argv[5]), argv[6], argv[7]))
			return (TCL_ERROR);

		    if (!strcmp(argv[3], "size")) {
			useatt |= ST_ATT_SIZE;
		    }
		    else if (!strcmp(argv[3], "color")) {
			useatt |= ST_ATT_COLOR;
		    }
		    else if (!strcmp(argv[3], "marker")) {
			useatt |= ST_ATT_MARKER;
		    }
		}
	    }*/

/*** ACS_MODIFY END - site_attr management *************************************/

	    GP_set_style(id, col, width, size, marker);
	}
	break;

    case VOL:

	/* First check if the user is trying to load a map */
	if (!strncmp(argv[2], "map", 3)) {
	    char *args[3];
	    int rows, cols, depths, max;

	    args[2] = argv[3];
	    load_obj(id, VOL, data, 2, args, interp);

	    /* resolution adjust */
	    GVL_get_dims(id, &rows, &cols, &depths);
	    max = (rows > cols) ? rows : cols;
	    max = (depths > max) ? depths : max;
	    max = max / 35;
	    if (max < 1)
		max = 1;

	    if (max > cols)
		max = cols / 2;
	    if (max > rows)
		max = rows / 2;
	    if (max > depths)
		max = depths / 2;

	    /* set default drawres and drawmode for isosurfaces */
	    GVL_isosurf_set_drawres(id, max, max, max);
	    GVL_isosurf_set_drawmode(id, DM_GOURAUD);

	    /* set default drawres and drawmode for slices */
	    GVL_slice_set_drawres(id, 1., 1., 1.);
	    GVL_slice_set_drawmode(id, DM_GOURAUD | DM_POLY);
	}
	break;

    default:
	Tcl_SetResult(interp,
		      "Internal Error: unknown map_obj type in set_att",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }
    /* fprintf (stdout,"ret: %d\n", ret); */

    /*  return (0 > ret? TCL_ERROR : TCL_OK); */
    return (TCL_OK);		/*quick bug fix by jaro 08/08/99 */
}

/*
 * unset_att --
 *     Syntax: <map_obj> unset_att [topo | color | mask | transp | shin | emi]
 *     Set the specified attribute for the surface to "unset"
 */
int unset_att(int id, int type, Tcl_Interp * interp, int argc, char *argv[])
{
    int att;

/*** ACS_MODIFY 1.0 BEGIN - site_attr management *******************************/
    int col, width, marker, useatt;
    float size;
    geosite *gp;

    att = att_atoi(argv[2]);

    if (type == SITE) {
	if (!strcmp(argv[2], "useatt") && argc == 5) {
	    GP_get_style(id, &col, &width, &size, &marker);
	    if ((gp = gp_get_site(id))) {
/*		site_attr_unset(interp, gp, atoi(argv[3]), argv[4]);

		if (!strcmp(argv[3], "size")) {
		    useatt &= ~ST_ATT_SIZE;
		}
		else if (!strcmp(argv[3], "color")) {
		    useatt &= ~ST_ATT_COLOR;
		}
		else if (!strcmp(argv[3], "marker")) {
		    useatt &= ~ST_ATT_MARKER;
		}
*/	    }

	    GP_set_style(id, col, width, size, marker);
	}
	return TCL_OK;
    }

/*** ACS_MODIFY 1.0 END - site_attr management *********************************/

    if (type != SURF) {
	Tcl_SetResult(interp, "Type must be SURF for unset_att",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }
    return ((0 > GS_unset_att(id, att)) ? TCL_ERROR : TCL_OK);

}

/*
 * get_mask_mode --
 *     Syntax: <map_obj> get_mask_mode
 *     Return whether or not the mask raster map should be inverted
 */
int get_mask_mode(int id, int type, Nv_data * data, Tcl_Interp * interp)
{
    int mode;
    char tmp[10];

    if (type != SURF) {
	Tcl_SetResult(interp, "Type must be SURF for get_mask_mode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    GS_get_maskmode(id, &mode);
    sprintf(tmp, "%d", mode);
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, tmp);

    return (TCL_OK);
}

/*
 * set_mask_mode --
 *     Syntax: <map_obj> set_mask_mode [0 | 1]
 *     Set or unset the invert flag for the mask raster map
 *     of the given surface.
 */
int set_mask_mode(int id, int type, Nv_data * data, Tcl_Interp * interp,
		  int argc, char *argv[])
{
    int mode;

    if (type != SURF) {
	Tcl_SetResult(interp, "Type must be SURF for set_mask_mode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (Tcl_GetBoolean(interp, argv[2], &mode) != TCL_OK) {
	Tcl_SetResult(interp, "Error: must be surf_id set_mask_mode BOOLEAN",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }
    else {
	GS_set_maskmode(id, mode);
    }

    return (TCL_OK);
}

/*
 * get_idnum --
 *     Extract the GSF library id number out of
 *     the tcl/tk command name for a particular object.
 *     All map objects have a command name prefixed with either
 *     Nsurf, Nsite, Nvect, or Nvol depending on the type of map object.
 */
int get_idnum(char *name)
{
    int id;

    switch (get_type(name)) {
    case VOL:
	sscanf(name + 4, "%d", &id);
	break;
    default:
	sscanf(name + 5, "%d", &id);
    }

    return (id);
}

/*
 * get_type --
 *     Extract the type of object from the tcl/tk
 *     command name for an object.
 */
int get_type(char *name)
{
    if (!strncmp(name, "Nsurf", 5))
	return (SURF);
    else if (!strncmp(name, "Nsite", 5))
	return (SITE);
    else if (!strncmp(name, "Nvect", 5))
	return (VECT);
    else if (!strncmp(name, "Nvol", 4))
	return (VOL);
    else
	return (-1);
}

/*
 * att_atoi --
 *     Map textual attribute name to internal code.
 */
int att_atoi(char *attname)
{
    if (!strncmp(attname, "topo", 4))
	return (ATT_TOPO);
    else if (!strncmp(attname, "color", 5))
	return (ATT_COLOR);
    else if (!strncmp(attname, "mask", 4))
	return (ATT_MASK);
    else if (!strncmp(attname, "transp", 6))
	return (ATT_TRANSP);
    else if (!strncmp(attname, "shin", 4))
	return (ATT_SHINE);
    else if (!strncmp(attname, "emi", 3))
	return (ATT_EMIT);
    else
	return (-1);
}

/*
 * sv_att_atoi --
 *     Map textual site and vector attributes to internal code.
 */
int sv_att_atoi(char *attname)
{
    if (!strncmp(attname, "color", 5))
	return (ATT_COLOR);
    else if (!strncmp(attname, "width", 5))
	return (SV_ATT_WIDTH);
    else if (!strncmp(attname, "flat", 5))
	return (SV_ATT_FLAT);
    else if (!strncmp(attname, "marker", 6))
	return (SV_ATT_MARKER);
    else if (!strncmp(attname, "size", 4))
	return (SV_ATT_SIZE);
    else if (!strncmp(attname, "useatt", 6))
	return (SV_ATT_USEATT);
    else if (!strncmp(attname, "display", 7))
	return (SV_ATT_DISPLAY);
    else if (!strncmp(attname, "map", 3))
	return (SV_ATT_MAP);
    else
	return (-1);
}

/*
 * get_char_marker --
 *     Map textual marker name to internal code.
 */
int get_char_marker(int m, char *marker)
{
    switch (m) {
    case ST_X:
	sprintf(marker, "x");
	break;
    case ST_BOX:
	sprintf(marker, "box");
	break;
    case ST_SPHERE:
	sprintf(marker, "sphere");
	break;
    case ST_CUBE:
	sprintf(marker, "cube");
	break;
    case ST_DIAMOND:
	sprintf(marker, "diamond");
	break;
    case ST_DEC_TREE:
	sprintf(marker, "dec_tree");
	break;
    case ST_CON_TREE:
	sprintf(marker, "con_tree");
	break;
    case ST_ASTER:
	sprintf(marker, "aster");
	break;
    case ST_GYRO:
	sprintf(marker, "gyro");
	break;

/*** ACS_MODIFY BEGIN - site_attr management ***********************************/
    case ST_HISTOGRAM:
	sprintf(marker, "histogram");
	break;

/*** ACS_MODIFY END - site_attr management *************************************/
    default:
	/* This is the equivalent of returning a NULL to tcl */
	sprintf(marker, "");
	break;
    }

    return (TCL_OK);
}

/*
 * get_int_marker --
 *
 */
int get_int_marker(char *marker)
{
    G_debug(3, "marker = %s\n", marker);
    if (!strcmp(marker, "x")) {
	return (ST_X);
    }
    else if (!strcmp(marker, "box")) {
	return (ST_BOX);
    }
    else if (!strcmp(marker, "sphere")) {
	return (ST_SPHERE);
    }
    else if (!strcmp(marker, "cube")) {
	return (ST_CUBE);
    }
    else if (!strcmp(marker, "diamond")) {
	return (ST_DIAMOND);
    }
    else if (!strcmp(marker, "dec_tree")) {
	return (ST_DEC_TREE);
    }
    else if (!strcmp(marker, "con_tree")) {
	return (ST_CON_TREE);
    }
    else if (!strcmp(marker, "aster")) {
	return (ST_ASTER);
    }
    else if (!strcmp(marker, "gyro")) {
	return (ST_GYRO);
    }

/*** ACS_MODIFY BEGIN - site_attr management ***********************************/
    else if (!strcmp(marker, "histogram")) {
	return (ST_HISTOGRAM);
    }

/*** ACS_MODIFY END - site_attr management *************************************/
    else {
	return (-1);
    }
}

/*
 * isosurf --
 *     Syntax: <map_obj> isosurf <command>
 *     Manipulation with isosurfaces
 */
int isosurf(int id, int type, Tcl_Interp * interp, int argc, char *argv[])
{
    if (type != VOL) {
	Tcl_SetResult(interp, "Type must be VOL for isosurf", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (!strcmp(argv[2], "set_res"))
	return (isosurf_set_res(id, interp, argc, argv));
    else if (!strcmp(argv[2], "get_res"))
	return (isosurf_get_res(id, interp, argc, argv));
    else if (!strcmp(argv[2], "set_drawmode"))
	return (isosurf_set_drawmode(id, interp, argc, argv));
    else if (!strcmp(argv[2], "get_drawmode"))
	return (isosurf_get_drawmode(id, interp, argc, argv));
    else if (!strcmp(argv[2], "num_isosurfs"))
	return (isosurf_num_isosurfs(id, interp, argc, argv));
    else if (!strcmp(argv[2], "add"))
	return (isosurf_add(id, interp, argc, argv));
    else if (!strcmp(argv[2], "del"))
	return (isosurf_del(id, interp, argc, argv));
    else if (!strcmp(argv[2], "move_up"))
	return (isosurf_move_up(id, interp, argc, argv));
    else if (!strcmp(argv[2], "move_down"))
	return (isosurf_move_down(id, interp, argc, argv));
    else if (!strcmp(argv[2], "get_att"))
	return (isosurf_get_att(id, interp, argc, argv));
    else if (!strcmp(argv[2], "set_att"))
	return (isosurf_set_att(id, interp, argc, argv));
    else if (!strcmp(argv[2], "unset_att"))
	return (isosurf_unset_att(id, interp, argc, argv));
    else if (!strcmp(argv[2], "get_mask_mode"))
	return (isosurf_get_mask_mode(id, interp, argc, argv));
    else if (!strcmp(argv[2], "set_mask_mode"))
	return (isosurf_set_mask_mode(id, interp, argc, argv));
    else if (!strcmp(argv[2], "get_flags"))
	return (isosurf_get_flags(id, interp, argc, argv));
    else if (!strcmp(argv[2], "set_flags"))
	return (isosurf_set_flags(id, interp, argc, argv));
    else {
	Tcl_SetResult(interp, "Error: unknown command for isosurf",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }
}

/*
 * slice --
 *     Syntax: <map_obj> slice <command>
 *     Manipulation with slices
 */
int slice(int id, int type, Tcl_Interp * interp, int argc, char *argv[])
{
    if (type != VOL) {
	Tcl_SetResult(interp, "Type must be VOL for slice", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (!strcmp(argv[2], "set_res"))
	return (slice_set_res(id, interp, argc, argv));
    else if (!strcmp(argv[2], "get_res"))
	return (slice_get_res(id, interp, argc, argv));
    else if (!strcmp(argv[2], "set_drawmode"))
	return (slice_set_drawmode(id, interp, argc, argv));
    else if (!strcmp(argv[2], "get_drawmode"))
	return (slice_get_drawmode(id, interp, argc, argv));
    else if (!strcmp(argv[2], "num_slices"))
	return (slice_num_slices(id, interp, argc, argv));
    else if (!strcmp(argv[2], "add"))
	return (slice_add(id, interp, argc, argv));
    else if (!strcmp(argv[2], "del"))
	return (slice_del(id, interp, argc, argv));
    else if (!strcmp(argv[2], "move_up"))
	return (slice_move_up(id, interp, argc, argv));
    else if (!strcmp(argv[2], "move_down"))
	return (slice_move_down(id, interp, argc, argv));
    else if (!strcmp(argv[2], "get_pos"))
	return (slice_get_pos(id, interp, argc, argv));
    else if (!strcmp(argv[2], "set_pos"))
	return (slice_set_pos(id, interp, argc, argv));
    else if (!strcmp(argv[2], "get_transp"))
	return (slice_get_transp(id, interp, argc, argv));
    else if (!strcmp(argv[2], "set_transp"))
	return (slice_set_transp(id, interp, argc, argv));

    else {
	Tcl_SetResult(interp, "Error: unknown command for slice",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }
}
