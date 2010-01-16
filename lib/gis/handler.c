
#include <stddef.h>
#include <grass/gis.h>

struct handler {
    void (*func)(void *);
    void *closure;
};

static struct handler *handlers;

static int num_handlers;
static int max_handlers;

static struct handler *alloc_handler(void)
{
    int i;

    for (i = 0; i < num_handlers; i++) {
	struct handler *h = &handlers[i];
	if (!h->func)
	    return h;
    }

    if (num_handlers >= max_handlers) {
	max_handlers += 10;
	handlers = G_realloc(handlers, max_handlers * sizeof(struct handler));
    }

    return &handlers[num_handlers++];
}

void G_add_error_handler(void (*func)(void *), void *closure)
{
    struct handler *h = alloc_handler();

    h->func = func;
    h->closure = closure;
}

void G_remove_error_handler(void (*func)(void *), void *closure)
{
    int i;

    for (i = 0; i < num_handlers; i++) {
	struct handler *h = &handlers[i];

	if (h->func == func && h->closure == closure) {
	    h->func = NULL;
	    h->closure = NULL;
	}
    }
}

void G__call_error_handlers(void)
{
    int i;

    for (i = 0; i < num_handlers; i++) {
	struct handler *h = &handlers[i];
	if (h->func)
	    (*h->func)(h->closure);
    }
}

