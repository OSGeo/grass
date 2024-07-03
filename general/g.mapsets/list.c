#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include <grass/parson.h>

// Function to initialize a JSON object with a mapsets array
static JSON_Object *initialize_json_object(void)
{
    JSON_Value *root_value = json_value_init_object();
    if (!root_value) {
        G_fatal_error(_("Failed to initialize JSON object. Out of memory?"));
    }

    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_value(root_object, "mapsets", json_value_init_array());

    JSON_Array *mapsets = json_object_get_array(root_object, "mapsets");
    if (!mapsets) {
        json_value_free(root_value);
        G_fatal_error(_("Failed to initialize mapsets array. Out of memory?"));
    }

    return root_object;
}

// Function to serialize and print JSON object
static void serialize_and_print_json_object(JSON_Value *root_value)
{
    char *serialized_string = json_serialize_to_string_pretty(root_value);
    if (!serialized_string) {
        json_value_free(root_value);
        G_fatal_error(_("Failed to serialize JSON to pretty format."));
    }

    fprintf(stdout, "%s\n", serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

void list_available_mapsets(const char **mapset_name, int nmapsets,
                            const char *fs)
{
    G_message(_("Available mapsets:"));

    for (int n = 0; n < nmapsets; n++) {
        fprintf(stdout, "%s", mapset_name[n]);
        if (n < nmapsets - 1) {
            fprintf(stdout, "%s", fs);
        }
    }
    fprintf(stdout, "\n");
}

void list_accessible_mapsets(const char *fs)
{
    const char *name;

    G_message(_("Accessible mapsets:"));

    for (int n = 0; (name = G_get_mapset_name(n)); n++) {
        /* match each mapset to its numeric equivalent */
        fprintf(stdout, "%s", name);
        if (G_get_mapset_name(n + 1)) {
            fprintf(stdout, "%s", fs);
        }
    }
    fprintf(stdout, "\n");
}

// Lists all accessible mapsets in JSON format
void list_accessible_mapsets_json(void)
{
    const char *name;
    JSON_Object *root_object = initialize_json_object();
    JSON_Array *mapsets = json_object_get_array(root_object, "mapsets");

    for (int n = 0; (name = G_get_mapset_name(n)); n++) {
        json_array_append_string(mapsets, name);
    }

    serialize_and_print_json_object(
        json_object_get_wrapping_value(root_object));
}

// Lists available mapsets from a provided array in JSON format
void list_avaliable_mapsets_json(const char **mapset_names, int nmapsets)
{
    JSON_Object *root_object = initialize_json_object();
    JSON_Array *mapsets = json_object_get_array(root_object, "mapsets");

    for (int n = 0; n < nmapsets; n++) {
        json_array_append_string(mapsets, mapset_names[n]);
    }

    serialize_and_print_json_object(
        json_object_get_wrapping_value(root_object));
}
