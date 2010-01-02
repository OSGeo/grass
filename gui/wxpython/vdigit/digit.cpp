/**
   \file vdigit/digit.cpp

   \brief wxvdigit - C++ interace for wxGUI vector digitizer.

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008-2009 by Martin Landa, and the GRASS development team

   \author Martin Landa <landa.martin gmail.com>
*/

extern "C" {
#include <grass/glocale.h>
}

#include "driver.h"
#include "digit.h"

#if defined _WIN32 && defined MSVC_VER
#ifndef _CPPRTTI
#error "compile with /GR!"
#endif
#endif

/**
   \brief Initialize digit interface

   \param driver display driver instance
   \param window parent window for message dialog
*/
Digit::Digit(DisplayDriver *ddriver, wxWindow *window)
{
    display = ddriver;
    if (!display)
	return;
    
    if (display->mapInfo) {
	InitCats();
    }

    changesetEnd = changesetCurrent = -1; // initial value for undo/redo

    display->msgCaption = _("Digitization error");
    
    // avoid GUI crash
    // Vect_set_fatal_error(GV_FATAL_PRINT);
}

/**
   \brief Digit class destructor

   Frees changeset structure
*/
Digit::~Digit()
{
    for(int changeset = 0; changeset < (int) changesets.size(); changeset++) {
	FreeChangeset(changeset);
    }
}

/**
   \brief Update digit settings

   \param breakLines break lines on intersection
   \param addCentroid add centroid to left/right area
   \param catBoundary attach category to boundary
*/
void Digit::UpdateSettings(bool breakLines,
			   bool addCentroid, bool catBoundary)
{
    settings.breakLines = breakLines;
    settings.addCentroid = addCentroid;
    settings.catBoundary = !catBoundary; /* do not attach */
    
    return;
}

/*!
  \brief Register action before operation
  
  \return changeset id
*/
int Digit::AddActionsBefore(void)
{
    int changeset;

    /* register changeset */
    changeset = changesets.size();
    for (int i = 0; i < display->selected.ids->n_values; i++) {
	int line = display->selected.ids->value[i];
	if (Vect_line_alive(display->mapInfo, line))
	    AddActionToChangeset(changeset, DEL, line);
    }
    
    return changeset;
}

/*!
  \brief Register action after operation
*/
void Digit::AddActionsAfter(int changeset, int nlines)
{
    for (int i = 0; i < display->selected.ids->n_values; i++) {
	int line = display->selected.ids->value[i];
	if (Vect_line_alive(display->mapInfo, line)) {
	    RemoveActionFromChangeset(changeset, DEL, line);
	}
    }

    for (int line = nlines + 1; line <= Vect_get_num_lines(display->mapInfo); line++) {
	if (Vect_line_alive(display->mapInfo, line)) {
	    AddActionToChangeset(changeset, ADD, line);
	}
    }
    
    return;
}
