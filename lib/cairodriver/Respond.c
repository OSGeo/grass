/*!
  \file cairodriver/Respond.c

  \brief GRASS cairo display driver - write image

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include "cairodriver.h"

/*!
  \brief Write image
*/
void Cairo_Respond(void)
{
    if (ca.auto_write)
	cairo_write_image();
}
