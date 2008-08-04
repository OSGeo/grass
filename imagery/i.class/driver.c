#include "globals.h"
#include "local_proto.h"


static int use = 1;
static int stop(void);
static int really_quit(void);
static int dont_stop(void);


int driver(void)
{
    static Objects objects[] = {
	INFO("Command Menu:", &use),
	MENU(" Zoom ", zoom_box, &use),
	MENU(" Define region ", define_region, &use),
	MENU(" Redisplay map ", redisplay, &use),
	MENU(" Analyze region ", analyze_sig, &use),
	MENU(" Quit ", really_quit, &use),
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
	MENU(" No ", dont_stop, &use),
	MENU(" Yes ", stop, &use),
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
