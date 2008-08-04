
/* change_view:
 ** callbacks for movement & perspective adjustments
 */

/*
 * added return values and test for position *REALLY* changed
 * in: 
 *      Nchange_position_cmd()
 *      Nchange_height_cmd()
 *      Nchange_height_cmd()
 *      Nchange_exag_cmd()
 *      Pierre de Mouveaux (24 oct. 1999) p_de_mouveaux@hotmail.com
 */

 /*
    added return values and test for position *REALLY* changed
    in: 
    Nchange_position_cmd()
    Nchange_height_cmd()
    Nchange_height_cmd()
    Nchange_exag_cmd()
    Pierre de Mouveaux (24 oct. 1999)
  */

#include "interface.h"
#include <stdlib.h>
int Nchange_persp_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{
    int fov, persp;

    if (argc != 2)
	return (TCL_ERROR);
    persp = atoi(argv[1]);

    fov = (int)(10 * persp);
    GS_set_fov(fov);
    Nquick_draw_cmd(data, interp);

    return 0;

    return 0;
}

/**********************************************/
int Nchange_twist_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{
    int twist;

    if (argc != 2)
	return (TCL_ERROR);
    twist = atoi(argv[1]);

    twist = (int)(10 * twist);
    GS_set_twist(twist);
    Nquick_draw_cmd(data, interp);

    return 0;
}

/********************************************************************/
int normalize(float *v)
{
    float len;

    len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;

    return 0;
}

/**********************************************************************/
int Nchange_position_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			 int argc,	/* Number of arguments. */
			 char **argv	/* Argument strings. */
    )
{
    float xpos, ypos, from[3];
    float tempx, tempy;

    xpos = (float)atof(argv[1]);
    xpos = (xpos < 0) ? 0 : (xpos > 1.0) ? 1.0 : xpos;
    ypos = 1.0 - (float)atof(argv[2]);
    ypos = (ypos < 0) ? 0 : (ypos > 1.0) ? 1.0 : ypos;


    GS_get_from(from);

    tempx = xpos * RANGE - RANGE_OFFSET;
    tempy = ypos * RANGE - RANGE_OFFSET;

    if ((from[X] != tempx) || (from[Y] != tempy)) {

	from[X] = tempx;
	from[Y] = tempy;

	GS_moveto(from);
	/*

	   if (argc == 4)
	   {
	   if (atoi(argv[3]))
	   {
	   normalize (from);
	   GS_setlight_position(1, from[X], from[Y], from[Z], 0);
	   }
	   }
	 */
	Nquick_draw_cmd(data, interp);
    }
    return 0;
}

/**********************************************************************/

int Nchange_height_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    float temp;
    float from[3];
    double atof();

    if (argc < 2)
	return (TCL_ERROR);

    GS_get_from_real(from);
    temp = (float)atof(argv[1]);

    if (temp != from[Z]) {
	from[Z] = temp;

	GS_moveto_real(from);

	/*
	   if (argc == 3)
	   {
	   if (atoi(argv[2]))
	   {
	   normalize (from);
	   GS_setlight_position(1, from[X], from[Y], from[Z], 0);
	   }
	   }
	 */

	Nquick_draw_cmd(data, interp);
    }
    return (0);
}

int Nset_light_to_view_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			   int argc,	/* Number of arguments. */
			   char **argv	/* Argument strings. */
    )
{
    float from[3];

    GS_get_from_real(from);
    normalize(from);
    GS_setlight_position(1, from[X], from[Y], from[Z], 0);

    Nquick_draw_cmd(data, interp);

    return (0);
}


/**********************************************************************/
/* call whenever a new surface is added, deleted, or exag changes */
int update_ranges(Nv_data * dc)
{
    float zmin, zmax, exag;

    GS_get_longdim(&(dc->XYrange));

    dc->Zrange = 0.;

    /* Zrange is based on a minimum of Longdim */
    if (GS_global_exag()) {
	exag = GS_global_exag();
	dc->Zrange = dc->XYrange / exag;
    }
    else {
	exag = 1.0;
    }

    GS_get_zrange_nz(&zmin, &zmax);	/* actual */

    zmax = zmin + (3. * dc->XYrange / exag);
    zmin = zmin - (2. * dc->XYrange / exag);

    if ((zmax - zmin) > dc->Zrange)
	dc->Zrange = zmax - zmin;

    return (0);
}

/**********************************************************************/

int Nchange_exag_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    /*int i; */
    float val;
    float temp;

    /*  float from[3]; */
    double atof();

    if (argc != 2)
	return (TCL_ERROR);
    val = (float)atof(argv[1]);

    temp = GS_global_exag();
    if (val != temp) {
	GS_set_global_exag(val);
	update_ranges(data);
	Nquick_draw_cmd(data, interp);
    }

    return 0;

}

/**********************************************************************/

int Nget_position_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{
    float from[3];
    float tempx, tempy;
    char *list[2], x_pos[32], y_pos[32];

    GS_get_from(from);

    tempx = (from[X] + RANGE_OFFSET) / RANGE;
    tempy = (from[Y] + RANGE_OFFSET) / RANGE;

    sprintf(x_pos, "%f", tempx);
    sprintf(y_pos, "%f", tempy);

    list[0] = x_pos;
    list[1] = y_pos;


    Tcl_SetResult(interp, Tcl_Merge(2, list), TCL_VOLATILE);

    return (TCL_OK);

}

/**********************************************************************/
