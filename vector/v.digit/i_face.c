#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <tcl.h>
#include <tk.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/Vect.h>
#include "global.h"
#include "proto.h"

/* Interface functions between GUI and C:
 *  i_*() functions in GUI,  called from C 
 */

/* Set GUI promt to given string */
int i_prompt(char *str)
{
    Tcl_SetVar(Toolbox, "prompt", str, TCL_GLOBAL_ONLY);
    return 1;
}

/* Set GUI promt to given string */
int i_prompt_buttons(char *l, char *m, char *r)
{
    Tcl_SetVar(Toolbox, "prompt_left", l, TCL_GLOBAL_ONLY);
    Tcl_SetVar(Toolbox, "prompt_middle", m, TCL_GLOBAL_ONLY);
    Tcl_SetVar(Toolbox, "prompt_right", r, TCL_GLOBAL_ONLY);
    return 1;
}

/* Set GUI coordinates to given values */
int i_coor(double x, double y)
{
    char buf[100];

    if (x == COOR_NULL || y == COOR_NULL)
	buf[0] = '\0';
    else
	sprintf(buf, "%.2f, %.2f", x, y);

    Tcl_SetVar(Toolbox, "coor", buf, TCL_GLOBAL_ONLY);
    return 1;
}

/* Set symbology color */
int i_set_color(char *name, int r, int g, int b)
{
    char var[50], col[20];

    G_debug(2, "i_set_color(): %s : %d %d %d", name, r, g, b);

    sprintf(col, "#%02x%02x%02x", r, g, b);

    G_debug(2, " -> color = %s", col);

    sprintf(var, "symb(%s,color)", name);
    Tcl_SetVar(Toolbox, var, col, TCL_GLOBAL_ONLY);

    return 1;
}

/* Set symbology on/off */
int i_set_on(char *name, int on)
{
    char var[50], val[20];

    G_debug(2, "i_set_on(): %s : %d", name, on);

    sprintf(var, "symb(%s,on)", name);
    sprintf(val, "%d", on);
    Tcl_SetVar(Toolbox, var, val, TCL_GLOBAL_ONLY);

    return 1;
}

/* create: 1 - create, 0 - destroy */
void i_new_line_options(int create)
{
    int i;
    char val[1000];

    G_debug(4, "i_new_line_options(), create = %d", create);

    if (create) {
	Tcl_Eval(Toolbox, "new_line_options 1");

	/* Set cat mode */
	sprintf(val, "$GWidget(cat_mode) configure -values [list");
	for (i = 0; i < CAT_MODE_COUNT; i++) {
	    sprintf(val, "%s \"%s\"", val, CatModeLab[i]);
	}
	sprintf(val, "%s]", val);

	G_debug(2, "Cat modes: %s", val);
	Tcl_Eval(Toolbox, val);

	sprintf(val, "$GWidget(cat_mode) setvalue @%d",
		var_geti(VAR_CAT_MODE));
	G_debug(2, "Cat mode: %s", val);
	Tcl_Eval(Toolbox, val);
    }
    else {
	Tcl_Eval(Toolbox, "new_line_options 0");
    }

    sprintf(val, "%d", var_geti(VAR_FIELD));
    Tcl_SetVar(Toolbox, "field", val, TCL_GLOBAL_ONLY);
    i_set_cat_mode();
}

/* set category options */
void i_set_cat_mode(void)
{

    G_debug(5, "i_set_cat_mode");

    if (var_geti(VAR_CAT_MODE) == CAT_MODE_NO) {
	Tcl_Eval(Toolbox, "$GWidget(field) configure -state disabled");
    }
    else {
	Tcl_Eval(Toolbox, "$GWidget(field) configure -state normal");
    }

    if (var_geti(VAR_CAT_MODE) == CAT_MODE_MAN) {
	Tcl_Eval(Toolbox, "$GWidget(cat) configure -state normal");
    }
    else {
	Tcl_Eval(Toolbox, "$GWidget(cat) configure -state disabled");
    }
}

/* set variable */
void i_var_seti(int code, int i)
{
    char cmd[100], *name;

    G_debug(5, "i_var_seti()");

    name = var_get_name_by_code(code);
    sprintf(cmd, "set GVariable(%s) %d", name, i);
    G_debug(5, "cmd: %s", cmd);
    Tcl_Eval(Toolbox, cmd);
}

/* set variable */
void i_var_setd(int code, double d)
{
    char cmd[100], *name;

    G_debug(5, "i_var_seti");

    name = var_get_name_by_code(code);
    sprintf(cmd, "set GVariable(%s) %f", name, d);
    G_debug(5, "cmd: %s", cmd);
    Tcl_Eval(Toolbox, cmd);
}

/* set variable */
void i_var_setc(int code, char *c)
{
    char cmd[100], *name;

    G_debug(5, "i_var_seti");

    name = var_get_name_by_code(code);

    Tcl_SetVar(Toolbox, "tmp", c, TCL_GLOBAL_ONLY);

    sprintf(cmd, "set GVariable(%s) $tmp", name);
    G_debug(5, "cmd: %s", cmd);
    Tcl_Eval(Toolbox, cmd);
}

/* open GUI window with message */
int i_message(int type, int icon, char *msg)
{
    int answer;
    char *tp = "ok", *ico = "error", buf[1000];

    G_debug(5, "i_message()");

    switch (type) {
    case MSG_OK:
	tp = "ok";
	break;
    case MSG_YESNO:
	tp = "yesno";
	break;
    }
    switch (icon) {
    case MSGI_ERROR:
	ico = "error";
	break;
    case MSGI_QUESTION:
	ico = "question";
	break;
    }

    var_setc(VAR_MESSAGE, msg);
    sprintf(buf,
	    "set GVariable(answer) [MessageDlg .msg -type %s -icon %s -message $GVariable(message)]",
	    tp, ico);
    Tcl_Eval(Toolbox, buf);

    sprintf(buf, "c_var_set answer $GVariable(answer)");
    Tcl_Eval(Toolbox, buf);

    answer = var_geti(VAR_ANSWER);
    G_debug(4, "answer = %d", answer);

    return answer;
}

/* add background command */
void i_add_bgcmd(int index)
{
    char cmd[2000];

    G_debug(3, "i_add_bgcmd()");

    sprintf(cmd, "set GBgcmd(%d,on) %d", index, Bgcmd[index].on);
    Tcl_Eval(Toolbox, cmd);

    sprintf(cmd, "GBgcmd(%d,cmd)", index);
    Tcl_SetVar(Toolbox, cmd, Bgcmd[index].cmd, TCL_GLOBAL_ONLY);

    sprintf(cmd, "set row [ frame $GWidget(bgcmd).row%d ]", index);
    Tcl_Eval(Toolbox, cmd);

    sprintf(cmd, "checkbutton $row.a -variable GBgcmd(%d,on) -height 1 "
	    "-command { c_set_bgcmd %d $GBgcmd(%d,on) $GBgcmd(%d,cmd) }",
	    index, index, index, index);
    Tcl_Eval(Toolbox, cmd);

    sprintf(cmd, "Entry $row.b -width 40 -textvariable GBgcmd(%d,cmd) "
	    "-command { c_set_bgcmd %d $GBgcmd(%d,on) $GBgcmd(%d,cmd) }",
	    index, index, index, index);
    Tcl_Eval(Toolbox, cmd);

    Tcl_Eval(Toolbox, "pack $row.a $row.b -side left;");

    Tcl_Eval(Toolbox, "pack $row -side top -fill x -expand no -anchor n");

    sprintf(cmd, "bind $GWidget(bgcmd).row%d.b <KeyRelease> "
	    " { c_set_bgcmd %d $GBgcmd(%d,on) $GBgcmd(%d,cmd) }",
	    index, index, index, index);
    Tcl_Eval(Toolbox, cmd);
}
