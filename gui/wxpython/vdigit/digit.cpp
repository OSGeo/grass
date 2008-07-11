/**
   \file digit.cpp

   \brief Experimental C++ interace for vector digitization used
   by wxPython GUI.

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author (C) by the GRASS Development Team
   Martin Landa <landa.martin gmail.com>

   \date 2008
*/

#include <clocale>

#include "driver.h"
#include "digit.h"

/**
   \brief Initialize digit interface used by SWIG

   \param driver display driver instance
*/
Digit::Digit(DisplayDriver *ddriver)
{
    setlocale(LC_NUMERIC, "C");

    display = ddriver;

    if (display->mapInfo) {
	InitCats();
    }

    changesetCurrent = -2; // initial value for undo/redo
    changesetDead = -1;

    // avoid GUI crash
    // Vect_set_fatal_error(GV_FATAL_PRINT);
}

/**
   Digit class destructor

   Frees changeset structure
*/
Digit::~Digit()
{
    for(int changeset = 0; changeset < (int) changesets.size(); changeset++) {
	FreeChangeset(changeset);
    }
}
