/**
   \file undo.cpp

   \brief Undo/Redo functionality

   \todo Implement Vect_restore_line() in Vlib

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008 by The GRASS development team

   \author Martin Landa <landa.martin gmail.com>

   \date 2008 
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
	level = changesetDead - changesetCurrent;
    }

    G_debug(2, "Digit.Undo(): changeset_last=%d changeset_dead=%d, changeset_current=%d, level=%d",
	    changesetLast, changesetDead, changesetCurrent, level);
    
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

    G_debug(2, "Digit.Undo(): changeset_dead=%d, changeset_current=%d",
	    changesetDead, changesetCurrent);

    return (changesetDead >= changesetCurrent) ? -1 : changesetCurrent;
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
    int ret;

    if (changeset < 0 || changeset > (int) changesets.size())
	return -1;

    ret = 0;
    std::vector<action_meta> action = changesets[changeset];
    for (std::vector<action_meta>::const_iterator i = action.begin(), e = action.end();
	 i != e; ++i) {
	if ((undo && (*i).type == ADD) ||
	    (!undo && (*i).type == DELETE)) {
	    if (Vect_line_alive(display->mapInfo, (*i).line)) {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=add, line=%d -> deleted",
			changeset, (*i).line);
		Vect_delete_line(display->mapInfo, (*i).line);
		if (!ret)
		    ret = 1;
	    }
	    else {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=add, line=%d dead",
			changeset, (*i).line);
	    }
	}
	else if ((*i).type == REWRITE) {
	    if (Vect_line_alive(display->mapInfo, (*i).line)) {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=rewrite, line=%d",
			changeset, (*i).line);
		if (Vect_rewrite_line (display->mapInfo, (*i).line, (*i).ltype, (*i).Points, (*i).Cats) < 0)
		    return -1;
	    }
	    else {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=rewrite, line=%d -> dead",
			changeset, (*i).line);
	    }
	}
	else { /* DELETE */
	    if (!Vect_line_alive(display->mapInfo, (*i).line)) {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=delete, line=%d -> added",
			changeset, (*i).line);
		if (Vect_write_line(display->mapInfo, (*i).ltype, (*i).Points, (*i).Cats) < 0)
		    return -1;
		if (!ret)
		    ret = 1;
	    }
	    else {
		G_debug(3, "Digit.ApplyChangeset(): changeset=%d, action=delete, line=%d alive",
			changeset, (*i).line);
	    }
	}
    }
    
    if (changeset < (int) changesets.size() - 1)
	changesetDead = changeset;

    return ret;
}

/**
   \brief Add action to changeset

   \todo Use Vect_restore_line() (TODO) instead!

   \param type action type (ADD, DELETE)

   \return 0 on success
   \return -1 on error
*/
int Digit::AddActionToChangeset(int changeset, Digit::action_type type, int line)
{
    int ltype;
    struct line_pnts *Points;
    struct line_cats *Cats;

    if (!display->mapInfo) {
	return -1;
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct(); 

    /* do copy */
    if (!Vect_line_alive(display->mapInfo, line))
	return -1;

    ltype = Vect_read_line(display->mapInfo, Points, Cats, line);

    action_meta data = { type, line, ltype, Points, Cats };
    if (changesets.find(changeset) == changesets.end()) {
	changesets[changeset] = std::vector<action_meta>();
	changesetCurrent = changeset;
    }
    changesets[changeset].push_back(data);
    G_debug (3, "Digit.AddActionToChangeset(): changeset=%d, type=%d, line=%d",
	     changeset, type, line);

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
	Vect_destroy_line_struct((*i).Points);
	Vect_destroy_cats_struct((*i).Cats);
	(*i).Points = NULL;
	(*i).Cats = NULL;
    }

    return;
}

/**
   \brief Remove action from changeset

   \param changeset changeset id
   \param type action type (ADD, DELETE, REWRITE)
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
    return changesetCurrent - changesetDead;
}
