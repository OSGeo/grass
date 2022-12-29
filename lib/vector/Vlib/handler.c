/*!
  \file lib/vector/Vlib/handler.c

  \brief Vector library - standard error handlers

  Higher level functions for reading/writing/manipulating vectors.
  
  (C) 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/gis.h>
#include <grass/vector.h>

struct handler_data_io {
    struct Map_info *In;
    struct Map_info *Out;
};

static struct handler_data_io *handler_io;

static void error_handler_io(void *p)
{
    char *name;
    struct Map_info *In, *Out;
    
    In  = handler_io->In;
    Out = handler_io->Out;
    
    if (In && In->open == VECT_OPEN_CODE)
	Vect_close(In);
    
    if (Out && Out->open == VECT_OPEN_CODE) {
	name = G_store(Out->name);
	Vect_close(Out);
	Vect_delete(name);
	G_free(name);
    }
}

/*!
  \brief Define standard error handler for input and output vector maps

  This handler:
   - close input vector map on error
   - close and delete output vector map on error
  
  Note: It's recommended to call this routine after Vect_open_old() or
  Vect_open_old2().

  \param In  pointer in Map_info struct (input vector map) or NULL
  \param Out pointer to Map_info struct (output vector map) or NULL
*/
void Vect_set_error_handler_io(struct Map_info *In, struct Map_info *Out)
{
    if (!handler_io)
	handler_io = G_malloc(sizeof(struct handler_data_io));
    
    handler_io->In  = In;
    handler_io->Out = Out;
    
    G_add_error_handler(error_handler_io, handler_io);
}
