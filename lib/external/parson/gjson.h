#ifndef GRASS_GJSON_H
#define GRASS_GJSON_H

#include <stddef.h>

/* *************************************************************** */
/* ***** WRAPPER FOR PARSON FUNCTIONS USED IN GRASS ************** */
/* *************************************************************** */

typedef struct G_json_object_t G_JSON_Object;
typedef struct G_json_array_t G_JSON_Array;
typedef struct G_json_value_t G_JSON_Value;

enum g_json_value_type {
    G_JSONError = -1,
    G_JSONNull = 1,
    G_JSONString = 2,
    G_JSONNumber = 3,
    G_JSONObject = 4,
    G_JSONArray = 5,
    G_JSONBoolean = 6
};
typedef int G_JSON_Value_Type;

enum g_json_result_t { G_JSONSuccess = 0, G_JSONFailure = -1 };
typedef int G_JSON_Status;

extern G_JSON_Value *G_json_value_init_object(void);
extern G_JSON_Value *G_json_value_init_array(void);

extern G_JSON_Value_Type G_json_value_get_type(const G_JSON_Value *value);
extern G_JSON_Object *G_json_value_get_object(const G_JSON_Value *);
extern G_JSON_Object *G_json_object(const G_JSON_Value *);
extern G_JSON_Object *G_json_object_get_object(const G_JSON_Object *,
                                               const char *);
extern G_JSON_Array *G_json_object_get_array(const G_JSON_Object *,
                                             const char *);
extern G_JSON_Value *G_json_object_get_value(const G_JSON_Object *,
                                             const char *);
extern const char *G_json_object_get_string(const G_JSON_Object *,
                                            const char *);
extern double G_json_object_get_number(const G_JSON_Object *, const char *);
extern int G_json_object_get_boolean(const G_JSON_Object *, const char *);
extern G_JSON_Value *G_json_object_get_wrapping_value(const G_JSON_Object *);
extern G_JSON_Status G_json_object_set_value(G_JSON_Object *, const char *,
                                             G_JSON_Value *);
extern G_JSON_Status G_json_object_set_string(G_JSON_Object *, const char *,
                                              const char *);
extern G_JSON_Status G_json_object_set_number(G_JSON_Object *, const char *,
                                              double);
extern G_JSON_Status G_json_object_set_boolean(G_JSON_Object *, const char *,
                                               int);
extern G_JSON_Status G_json_object_set_null(G_JSON_Object *, const char *);

extern G_JSON_Status G_json_object_dotset_string(G_JSON_Object *, const char *,
                                                 const char *);
extern const char *G_json_object_dotget_string(G_JSON_Object *, const char *);
extern G_JSON_Status G_json_object_dotset_number(G_JSON_Object *, const char *,
                                                 double);
extern double G_json_object_dotget_number(G_JSON_Object *, const char *);
extern G_JSON_Status G_json_object_dotset_null(G_JSON_Object *object,
                                               const char *name);
extern G_JSON_Array *G_json_array(const G_JSON_Value *);
extern G_JSON_Value *G_json_array_get_value(const G_JSON_Array *, size_t);
extern const char *G_json_array_get_string(const G_JSON_Array *, size_t);
extern double G_json_array_get_number(const G_JSON_Array *, size_t);
extern int G_json_array_get_boolean(const G_JSON_Array *, size_t);

extern G_JSON_Status G_json_array_append_value(G_JSON_Array *, G_JSON_Value *);
extern G_JSON_Status G_json_array_append_string(G_JSON_Array *, const char *);
extern G_JSON_Status G_json_array_append_number(G_JSON_Array *, double);
extern G_JSON_Status G_json_array_append_boolean(G_JSON_Array *, int);
extern G_JSON_Status G_json_array_append_null(G_JSON_Array *);

extern void G_json_set_float_serialization_format(const char *format);
extern char *G_json_serialize_to_string_pretty(const G_JSON_Value *);
extern char *G_json_serialize_to_string(const G_JSON_Value *);
extern void G_json_free_serialized_string(char *);
extern void G_json_value_free(G_JSON_Value *);

#endif /* GRASS_GJSON_H */
