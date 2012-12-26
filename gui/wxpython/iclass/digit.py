"""!
@package iclass.digit

@brief wxIClass digitizer classes

Classes:
 - digit::IClassVDigit
 - digit::IClassVDigitWindow

(C) 2006-2012 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx

from vdigit.mapwindow import VDigitWindow
from vdigit.wxdigit   import IVDigit
from vdigit.wxdisplay import DisplayDriver, TYPE_AREA
try:
    from grass.lib.vector import *
    from grass.lib.vedit  import *
except ImportError:
    pass

class IClassVDigitWindow(VDigitWindow):
    """! Class similar to VDigitWindow but specialized for wxIClass."""
    def __init__(self, parent, giface, map, frame):
        """!
        
        @a parent should has toolbar providing current class (category).
        
        @param parent gui parent
        @param map map renderer instance
        """
        VDigitWindow.__init__(self, parent = parent, giface = giface, Map = map, frame = frame)
        
    def _onLeftDown(self, event):
        action = self.toolbar.GetAction()
        if not action:
            return
            
        cat = self.GetCurrentCategory()
        
        if cat is None and action == "addLine":
            dlg = wx.MessageDialog(parent = self.parent,
                      message = _("In order to create a training area, "
                                  "you have to select class first.\n\n"
                                  "There is no class yet, "
                                  "do you want to create one?"),
                      caption = _("No class selected"),
                      style = wx.YES_NO)
            if dlg.ShowModal() == wx.ID_YES:
                self.parent.OnCategoryManager(None)
                
            dlg.Destroy()
            event.Skip()
            return
        
        super(IClassVDigitWindow, self)._onLeftDown(event)
        
    def _addRecord(self):
        return False
        
    def _updateATM(self):
        pass
        
    def _onRightUp(self, event):
        super(IClassVDigitWindow, self)._onRightUp(event)
        self.parent.UpdateChangeState(changes = True)
        
    def GetCurrentCategory(self):
        """!Returns current category (class).
        
        Category should be assigned to new features (areas).
        It is taken from parent's toolbar.
        """
        return self.parent.GetToolbar("iClass").GetSelectedCategoryIdx()

    def GetCategoryColor(self, cat):
        """!Get color associated with given category"""
        r, g, b = map(int, self.parent.GetClassColor(cat).split(':'))
        return wx.Colour(r, g, b)
        
class IClassVDigit(IVDigit):
    """! Class similar to IVDigit but specialized for wxIClass."""
    def __init__(self, mapwindow):
        IVDigit.__init__(self, mapwindow, driver = IClassDisplayDriver)
        self._settings['closeBoundary'] = True # snap to the first node
        
    def _getNewFeaturesLayer(self):
        return 1
        
    def _getNewFeaturesCat(self):
        cat = self.mapWindow.GetCurrentCategory()
        return cat
        
    def DeleteAreasByCat(self, cats):
        """!Delete areas (centroid+boundaries) by categories

        @param cats list of categories
        """
        for cat in cats:
            Vedit_delete_areas_cat(self.poMapInfo, 1, cat)
       
class IClassDisplayDriver(DisplayDriver):
    """! Class similar to DisplayDriver but specialized for wxIClass

    @todo needs refactoring (glog, gprogress)
    """
    def __init__(self, device, deviceTmp, mapObj, window, glog, gprogress):
        DisplayDriver.__init__(self, device, deviceTmp, mapObj, window, glog, gprogress)
        self._cat = -1
        
    def _drawObject(self, robj):
        """!Draw given object to the device

        @param robj object to draw
        """
        if robj.type == TYPE_AREA:
            self._cat = Vect_get_area_cat(self.poMapInfo, robj.fid, 1)
        DisplayDriver._drawObject(self, robj)
        
    def _definePen(self, rtype):
        """!Define pen/brush based on rendered object)

        @param rtype type of the object

        @return pen, brush
        """
        pen, brush = DisplayDriver._definePen(self, rtype)
        if self._cat > 0 and rtype == TYPE_AREA:
            brush = wx.Brush(self.window.GetCategoryColor(self._cat), wx.SOLID)
        
        return pen, brush
