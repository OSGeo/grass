/*!
 * \file lib/gis/stac.c
 *
 * \brief GIS Library - STAC API functions
 *
 * (C) 2001-2025 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Corey T. White
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/parson.h>

static char *get_file_path(void)
{
    char *path = G_find_etc("stac/stac_api.json");
    if (path == NULL) {
        G_fatal_error(_("Unable to find file: %s"), "stac/stac_api.json");
    }
    return path;
}

/*!
  \brief Read the STAC API JSON file

  The JSON file is expected to be located in the etc/stac directory the GRASS.

  \return JSON_Value pointer to the parsed JSON object
 */
JSON_Value *get_stac_api_json(void)
{
    JSON_Value *stac_json;
    stac_json = json_parse_file(get_file_path());
    if (stac_json == NULL) {
        G_fatal_error(_("Unable to read JSON file: %s"), "stac_api.json");
    }
    /* Caller must free the returned JSON_Value using json_value_free */
    return stac_json;
}

/*!
  \brief Get the STAC API options

  Provides STAC API URLs as a comma-separated string.
  The list of URLs is read from the JSON file stac_api.json that was
  sourced from https://stacindex.org/catalogs?access=public&type=api on
  2025-04-02. The list is not exhaustive, only including public, STAC APIs
  and may be updated in the future.

  \return allocated string buffer with options representing the STAC API URLs
 */
char *G_stac_api_options(void)
{
    JSON_Value *stac_json = get_stac_api_json();
    if (json_value_get_type(stac_json) != JSONArray) {
        json_value_free(stac_json);
        G_fatal_error(_("Invalid JSON format"));
    }
    char *buf = NULL;
    size_t i;
    JSON_Object *api;
    JSON_Array *apis = json_value_get_array(stac_json);
    for (i = 0; i < json_array_get_count(apis); i++) {
        api = json_array_get_object(apis, i);
        const char *url = json_object_get_string(api, "url");
        if (url) {
            if (buf) {
                char *temp = G_store(buf);
                G_free(buf);
                buf = G_malloc(strlen(temp) + strlen(url) +
                               2); // +2 for newline and null terminator
                sprintf(buf, "%s,%s", temp, url);
                G_free(temp);
            }
            else {
                buf = G_store(url);
            }
        }
    }

    json_value_free(stac_json);
    return buf;
}

/*!
  \brief Get the STAC API descriptions

  \return allocated string buffer with options
 */
char *G_stac_api_descriptions(void)
{
    JSON_Value *stac_json = get_stac_api_json();
    if (json_value_get_type(stac_json) != JSONArray) {
        json_value_free(stac_json);
        G_fatal_error(_("Invalid JSON format"));
    }
    char *buf = NULL;
    size_t i;
    JSON_Object *api;
    JSON_Array *apis = json_value_get_array(stac_json);
    for (i = 0; i < json_array_get_count(apis); i++) {
        api = json_array_get_object(apis, i);
        const char *name = json_object_get_string(api, "url");
        const char *desc = json_object_get_string(api, "tittle");

        if (!desc) {
            desc = _("no description");
        }

        if (name) {
            if (buf) {
                char *temp = G_store(buf);
                G_free(buf);
                buf = G_malloc(strlen(temp) + strlen(name) + strlen(desc) +
                               3); // 2 semicolons + null terminator
                sprintf(buf, "%s;%s;%s", temp, name, desc);
                G_free(temp);
            }
            else {
                buf = G_malloc(
                    strlen(name) + strlen(desc) +
                    2); // 1 semicolon between name and desc + null terminator
                sprintf(buf, "%s;%s", name, desc);
            }
        }
    }

    json_value_free(stac_json);
    return buf; // Caller must free this string
}
