#include <stdio.h>
#include <string.h>
#include "G3d_intern.h"
#include "g3dkeys.h"

/*---------------------------------------------------------------------------*/

int
G3d_keyGetInt  (struct Key_Value *keys, char *key, int *i)

{
  char msg[1024];
  char *str;

  if ((str = G_find_key_value (key, keys)) == NULL) {
    sprintf (msg, 
	     "G3d_keyGetInt: cannot find field %s in key structure", key);
    G3d_error (msg);
    return 0;
  }
  
  G_strip(str);
  if (sscanf (str, "%d", i) == 1) return 1;

  sprintf (msg, 
	   "G3d_keyGetInt: invalid value: field %s in key structure", key);
  G3d_error (msg);
  return 0;
}

/*---------------------------------------------------------------------------*/

int
G3d_keyGetDouble  (struct Key_Value *keys, char *key, double *d)

{
  char msg[1024];
  char *str;

  if ((str = G_find_key_value (key, keys)) == NULL) {
    sprintf (msg, 
	     "G3d_keyGetDouble: cannot find field %s in key structure", key);
    G3d_error (msg);
    return 0;
  }
  
  G_strip(str);
  if (sscanf (str, "%lf", d) == 1) return 1;

  sprintf (msg, 
	   "G3d_keyGetDouble: invalid value: field %s in key structure", key);
  G3d_error (msg);
  return 0;
}

/*---------------------------------------------------------------------------*/

int
G3d_keyGetString  (struct Key_Value *keys, char *key, char **returnStr)

{
  char msg[1024];
  char *str;

  if ((str = G_find_key_value (key, keys)) == NULL) {
    sprintf (msg, 
	     "G3d_keyGetString: cannot find field %s in key structure", key);
    G3d_error (msg);
    return 0;
  }

  G_strip(str);
  *returnStr = G_store (str);
  return 1;
}

/*---------------------------------------------------------------------------*/

int
G3d_keyGetValue  (struct Key_Value *keys, char *key, char *val1, char *val2, int result1, int result2, int *resultVar)

{
  char msg[1024];
  char *str;

  if ((str = G_find_key_value (key, keys)) == NULL) {
    sprintf (msg, 
	     "G3d_keyGetValue: cannot find field %s in key structure", key);
    G3d_error (msg);
    return 0;
  }

  G_strip(str);
  if (strcmp (str, val1) == 0) {
    *resultVar = result1;
    return 1;
  }
  if (strcmp (str, val2) == 0) {
    *resultVar = result2;
    return 1;
  }

  sprintf (msg, 
	   "G3d_keyGetValue: invalid type: field %s in key structure", key);
  G3d_error (msg);
  return 0;
}

/*---------------------------------------------------------------------------*/

int
G3d_keySetInt  (struct Key_Value *keys, char *key, int *i)

{
  char keyValStr[200];

  sprintf (keyValStr, "%d", *i);
  return (G_set_key_value (key, keyValStr, keys) != 0);
}

/*---------------------------------------------------------------------------*/

int
G3d_keySetDouble  (struct Key_Value *keys, char *key, double *d)

{
  char keyValStr[200];

  sprintf (keyValStr, "%.50f", *d); 
  return (G_set_key_value (key, keyValStr, keys) != 0);
}

/*---------------------------------------------------------------------------*/

int
G3d_keySetString  (struct Key_Value *keys, char *key, char **keyValStr)

{
  return (G_set_key_value (key, *keyValStr, keys) != 0);
}

/*---------------------------------------------------------------------------*/

int
G3d_keySetValue  (struct Key_Value *keys, char *key, char *val1, char *val2, int keyval1, int keyval2, int *keyvalVar)

{
  if (*keyvalVar == keyval1) 
    return (G_set_key_value (key, val1, keys) != 0);
  if (*keyvalVar == keyval2) 
    return (G_set_key_value (key, val2, keys) != 0);

  G3d_error ("G3d_keySetValue: wrong key value");
  return 0;
}

