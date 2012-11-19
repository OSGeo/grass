"""!
@package core.events

@brief General events

Put here only truly general events. Once you find that your event can be
generated in more than one class, put your event here. Otherwise,
leave it in your class file.
        
(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
"""


from wx.lib.newevent import NewCommandEvent


# Notification event intended to update statusbar.
gShowNotification, EVT_SHOW_NOTIFICATION = NewCommandEvent()

