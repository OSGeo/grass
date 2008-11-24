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

#include "driver.h"
#include "digit.h"

/**
   \brief Initialize digit interface used by SWIG

   \param driver display driver instance
   \param window parent window for message dialog
*/
Digit::Digit(DisplayDriver *ddriver, void *window)
{
    display = ddriver;
    display->parentWin = (wxWindow *) window;

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
*/
void Digit::UpdateSettings(bool breakLines)
{
    settings.breakLines = breakLines;

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
    for (int i = 0; i < display->selected.values->n_values; i++) {
	int line = display->selected.values->value[i];
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
    for (int i = 0; i < display->selected.values->n_values; i++) {
	int line = display->selected.values->value[i];
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
