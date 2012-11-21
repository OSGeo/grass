"""!
@package core.events

@brief General events

Put here only truly general events. Once you find that your event can be
generated in more than one class, put your event here. Otherwise,
leave it in your class file.

General notice:
Command events are propagated to parent windows. However they do not propagate 
beyond dialogs. Events do not propagate at all.


(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
"""


from wx.lib.newevent import NewCommandEvent


# Notification event intended to update statusbar.
# The message attribute contains the text of the message (plain text)
gShowNotification, EVT_SHOW_NOTIFICATION = NewCommandEvent()


# Occurs event when some map is created or updated by a module.
# attributes: name: map name, ltype: map type,
# add: if map should be added to layer tree (questionable attribute)
gMapCreated, EVT_MAP_CREATED = NewCommandEvent()
