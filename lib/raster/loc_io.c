
#include <grass/config.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/graphics.h>

#include "driver.h"
#include "transport.h"
#include "open.h"
#include "pad.h"

extern const struct driver *PNG_Driver(void);
extern const struct driver *PS_Driver(void);

static void LOC_init(void)
{
    const char *name = "full_screen";
    const char *fenc = getenv("GRASS_ENCODING");
    const char *font = getenv("GRASS_FONT");
    int t = R_screen_top();
    int b = R_screen_bot();
    int l = R_screen_left();
    int r = R_screen_rite();
    char buff[256];

    R_font(font ? font : "romans");

    if (fenc)
	R_charset(fenc);

    R_pad_select("");
    R_pad_set_item("time", "1");
    R_pad_set_item("cur_w", name);

    R_pad_create(name);
    R_pad_select(name);
    R_pad_set_item("time", "1");

    sprintf(buff, "%d %d %d %d", t, b, l, r);
    R_pad_set_item("d_win", buff);

    R_set_window(t, b, l, r);
}

int LOC_open_driver(void)
{
    const char *p = getenv("GRASS_RENDER_IMMEDIATE");
    const struct driver *drv = (p && G_strcasecmp(p, "PS") == 0)
	? PS_Driver()
	: PNG_Driver();

    LIB_init(drv, 0, NULL);

    LOC_init();

    COM_Client_Open();

    return OK;
}

int LOC__open_quiet(void)
{
    return 0;
}

void LOC_stabilize(void)
{
    COM_Respond();
}

void LOC_kill_driver(void)
{
    COM_Graph_close();
}

void LOC_close_driver(void)
{
    LOC_stabilize();
    COM_Client_Close();
    LOC_kill_driver();
}

void LOC_release_driver(void)
{
    LOC_stabilize();
    COM_Client_Close();
}
