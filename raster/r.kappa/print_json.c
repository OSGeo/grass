#include <stdbool.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gjson.h>
#include "kappa.h"
#include "local_proto.h"

void print_json(void)
{
    FILE *fd;
    G_JSON_Value *root_value = NULL;
    G_JSON_Object *root_object = NULL;

    if (output != NULL)
        fd = fopen(output, "w");
    else
        fd = stdout;

    if (fd == NULL)
        G_fatal_error(_("Cannot open file <%s> to write JSON output"), output);

    root_value = G_json_value_init_object();
    if (!root_value)
        G_fatal_error(_("Failed to initialize JSON object. Out of memory?"));

    root_object = G_json_object(root_value);

    G_json_object_set_string(root_object, "reference", maps[0]);
    G_json_object_set_string(root_object, "classification", maps[1]);
    G_json_object_set_number(root_object, "observations",
                             metrics->observations);
    G_json_object_set_number(root_object, "correct", metrics->correct);

    G_json_object_set_number(root_object, "overall_accuracy",
                             metrics->overall_accuracy);

    if (metrics->kappa == na_value)
        G_json_object_set_null(root_object, "kappa");
    else
        G_json_object_set_number(root_object, "kappa", metrics->kappa);

    if (metrics->kappa_variance == na_value)
        G_json_object_set_null(root_object, "kappa_variance");
    else
        G_json_object_set_number(root_object, "kappa_variance",
                                 metrics->kappa_variance);

    G_JSON_Value *cats_value = G_json_value_init_array();
    G_JSON_Array *cats_array = G_json_array(cats_value);
    for (int i = 0; i < ncat; i++) {
        G_json_array_append_number(cats_array, rlst[i]);
    }
    G_json_object_set_value(root_object, "cats", cats_value);

    G_JSON_Value *matrix_value = G_json_value_init_array();
    G_JSON_Array *matrix_array = G_json_array(matrix_value);

    if (ncat == 0) {
        G_JSON_Value *empty_row_value = G_json_value_init_array();
        G_json_array_append_value(matrix_array, empty_row_value);
    }
    else {
        for (int i = 0; i < ncat; i++) {
            G_JSON_Value *row_value = G_json_value_init_array();
            G_JSON_Array *row_array = G_json_array(row_value);
            for (int j = 0; j < ncat; j++) {
                G_json_array_append_number(row_array,
                                           metrics->matrix[ncat * i + j]);
            }
            G_json_array_append_value(matrix_array, row_value);
        }
    }
    G_json_object_set_value(root_object, "matrix", matrix_value);

    G_JSON_Value *row_sum_value = G_json_value_init_array();
    G_JSON_Array *row_sum_array = G_json_array(row_sum_value);
    for (int i = 0; i < ncat; i++) {
        G_json_array_append_number(row_sum_array, metrics->row_sum[i]);
    }
    G_json_object_set_value(root_object, "row_sum", row_sum_value);

    G_JSON_Value *col_sum_value = G_json_value_init_array();
    G_JSON_Array *col_sum_array = G_json_array(col_sum_value);
    for (int i = 0; i < ncat; i++) {
        G_json_array_append_number(col_sum_array, metrics->col_sum[i]);
    }
    G_json_object_set_value(root_object, "col_sum", col_sum_value);

    G_JSON_Value *prod_acc_value = G_json_value_init_array();
    G_JSON_Array *prod_acc_array = G_json_array(prod_acc_value);
    for (int i = 0; i < ncat; i++) {
        if (metrics->producers_accuracy[i] == na_value)
            G_json_array_append_null(prod_acc_array);
        else
            G_json_array_append_number(prod_acc_array,
                                       metrics->producers_accuracy[i]);
    }
    G_json_object_set_value(root_object, "producers_accuracy", prod_acc_value);

    G_JSON_Value *user_acc_value = G_json_value_init_array();
    G_JSON_Array *user_acc_array = G_json_array(user_acc_value);
    for (int i = 0; i < ncat; i++) {
        if (metrics->users_accuracy[i] == na_value)
            G_json_array_append_null(user_acc_array);
        else
            G_json_array_append_number(user_acc_array,
                                       metrics->users_accuracy[i]);
    }
    G_json_object_set_value(root_object, "users_accuracy", user_acc_value);

    G_JSON_Value *cond_kappa_value = G_json_value_init_array();
    G_JSON_Array *cond_kappa_array = G_json_array(cond_kappa_value);
    for (int i = 0; i < ncat; i++) {
        if (metrics->conditional_kappa[i] == na_value)
            G_json_array_append_null(cond_kappa_array);
        else
            G_json_array_append_number(cond_kappa_array,
                                       metrics->conditional_kappa[i]);
    }
    G_json_object_set_value(root_object, "conditional_kappa", cond_kappa_value);

    if (metrics->mcc == na_value)
        G_json_object_set_null(root_object, "mcc");
    else
        G_json_object_set_number(root_object, "mcc", metrics->mcc);

    char *json_str = G_json_serialize_to_string(root_value);
    if (!json_str) {
        G_json_value_free(root_value);
        G_fatal_error(_("Failed to serialize JSON"));
    }

    fputs(json_str, fd);
    fputc('\n', fd);

    G_json_free_serialized_string(json_str);
    G_json_value_free(root_value);

    if (output != NULL)
        fclose(fd);
}
