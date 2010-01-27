#include <stdio.h>
#include <string.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

int G3d_keyGetInt(struct Key_Value *keys, const char *key, int *i)
{
    const char *str;

    if ((str = G_find_key_value(key, keys)) == NULL) {
	G3d_error("G3d_keyGetInt: cannot find field %s in key structure",
		  key);
	return 0;
    }

    if (sscanf(str, "%d", i) == 1)
	return 1;

    G3d_error("G3d_keyGetInt: invalid value: field %s in key structure", key);
    return 0;
}

/*---------------------------------------------------------------------------*/

int G3d_keyGetDouble(struct Key_Value *keys, const char *key, double *d)
{
    const char *str;

    if ((str = G_find_key_value(key, keys)) == NULL) {
	G3d_error("G3d_keyGetDouble: cannot find field %s in key structure",
		  key);
	return 0;
    }

    if (sscanf(str, "%lf", d) == 1)
	return 1;

    G3d_error("G3d_keyGetDouble: invalid value: field %s in key structure",
	      key);
    return 0;
}

/*---------------------------------------------------------------------------*/

int
G3d_keyGetString(struct Key_Value *keys, const char *key, char **returnStr)
{
    const char *str;

    if ((str = G_find_key_value(key, keys)) == NULL) {
	G3d_error("G3d_keyGetString: cannot find field %s in key structure",
		  key);
	return 0;
    }

    *returnStr = G_store(str);
    return 1;
}

/*---------------------------------------------------------------------------*/

int
G3d_keyGetValue(struct Key_Value *keys, const char *key, char *val1,
		char *val2, int result1, int result2, int *resultVar)
{
    const char *str;

    if ((str = G_find_key_value(key, keys)) == NULL) {
	G3d_error("G3d_keyGetValue: cannot find field %s in key structure",
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

    G3d_error("G3d_keyGetValue: invalid type: field %s in key structure",
	      key);
    return 0;
}

/*---------------------------------------------------------------------------*/

int G3d_keySetInt(struct Key_Value *keys, const char *key, const int *i)
{
    char keyValStr[200];

    sprintf(keyValStr, "%d", *i);
    G_set_key_value(key, keyValStr, keys);
    return 1;
}

/*---------------------------------------------------------------------------*/

int G3d_keySetDouble(struct Key_Value *keys, const char *key, const double *d)
{
    char keyValStr[200];

    sprintf(keyValStr, "%.50f", *d);
    G_set_key_value(key, keyValStr, keys);
    return 1;
}

/*---------------------------------------------------------------------------*/

int
G3d_keySetString(struct Key_Value *keys, const char *key,
		 char *const *keyValStr)
{
    G_set_key_value(key, *keyValStr, keys);
    return 1;
}

/*---------------------------------------------------------------------------*/

int
G3d_keySetValue(struct Key_Value *keys, const char *key, const char *val1,
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

    G3d_error("G3d_keySetValue: wrong key value");
    return 0;
}
