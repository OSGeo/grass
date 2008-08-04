#include "globals.h"

static int use = 1;
static int really_quit(void);
static int dont_stop(void);
static int stop(void);

int driver(void)
{
    static Objects objects[] = {
	MENU("QUIT", really_quit, &use),
	MENU("ZOOM", zoom, &use),
	MENU("PLOT CELL", plotcell, &use),
	MENU("CAMERA", drawcam, &use),
	MENU("ANALYZE", analyze, &use),
	INFO("  Input method -> ", &use),
	OPTION("KEYBOARD", 2, &from_keyboard),
	OPTION("CAMERA FILE", 2, &from_screen),
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
	INFO("really quit? ", &use),
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
