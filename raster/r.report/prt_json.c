#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <grass/parson.h>

void serialization_example(void)
{

    // Create root object
    JSON_Value *root_value = json_value_init_object();
    // JSON_Array *maps_arr;
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "location", "nc_spm_08_grass7");
    json_object_set_string(root_object, "created", "Fri Dec 6 17:00:21 2013");
    json_object_dotset_number(root_object, "region.north", 279073.97546639);
    json_object_dotset_number(root_object, "region.south", 113673.97546639);

    // Create array and append map object to array
    json_object_set_value(root_object, "maps", json_value_init_array());
    // maps_arr = json_object_get_array(root_object, "maps");

    // Loop the maps used by r.report to create object and add to array
    //     for (i = 0; i < nmaps; i++) {
    //          // Create map object
    //         JSON_Value *root_map_value = json_value_init_object();
    //         JSON_Object *root_map_object =
    //         json_value_get_object(root_map_value);
    //         json_object_set_string(root_map_object, "name", "South-West Wake
    //         county"); json_object_set_string(root_map_object, "description",
    //         "geology derived from vector map");
    //         json_object_set_string(root_map_object, "layer", "geology_30m");
    //         json_object_set_string(root_map_object, "type", "raster");
    //         json_array_append_value(maps_arr, root_map_value);
    //    }

    serialized_string = json_serialize_to_string_pretty(root_value);
    puts(serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}
