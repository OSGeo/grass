#include <grass/gis.h>
static int which_env = -1;	/* 0 = cur, 1 = target */

int select_current_env(void)
{
    if (which_env < 0) {
	G_create_alt_env();
	which_env = 0;
    }
    if (which_env != 0) {
	G_switch_env();
	which_env = 0;
    }

    return 0;
}

int select_target_env(void)
{
    if (which_env < 0) {
	G_create_alt_env();
	which_env = 1;
    }
    if (which_env != 1) {
	G_switch_env();
	which_env = 1;
    }

    return 0;
}
