#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/parson.h>
#include "kappa.h"
#include "local_proto.h"

void print_json(void)
{
    FILE *fd = NULL;
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;
    JSON_Value *array_value = NULL;
    JSON_Array *array = NULL;
    char *json_text = NULL;
    bool first;

    if (output != NULL)
        fd = fopen(output, "w");
    if (!fd)
        G_fatal_error(_("Cannot open file <%s> to write JSON output"), output);

    root_value = json_value_init_object();
    root_object = json_value_get_object(root_value);

    json_object_set_string(root_object, "reference", maps[0]);
    json_object_set_string(root_object, "classification", maps[1]);
    json_object_set_number(root_object, "observations", (double)metrics->observations);
    json_object_set_number(root_object, "correct", (double)metrics->correct);
    json_object_set_number(root_object, "overall_accuracy", metrics->overall_accuracy);

    if (metrics->kappa == na_value)
        json_object_set_null(root_object, "kappa");
    else
        json_object_set_number(root_object, "kappa", metrics->kappa);

    if (metrics->kappa_variance == na_value)
        json_object_set_null(root_object, "kappa_variance");
    else
        json_object_set_number(root_object, "kappa_variance", metrics->kappa_variance);

    array_value = json_value_init_array();
    array = json_value_get_array(array_value);
    for (int i = 0; i < ncat; i++)
        json_array_append_number(array, (double)rlst[i]);
    json_object_set_value(root_object, "cats", array_value);

    array_value = json_value_init_array();
    array = json_value_get_array(array_value);
    for (int i = 0; i < ncat; i++) {
        JSON_Value *row_value = json_value_init_array();
        JSON_Array *row_array = json_value_get_array(row_value);
        for (int j = 0; j < ncat; j++)
            json_array_append_number(row_array, (double)metrics->matrix[ncat * i + j]);
        json_array_append_value(array, row_value);
    }
    json_object_set_value(root_object, "matrix", array_value);

    array_value = json_value_init_array();
    array = json_value_get_array(array_value);
    for (int i = 0; i < ncat; i++)
        json_array_append_number(array, (double)metrics->row_sum[i]);
    json_object_set_value(root_object, "row_sum", array_value);

    array_value = json_value_init_array();
    array = json_value_get_array(array_value);
    for (int i = 0; i < ncat; i++)
        json_array_append_number(array, (double)metrics->col_sum[i]);
    json_object_set_value(root_object, "col_sum", array_value);

    array_value = json_value_init_array();
    array = json_value_get_array(array_value);
    for (int i = 0; i < ncat; i++)
        metrics->producers_accuracy[i] == na_value
            ? json_array_append_null(array)
            : json_array_append_number(array, metrics->producers_accuracy[i]);
    json_object_set_value(root_object, "producers_accuracy", array_value);
    array_value = json_value_init_array();
    array = json_value_get_array(array_value);
    for (int i = 0; i < ncat; i++)
        metrics->users_accuracy[i] == na_value
            ? json_array_append_null(array)
            : json_array_append_number(array, metrics->users_accuracy[i]);
    json_object_set_value(root_object, "users_accuracy", array_value);
    array_value = json_value_init_array();
    array = json_value_get_array(array_value);
    for (int i = 0; i < ncat; i++)
        metrics->conditional_kappa[i] == na_value
            ? json_array_append_null(array)
            : json_array_append_number(array, metrics->conditional_kappa[i]);
    json_object_set_value(root_object, "conditional_kappa", array_value);
    if (metrics->mcc == na_value)
        json_object_set_null(root_object, "mcc");
    else
        json_object_set_number(root_object, "mcc", metrics->mcc);
    json_text = json_serialize_to_string_pretty(root_value);
    fprintf(fd, "%s\n", json_text);
    json_free_serialized_string(json_text);
    json_value_free(root_value);

    if (fd)
        fclose(fd);
}
