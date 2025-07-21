#ifndef GRASS_GJSON_H
#define GRASS_GJSON_H

#include "parson.h"

/* *************************************************************** */
/* ***** WRAPPER FOR PARSON FUNCTIONS USED IN GRASS ************** */
/* *************************************************************** */

extern JSON_Value *G_json_value_init_object(void);
extern JSON_Value *G_json_value_init_array(void);

extern JSON_Object *G_json_value_get_object(const JSON_Value *);
extern JSON_Object *G_json_object(const JSON_Value *);
extern JSON_Object *G_json_object_get_object(const JSON_Object *, const char *);
extern JSON_Array *G_json_object_get_array(const JSON_Object *, const char *);
extern JSON_Value *G_json_object_get_value(const JSON_Object *, const char *);
extern const char *G_json_object_get_string(const JSON_Object *, const char *);
extern double G_json_object_get_number(const JSON_Object *, const char *);
extern int G_json_object_get_boolean(const JSON_Object *, const char *);
extern JSON_Value *G_json_object_get_wrapping_value(const JSON_Object *);

extern JSON_Status G_json_object_set_value(JSON_Object *, const char *,
                                           JSON_Value *);
extern JSON_Status G_json_object_set_string(JSON_Object *, const char *,
                                            const char *);
extern JSON_Status G_json_object_set_number(JSON_Object *, const char *,
                                            double);
extern JSON_Status G_json_object_set_boolean(JSON_Object *, const char *, int);
extern JSON_Status G_json_object_set_null(JSON_Object *, const char *);

extern JSON_Status G_json_object_dotset_string(JSON_Object *, const char *,
                                               const char *);
extern const char *G_json_object_dotget_string(JSON_Object *, const char *);

extern JSON_Array *G_json_array(const JSON_Value *);
extern JSON_Value *G_json_array_get_value(const JSON_Array *, size_t);
extern const char *G_json_array_get_string(const JSON_Array *, size_t);
extern double G_json_array_get_number(const JSON_Array *, size_t);
extern int G_json_array_get_boolean(const JSON_Array *, size_t);

extern JSON_Status G_json_array_append_value(JSON_Array *, JSON_Value *);
extern JSON_Status G_json_array_append_string(JSON_Array *, const char *);
extern JSON_Status G_json_array_append_number(JSON_Array *, double);
extern JSON_Status G_json_array_append_boolean(JSON_Array *, int);
extern JSON_Status G_json_array_append_null(JSON_Array *);

extern char *G_json_serialize_to_string_pretty(const JSON_Value *);
extern void G_json_free_serialized_string(char *);
extern void G_json_value_free(JSON_Value *);

#endif /* GRASS_GJSON_H */
