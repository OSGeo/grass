#include "globals.h"

static int use = 1;
static int really_quit(void);
static int dont_stop(void);
static int stop(void);
static int go_refresh(void);
static int do_re_fresh(void);

int driver(void)
{
    static Objects objects[] = {
	MENU("QUIT", really_quit, &use),
	MENU("ZOOM", zoom, &use),
	MENU("RASTER", plotcell, &use),
	MENU("VECTOR", plotvect, &use),
	MENU("REFRESH", do_re_fresh, &use),
	/*      MENU("cursorcol", cursor_color, &use), */
	MENU("ANALYZE", analyze, &use),
	INFO(" Input: ", &from_flag),
	/*      OPTION("DIGITIZER",2,&from_digitizer),  */
	OPTION("KEYBOARD", 2, &from_keyboard),
	OPTION("SCREEN", 2, &from_screen),
	OTHER(mark, &use),
	{0}
    };

    Input_pointer(objects);
    Menu_msg("");

    return 0;
}

static int really_quit(void)
{
    static Objects objects[] = {
	INFO("Quit Program? ", &use),
	MENU("NO", dont_stop, &use),
	MENU("YES", stop, &use),
	{0}
    };
    if (Input_pointer(objects) < 0)
	return -1;
    return 0;			/* don't quit */
}

static int dont_stop(void)
{
    return 1;
}

static int stop(void)
{
    return -1;
}

static int go_refresh(void)
{
    if (cellmap_present)
	re_fresh_rast();
    re_fresh_vect();
    return 1;
}

static int do_re_fresh(void)
{
    static Objects objects[] = {
	INFO("Refresh display? ", &use),
	MENU("NO", dont_stop, &use),
	MENU("YES", go_refresh, &use),
	{0}
    };

    Input_pointer(objects);
    return 0;			/* don't quit */
}
