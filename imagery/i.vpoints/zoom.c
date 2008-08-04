#include "globals.h"

static int cancel(void);

int zoom(void)
{
    static int use = 1;

    static Objects objects[] = {
	MENU("CANCEL", cancel, &use),
	MENU("BOX", zoom_box, &use),
	MENU("POINT", zoom_point, &use),
	INFO("Select type of zoom", &use),
	{0}
    };

    Input_pointer(objects);
    return 0;			/* return, but don't QUIT */
}

static int cancel(void)
{
    return -1;
}
