#include <grass/gis.h>
#include <grass/glocale.h>
#include "profile.h"

int is_null_value(RASTER_MAP_PTR * ptr, int col)
{
    if (ptr == NULL)
	G_fatal_error(_("%s: 'is_null_value()' got NULL pointer!"),
		      G_program_name());
    if (col < 0)
	G_fatal_error(_("%s: 'is_null_value()' got negative column index"),
		      G_program_name());

    switch (ptr->type) {
    case CELL_TYPE:
	return G_is_c_null_value(&ptr->data.c[col]);
    case FCELL_TYPE:
	return G_is_f_null_value(&ptr->data.f[col]);
    case DCELL_TYPE:
	return G_is_d_null_value(&ptr->data.d[col]);
    default:
	G_fatal_error(_("%s: 'is_null_value()' Unknown "
			"RASTER_MAP_TYPE '%d'"), G_program_name(), ptr->type);
    }

    return 0;
}

/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
