#include <string.h>
#include "global.h"

static int num_layers = 0;
static char **layers = NULL;

void add_layer_to_list(const char *layer, int print)
{
    if (is_layer_in_list(layer))
	return;

    layers = (char **)G_realloc(layers, (num_layers + 2) * sizeof(char *));
    layers[num_layers] = G_store(layer);
    G_str_to_lower(layers[num_layers]);
    if (print) {
	fprintf(stdout, _("Layer %d: %s\n"), num_layers + 1,
		layers[num_layers]);
	fflush(stdout);
    }
    num_layers++;
    layers[num_layers] = NULL;

    return;
}

int is_layer_in_list(const char *layer)
{
    char **p;

    if (!layers)
	return 0;

    p = layers;
    while (*p && G_strcasecmp(layer, *p) != 0)
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
