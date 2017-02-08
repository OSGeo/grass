#include <grass/gis.h>
#include <grass/vector.h>

struct handler_input
{
    struct Map_info *Map;
    dbDriver **driver;
};

static void error_handler(void *p)
{
    const struct handler_input *input = (const struct handler_input *)p;

    if (input->driver && *input->driver)
	db_close_database_shutdown_driver(*input->driver);
    if (input->Map) {
	char *name = G_store(input->Map->name);
	if (input->Map->open == VECT_OPEN_CODE)
	    Vect_close(input->Map);
	Vect_delete(name);
	G_free(name);
    }
}

void set_error_handler(struct Map_info *Map, dbDriver **driver)
{
    struct handler_input *input = G_malloc(sizeof(struct handler_input));

    input->Map = Map;
    input->driver = driver;

    G_add_error_handler(error_handler, input);
}
