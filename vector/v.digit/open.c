#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "global.h"
#include "proto.h"

int first = 1;

/* Open new form
 *
 *  returns: 0 success
 */
int F_open(char *title, char *html)
{
    int len;
    char *buf;

    if (first) {
	Tcl_Eval(Toolbox, "init_form");
	first = 0;
    }

    G_debug(2, "PARENT HTML:\n%s\n", html);

    len = strlen(title) + strlen(html) + 20;
    buf = G_malloc(len);
    sprintf(buf, "open_form {%s} {%s}", title, html);
    Tcl_Eval(Toolbox, buf);
    G_free(buf);

    return 0;
}

/* Clear old forms from window
 *
 */
void F_clear(void)
{
    G_debug(2, "F_clear()");

    if (first)
	return;

    Tcl_Eval(Toolbox, "clear_form");
}

void F_close(void)
{
    G_debug(2, "F_close()");

    if (first)
	return;

    Tcl_Eval(Toolbox, "done_form");

    first = 1;
}
