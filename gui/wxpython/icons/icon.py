"""!
@package icons.icon

@brief Icon metadata

Classes:
 - MetaIcon

(C) 2007-2008, 2010-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys
import copy

import wx

from core.settings import UserSettings
from core.utils import _

# default icon set
import grass_icons
iconSetDefault  = grass_icons.iconSet
iconPathDefault = grass_icons.iconPath

iconTheme = UserSettings.Get(group = 'appearance', key = 'iconTheme', subkey = 'type')
if iconTheme != 'grass':
    sys.stderr.write(_("Unknown iconset '%s', using default 'grass'...\n") % (iconTheme))

iconSet  = iconSetDefault
iconPath = iconPathDefault

# join paths
try:
    if iconPath and not os.path.exists(iconPath):
        raise OSError
    
    for key, img in iconSet.iteritems():
        if key not in iconSet or \
                iconSet[key] is None: # add key
            iconSet[key] = img
        
        iconSet[key] = os.path.join(iconPath, iconSet[key])
except StandardError, e:
    sys.exit(_("Unable to load icon theme. Reason: %s. Quiting wxGUI...") % e)
    
class MetaIcon:
    """!Handle icon metadata (image path, tooltip, ...)
    """
    def __init__(self, img, label = None, desc = None):
        self.imagepath = iconSet.get(img, wx.ART_MISSING_IMAGE)
        if not self.imagepath:
            self.type = 'unknown'
        else:
            if self.imagepath.find ('wxART_') > -1:
                self.type = 'wx'
            else:
                self.type = 'img'
        
        self.label = label
        
        if desc:
            self.description = desc
        else:
            self.description = ''
        
    def __str__(self):
        return "label=%s, img=%s, type=%s" % (self.label, self.imagepath, self.type)

    def GetBitmap(self, size = None):
        bmp = None
        
        if self.type == 'wx':
            bmp = wx.ArtProvider.GetBitmap(id = self.imagepath, client = wx.ART_TOOLBAR, size = size)
        elif self.type == 'img':
            if os.path.isfile(self.imagepath) and os.path.getsize(self.imagepath):
                if size and len(size) == 2:
                    image = wx.Image(name = self.imagepath)
                    image.Rescale(size[0], size[1])
                    bmp = image.ConvertToBitmap()
                elif self.imagepath:
                    bmp = wx.Bitmap(name = self.imagepath)
        
        return bmp
    
    def GetLabel(self):
        return self.label

    def GetDesc(self):
        return self.description
    
    def GetImageName(self):
        return os.path.basename(self.imagepath)

    def SetLabel(self, label = None, desc = None):
        """!Set label/description for icon

        @param label icon label (None for no change)
        @param desc icon description (None for no change)
        
        @return copy of original object
        """
        cobj = copy.copy(self)
        if label:
            cobj.label = label
        if desc:
            cobj.description = desc
        
        return cobj
