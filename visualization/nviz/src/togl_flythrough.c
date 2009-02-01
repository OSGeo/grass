
/***************************************************************
 *
 * MODULE:       togl_flythrough.c 0.993
 *
 * AUTHOR(S):    ACS - Massimo Cuomo - m.cuomo at acsys.it
 *
 * PURPOSE:	"Immersive" navigation by means of mouse buttons and movement
 * 		 In conjunction with flythrough.tcl
 *
 * COPYRIGHT:    (C) 2005 by the ACS / GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 * changes from 0.99
 * 09/02/05 0.991 solved drawing at each frame in do_navigation() when TOGL_FLY_ORBIT
 *		and buttonAny now works fine
 * 23/02/05 0.992 now "orbit" flythrough works also when kanimator plays
 *		only one line in file nviz_init.c must be added
 * 		(a call to togl_flythrough_init_tcl)
 *		Function Ndraw_all_together_cmd overrides tcl command Ndraw_all
 *		New Tcl commands Nset/get_viewdir are defined here
 * 02/03/05 0.993 added flythrough_postdraw_cb mechanism to call registered functions
 *		after drawing (here and in quick_draw.c)
 **************************************************************/

/* from togl_cb.c */
#include <stdlib.h>
#include <string.h>
#include "togl.h"
#include "interface.h"

#include <sys/time.h>
#ifdef __MINGW32__
#include <sys/timeb.h>
#endif
#include <math.h>
#include <grass/gis.h>

#define TOGL_FLY_OTHER	-1
#define TOGL_FLY_BASIC	0
#define TOGL_FLY_ORBIT	1
#define TOGL_FLY_SIMPLE	2
#define TOGL_FLY_MAX	2

#define QUASI_ZERO	0.0001

#define TOGL_MOVE	0
#define TOGL_TURN	1
#define TOGL_SCALE_DIM	2

extern void display_cb();

void event_proc(ClientData clientData, XEvent * eventPtr);
void togl_flythrough_timer_cb(struct Togl *togl);
void mouse_valuator(struct Togl *togl);
void do_navigation(struct Togl *togl);
double this_time(void);

static float pi, half_pi, double_pi, quasi_half_pi;

struct mouseData
{
    int x;
    int y;
    int button[3];
    int buttonAny;
};

struct flyData
{
    int mode;
    float valuator[3];
    float scale[TOGL_SCALE_DIM];
    float accelRate;
    float maxSpeed;
    float curSpeed;
    double prevTime;
    float dx;
    float dy;
    float mx;
    float my;
    int lateral;
    int twist;
    float center[3];
};

struct cbData
{
    int coarse_draw;
    struct mouseData mouse;
    struct flyData fly;
    Nv_data *nv_data;
    Tcl_Interp *interp;
};

/* This contains almost everything */
static struct cbData cb_data;

/*
   PostDraw Callbacks called here and in quick_draw.c
 */
static int postdraw_count = 0;
static void (*postdraw_func[256]) (void *);
static void *postdraw_data[256];

void flythrough_postdraw_cb()
{
    int i;

    for (i = 0; i < postdraw_count; i++)
	(*postdraw_func[i]) (postdraw_data[i]);
}

void flythrough_postdraw_cb_set(void (*func) (void *), void *data)
{
    if (postdraw_count > 255) {
	printf("flythrough_postdraw_cb_set: TOO MANY CALLBACKS!\n");
	return;
    }
    postdraw_func[postdraw_count] = func;
    postdraw_data[postdraw_count] = data;
    postdraw_count++;
}

void flythrough_postdraw_cb_remove(void (*func) (void *))
{
    int i;

    for (i = 0; i < postdraw_count && postdraw_func[i] != func; i++) ;
    for (i++; i < postdraw_count; i++) {
	postdraw_func[i - 1] = postdraw_func[i];
	postdraw_data[i - 1] = postdraw_data[i];
    }
    --postdraw_count;
}


/*
   New tcl commands for changing navigations mode and coarse/fine draw styles
 */
int Nset_fly_mode_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		      char **argv)
{
    int mode;

    if (argc != 2)
	return (TCL_ERROR);

    mode = atoi(argv[1]);

    if (mode != TOGL_FLY_SIMPLE)
	GS_set_twist(0);

    if (mode < 0 || mode > TOGL_FLY_MAX) {
	cb_data.fly.mode = TOGL_FLY_OTHER;
	Tcl_Eval(interp, "fly_deselect");
    }
    else {
	cb_data.fly.mode = mode;
	Tcl_Eval(interp, "fly_select");
    }

    return TCL_OK;
}

int Nset_fly_scale_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		       char **argv)
{
    if (argc != 3)
	return (TCL_ERROR);

    cb_data.fly.scale[TOGL_MOVE] = atof(argv[1]);
    cb_data.fly.scale[TOGL_TURN] = atof(argv[2]);
    return TCL_OK;
}

int Nget_fly_scale_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		       char **argv)
{
    char *list[2];
    char scale0[32], scale1[32];

    if (argc != 1)
	return (TCL_ERROR);

    sprintf(scale0, "%f", cb_data.fly.scale[TOGL_MOVE]);
    sprintf(scale1, "%f", cb_data.fly.scale[TOGL_TURN]);

    list[0] = scale0;
    list[1] = scale1;

    Tcl_SetResult(interp, Tcl_Merge(2, list), TCL_DYNAMIC);
    return (TCL_OK);
}

/*
   Init function: called first time by togl_flythrough_timer_cb
   (needs togl to be alredy created)
 */
void togl_flythrough_init(struct Togl *togl)
{
    float n, s, w, e;

    /* Useful constants */
    half_pi = 2.0f * atan(1.0);
    pi = 2.0f * half_pi;
    double_pi = 2.0f * pi;
    quasi_half_pi = half_pi - 0.001f;

    cb_data.coarse_draw = TRUE;

    /* Navigation parameters, can be read from a resource file */
    cb_data.fly.mode = TOGL_FLY_OTHER;
    cb_data.fly.accelRate = 1.0f;
    cb_data.fly.maxSpeed = 5.0f;

    cb_data.fly.curSpeed = 0.0f;
    cb_data.fly.prevTime = this_time();

    cb_data.fly.dx = 0.01f;	/* dead zone */
    cb_data.fly.dy = 0.01f;	/* dead zone */

    cb_data.fly.scale[TOGL_MOVE] = 1.0f;
    cb_data.fly.scale[TOGL_TURN] = 1.0f;

    GS_get_region(&n, &s, &w, &e);
    cb_data.fly.center[0] = (e + w) / 2.0;
    cb_data.fly.center[1] = (n + s) / 2.0;
    cb_data.fly.center[2] = 0.0;
    gsd_real2model(cb_data.fly.center);

    /* Data to be passed among callbacks */
    Togl_SetClientData(togl, (ClientData) (&cb_data));

    /* Private event handler function */
    Tk_CreateEventHandler(Togl_TkWin(togl),
			  ButtonPressMask | ButtonReleaseMask |
			  PointerMotionMask, event_proc,
			  (ClientData) (&cb_data));
}


/* Creates tcl commands and variables
   Called by Ninit() in file "nviz_init.c"
 */
void togl_flythrough_init_tcl(Tcl_Interp * interp, Nv_data * data)
{
    /* Added Togl_Timer_Function: be sure others don't replace it with another one */
    Togl_TimerFunc(togl_flythrough_timer_cb);

    cb_data.nv_data = data;
    cb_data.interp = interp;

    Tcl_CreateCommand(interp, "Nset_fly_scale",
		      (Tcl_CmdProc *) Nset_fly_scale_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_fly_scale",
		      (Tcl_CmdProc *) Nget_fly_scale_cmd, data, NULL);

    Tcl_CreateCommand(interp, "Nset_fly_mode",
		      (Tcl_CmdProc *) Nset_fly_mode_cmd, data, NULL);

    Tcl_LinkVar(interp, "coarse_draw", (char *)&(cb_data.coarse_draw),
		TCL_LINK_BOOLEAN);

    Tcl_CreateCommand(interp, "Nget_viewdir",
		      (Tcl_CmdProc *) Nget_viewdir_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_viewdir",
		      (Tcl_CmdProc *) Nset_viewdir_cmd, data, NULL);

    /* Override Ndraw_all_cmd */
    /* declared in init_commands 
       Tcl_CreateCommand(interp, "Ndraw_all", Ndraw_all_together_cmd, data, NULL);
     */
    /*      Tcl_CreateCommand(interp, "Ndraw_all_together", (Tcl_CmdProc*)Ndraw_all_together_cmd, data, NULL);
     */
}


/* Callback registered by NVIZ_AppInit() in file "nvizAppInit"
 */
void togl_flythrough_timer_cb(struct Togl *togl)
{
    /* it's here in order to avoid to modify other files to call togl_flythrough_init() */
    static int first_time = 1;
    struct cbData *cb = (struct cbData *)Togl_GetClientData(togl);


    if (first_time) {
	first_time = 0;
	togl_flythrough_init(togl);
	return;
    }

    if (cb->fly.mode != TOGL_FLY_OTHER) {
	mouse_valuator(togl);
	do_navigation(togl);
    }
}



/* INTERNAL FUNCTIONS */

double this_time(void)
{
#ifdef __MINGW32__
    struct timeb tb;

    ftime(&tb);
    return ((float)tb.time + ((float)tb.millitm / 1000.0));
#else
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return ((float)tv.tv_sec + ((float)tv.tv_usec / 1000000.0));
#endif
}

void event_proc(ClientData clientData, XEvent * eventPtr)
{
    struct cbData *cb = (struct cbData *)clientData;

    switch (eventPtr->type) {
    case MotionNotify:
	cb->mouse.x = eventPtr->xbutton.x;
	cb->mouse.y = eventPtr->xbutton.y;
	break;

    case ButtonPress:
	if (eventPtr->xbutton.button < 4) {
	    cb->mouse.button[eventPtr->xbutton.button - 1] = 1;
	}
	cb->mouse.x = eventPtr->xbutton.x;
	cb->mouse.y = eventPtr->xbutton.y;
	break;

    case ButtonRelease:
	if (eventPtr->xbutton.button < 4) {
	    cb->mouse.button[eventPtr->xbutton.button - 1] = 0;
	}
	cb->mouse.x = eventPtr->xbutton.x;
	cb->mouse.y = eventPtr->xbutton.y;
	break;

    default:
	break;
    }
    cb->mouse.buttonAny = cb->mouse.button[0] || cb->mouse.button[1] ||
	cb->mouse.button[2];

}

void calc_mxmy(struct Togl *togl)
{
    float mx, my;
    struct cbData *cb = (struct cbData *)Togl_GetClientData(togl);

    mx = 2.0f * ((float)cb->mouse.x / (float)Togl_Width(togl)) - 1.0f;
    my = 2.0f * ((float)cb->mouse.y / (float)Togl_Height(togl)) - 1.0f;

    if (mx < -cb->fly.dx)
	mx += cb->fly.dx;
    else if (mx > cb->fly.dx)
	mx -= cb->fly.dx;
    else
	mx = 0.0f;
    if (my < -cb->fly.dy)
	my += cb->fly.dy;
    else if (my > cb->fly.dy)
	my -= cb->fly.dy;
    else
	my = 0.0f;

    cb->fly.mx = mx / (1.0f - cb->fly.dx);
    cb->fly.my = my / (1.0f - cb->fly.dy);

    /* Quadratic seems smoother */
    cb->fly.mx = cb->fly.mx * fabsf(cb->fly.mx);
    cb->fly.my = cb->fly.my * fabsf(cb->fly.my);
}


void mouse_valuator(struct Togl *togl)
{
    double thisTime;
    double deltaTime;
    struct cbData *cb = (struct cbData *)Togl_GetClientData(togl);

    cb->fly.valuator[0] = 0.0;
    cb->fly.valuator[1] = 0.0;
    cb->fly.valuator[2] = 0.0;
    cb->fly.lateral = 0;
    cb->fly.twist = 0;

    thisTime = this_time();
    deltaTime = thisTime - cb->fly.prevTime;
    cb->fly.prevTime = thisTime;

    switch (cb->fly.mode) {

    case TOGL_FLY_BASIC:
    case TOGL_FLY_ORBIT:

	if (cb->mouse.button[1]) {
	    if (cb->mouse.button[0]) {	/* accelerate up to maxSpeed */
		cb->fly.curSpeed += cb->fly.accelRate * deltaTime;
		if (cb->fly.curSpeed > cb->fly.maxSpeed)
		    cb->fly.curSpeed = cb->fly.maxSpeed;
	    }
	    else if (cb->mouse.button[2]) {	/* decelerate down to -maxSpeed */
		cb->fly.curSpeed -= cb->fly.accelRate * deltaTime;
		if (cb->fly.curSpeed < -cb->fly.maxSpeed)
		    cb->fly.curSpeed = -cb->fly.maxSpeed;
	    }
	    calc_mxmy(togl);

	    cb->fly.valuator[0] = cb->fly.curSpeed;	/* speed (forward) */
	    cb->fly.valuator[1] = cb->fly.mx * deltaTime;	/* heading */
	    cb->fly.valuator[2] = cb->fly.my * deltaTime;	/* picth */
	    return;
	}

	else if (cb->mouse.button[0] && cb->mouse.button[2]) {
	    cb->fly.lateral = 1;
	    cb->fly.curSpeed = 0.0;

	    calc_mxmy(togl);

	    cb->fly.valuator[0] = cb->fly.mx * 100.0 * deltaTime;	/* lateral */
	    cb->fly.valuator[2] = -cb->fly.my * 100.0 * deltaTime;	/* vertical */
	    return;
	}

	else {
	    cb->fly.curSpeed = 0.0;
	    return;
	}
	break;

    case TOGL_FLY_SIMPLE:
	if (cb->mouse.button[0] || cb->mouse.button[2])
	    calc_mxmy(togl);
	else
	    return;

	if (cb->mouse.button[0] && !cb->mouse.button[2]) {
	    /* only left button */
	    cb->fly.valuator[0] = -cb->fly.my * 100.0 * deltaTime;	/* forward */
	    cb->fly.valuator[1] = cb->fly.mx * deltaTime;	/* heading */
	    return;
	}

	if (!cb->mouse.button[0] && cb->mouse.button[2]) {
	    /* only right button */
	    cb->fly.lateral = 1;
	    cb->fly.valuator[0] = cb->fly.mx * 100.0 * deltaTime;	/* lateral */
	    cb->fly.valuator[2] = -cb->fly.my * 100.0 * deltaTime;	/* vertical */
	    return;
	}

	if (cb->mouse.button[0] && cb->mouse.button[2]) {
	    /* rigth and left buttons */
	    cb->fly.twist = 1;
	    cb->fly.valuator[1] = cb->fly.mx * deltaTime;	/* roll */
	    cb->fly.valuator[2] = cb->fly.my * deltaTime;	/* pitch */
	    return;
	}
	break;
    }
}



void do_navigation(struct Togl *togl)
{
    float dir[3], from[4], cur_from[4], cur_dir[4], cur[3];
    float speed, h, p, sh, ch, sp, cp, radius;
    float diff_x, diff_y, diff_z;
    int twist, cur_twist;

    struct cbData *cb = (struct cbData *)Togl_GetClientData(togl);
    struct flyData *fly = &(cb->fly);

    static int draw_all = 0;

    if (!cb->mouse.buttonAny) {
	if (draw_all == 1) {
	    draw_all = 0;
	    /*                      Tcl_SetVar(cb->interp, "autoc", "1", TCL_LEAVE_ERR_MSG);
	       Ndraw_all_cmd(cb->nv_data, cb->interp, NULL, NULL);
	     */
	    Ndraw_all_together_cmd(cb->nv_data, cb->interp, 1, NULL);
	}
	return;
    }

    GS_get_from(cur_from);
    GS_get_viewdir(cur_dir);
    twist = cur_twist = GS_get_twist();

    p = asin(cur_dir[Z]);
    h = atan2(-cur_dir[X], -cur_dir[Y]);

    speed = fly->scale[TOGL_MOVE] * fly->valuator[0];

    if (!fly->twist)		/* in case of "twist" doesn't change heading */
	h += fly->scale[TOGL_TURN] * fly->valuator[1];

    if (!fly->lateral)		/* in case of "lateral" doesn't change pitch */
	p -= fly->scale[TOGL_TURN] * fly->valuator[2];

    h = fmod(h + pi, double_pi) - pi;

    if (p < -quasi_half_pi)
	p = -quasi_half_pi;	/* Internal flythrough tcl callbacks */
    else if (p > quasi_half_pi)
	p = quasi_half_pi;

    sh = sin(h);
    ch = cos(h);
    sp = sin(p);
    cp = cos(p);

    dir[X] = -sh * cp;
    dir[Y] = -ch * cp;
    dir[Z] = sp;

    switch (fly->mode) {
    case TOGL_FLY_BASIC:
    case TOGL_FLY_SIMPLE:
	if (fly->lateral) {
	    from[X] = cur_from[X] + speed * dir[Y];
	    from[Y] = cur_from[Y] - speed * dir[X];
	    from[Z] = cur_from[Z] + fly->scale[TOGL_MOVE] * fly->valuator[2];
	}
	else {
	    from[X] = cur_from[X] + speed * dir[X];
	    from[Y] = cur_from[Y] + speed * dir[Y];
	    from[Z] = cur_from[Z] + speed * dir[Z];
	}

	if (fly->twist) {
	    twist =
		cur_twist +
		(int)((fly->scale[TOGL_TURN] * fly->valuator[1] * 1800.0) /
		      pi);
	    if (twist > 890)
		twist = 890;
	    else if (twist < -890)
		twist = -890;
	}

	break;
    case TOGL_FLY_ORBIT:
	cur[X] = cur_from[X] - fly->center[X];
	cur[Y] = cur_from[Y] - fly->center[Y];
	cur[Z] = cur_from[Z] - fly->center[Z];

	GS_v3mag(cur, &radius);
	radius -= speed;
	if (radius < 0.0f)
	    radius = 0.0f;

	from[X] = -radius * dir[X] + fly->center[X];
	from[Y] = -radius * dir[Y] + fly->center[Y];
	from[Z] = -radius * dir[Z] + fly->center[Z];
	break;
    }

    diff_x = fabs(cur_dir[X] - dir[X]);
    diff_y = fabs(cur_dir[Y] - dir[Y]);
    diff_z = fabs(cur_dir[Z] - dir[Z]);

    if (			/* something has changed */
	   (diff_x > QUASI_ZERO) || (diff_y > QUASI_ZERO) ||
	   (diff_z > QUASI_ZERO) || (cur_from[X] != from[X]) ||
	   (cur_from[Y] != from[Y]) || (cur_from[Z] != from[Z]) ||
	   (cur_twist != twist)
	) {
	GS_moveto(from);

	/* Accomodates for up vector "jumps" when pitch changes sign */
	if (p > 0.0f)
	    GS_set_twist(twist + 1800);
	else
	    GS_set_twist(twist);

	GS_set_viewdir(dir);	/* calls gsd_set_view */

	GS_set_draw(GSD_BACK);	/* needs to be BACK to avoid flickering */
	GS_clear(cb->nv_data->BGcolor);

	if (cb->coarse_draw) {
	    GS_set_draw(GSD_BACK);
	    GS_set_draw(GSD_BACK);
	    GS_ready_draw();
	    GS_alldraw_wire();
	    GS_done_draw();
	    /* display_cb (togl); */
	    flythrough_postdraw_cb();
	    draw_all = 1;
	}
	else {
	    /* Draws without clearng buffer at each map type */
	    Ndraw_all_together_cmd(cb->nv_data, cb->interp, 1, NULL);
	    draw_all = 0;
	}
	/* prepare twist for next frame GS_get_twist() call */
	if (p > 0.0f)
	    GS_set_twist(twist);
    }
}

/*******************************************************************************
	Ndraw_all override Tcl Command
*******************************************************************************/
int surf_draw_all_together(Nv_data * dc, Tcl_Interp * interp)
{
    int i, nsurfs;
    int sortSurfs[MAX_SURFS], sorti[MAX_SURFS];
    int *surf_list;
    float x, y, z;
    int num, w;

    /* Get position for Light 1 */
    num = 1;
    x = dc->light[num].x;
    y = dc->light[num].y;
    z = dc->light[num].z;
    w = dc->light[num].z;

    surf_list = GS_get_surf_list(&nsurfs);
    sort_surfs_max(surf_list, sortSurfs, sorti, nsurfs);
    G_free(surf_list);

    /* re-initialize lights */
    GS_setlight_position(num, x, y, z, w);
    num = 2;
    GS_setlight_position(num, 0., 0., 1., 0);

    for (i = 0; i < nsurfs; i++) {
	if (check_blank(interp, sortSurfs[i]) == 0) {
	    GS_draw_surf(sortSurfs[i]);
	}
    }

    /* GS_draw_cplane_fence params will change - surfs aren't used anymore */
    for (i = 0; i < MAX_CPLANES; i++) {
	if (dc->Cp_on[i])
	    GS_draw_cplane_fence(sortSurfs[0], sortSurfs[1], i);
    }
    return (TCL_OK);
}

int vect_draw_all_together(Nv_data * data, Tcl_Interp * interp)
{
    int i, nvects;
    int *vect_list;

    GS_set_cancel(0);
    vect_list = GV_get_vect_list(&nvects);

    for (i = 0; i < nvects; i++) {
	if (check_blank(interp, vect_list[i]) == 0) {
	    GV_draw_vect(vect_list[i]);
	}
    }
    G_free(vect_list);

    GS_set_cancel(0);
    return (TCL_OK);
}

int site_draw_all_together(Nv_data * data, Tcl_Interp * interp)
{
    int i, nsites;
    int *site_list;

    GS_set_cancel(0);

    site_list = GP_get_site_list(&nsites);

    for (i = 0; i < nsites; i++) {
	if (check_blank(interp, site_list[i]) == 0) {
	    GP_draw_site(site_list[i]);
	}
    }
    G_free(site_list);

    GS_set_cancel(0);
    return (TCL_OK);
}

int vol_draw_all_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    int i, nvols;
    int *vol_list;

    GS_set_cancel(0);
    vol_list = GVL_get_vol_list(&nvols);

    for (i = 0; i < nvols; i++) {
	if (check_blank(interp, vol_list[i]) == 0) {
	    GVL_draw_vol(vol_list[i]);
	}
    }
    G_free(vol_list);


    GS_set_cancel(0);
    return (TCL_OK);
}

int Ndraw_all_together_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			   int argc,	/* Number of arguments. */
			   char **argv	/* Argument strings. */
    )
{
    const char *buf_surf, *buf_vect, *buf_site, *buf_vol;
    const char *buf_north_arrow, *arrow_x, *buf_label, *buf_legend;
    const char *buf_fringe, *buf_scalebar, *bar_x;
    const char *buf_is_drawing =
	Tcl_GetVar(interp, "is_drawing", TCL_GLOBAL_ONLY);
    const char *EMPTYSTRING = "";

    if (buf_is_drawing && atoi(buf_is_drawing))
	return (TCL_OK);

    Tcl_SetVar(interp, "is_drawing", "1", TCL_GLOBAL_ONLY);

    GS_set_draw(GSD_BACK);	/* needs to be BACK to avoid flickering */
    GS_clear(data->BGcolor);
    GS_ready_draw();

    buf_surf = Tcl_GetVar(interp, "surface", TCL_GLOBAL_ONLY);
    buf_vect = Tcl_GetVar(interp, "vector", TCL_GLOBAL_ONLY);
    buf_site = Tcl_GetVar(interp, "sites", TCL_GLOBAL_ONLY);
    buf_vol = Tcl_GetVar(interp, "volume", TCL_GLOBAL_ONLY);
    buf_north_arrow = Tcl_GetVar(interp, "n_arrow", TCL_GLOBAL_ONLY);
    arrow_x = Tcl_GetVar(interp, "n_arrow_x", TCL_GLOBAL_ONLY);
    buf_label = Tcl_GetVar(interp, "labels", TCL_GLOBAL_ONLY);
    buf_legend = Tcl_GetVar(interp, "legend", TCL_GLOBAL_ONLY);
    buf_fringe = Tcl_GetVar(interp, "fringe", TCL_GLOBAL_ONLY);
    buf_scalebar = Tcl_GetVar(interp, "scalebar", TCL_GLOBAL_ONLY);
    bar_x = Tcl_GetVar(interp, "scalebar_x", TCL_GLOBAL_ONLY);

    if (buf_surf && atoi(buf_surf) == 1)
	surf_draw_all_together(data, interp);
    if (buf_vect && atoi(buf_vect) == 1)
	vect_draw_all_together(data, interp);
    if (buf_site && atoi(buf_site) == 1)
	site_draw_all_together(data, interp);
    if (buf_vol && atoi(buf_vol) == 1)
	vol_draw_all_cmd(data, interp, argc, argv);

    GS_done_draw();
    GS_set_draw(GSD_BACK);

    if (buf_north_arrow == NULL)
	buf_north_arrow = EMPTYSTRING;

    if (arrow_x == NULL)
	arrow_x = EMPTYSTRING;

    if (buf_scalebar == NULL)
	buf_scalebar = EMPTYSTRING;

    if (bar_x == NULL)
	bar_x = EMPTYSTRING;

    if (buf_fringe == NULL)
	buf_fringe = EMPTYSTRING;

    if (buf_label == NULL)
	buf_label = EMPTYSTRING;

    if (buf_legend == NULL)
	buf_legend = EMPTYSTRING;


    /* Draw decorations */

    /* North Arrow */
    if (atoi(buf_north_arrow) == 1 && atoi(arrow_x) != 999) {
	const char *arrow_y, *arrow_z, *arrow_len;
	float coords[3], len;
	int arrow_clr, text_clr;

	arrow_y = Tcl_GetVar(interp, "n_arrow_y", TCL_GLOBAL_ONLY);
	arrow_z = Tcl_GetVar(interp, "n_arrow_z", TCL_GLOBAL_ONLY);
	arrow_len = Tcl_GetVar(interp, "n_arrow_size", TCL_GLOBAL_ONLY);
	arrow_clr =
	    (int)
	    tcl_color_to_int(Tcl_GetVar(interp, "arw_clr", TCL_GLOBAL_ONLY));
	text_clr =
	    (int)
	    tcl_color_to_int(Tcl_GetVar
			     (interp, "arw_text_clr", TCL_GLOBAL_ONLY));
	coords[0] = atoi(arrow_x);
	coords[1] = atoi(arrow_y);
	coords[2] = atoi(arrow_z);
	len = atof(arrow_len);

	FontBase = load_font(TOGL_BITMAP_HELVETICA_18);
	gsd_north_arrow(coords, len, FontBase, arrow_clr, text_clr);
    }

    /* Scale Bar */
    if (atoi(buf_scalebar) == 1 && atoi(bar_x) != 999) {
	const char *bar_y, *bar_z, *bar_len;
	float coords[3], len;
	int bar_clr, text_clr;

	bar_y = Tcl_GetVar(interp, "scalebar_y", TCL_GLOBAL_ONLY);
	bar_z = Tcl_GetVar(interp, "scalebar_z", TCL_GLOBAL_ONLY);
	bar_len = Tcl_GetVar(interp, "scalebar_size", TCL_GLOBAL_ONLY);
	bar_clr =
	    (int)
	    tcl_color_to_int(Tcl_GetVar(interp, "bar_clr", TCL_GLOBAL_ONLY));
	text_clr =
	    (int)
	    tcl_color_to_int(Tcl_GetVar
			     (interp, "bar_text_clr", TCL_GLOBAL_ONLY));
	coords[0] = atoi(bar_x);
	coords[1] = atoi(bar_y);
	coords[2] = atoi(bar_z);
	len = atof(bar_len);

	FontBase = load_font(TOGL_BITMAP_HELVETICA_18);
	gsd_scalebar(coords, len, FontBase, bar_clr, bar_clr);
    }

    /* fringe */
    if (atoi(buf_fringe) == 1) {
	const char *fringe_ne, *fringe_nw, *fringe_se, *fringe_sw;
	const char *surf_id;
	int flags[4], id;
	int fringe_clr;
	float fringe_elev;

	fringe_clr =
	    (int)
	    tcl_color_to_int(Tcl_GetVar
			     (interp, "fringe_color", TCL_GLOBAL_ONLY));
	fringe_elev =
	    (float)atof(Tcl_GetVar(interp, "fringe_elev", TCL_GLOBAL_ONLY));
	fringe_ne = Tcl_GetVar(interp, "fringe_ne", TCL_GLOBAL_ONLY);
	fringe_nw = Tcl_GetVar(interp, "fringe_nw", TCL_GLOBAL_ONLY);
	fringe_se = Tcl_GetVar(interp, "fringe_se", TCL_GLOBAL_ONLY);
	fringe_sw = Tcl_GetVar(interp, "fringe_sw", TCL_GLOBAL_ONLY);
	flags[0] = atoi(fringe_nw);
	flags[1] = atoi(fringe_ne);
	flags[2] = atoi(fringe_sw);
	flags[3] = atoi(fringe_se);
	surf_id = Tcl_GetVar2(interp, "Nv_", "CurrSurf", TCL_GLOBAL_ONLY);
	id = atoi(surf_id);

	GS_draw_fringe(id, fringe_clr, fringe_elev, flags);
    }

    /* Legend and/or labels */
    if (atoi(buf_label) == 1 || atoi(buf_legend) == 1)
	GS_draw_all_list();

    Tcl_SetVar(interp, "is_drawing", "0", TCL_GLOBAL_ONLY);
    flythrough_postdraw_cb();

    return (TCL_OK);
}


/*******************************************************************************
	Nset/Nget_viewdir Tcl Commands
*******************************************************************************/
int Nset_viewdir_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		     char **argv)
{
    float dir[3];

    if (argc != 4)
	return (TCL_ERROR);

    dir[0] = atof(argv[1]);
    dir[1] = atof(argv[2]);
    dir[2] = atof(argv[3]);

    GS_set_viewdir(dir);

    return TCL_OK;
}


int Nget_viewdir_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    float dir[3];
    char x[32], y[32], z[32];
    char *list[4];

    GS_get_viewdir(dir);
    sprintf(x, "%f", dir[0]);
    sprintf(y, "%f", dir[1]);
    sprintf(z, "%f", dir[2]);

    list[0] = x;
    list[1] = y;
    list[2] = z;
    list[3] = NULL;

    Tcl_SetResult(interp, Tcl_Merge(3, list), TCL_VOLATILE);

    return (TCL_OK);
}
