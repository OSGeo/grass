#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/graphics.h>
#include "transport.h"

/* PAD FUNCTIONS
   The monitor has a very simple database management capability
   which supports the windowing.  There are scratch pads
   to be written on. Each scratch pad can contain items,  and
   each  item can have a list of values.  These are NOT to be
   used by the programmer.  They are used indirectly  through
   the displaylib library calls.
*/

int R_pad_create(const char *pad)
{
	return trans->pad_create(pad);
}

int R_pad_current(char *name)
{
	return trans->pad_current(name);
}

int R_pad_delete(void)
{
	return trans->pad_delete();
}

int R_pad_invent(char *pad)
{
	return trans->pad_invent(pad);
}

int R_pad_list(char ***list, int *count)
{
	return trans->pad_list(list, count);
}

int R_pad_select(const char *pad)
{
	return trans->pad_select(pad);
}

int R_pad_append_item(const char *item, const char *value, int replace)
{
	return trans->pad_append_item(item, value, replace);
}

int R_pad_delete_item(const char *name)
{
	return trans->pad_delete_item(name);
}

int R_pad_get_item(const char *name, char ***list, int *count)
{
	return trans->pad_get_item(name, list, count);
}

int R_pad_list_items(char ***list, int *count)
{
	return trans->pad_list_items(list, count);
}

int R_pad_set_item(const char *name, const char *value)
{
	return trans->pad_set_item(name, value);
}

