/**
   \file init.cpp
   
   \brief Experimental C++ wxWidgets Nviz prototype -- initialization

   Used by wxGUI Nviz extension.

   Copyright: (C) by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)

   \date 2008
*/

#include <clocale>

#include "nviz.h"

static void swap_gl();

/*!
  \brief Initialize Nviz class instance
*/
Nviz::Nviz()
{
    setlocale(LC_NUMERIC, "C");

    G_gisinit(""); /* GRASS functions */

    G_set_verbose(0); // TODO: read progress info

    GS_libinit();
    /* GVL_libinit(); TODO */

    GS_set_swap_func(swap_gl);

    data = (nv_data*) G_malloc(sizeof (nv_data));

    /* GLCanvas */
    glCanvas = NULL;

    G_debug(1, "Nviz::Nviz()");
}

/*!
  \brief Destroy Nviz class instance
*/
Nviz::~Nviz()
{
    G_free((void *) data);

    data = NULL;
    glCanvas = NULL;
}

/*!
  \brief Associate display with render window

  \return 1 on success
  \return 0 on failure
*/
int Nviz::SetDisplay(void *display)
{
    glCanvas = (wxGLCanvas *) display;
    // glCanvas->SetCurrent();

    G_debug(1, "Nviz::SetDisplay()");

    return 1;
}

void Nviz::InitView()
{
    /* initialize nviz data */
    Nviz_init_data(data);

    /* define default attributes for map objects */
    Nviz_set_attr_default();
    /* set background color */
    Nviz_set_bgcolor(data, Nviz_color_from_str("white")); /* TODO */

    /* initialize view */
    Nviz_init_view();

    /* set default lighting model */
    SetLightsDefault();

    /* clear window */
    GS_clear(data->bgcolor);

    G_debug(1, "Nviz::InitView()");

    return;
}

/*!
  \brief Reset session

  Unload all data layers

  @todo vector, volume
*/
void Nviz::Reset()
{
    int i;
    int *surf_list, nsurfs;

    surf_list = GS_get_surf_list(&nsurfs);
    for (i = 0; i < nsurfs; i++) {
	GS_delete_surface(surf_list[i]);
    }
}

void swap_gl()
{
    return;
}

/*!
  \brief Set background color

  \param color_str color string
*/
void Nviz::SetBgColor(const char *color_str)
{
    data->bgcolor = Nviz_color_from_str(color_str);

    return;
}
