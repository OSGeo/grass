/**
   \file vdigit/message.cpp

   \brief wxvdigit - Error message dialogs

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

/**
   \brief Error message - no display driver available
*/
void DisplayDriver::DisplayMsg(void)
{
    wxMessageDialog dlg(parentWin, _("Display driver not available."),
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();

    return;
}

/**
   \brief Error message - cannot edit 3d features
*/
void DisplayDriver::Only2DMsg(void)
{
    wxMessageDialog dlg(parentWin, _("3D vector features are not currently supported."),
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - unable to write line
*/
void DisplayDriver::WriteLineMsg(void)
{
    wxMessageDialog dlg(parentWin, _("Unable to write new line"),
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - unable to read line

   \param line line id
*/
void DisplayDriver::ReadLineMsg(int line)
{
    wxString msg;
    msg.Printf(_("Unable to read line %d"), line);
    wxMessageDialog dlg(parentWin, msg,
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - trying to read dead line

   \param line line id
*/
void DisplayDriver::DeadLineMsg(int line)
{
    wxString msg;
    msg.Printf(_("Unable to read line %d, line is dead"), line);
    wxMessageDialog dlg(parentWin, msg,
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - unable to open background map

   \param bgmap map name
*/
void DisplayDriver::BackgroundMapMsg(const char *bgmap)
{
    wxString msg;
    msg.Printf(_("Unable to open background vector map <%s>. Please check digitizer settings."),
	       wxString (bgmap, wxConvUTF8).c_str());
    wxMessageDialog dlg(parentWin, msg,
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - dblink not defined

   \param layer layer id
*/
void DisplayDriver::DblinkMsg(int layer)
{
    wxString msg;
    msg.Printf(_("Database connection not defined for layer %d"), layer);
    wxMessageDialog dlg(parentWin, msg,
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - unable to start driver

   \param driver driver name
*/
void DisplayDriver::DbDriverMsg(const char *driver)
{
    wxString msg;
    msg.Printf(_("Unable to start driver <%s>"),
	       wxString(driver, wxConvUTF8).c_str());
    wxMessageDialog dlg(parentWin, msg,
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - unable to open database

   \param driver driver name
   \param database database name
*/
void DisplayDriver::DbDatabaseMsg(const char *driver, const char *database)
{
    wxString msg;
    msg.Printf(_("Unable to open database <%s> by driver <%s>"),
	       wxString(database, wxConvUTF8).c_str(),
	       wxString(driver, wxConvUTF8).c_str());
    wxMessageDialog dlg(parentWin, msg,
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - unable to execute SQL command

   \param sql sql command
*/
void DisplayDriver::DbExecuteMsg(const char *sql)
{
    wxString msg;
    msg.Printf(_("Unable to execute: '%s'"),
	       wxString(sql, wxConvUTF8).c_str());
    wxMessageDialog dlg(parentWin, msg,
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - unable to open select cursor

   \param sql sql command
*/
void DisplayDriver::DbSelectCursorMsg(const char *sql)
{
    wxString msg;
    msg.Printf(_("Unable to open select cursor: '%s'"),
	       wxString(sql, wxConvUTF8).c_str());
    wxMessageDialog dlg(parentWin, msg,
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}

/**
   \brief Error message - unable to get line categories

   \param line line id
*/
void DisplayDriver::GetLineCatsMsg(int line)
{
    wxString msg;
    msg.Printf(_("Unable to get feature (%d) categories"), line);
    wxMessageDialog dlg(parentWin, msg,
			msgCaption, wxOK | wxICON_ERROR | wxCENTRE);
    dlg.ShowModal();
    
    return;
}
