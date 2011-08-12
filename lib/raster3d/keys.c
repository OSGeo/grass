#include <stdio.h>
#include <string.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int Rast3d_key_get_int(struct Key_Value *keys, const char *key, int *i)
{
    const char *str;

    if ((str = G_find_key_value(key, keys)) == NULL) {
	Rast3d_error("Rast3d_key_get_int: cannot find field %s in key structure",
		  key);
	return 0;
    }

    if (sscanf(str, "%d", i) == 1)
	return 1;

    Rast3d_error("Rast3d_key_get_int: invalid value: field %s in key structure", key);
    return 0;
}

/*---------------------------------------------------------------------------*/

int Rast3d_key_get_double(struct Key_Value *keys, const char *key, double *d)
{
    const char *str;

    if ((str = G_find_key_value(key, keys)) == NULL) {
	Rast3d_error("Rast3d_key_get_double: cannot find field %s in key structure",
		  key);
	return 0;
    }

    if (sscanf(str, "%lf", d) == 1)
	return 1;

    Rast3d_error("Rast3d_key_get_double: invalid value: field %s in key structure",
	      key);
    return 0;
}

/*---------------------------------------------------------------------------*/

int
Rast3d_key_get_string(struct Key_Value *keys, const char *key, char **returnStr)
{
    const char *str;

    if ((str = G_find_key_value(key, keys)) == NULL) {
	Rast3d_error("Rast3d_key_get_string: cannot find field %s in key structure",
		  key);
	return 0;
    }

    *returnStr = G_store(str);
    return 1;
}

/*---------------------------------------------------------------------------*/

int
Rast3d_key_get_value(struct Key_Value *keys, const char *key, char *val1,
		char *val2, int result1, int result2, int *resultVar)
{
    const char *str;

    if ((str = G_find_key_value(key, keys)) == NULL) {
	Rast3d_error("Rast3d_key_get_value: cannot find field %s in key structure",
		  key);
	return 0;
    }

    if (strcmp(str, val1) == 0) {
	*resultVar = result1;
	return 1;
    }
    if (strcmp(str, val2) == 0) {
	*resultVar = result2;
	return 1;
    }

    Rast3d_error("Rast3d_key_get_value: invalid type: field %s in key structure",
	      key);
    return 0;
}

/*---------------------------------------------------------------------------*/

int Rast3d_key_set_int(struct Key_Value *keys, const char *key, const int *i)
{
    char keyValStr[200];

    sprintf(keyValStr, "%d", *i);
    G_set_key_value(key, keyValStr, keys);
    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_key_set_double(struct Key_Value *keys, const char *key, const double *d)
{
    char keyValStr[200];

    sprintf(keyValStr, "%.50f", *d);
    G_set_key_value(key, keyValStr, keys);
    return 1;
}

/*---------------------------------------------------------------------------*/

int
Rast3d_key_set_string(struct Key_Value *keys, const char *key,
		 char *const *keyValStr)
{
    G_set_key_value(key, *keyValStr, keys);
    return 1;
}

/*---------------------------------------------------------------------------*/

int
Rast3d_key_set_value(struct Key_Value *keys, const char *key, const char *val1,
		const char *val2, int keyval1, int keyval2,
		const int *keyvalVar)
{
    if (*keyvalVar == keyval1) {
	G_set_key_value(key, val1, keys);
	return 1;
    }

    if (*keyvalVar == keyval2) {
	G_set_key_value(key, val2, keys);
	return 1;
    }

    Rast3d_error("Rast3d_key_set_value: wrong key value");
    return 0;
}
