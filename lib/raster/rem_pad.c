#include <grass/config.h>

#ifdef HAVE_SOCKET

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/graphics.h>

#include "transport.h"

/* PAD FUNCTIONS
   The monitor has a very simple database management capabil­
   ity  which supports the windowing.  There are scratch pads
   to be written on. Each scratch pad can contain items,  and
   each  item can have a list of values.  These are NOT to be
   used by the programmer.  They are used indirectly  through
   the displaylib library calls.
 */

static void _get_list(char ***list, int *count)
{
    char **a;
    int n;
    char *buf;

    *list = NULL;
    *count = 0;

    buf = _get_text_2();

    for (n = 0; *buf; n++) {
	if (n == 0)
	    a = G_malloc(sizeof(char *));
	else
	    a = G_realloc(a, (n + 1) * sizeof(char *));

	a[n] = G_strdup(buf);

	buf = _get_text_2();
    }

    *list = a;
    *count = n;
}

int REM_pad_create(const char *pad)
{
    char result;

    _hold_signals(1);

    _send_ident(PAD_CREATE);
    _send_text(pad);
    _get_char(&result);

    _hold_signals(0);

    return result;
}

int REM_pad_current(char *name)
{
    char result;

    _hold_signals(1);

    _send_ident(PAD_CURRENT);
    _get_char(&result);
    _get_text(name);

    _hold_signals(0);

    return result;
}

int REM_pad_delete(void)
{
    char result;

    _hold_signals(1);

    _send_ident(PAD_DELETE);
    _get_char(&result);

    _hold_signals(0);

    return result;
}

int REM_pad_invent(char *pad)
{
    _hold_signals(1);

    _send_ident(PAD_INVENT);
    _get_text(pad);

    _hold_signals(0);

    return 0;
}

int REM_pad_list(char ***list, int *count)
{
    _hold_signals(1);

    _send_ident(PAD_LIST);
    _get_list(list, count);

    _hold_signals(0);

    return 0;
}

int REM_pad_select(const char *pad)
{
    char result;

    _hold_signals(1);

    _send_ident(PAD_SELECT);
    _send_text(pad);
    _get_char(&result);

    _hold_signals(0);

    return result;
}

int REM_pad_append_item(const char *item, const char *value, int replace)
{
    char result;

    _hold_signals(1);

    _send_ident(PAD_APPEND_ITEM);
    _send_text(item);
    _send_text(value);
    _send_int(&replace);
    _get_char(&result);

    _hold_signals(0);

    return result;
}

int REM_pad_delete_item(const char *name)
{
    char result;

    _hold_signals(1);

    _send_ident(PAD_DELETE_ITEM);
    _send_text(name);
    _get_char(&result);

    _hold_signals(0);

    return result;
}

int REM_pad_get_item(const char *item, char ***list, int *count)
{
    char result;

    _hold_signals(1);

    _send_ident(PAD_GET_ITEM);
    _send_text(item);
    _get_char(&result);

    if (result == OK)
	_get_list(list, count);

    _hold_signals(0);

    return result;
}

int REM_pad_list_items(char ***list, int *count)
{
    char result;

    _hold_signals(1);

    _send_ident(PAD_LIST_ITEMS);
    _get_char(&result);
    if (result == OK)
	_get_list(list, count);

    _hold_signals(0);

    return result;
}

int REM_pad_set_item(const char *item, const char *value)
{
    char result;

    _hold_signals(1);

    _send_ident(PAD_SET_ITEM);
    _send_text(item);
    _send_text(value);
    _get_char(&result);

    _hold_signals(0);

    return result;
}

#endif /* HAVE_SOCKET */
