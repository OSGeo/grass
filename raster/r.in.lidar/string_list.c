
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

#define SIZE_INCREMENT 10

static int string_list_add_item(struct StringList *string_list, char *item)
{
    int n = string_list->num_items++;

    if (string_list->num_items >= string_list->max_items) {
        string_list->max_items += SIZE_INCREMENT;
        string_list->items = G_realloc(string_list->items,
                                       (size_t) string_list->max_items *
                                       sizeof(char *));
    }
    /* n contains the index */
    string_list->items[n] = item;
    return n;
}

void string_list_from_file(struct StringList *string_list, char *filename)
{
    string_list->num_items = 0;
    string_list->max_items = 0;
    string_list->items = NULL;
    FILE *file = fopen(filename, "r");  /* should check the result */
    if (!file)
        G_fatal_error(_("Cannot open file %s for reading"), filename);
    char *line = G_malloc(GPATH_MAX * sizeof(char));

    while (G_getl2(line, GPATH_MAX * sizeof(char), file)) {
        G_debug(5, "line content from file %s: %s\n", filename, line);
        string_list_add_item(string_list, line);
        line = G_malloc(GPATH_MAX);
    }
    /* last allocation was not necessary */
    G_free(line);
    fclose(file);
}

void string_list_from_one_item(struct StringList *string_list, char *item)
{
    string_list->num_items = 0;
    string_list->max_items = 0;
    string_list->items = NULL;
    string_list_add_item(string_list, strdup(item));
}

void string_list_free(struct StringList *string_list)
{
    int i;

    for (i = 0; i < string_list->num_items; i++)
        G_free(string_list->items[i]);
    G_free(string_list->items);
    string_list->num_items = 0;
    string_list->max_items = 0;
    string_list->items = NULL;
}
