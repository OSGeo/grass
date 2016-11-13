/*!
  \file lib/gis/handler.c
 
  \brief GIS Library - Error handlers
 
  (C) 2010-2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Glynn Clements
*/

#include <stddef.h>
#include <grass/gis.h>

/*!
  \brief Error handler (see G_add_error_handler() for usage)
*/
struct handler {
    /*!
      \brief Pointer to the handler routine
    */
    void (*func)(void *);
    /*!
      \brief Pointer to closure data
    */
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

/*!
  \brief Add new error handler

  Example
  \code
  static void error_handler(void *p) {
      const char *map = (const char *) p;
      Vect_delete(map);
  }
  G_add_error_handler(error_handler, new->answer);
  \endcode
  
  \param func handler to add
  \param closure pointer to closure data
*/
void G_add_error_handler(void (*func)(void *), void *closure)
{
    struct handler *h = alloc_handler();

    h->func = func;
    h->closure = closure;
}

/*!
  \brief Remove existing error handler

  \param func handler to be remove
  \param closure pointer to closure data
*/
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

/*!
  \brief Call available error handlers (internal use only)
*/
void G__call_error_handlers(void)
{
    int i;

    for (i = 0; i < num_handlers; i++) {
	struct handler *h = &handlers[i];
	if (h->func)
	    (*h->func)(h->closure);
    }
}

