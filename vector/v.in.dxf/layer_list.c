#include <string.h>
#include "global.h"

void add_layer_to_list(char *layer)
{
    /* capital column names are a pain in SQL */
    G_str_to_lower(layer);

    if (is_layer_in_list(layer))
	return;

    layers = (char **)G_realloc(layers, (num_layers + 2) * sizeof(char *));
    layers[num_layers++] = G_store(layer);
    layers[num_layers] = NULL;

    return;
}

int is_layer_in_list(char *layer)
{
    char **p;

   /* capital column names are a pain in SQL */
    G_str_to_lower(layer);

    if (!layers)
	return 0;

    p = layers;
    while (*p && strcmp(layer, *p) != 0)
	p++;

    return *p != NULL;
}

void init_list(void)
{
    char **p;

    if (!layers)
	return;

    p = layers;
    while (*p) {
	G_free(*p);
	p++;
    }

    G_free(layers);
    layers = NULL;

    return;
}
