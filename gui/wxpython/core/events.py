"""!
@package core.events

@brief General events

Put here only truly general events. Once you find that your event can be
generated in more than one class, put your event here. Otherwise,
leave it in your class file. Events are expected to be grass/gis related.

General notice:
Command events are propagated to parent windows. However, they do not propagate 
beyond dialogs. Events do not propagate at all.
Command events works only with windows, not EvtHandlers, so for other objects
than windows you need to have extra parameter - guiparent - which can be used
for creating command events.
\code
mapEvent = gMapCreated(self._guiparent.GetId(), ...)
wx.PostEvent(self._guiparent, mapEvent)
\endcode

@todo naming conventions for events

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
"""


from wx.lib.newevent import NewCommandEvent
from wx.lib.newevent import NewEvent


# Notification event intended to update statusbar.
# The message attribute contains the text of the message (plain text)
gShowNotification, EVT_SHOW_NOTIFICATION = NewCommandEvent()


# Occurs event when some map is created or updated by a module.
# attributes: name: map name, ltype: map type,
# add: if map should be added to layer tree (questionable attribute)
gMapCreated, EVT_MAP_CREATED = NewCommandEvent()

gZoomChanged, EVT_ZOOM_CHANGED = NewEvent()

# Post it to BufferedWindow instance, which you want to update.
# For relevant attributes for the event see 
# mapdisp.mapwindow.BufferedWindow UpdateMap method arguments.
# If event does not contain attribute corresponding to argument with default
# value, the default value will be used.
# Arguments with no default value must be among event attributes
# in order to be the event processed.
# TODO implement to NVIZ GLWindow
# TODO change direct calling of UpdateMap method to posting this event 
gUpdateMap, EVT_UPDATE_MAP = NewEvent()
