#include <stdlib.h>
#include "interface.h"

static int setlgt_ambient(Nv_data *, Tcl_Interp *, int, char **);
static int getlgt_bright(Nv_data *, Tcl_Interp *, int, char **);
static int getlgt_ambient(Nv_data *, Tcl_Interp *, int, char **);
static int getlgt_color(Nv_data *, Tcl_Interp *, int, char **);
static int getlgt_position(Nv_data *, Tcl_Interp *, int, char **);
static int setlgt_bright(Nv_data *, Tcl_Interp *, int, char **);
static int setlgt_color(Nv_data *, Tcl_Interp *, int, char **);
static int setlgt_position(Nv_data *, Tcl_Interp *, int, char **);
static int switchlight(int, char **);
static int get_light_num(char *);
static int init_new_light(Nv_data *, int);

int Nlight_obj_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    if (!strcmp(argv[1], "set_ambient"))
	return (setlgt_ambient(data, interp, argc, argv));
    else if (!strcmp(argv[1], "get_ambient"))
	return (getlgt_ambient(data, interp, argc, argv));
    else if (!strcmp(argv[1], "set_bright"))
	return (setlgt_bright(data, interp, argc, argv));
    else if (!strcmp(argv[1], "get_bright"))
	return (getlgt_bright(data, interp, argc, argv));
    else if (!strcmp(argv[1], "set_color"))
	return (setlgt_color(data, interp, argc, argv));
    else if (!strcmp(argv[1], "get_color"))
	return (getlgt_color(data, interp, argc, argv));
    else if (!strcmp(argv[1], "set_position"))
	return (setlgt_position(data, interp, argc, argv));
    else if (!strcmp(argv[1], "get_position"))
	return (getlgt_position(data, interp, argc, argv));
    else if (!strcmp(argv[1], "switch"))
	return (switchlight(argc, argv));
    else
	return (TCL_ERROR);
}

int Nnew_light_cmd(Nv_data * data, Tcl_Interp * interp	/* Current interpreter. */
    )
{
    char buf[128];
    int num;

    if (0 > (num = GS_new_light())) {
	Tcl_SetResult(interp, "too many lights", TCL_VOLATILE);
	return (TCL_ERROR);
    }
    sprintf(buf, "Nlight%d", num);
    init_new_light(data, num);
    Tcl_CreateCommand(interp, buf, (Tcl_CmdProc *) Nlight_obj_cmd, data,
		      NULL);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);
    return (TCL_OK);
}

int Nlights_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		int argc,	/* Number of arguments. */
		char **argv	/* Argument strings. */
    )
{
    if (argc == 2)
	if (!strcmp(argv[1], "off")) {
	    GS_lights_off();
	    return (TCL_OK);
	}
    GS_lights_on();
    return (TCL_OK);
}

static int setlgt_ambient(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			  int argc,	/* Number of arguments. */
			  char **argv	/* Argument strings. */
    )
{
    int num;
    float r, g, b;

    if (argc != 5)
	return (TCL_ERROR);
    num = get_light_num(argv[0]);
    data->light[num].ar = r = (float)atof(argv[2]);
    data->light[num].ag = g = (float)atof(argv[3]);
    data->light[num].ab = b = (float)atof(argv[4]);
    GS_setlight_ambient(num, r, g, b);
    return (TCL_OK);
}

static int getlgt_bright(Nv_data * data, Tcl_Interp * interp, int argc,	/* Number of arguments. */
			 char **argv	/* Argument strings. */
    )
{
    int num;
    char bright[32];

    num = get_light_num(argv[0]);
    sprintf(bright, "%f", data->light[num].brt);
    Tcl_SetResult(interp, bright, TCL_VOLATILE);
    return (TCL_OK);
}

static int getlgt_ambient(Nv_data * data, Tcl_Interp * interp, int argc,	/* Number of arguments. */
			  char **argv	/* Argument strings. */
    )
{
    int num;
    char r[32], g[32], b[32];
    char *list[4];

    num = get_light_num(argv[0]);
    sprintf(r, "%f", data->light[num].ar);
    sprintf(g, "%f", data->light[num].ag);
    sprintf(b, "%f", data->light[num].ab);
    list[0] = r;
    list[1] = g;
    list[2] = b;
    list[3] = NULL;

    interp->result = Tcl_Merge(3, list);
    interp->freeProc = TCL_DYNAMIC;
    return (TCL_OK);
}

static int getlgt_color(Nv_data * data, Tcl_Interp * interp, int argc,	/* Number of arguments. */
			char **argv	/* Argument strings. */
    )
{
    int num;
    char r[32], g[32], b[32];
    char *list[4];

    num = get_light_num(argv[0]);
    sprintf(r, "%f", data->light[num].r);
    sprintf(g, "%f", data->light[num].g);
    sprintf(b, "%f", data->light[num].b);
    list[0] = r;
    list[1] = g;
    list[2] = b;
    list[3] = NULL;

    interp->result = Tcl_Merge(3, list);
    interp->freeProc = TCL_DYNAMIC;
    return (TCL_OK);
}

static int getlgt_position(Nv_data * data, Tcl_Interp * interp, int argc,	/* Number of arguments. */
			   char **argv	/* Argument strings. */
    )
{
    int num;
    char x[32], y[32], z[32], w[32];
    char *list[5];

    num = get_light_num(argv[0]);
    sprintf(x, "%f", data->light[num].x);
    sprintf(y, "%f", data->light[num].y);
    sprintf(z, "%f", data->light[num].z);
    sprintf(w, "%f", data->light[num].w);
    list[0] = x;
    list[1] = y;
    list[2] = z;
    list[3] = w;
    list[4] = NULL;

    interp->result = Tcl_Merge(4, list);
    interp->freeProc = TCL_DYNAMIC;
    return (TCL_OK);
}


static int setlgt_bright(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			 int argc,	/* Number of arguments. */
			 char **argv	/* Argument strings. */
    )
{
    int num;
    float r, g, b;
    double atof();

    if (argc != 3)
	return (TCL_ERROR);
    num = get_light_num(argv[0]);

    data->light[num].brt = (float)atof(argv[2]);

    r = data->light[num].r * data->light[num].brt;
    g = data->light[num].g * data->light[num].brt;
    b = data->light[num].b * data->light[num].brt;

    GS_setlight_color(num, r, g, b);
    return (TCL_OK);
}

static int setlgt_color(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			int argc,	/* Number of arguments. */
			char **argv	/* Argument strings. */
    )
{
    int num;
    float r, g, b;
    double atof();

    if (argc != 5)
	return (TCL_ERROR);
    num = get_light_num(argv[0]);

    data->light[num].r = (float)atof(argv[2]);
    data->light[num].g = (float)atof(argv[3]);
    data->light[num].b = (float)atof(argv[4]);

    r = data->light[num].r * data->light[num].brt;
    g = data->light[num].g * data->light[num].brt;
    b = data->light[num].b * data->light[num].brt;

    GS_setlight_color(num, r, g, b);
    return (TCL_OK);
}

static int setlgt_position(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			   int argc,	/* Number of arguments. */
			   char **argv	/* Argument strings. */
    )
{
    int num, w;
    float x, y, z;
    double atof();

    if (argc != 6)
	return (TCL_ERROR);

    num = get_light_num(argv[0]);

    data->light[num].x = x = (float)atof(argv[2]);
    data->light[num].y = y = (float)atof(argv[3]);
    data->light[num].z = z = (float)atof(argv[4]);
    data->light[num].w = w = (float)atoi(argv[5]);
    GS_setlight_position(num, x, y, z, w);
    return (TCL_OK);
}

static int switchlight(int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    int on, num;

    if (argc != 3)
	return (TCL_ERROR);
    on = (strcmp(argv[0], "off"));
    num = get_light_num(argv[3]);
    GS_switchlight(num, on);
    return (TCL_OK);
}

static int get_light_num(char *lgt)
{
    int num;

    sscanf(lgt, "Nlight%d", &num);

    return num;
}

static int init_new_light(Nv_data * data, int n)
{
    data->light[n].brt = 0.8;
    data->light[n].ar = 0.3;
    data->light[n].ag = 0.3;
    data->light[n].ab = 0.3;
    data->light[n].r = 1.0;
    data->light[n].b = 1.0;
    data->light[n].g = 1.0;
    data->light[n].x = 1.0;
    data->light[n].y = 1.0;
    data->light[n].z = 1.0;
    data->light[n].w = 1.0;

    return 0;
}
