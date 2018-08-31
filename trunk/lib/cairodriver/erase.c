/*!
  \file lib/cairodriver/erase.c

  \brief GRASS cairo display driver - erase screen

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include "cairodriver.h"

/*!
  \brief Erase screen
*/
void Cairo_Erase(void)
{
    G_debug(1, "Cairo_Erase");

    cairo_save(cairo);
    cairo_set_source_rgba(cairo, ca.bgcolor_r, ca.bgcolor_g, ca.bgcolor_b, ca.bgcolor_a);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);
    cairo_restore(cairo);

    ca.modified = 1;

    return;
}
