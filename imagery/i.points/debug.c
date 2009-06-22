
#include "globals.h"
#include "local_proto.h"
#include <grass/display.h>
int debug(char *msg)
{
    R_stabilize();
    Curses_write_window(PROMPT_WINDOW, 1, 1, msg);
    Curses_getch(0);

    return 0;
}
