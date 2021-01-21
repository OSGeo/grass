/*!
   \file lib/gis/parser_json.c

   \brief GIS Library - converts the command line arguments into actinia JSON process
   chain building blocks

   (C) 2018-2021 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Soeren Gebbert
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/glocale.h>
#include <grass/gis.h>

#include "parser_local_proto.h"

void check_create_import_opts(struct Option *, char *, FILE *);
void check_create_export_opts(struct Option *, char *, FILE *);
char *check_mapset_in_layer_name(char *, int);

/*!
   \brief This function generates actinia JSON process chain building blocks
   from the command line arguments that can be used in the actinia processing API.

   The following commands will create according JSON output:

   r.slope.aspect elevation="elevation@https://storage.googleapis.com/graas-geodata/elev_ned_30m.tif" slope="slope+GTiff" aspect="aspect+GTiff" --json

    {
      "module": "r.slope.aspect",
      "id": "r.slope.aspect_1804289383",
      "inputs":[
         {"import_descr": {"source":"https://storage.googleapis.com/graas-geodata/elev_ned_30m.tif", "type":"raster"},
          "param": "elevation", "value": "elevation"},
         {"param": "format", "value": "degrees"},
         {"param": "precision", "value": "FCELL"},
         {"param": "zscale", "value": "1.0"},
         {"param": "min_slope", "value": "0.0"}
       ],
      "outputs":[
         {"export": {"format":"GTiff", "type":"raster"},
          "param": "slope", "value": "slope"},
         {"export": {"format":"GTiff", "type":"raster"},
          "param": "aspect", "value": "aspect"}
       ]
    }

   v.out.ascii input="hospitals@PERMANENT" output="myfile+TXT" --json

    {
      "module": "v.out.ascii",
      "id": "v.out.ascii_1804289383",
      "inputs":[
         {"param": "input", "value": "hospitals@PERMANENT"},
         {"param": "layer", "value": "1"},
         {"param": "type", "value": "point,line,boundary,centroid,area,face,kernel"},
         {"param": "format", "value": "point"},
         {"param": "separator", "value": "pipe"},
         {"param": "precision", "value": "8"}
       ],
      "outputs":[
         {"export": {"format":"TXT", "type":"file"},
          "param": "output", "value": "$file::myfile"}
       ]
    }

   v.info map="hospitals@PERMANENT" -c --json

    {
      "module": "v.info",
      "id": "v.info_1804289383",
      "flags":"c",
      "inputs":[
         {"param": "map", "value": "hospitals@PERMANENT"},
         {"param": "layer", "value": "1"}
       ]
    }


   A process chain has the following form

{
    'list': [{
        'module': 'g.region',
        'id': 'g_region_1',
        'inputs': [{'import_descr': {'source': 'https://storage.googleapis.com/graas-geodata/elev_ned_30m.tif',
                                     'type': 'raster'},
                    'param': 'raster',
                    'value': 'elev_ned_30m_new'}],
        'flags': 'p'
        },
        {
            'module': 'r.slope.aspect',
            'id': 'r_slope_aspect_1',
            'inputs': [{'param': 'elevation',
                        'value': 'elev_ned_30m_new'}],
            'outputs': [{'export': {'format': 'GTiff',
                                    'type': 'raster'},
                         'param': 'slope',
                         'value': 'elev_ned_30m_new_slope'}],
            'flags': 'a'},
        {
            'module': 'r.univar',
            'id': 'r_univar_1',
            'inputs': [{"import_descr": {"source": "LT52170762005240COA00",
                                         "type": "landsat",
                                         "landsat_atcor": "dos1"},
                        'param': 'map',
                        'value': 'LT52170762005240COA00_dos1.1'}],
            'stdout': {'id': 'stats', 'format': 'kv', 'delimiter': '='},
            'flags': 'a'
        },
        {
            'module': 'exporter',
            'id': 'exporter_1',
            'outputs': [{'export': {'format': 'GTiff',
                                    'type': 'raster'},
                         'param': 'map',
                         'value': 'LT52170762005240COA00_dos1.1'}]
        },
        {
            "id": "ascii_out",
            "module": "r.out.ascii",
            "inputs": [{"param": "input",
                        "value": "elevation@PERMANENT"},
                       {"param": "precision", "value": "0"}],
            "stdout": {"id": "elev_1", "format": "table", "delimiter": " "},
            "flags": "h"
        },
        {
            "id": "ascii_export",
            "module": "r.out.ascii",
            "inputs": [{"param": "input",
                        "value": "elevation@PERMANENT"}],
            "outputs": [
                {"export": {"type": "file", "format": "TXT"},
                 "param": "output",
                 "value": "$file::out1"}
            ]
        },
        {
            "id": "raster_list",
            "module": "g.list",
            "inputs": [{"param": "type",
                        "value": "raster"}],
            "stdout": {"id": "raster", "format": "list", "delimiter": "\n"}
        },
        {
            "module": "r.what",
            "id": "r_what_1",
            "verbose": True,
            "flags": "nfic",
            "inputs": [
                {
                    "param": "map",
                    "value": "landuse96_28m@PERMANENT"
                },
                {
                    "param": "coordinates",
                    "value": "633614.08,224125.12,632972.36,225382.87"
                },
                {
                    "param": "null_value",
                    "value": "null"
                },
                {
                    "param": "separator",
                    "value": "pipe"
                }
            ],
            "stdout": {"id": "sample", "format": "table", "delimiter": "|"}
        }
    ],
    'webhooks': {'update': 'http://business-logic.company.com/api/v1/actinia-update-webhook',
                 'finished': 'http://business-logic.company.com/api/v1/actinia-finished-webhook'},
    'version': '1'
}

*/
char *G__json(void)
{
    FILE *fp = stdout;

    /*FILE *fp = NULL; */
    char *type;
    char *file_name = NULL;
    int c;
    int random_int = rand();
    int num_flags = 0;
    int num_inputs = 0;
    int num_outputs = 0;
    int i = 0;

    char age[KEYLENGTH];
    char element[KEYLENGTH];    /*cell, file, grid3, vector */
    char desc[KEYLENGTH];

    file_name = G_tempfile();

    /* fprintf(stderr, "Filename: %s\n", file_name); */
    fp = fopen(file_name, "w+");
    if (fp == NULL) {
        fprintf(stderr, "Unable to open temporary file <%s>\n", file_name);
        exit(EXIT_FAILURE);
    }

    if (st->n_flags) {
        struct Flag *flag;

        for (flag = &st->first_flag; flag; flag = flag->next_flag) {
            if (flag->answer)
                num_flags += 1;;
        }
    }

    /* Count input and output options */
    if (st->n_opts) {
        struct Option *opt;

        for (opt = &st->first_option; opt; opt = opt->next_opt) {
            if (opt->answer) {
                if (opt->gisprompt) {
                    G__split_gisprompt(opt->gisprompt, age, element, desc);
                    /* fprintf(stderr, "age: %s element: %s desc: %s\n", age, element, desc); */
                    if (G_strncasecmp("new", age, 3) == 0) {
                        /*fprintf(fp, "new: %s\n", opt->gisprompt); */
                        num_outputs += 1;
                    }
                    else {
                        /*fprintf(fp, "%s\n", opt->gisprompt); */
                        num_inputs += 1;
                    }
                }
                else {
                    num_inputs += 1;
                }
            }
        }
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"module\": \"%s\",\n", G_program_name());
    fprintf(fp, "  \"id\": \"%s_%i\"", G_program_name(), random_int);

    if (st->n_flags && num_flags > 0) {
        struct Flag *flag;

        fprintf(fp, ",\n");
        fprintf(fp, "  \"flags\":\"");

        for (flag = &st->first_flag; flag; flag = flag->next_flag) {
            if (flag->answer)
                fprintf(fp, "%c", flag->key);
        }
        fprintf(fp, "\"");
    }

    /* Print the input options
     */
    if (st->n_opts && num_inputs > 0) {
        struct Option *opt;

        i = 0;
        fprintf(fp, ",\n");
        fprintf(fp, "  \"inputs\":[\n");
        for (opt = &st->first_option; opt; opt = opt->next_opt) {
            if (opt->gisprompt) {
                G__split_gisprompt(opt->gisprompt, age, element, desc);
                if (G_strncasecmp("new", age, 3) != 0) {
                    if (opt->answer) {
                        check_create_import_opts(opt, element, fp);
                        i++;
                        if (i < num_inputs) {
                            fprintf(fp, ",\n");
                        }
                        else {
                            fprintf(fp, "\n");
                        }
                    }
                }
            }
            else if (opt->answer) {
                /* Check for input options */
                fprintf(fp, "     {\"param\": \"%s\", ", opt->key);
                fprintf(fp, "\"value\": \"%s\"}", opt->answer);
                i++;
                if (i < num_inputs) {
                    fprintf(fp, ",\n");
                }
                else {
                    fprintf(fp, "\n");
                }
            }
        }
        fprintf(fp, "   ]");
    }

    /* Print the output options
     */
    if (st->n_opts && num_outputs > 0) {
        struct Option *opt;

        i = 0;
        fprintf(fp, ",\n");
        fprintf(fp, "  \"outputs\":[\n");
        for (opt = &st->first_option; opt; opt = opt->next_opt) {
            if (opt->gisprompt) {
                G__split_gisprompt(opt->gisprompt, age, element, desc);
                if (G_strncasecmp("new", age, 3) == 0) {
                    if (opt->answer) {
                        check_create_export_opts(opt, element, fp);
                        i++;
                        if (i < num_outputs) {
                            fprintf(fp, ",\n");
                        }
                        else {
                            fprintf(fp, "\n");
                        }
                    }
                }
            }
        }
        fprintf(fp, "   ]\n");
    }

    fprintf(fp, "}\n");
    fclose(fp);

    /* Print the file content to stdout */
    fp = fopen(file_name, "r");
    if (fp == NULL) {
        fprintf(stderr, "Unable to open temporary file <%s>\n", file_name);
        exit(EXIT_FAILURE);
    }

    c = fgetc(fp);
    while (c != EOF) {
        fprintf(stdout, "%c", c);
        c = fgetc(fp);
    }
    fclose(fp);

    return file_name;
}

/* \brief Check the provided answer and generate the import statement
   dependent on the element type (cell, vector, grid3, file)

   {'import_descr': {'source': 'https://storage.googleapis.com/graas-geodata/elev_ned_30m.tif',
   'type': 'raster'},
   'param': 'map',
   'value': 'elevation'}
 */
void check_create_import_opts(struct Option *opt, char *element, FILE * fp)
{
    int i = 0, urlfound = 0;
    int has_import = 0;
    char **tokens;

    G_debug(2, "tokenize opt string: <%s> with '@'", opt->answer);
    tokens = G_tokenize(opt->answer, "@");
    while (tokens[i]) {
        G_chop(tokens[i]);
        i++;
    }
    if (i > 2)
        G_fatal_error(_("Input string not understood: <%s>. Multiple '@' chars?"),
                      opt->answer);

    if (i > 1) {
        /* check if tokens[1] starts with an URL or name@mapset */
        G_debug(2, "tokens[1]: <%s>", tokens[1]);
        if (strncmp(tokens[1], "http://", 7) == 0 ||
            strncmp(tokens[1], "https://", 8) == 0 ||
            strncmp(tokens[1], "ftp://", 6) == 0) {
            urlfound = 1;
            G_debug(2, "URL found");
        }
        else {
            urlfound = 0;
            G_debug(2, "name@mapset found");
        }
    }

    fprintf(fp, "     {");

    if (i > 1 && urlfound == 1) {
        if (G_strncasecmp("cell", element, 4) == 0) {
            fprintf(fp,
                    "\"import_descr\": {\"source\":\"%s\", \"type\":\"raster\"},\n      ",
                    tokens[1]);
            has_import = 1;
        }
        else if (G_strncasecmp("file", element, 4) == 0) {
            fprintf(fp,
                    "\"import_descr\": {\"source\":\"%s\", \"type\":\"file\"},\n      ",
                    tokens[1]);
            has_import = 1;
        }
        else if (G_strncasecmp("vector", element, 4) == 0) {
            fprintf(fp,
                    "\"import_descr\": {\"source\":\"%s\", \"type\":\"vector\"},\n      ",
                    tokens[1]);
            has_import = 1;
        }
    }

    fprintf(fp, "\"param\": \"%s\", ", opt->key);
    /* In case of import the mapset must be removed always */
    if (urlfound == 1) {
        fprintf(fp, "\"value\": \"%s\"",
                check_mapset_in_layer_name(tokens[0], has_import));
    }
    else {
        fprintf(fp, "\"value\": \"%s\"",
                check_mapset_in_layer_name(opt->answer, has_import));
    };
    fprintf(fp, "}");

    G_free_tokens(tokens);
}

/* \brief Check the provided answer and generate the export statement
   dependent on the element type (cell, vector, grid3, file)

   "outputs": [
   {"export": {"type": "file", "format": "TXT"},
   "param": "output",
   "value": "$file::out1"},
   {'export': {'format': 'GTiff', 'type': 'raster'},
   'param': 'map',
   'value': 'LT52170762005240COA00_dos1.1'}
   ]
 */
void check_create_export_opts(struct Option *opt, char *element, FILE * fp)
{
    int i = 0;
    int has_file_export = 0;
    char **tokens;

    tokens = G_tokenize(opt->answer, "+");
    while (tokens[i]) {
        G_chop(tokens[i]);
        i++;
    }

    fprintf(fp, "     {");

    if (i > 1) {
        if (G_strncasecmp("cell", element, 4) == 0) {
            fprintf(fp,
                    "\"export\": {\"format\":\"%s\", \"type\":\"raster\"},\n      ",
                    tokens[1]);
        }
        else if (G_strncasecmp("file", element, 4) == 0) {
            fprintf(fp,
                    "\"export\": {\"format\":\"%s\", \"type\":\"file\"},\n      ",
                    tokens[1]);
            has_file_export = 1;
        }
        else if (G_strncasecmp("vector", element, 4) == 0) {
            fprintf(fp,
                    "\"export\": {\"format\":\"%s\", \"type\":\"vector\"},\n      ",
                    tokens[1]);
        }
    }

    fprintf(fp, "\"param\": \"%s\", ", opt->key);
    if (has_file_export == 1) {
        fprintf(fp, "\"value\": \"$file::%s\"",
                check_mapset_in_layer_name(tokens[0], 1));
    }
    else {
        fprintf(fp, "\"value\": \"%s\"",
                check_mapset_in_layer_name(tokens[0], 1));
    }
    fprintf(fp, "}");

    G_free_tokens(tokens);
}

/*
   \brief Check if the current mapset is present in the layer name and remove it

   The flag always_remove tells this function to always remove all mapset names.

   \return pointer to the layer name without the current mapset
*/
char *check_mapset_in_layer_name(char *layer_name, int always_remove)
{
    int i = 0;
    char **tokens;
    const char *mapset;

    mapset = G_mapset();

    tokens = G_tokenize(layer_name, "@");

    while (tokens[i]) {
        G_chop(tokens[i]);
        /* fprintf(stderr, "Token %i: %s\n", i, tokens[i]); */
        i++;
    }

    if (always_remove == 1)
        return tokens[0];

    if (i > 1 && G_strcasecmp(mapset, tokens[1]) == 0)
        return tokens[0];

    return layer_name;
}
