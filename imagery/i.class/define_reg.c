#include "globals.h"
#include "local_proto.h"


static int use = 1;

/* function prototypes */
static int done(void);


int define_region(void)
{
    static Objects objects[] = {
	INFO("Region Menu:", &use),
	MENU(" Erase region ", erase_region, &use),
	MENU(" Draw region ", draw_region, &use),
	MENU(" Restore last region ", restore_region, &use),
	MENU(" Complete region ", complete_region, &use),
	MENU(" Done ", done, &use),
	{0}
    };

    Input_pointer(objects);

    return (0);
}


static int done(void)
{
    return (-1);
}
