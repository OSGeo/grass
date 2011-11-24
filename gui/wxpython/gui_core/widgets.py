"""!
@package gui_core.widgets

@brief Core GUI widgets

Classes:
 - ScrolledPanel
 - NTCValidator
 - NumTextCtrl
 - FloatSlider
 - SymbolButton

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
@author Enhancements by Michael Barton <michael.barton asu.edu>
@author Anna Kratochvilova <kratochanna gmail.com> (Google SoC 2011)
"""

import string

import wx
import wx.lib.scrolledpanel as SP
try:
    import wx.lib.agw.flatnotebook   as FN
except ImportError:
    import wx.lib.flatnotebook   as FN
try:
    from wx.lib.buttons import ThemedGenBitmapTextButton as BitmapTextButton
except ImportError: # not sure about TGBTButton version
    from wx.lib.buttons import GenBitmapTextButton as BitmapTextButton

from core import globalvar

class GNotebook(FN.FlatNotebook):
    """!Generic notebook widget
    """
    def __init__(self, parent, style, **kwargs):
        if globalvar.hasAgw:
            FN.FlatNotebook.__init__(self, parent, id = wx.ID_ANY, agwStyle = style, **kwargs)
        else:
            FN.FlatNotebook.__init__(self, parent, id = wx.ID_ANY, style = style, **kwargs)
        
        self.notebookPages = {}
            
    def AddPage(self, **kwargs):
        """!Add a new page
        """
        if 'name' in kwargs:
            self.notebookPages[kwargs['name']] = kwargs['page']
            del kwargs['name']
        super(GNotebook, self).AddPage(**kwargs)

    def InsertPage(self, **kwargs):
        """!Insert a new page
        """
        if 'name' in kwargs:
            self.notebookPages[kwargs['name']] = kwargs['page']
            del kwargs['name']
        super(GNotebook, self).InsertPage(**kwargs)

    def SetSelectionByName(self, page):
        """!Set notebook
        
        @param page names, eg. 'layers', 'output', 'search', 'pyshell', 'nviz'
        """
        idx = self.GetPageIndexByName(page)
        if self.GetSelection() != idx:
            self.SetSelection(idx)
        
    def GetPageIndexByName(self, page):
        """!Get notebook page index
        
        @param page name
        """
        if page not in self.notebookPages:
            return -1
        
        return self.GetPageIndex(self.notebookPages[page])

class ScrolledPanel(SP.ScrolledPanel):
    """!Custom ScrolledPanel to avoid strange behaviour concerning focus"""
    def __init__(self, parent):
        SP.ScrolledPanel.__init__(self, parent = parent, id = wx.ID_ANY)
    def OnChildFocus(self, event):
        pass
        
        
class NTCValidator(wx.PyValidator):
    """!validates input in textctrls, taken from wxpython demo"""
    def __init__(self, flag = None):
        wx.PyValidator.__init__(self)
        self.flag = flag
        self.Bind(wx.EVT_CHAR, self.OnChar)

    def Clone(self):
        return NTCValidator(self.flag)

    def OnChar(self, event):
        key = event.GetKeyCode()
        if key < wx.WXK_SPACE or key == wx.WXK_DELETE or key > 255:
            event.Skip()
            return
        if self.flag == 'DIGIT_ONLY' and chr(key) in string.digits + '.-':
            event.Skip()
            return
        if not wx.Validator_IsSilent():
            wx.Bell()
        # Returning without calling even.Skip eats the event before it
        # gets to the text control
        return  
    
class NumTextCtrl(wx.TextCtrl):
    """!Class derived from wx.TextCtrl for numerical values only"""
    def __init__(self, parent,  **kwargs):
##        self.precision = kwargs.pop('prec')
        wx.TextCtrl.__init__(self, parent = parent,
            validator = NTCValidator(flag = 'DIGIT_ONLY'), **kwargs)
        
            
    def SetValue(self, value):
        super(NumTextCtrl, self).SetValue( str(value))
        
    def GetValue(self):
        val = super(NumTextCtrl, self).GetValue()
        if val == '':
            val = '0'
        try:
            return float(val)
        except ValueError:
            val = ''.join(''.join(val.split('-')).split('.'))
            return float(val)
        
    def SetRange(self, min, max):
        pass
   
class FloatSlider(wx.Slider):
    """!Class derived from wx.Slider for floats"""
    def __init__(self, **kwargs):
        Debug.msg(1, "FloatSlider.__init__()")
        wx.Slider.__init__(self, **kwargs)
        self.coef = 1.
        #init range
        self.minValueOrig = 0
        self.maxValueOrig = 1
        
    def SetValue(self, value):
        value *= self.coef 
        if abs(value) < 1 and value != 0:
            while abs(value) < 1:
                value *= 100
                self.coef *= 100
            super(FloatSlider, self).SetRange(self.minValueOrig * self.coef, self.maxValueOrig * self.coef)
        super(FloatSlider, self).SetValue(value)
        
        Debug.msg(4, "FloatSlider.SetValue(): value = %f" % value)
        
    def SetRange(self, minValue, maxValue):
        self.coef = 1.
        self.minValueOrig = minValue
        self.maxValueOrig = maxValue
        if abs(minValue) < 1 or abs(maxValue) < 1:
            while (abs(minValue) < 1 and minValue != 0) or (abs(maxValue) < 1 and maxValue != 0):
                minValue *= 100
                maxValue *= 100
                self.coef *= 100
            super(FloatSlider, self).SetValue(super(FloatSlider, self).GetValue() * self.coef)
        super(FloatSlider, self).SetRange(minValue, maxValue)
        Debug.msg(4, "FloatSlider.SetRange(): minValue = %f, maxValue = %f" % (minValue, maxValue))
            
    def GetValue(self):
        val = super(FloatSlider, self).GetValue()
        Debug.msg(4, "FloatSlider.GetValue(): value = %f" % (val/self.coef))
        return val/self.coef
        
        
class SymbolButton(BitmapTextButton):
    """!Button with symbol and label."""
    def __init__(self, parent, usage, label, **kwargs):
        """!Constructor
        
        @param parent parent (usually wx.Panel)
        @param usage determines usage and picture
        @param label displayed label
        """
        BitmapTextButton.__init__(self, parent = parent, label = " " + label, **kwargs)
        
        size = (15, 15)
        buffer = wx.EmptyBitmap(*size)
        dc = wx.MemoryDC()
        dc.SelectObject(buffer)
        maskColor = wx.Color(255, 255, 255)
        dc.SetBrush(wx.Brush(maskColor))
        dc.Clear()
        
        if usage == 'record':
            self.DrawRecord(dc, size)
        elif usage == 'stop':
            self.DrawStop(dc, size)
        elif usage == 'play':
            self.DrawPlay(dc, size)
        elif usage == 'pause':
            self.DrawPause(dc, size)

        buffer.SetMaskColour(maskColor)
        self.SetBitmapLabel(buffer)
        dc.SelectObject(wx.NullBitmap)
        
    def DrawRecord(self, dc, size):
        """!Draw record symbol"""
        dc.SetBrush(wx.Brush(wx.Color(255, 0, 0)))
        dc.DrawCircle(size[0]/2, size[1] / 2, size[0] / 2)
        
    def DrawStop(self, dc, size):
        """!Draw stop symbol"""
        dc.SetBrush(wx.Brush(wx.Color(50, 50, 50)))
        dc.DrawRectangle(0, 0, size[0], size[1])
        
    def DrawPlay(self, dc, size):
        """!Draw play symbol"""
        dc.SetBrush(wx.Brush(wx.Color(0, 255, 0)))
        points = (wx.Point(0, 0), wx.Point(0, size[1]), wx.Point(size[0], size[1] / 2))
        dc.DrawPolygon(points)
        
    def DrawPause(self, dc, size):
        """!Draw pause symbol"""
        dc.SetBrush(wx.Brush(wx.Color(50, 50, 50)))
        dc.DrawRectangle(0, 0, 2 * size[0] / 5, size[1])
        dc.DrawRectangle(3 * size[0] / 5, 0, 2 * size[0] / 5, size[1])
