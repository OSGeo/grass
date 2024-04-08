#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include <grass/parson.h>

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

void list_accessible_mapsets_json()
{
    const char *name;
    char *serialized_string = NULL;
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;
    JSON_Array *mapsets = NULL;

    root_value = json_value_init_object();
    root_object = json_value_get_object(root_value);

    // Create and add mapsets array to root object
    json_object_set_value(root_object, "mapsets", json_value_init_array());
    mapsets = json_object_get_array(root_object, "mapsets");

    // Check that memory was allocated to root json object and array
    if (root_value == NULL || mapsets == NULL) {
        G_fatal_error(_("Failed to initialize JSON. Out of memory?"));
    }

    // Add mapsets to mapsets array
    for (int n = 0; (name = G_get_mapset_name(n)); n++) {
        // Append mapset name to mapsets array
        json_array_append_string(mapsets, name);
    }

    // Serialize root object to string and print it to stdout
    serialized_string = json_serialize_to_string_pretty(root_value);
    puts(serialized_string);

    // Free memory
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

void list_avaliable_mapsets_json(const char **mapset_name, int nmapsets)
{
    char *serialized_string = NULL;
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;
    JSON_Array *mapsets = NULL;

    root_value = json_value_init_object();
    root_object = json_value_get_object(root_value);

    // Create mapsets array
    json_object_set_value(root_object, "mapsets", json_value_init_array());
    mapsets = json_object_get_array(root_object, "mapsets");

    // Check that memory was allocated to root json object and array
    if (root_value == NULL || mapsets == NULL) {
        G_fatal_error(_("Failed to initialize JSON. Out of memory?"));
    }

    // Append mapsets to mapsets array
    for (int n = 0; n < nmapsets; n++) {
        json_array_append_string(mapsets, mapset_name[n]);
    }

    // Serialize root object to string and print it to stdout
    serialized_string = json_serialize_to_string_pretty(root_value);
    puts(serialized_string);

    // Free memory
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}
