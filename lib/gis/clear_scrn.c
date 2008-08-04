
/**********************************************************************
 *
 *   G_clear_screen()
 *
 *   clears the terminal screen
 *
 **********************************************************************/
#include <stdlib.h>
#include <grass/gis.h>

int G_clear_screen(void)
{
#ifdef __MINGW32__
    system("cls");
#else
    system("clear");
#endif

    return 0;
}
