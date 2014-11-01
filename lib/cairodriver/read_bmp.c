/*!
  \file lib/cairodriver/read_bmp.c

  \brief GRASS cairo display driver - read bitmap (lower level functions)

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include "cairodriver.h"

static unsigned int get_2(const unsigned char **q)
{
    const unsigned char *p = *q;
    unsigned int n = (p[0] << 0) | (p[1] << 8);

    *q += 2;
    return n;
}

static unsigned int get_4(const unsigned char **q)
{
    const unsigned char *p = *q;
    unsigned int n = (p[0] << 0) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);

    *q += 4;
    return n;
}

static int read_bmp_header(const unsigned char *p)
{
    if (*p++ != 'B')
	return 0;
    if (*p++ != 'M')
	return 0;

    if (get_4(&p) != HEADER_SIZE + ca.width * ca.height * 4)
	return 0;

    get_4(&p);

    if (get_4(&p) != HEADER_SIZE)
	return 0;

    if (get_4(&p) != 40)
	return 0;

    if (get_4(&p) != ca.width)
	return 0;
    if (get_4(&p) != -ca.height)
	return 0;

    get_2(&p);
    if (get_2(&p) != 32)
	return 0;

    if (get_4(&p) != 0)
	return 0;
    if (get_4(&p) != ca.width * ca.height * 4)
	return 0;

    get_4(&p);
    get_4(&p);
    get_4(&p);
    get_4(&p);

    return 1;
}

void cairo_read_bmp(void)
{
    char header[HEADER_SIZE];
    FILE *input;

    input = fopen(ca.file_name, "rb");
    if (!input)
	G_fatal_error(_("Cairo: unable to open input file <%s>"),
		      ca.file_name);

    if (fread(header, sizeof(header), 1, input) != 1)
	G_fatal_error(_("Cairo: invalid input file <%s>"),
		      ca.file_name);

    if (!read_bmp_header(header))
	G_fatal_error(_("Cairo: Invalid BMP header for <%s>"),
		      ca.file_name);

    fread(ca.grid, ca.stride, ca.height, input);

    fclose(input);
}
