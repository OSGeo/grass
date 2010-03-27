/**
   \file vdigit/undo.cpp

   \brief wxvdigit - Undo/Redo functionality

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008-2010 by Martin Landa, and the GRASS development team

   \author Martin Landa <landa.martin gmail.com>
*/

#include "driver.h"
#include "digit.h"

/**
   \brief Undo/Redo changes in geometry

   level=0 to revert all changes

   \param level level for undo/redo

   \return id of current chanset
   \return -2 on error
*/
int Digit::Undo(int level)
{
    int changesetLast;

    changesetLast = (int) changesets.size() - 1;

    if (changesetLast < 0)
	return changesetLast;

    if (changesetCurrent == -2) { /* value uninitialized */
	changesetCurrent = changesetLast;
    }

    if (level > 0 && changesetCurrent < 0) {
	changesetCurrent = 0;
    }

    if (level == 0) {
	/* 0 -> undo all */
	level = -(changesetLast + 1);
    }

    G_debug(2, "Digit.Undo(): changeset_last=%d, changeset_current=%d, level=%d",
	    changesetLast, changesetCurrent, level);
    
    if (level < 0) { /* undo */
	if (changesetCurrent + level < -1)
	    return changesetCurrent;
	for (int changeset = changesetCurrent; changeset > changesetCurrent + level; --changeset) {
	    ApplyChangeset(changeset, true);
	}
    }
    else if (level > 0) { /* redo */
	if (changesetCurrent + level > (int) changesets.size())
	    return changesetCurrent;
	for (int changeset = changesetCurrent; changeset < changesetCurrent + level; ++changeset) {
	    ApplyChangeset(changeset, false);
	}
    }

    changesetCurrent += level;

    G_debug(2, "Digit.Undo(): changeset_current=%d, changeset_last=%d, changeset_end=%d",
	    changesetCurrent, changesetLast, changesetEnd);
    
    if (changesetCurrent == changesetEnd) {
	changesetEnd = changesetLast;
	return -1;
    }
    
    return changesetCurrent;
}

/**
   \brief Apply changeset (undo/redo changeset)

   \param changeset changeset id
   \param undo if true -> undo otherwise redo

   \return 1 changeset applied
   \return 0 changeset not applied
   \return -1 on error
*/
int Digit::ApplyChangeset(int changeset, bool undo)
{ 
    int ret, line, type;

    if (changeset < 0 || changeset > (int) changesets.size())
	return -1;

    if (changesetEnd < 0)
	changesetEnd = changeset;
    
    ret = 0;
    std::vector<action_meta> action = changesets[changeset];
    for (std::vector<action_meta>::const_reverse_iterator i = action.rbegin(), e = action.rend();
	 i != e; ++i) {
	type = (*i).type;
	line = (*i).line;
	
	if ((undo && type == ADD) ||
	    (!undo && type == DEL)) {
	    if (Vect_line_alive(display->mapInfo, line)) {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=add, line=%d -> deleted",
			changeset, line);
		Vect_delete_line(display->mapInfo, line);
		if (!ret)
		    ret = 1;
	    }
	    else {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=add, line=%d dead",
			changeset, (*i).line);
	    }
	}
	else { /* DELETE */
	    long offset = (*i).offset;
	    if (!Vect_line_alive(display->mapInfo, line)) {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=delete, line=%d -> added",
			changeset, line);
		if (Vect_restore_line(display->mapInfo, line, offset) < 0)
		    return -1;
		if (!ret)
		    ret = 1;
	    }
	    else {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=delete, line=%d alive",
			changeset, line);
	    }
	}
    }
    
    return ret;
}

/**
   \brief Add action to changeset

   \param type action type (ADD, DEL)

   \return 0 on success
   \return -1 on error
*/
int Digit::AddActionToChangeset(int changeset, Digit::action_type type, int line)
{
    long offset;

    if (!display->mapInfo) {
	display->DisplayMsg();
	return -1;
    }

    if (!Vect_line_alive(display->mapInfo, line)) {
	// display->DeadLineMsg(line);
	return -1;
    }

    offset = Vect_get_line_offset(display->mapInfo, line);

    action_meta data = { type, line, offset };
    if (changesets.find(changeset) == changesets.end()) {
	changesets[changeset] = std::vector<action_meta>();
	changesetCurrent = changeset;
    }
    changesets[changeset].push_back(data);
    G_debug (3, "Digit.AddActionToChangeset(): changeset=%d, type=%d, line=%d, offset=%ld",
	     changeset, type, line, offset);

    return 0;
}

/**
   \brief Free changeset structures

   \param changeset changeset id
*/
void Digit::FreeChangeset(int changeset)
{
    if (changesets.find(changeset) == changesets.end())
	return;

    std::vector<action_meta> action = changesets[changeset];
    for (std::vector<action_meta>::iterator i = action.begin(), e = action.end();
	 i != e; ++i) {
	;
    }

    return;
}

/**
   \brief Remove action from changeset

   \param changeset changeset id
   \param type action type (ADD, DEL)
   \param line line id

   \return number of actions in changeset
   \return -1 on error
*/
int Digit::RemoveActionFromChangeset(int changeset, Digit::action_type type, int line)
{
    if (changesets.find(changeset) == changesets.end())
	return -1;

    std::vector<action_meta>& action = changesets[changeset];
    for (std::vector<action_meta>::iterator i = action.begin(); i != action.end(); ++i) {
	if ((*i).type == type && (*i).line == line) {
	    G_debug (3, "Digit.RemoveActionFromChangeset(): changeset=%d, type=%d, line=%d",
		     changeset, type, line);
	    action.erase(i--);
	}
    }

    return action.size();
}

/**
   \brief Get undo level (number of active changesets)

   \return number
*/
int Digit::GetUndoLevel()
{
    return changesetCurrent;
}
