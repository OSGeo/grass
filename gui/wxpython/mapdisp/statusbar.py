"""!
@package mapdisp.statusbar

@brief Classes for statusbar management

Classes:
 - statusbar::SbException
 - statusbar::SbManager
 - statusbar::SbItem
 - statusbar::SbRender
 - statusbar::SbShowRegion
 - statusbar::SbAlignExtent
 - statusbar::SbResolution
 - statusbar::SbMapScale
 - statusbar::SbGoTo
 - statusbar::SbProjection
 - statusbar::SbMask
 - statusbar::SbTextItem
 - statusbar::SbDisplayGeometry
 - statusbar::SbCoordinates
 - statusbar::SbRegionExtent
 - statusbar::SbCompRegionExtent
 - statusbar::SbProgress
 - statusbar::SbRMSError
 - statusbar::SbGoToGCP

(C) 2006-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import copy
import wx

from core          import utils
from core.gcmd     import GMessage, RunCommand
from core.settings import UserSettings

from grass.script  import core as grass

from grass.pydispatch.signal import Signal

class SbException:
    """! Exception class used in SbManager and SbItems"""
    def __init__(self, message):
        self.message = message
    def __str__(self):
        return self.message


class SbManager:
    """!Statusbar manager for wx.Statusbar and SbItems.
    
    Statusbar manager manages items added by AddStatusbarItem method.
    Provides progress bar (SbProgress) and choice (wx.Choice).
    Items with position 0 are shown according to choice selection.
    Only one item of the same class is supposed to be in statusbar.
    Manager user have to create statusbar on his own, add items to manager
    and call Update method to show particular widgets.
    User settings (group = 'display', key = 'statusbarMode', subkey = 'selection')
    are taken into account.
    
    @todo generalize access to UserSettings (specify group, etc.) 
    @todo add GetMode method using name instead of index
    """
    def __init__(self, mapframe, statusbar):
        """!Connects manager to statusbar
        
        Creates choice and progress bar.
        """
        self.mapFrame = mapframe
        self.statusbar = statusbar
        
        self.choice = wx.Choice(self.statusbar, wx.ID_ANY)
        
        self.choice.Bind(wx.EVT_CHOICE, self.OnToggleStatus)
        
        self.statusbarItems = dict()
        
        self._postInitialized = False
        
        self.progressbar = SbProgress(self.mapFrame, self.statusbar, self)
        self.progressbar.progressShown.connect(self._progressShown)
        self.progressbar.progressHidden.connect(self._progressHidden)

        self._oldStatus = ''

        self._hiddenItems = {}
    
    def SetProperty(self, name, value):
        """!Sets property represented by one of contained SbItems
            
        @param name name of SbItem (from name attribute)
        @param value value to be set
        """
        self.statusbarItems[name].SetValue(value)
        
    def GetProperty(self, name):
        """!Returns property represented by one of contained SbItems
        
        @param name name of SbItem (from name attribute)
        """
        return self.statusbarItems[name].GetValue()
        
    def HasProperty(self, name):
        """!Checks whether property is represented by one of contained SbItems
        
        @param name name of SbItem (from name attribute)
        
        @returns True if particular SbItem is contained, False otherwise
        """
        if name in self.statusbarItems:
            return True
        return False
    
    def AddStatusbarItem(self, item):
        """!Adds item to statusbar
        
        If item position is 0, item is managed by choice.        
        
        @see AddStatusbarItemsByClass
        """
        self.statusbarItems[item.name] = item
        if item.GetPosition() == 0:
            self.choice.Append(item.label, clientData = item) #attrError?
            
    def AddStatusbarItemsByClass(self, itemClasses, **kwargs):
        """!Adds items to statusbar

        @param itemClasses list of classes of items to be add
        @param kwargs SbItem constructor parameters
        
        @see AddStatusbarItem
        """
        for Item in itemClasses:
            item = Item(**kwargs)
            self.AddStatusbarItem(item)
                      
    def HideStatusbarChoiceItemsByClass(self, itemClasses):
        """!Hides items showed in choice
        
        Hides items with position 0 (items showed in choice) by removing
        them from choice.
        
        @param itemClasses list of classes of items to be hided
        
        @see ShowStatusbarChoiceItemsByClass
        @todo consider adding similar function which would take item names
        """
        index = []
        for itemClass in itemClasses:
            for i in range(0, self.choice.GetCount() - 1):
                item = self.choice.GetClientData(i)
                if item.__class__ == itemClass:
                    index.append(i)
                    self._hiddenItems[i] = item
        # must be sorted in reverse order to be removed correctly
        for i in sorted(index, reverse = True):
            self.choice.Delete(i)
        
    def ShowStatusbarChoiceItemsByClass(self, itemClasses):
        """!Shows items showed in choice
        
        Shows items with position 0 (items showed in choice) by adding
        them to choice.
        Items are restored in their old positions.
        
        @param itemClasses list of classes of items to be showed
        
        @see HideStatusbarChoiceItemsByClass
        """
        # must be sorted to be inserted correctly
        for pos in sorted(self._hiddenItems.keys()):
            item = self._hiddenItems[pos]
            if item.__class__ in itemClasses:
                self.choice.Insert(item.label, pos, item)
        
    def ShowItem(self, itemName):
        """!Invokes showing of particular item
        
        @see Update
        """
        if self.statusbarItems[itemName].GetPosition() != 0 or \
           not self.progressbar.IsShown():
            self.statusbarItems[itemName].Show()
        
    def _postInit(self):
        """!Post-initialization method
        
        It sets internal user settings,
        set choice's selection (from user settings) and does reposition.
        It needs choice filled by items.
        it is called automatically.
        """
        UserSettings.Set(group = 'display',
                         key = 'statusbarMode',
                         subkey = 'choices',
                         value = self.choice.GetItems(),
                         internal = True)
        
        self.choice.SetSelection(UserSettings.Get(group = 'display',
                                                  key = 'statusbarMode',
                                                  subkey = 'selection')) 
        self.Reposition()
        
        self._postInitialized = True
        
    def Update(self):
        """!Updates statusbar

        It always updates mask.
        """
        self.progressbar.Update()

        if not self._postInitialized:
            self._postInit()
        for item in self.statusbarItems.values():
            if item.GetPosition() == 0:
                if not self.progressbar.IsShown():
                    item.Hide()
            else:
                item.Update() # mask, render

        if self.progressbar.IsShown():
            pass
        elif self.choice.GetCount() > 0:
            item = self.choice.GetClientData(self.choice.GetSelection())
            item.Update()
        
    def Reposition(self):
        """!Reposition items in statusbar
        
        Set positions to all items managed by statusbar manager.
        It should not be necessary to call it manually.
        """
        
        widgets = []
        for item in self.statusbarItems.values():
            widgets.append((item.GetPosition(), item.GetWidget()))
            
        widgets.append((1, self.choice))
        widgets.append((1, self.progressbar.GetWidget()))
                
        for idx, win in widgets:
            if not win:
                continue
            rect = self.statusbar.GetFieldRect(idx)
            if idx == 0: # show region / mapscale / process bar
                # -> size
                wWin, hWin = win.GetBestSize()
                # -> position
                # if win == self.statusbarWin['region']:
                # x, y = rect.x + rect.width - wWin, rect.y - 1
                # align left
                # else:
                x, y = rect.x + 3, rect.y - 1
                w, h = wWin, rect.height + 2
            else: # choice || auto-rendering
                x, y = rect.x, rect.y
                w, h = rect.width, rect.height + 1
                if win == self.progressbar.GetWidget():
                    wWin = rect.width - 6
                if idx == 2: # mask
                    x += 5
                    y += 4
                elif idx == 3: # render
                    x += 5
            win.SetPosition((x, y))
            win.SetSize((w, h))
        
    def GetProgressBar(self):
        """!Returns progress bar"""
        return self.progressbar

    def _progressShown(self):
        self._oldStatus = self.statusbar.GetStatusText(0)

    def _progressHidden(self):
        self.statusbar.SetStatusText(self._oldStatus, 0)

    def OnToggleStatus(self, event):
        """!Toggle status text
        """
        self.Update()
        
    def SetMode(self, modeIndex):
        """!Sets current mode
        
        Mode is usually driven by user through choice.
        """
        self.choice.SetSelection(modeIndex)
    
    def GetMode(self):
        """!Returns current mode"""
        return self.choice.GetSelection()

    def SetProgress(self, range, value, text):
        """Update progress."""
        self.progressbar.SetRange(range)
        self.progressbar.SetValue(value)
        if text:
            self.statusbar.SetStatusText(text)
        
class SbItem:
    """!Base class for statusbar items.
    
    Each item represents functionality (or action) controlled by statusbar
    and related to MapFrame.
    One item is usually connected with one widget but it is not necessary.
    Item can represent property (depends on manager).
    Items are not widgets but can provide interface to them.
    Items usually has requirements to MapFrame instance
    (specified as MapFrame.methodname or MapWindow.methodname).
    
    @todo consider externalizing position (see SbProgress use in SbManager)
    """
    def __init__(self, mapframe, statusbar, position = 0):
        """!
        
        @param mapframe instance of class with MapFrame interface
        @param statusbar statusbar instance (wx.Statusbar)
        @param position item position in statusbar
        
        @todo rewrite Update also in derived classes to take in account item position
        """
        self.mapFrame = mapframe
        self.statusbar = statusbar
        self.position = position
    
    def Show(self):
        """!Invokes showing of underlying widget.
        
        In derived classes it can do what is appropriate for it,
        e.g. showing text on statusbar (only).
        """
        self.widget.Show()
        
    def Hide(self):
        self.widget.Hide()
        
    def SetValue(self, value):
        self.widget.SetValue(value)
    
    def GetValue(self):
        return self.widget.GetValue()
        
    def GetPosition(self):
        return self.position
    
    def GetWidget(self):
        """!Returns underlaying winget.
        
        @return widget or None if doesn't exist
        """
        return self.widget
    
    def _update(self, longHelp):
        """!Default implementation for Update method.
        
        @param longHelp True to enable long help (help from toolbars)
        """
        self.statusbar.SetStatusText("", 0)
        self.Show()
        self.mapFrame.StatusbarEnableLongHelp(longHelp)
        
    def Update(self):
        """!Called when statusbar action is activated (e.g. through wx.Choice).
        """
        self._update(longHelp = False)

class SbRender(SbItem):
    """!Checkbox to enable and disable auto-rendering.
    
    Requires MapFrame.OnRender method.
    """
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'render'
        
        self.widget = wx.CheckBox(parent = self.statusbar, id = wx.ID_ANY,
                                  label = _("Render"))
        
        self.widget.SetValue(UserSettings.Get(group = 'display',
                                              key = 'autoRendering',
                                              subkey = 'enabled'))
        self.widget.Hide()
        self.widget.SetToolTip(wx.ToolTip (_("Enable/disable auto-rendering")))
                                           
        self.autoRender = Signal('SbRender.autoRender')
        self.widget.Bind(wx.EVT_CHECKBOX, lambda evt: 
                                          self.autoRender.emit(state = self.GetValue()))

    def Update(self):
        self.Show()
        
class SbShowRegion(SbItem):
    """!Checkbox to enable and disable showing of computational region.
    
    Requires MapFrame.OnRender, MapFrame.IsAutoRendered, MapFrame.GetWindow.
    Expects that instance returned by MapFrame.GetWindow will handle
    regionCoords attribute. 
    """
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'region'
        self.label = _("Show comp. extent")
        
        self.widget = wx.CheckBox(parent = self.statusbar, id = wx.ID_ANY,
                                  label = _("Show computational extent"))
        
        self.widget.SetValue(False)
        self.widget.Hide()
        self.widget.SetToolTip(wx.ToolTip (_("Show/hide computational "
                                             "region extent (set with g.region). "
                                             "Display region drawn as a blue box inside the "
                                             "computational region, "
                                             "computational region inside a display region "
                                             "as a red box).")))
                                            
        self.widget.Bind(wx.EVT_CHECKBOX, self.OnToggleShowRegion)
    
    def OnToggleShowRegion(self, event):
        """!Shows/Hides extent (comp. region) in map canvas.
        
        Shows or hides according to checkbox value.

        @todo needs refactoring
        """
        if self.widget.GetValue():
            # show extent
            for mapWindow in self.mapFrame.GetWindows():
                mapWindow.regionCoords = []
        elif hasattr(self.mapFrame.GetWindow(), 'regionCoords'):
            for mapWindow in self.mapFrame.GetWindows():
                del mapWindow.regionCoords

        # redraw map if auto-rendering is enabled
        if self.mapFrame.IsAutoRendered():
            self.mapFrame.OnRender(None)

    def SetValue(self, value):
        SbItem.SetValue(self, value)
        if value:
            for mapWindow in self.mapFrame.GetWindows():
                mapWindow.regionCoords = []
        elif hasattr(self.mapFrame.GetWindow(), 'regionCoords'):
            # TODO: this maybe never happends
            for mapWindow in self.mapFrame.GetWindows():
                mapWindow.regionCoords = []
        
class SbAlignExtent(SbItem):
    """!Checkbox to select zoom behavior.
    
    Used by BufferedWindow (through MapFrame property).
    See tooltip for explanation.
    """
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'alignExtent'
        self.label = _("Display mode")
        
        self.widget = wx.CheckBox(parent = self.statusbar, id = wx.ID_ANY,
                                  label = _("Align region extent based on display size"))
        
        self.widget.SetValue(UserSettings.Get(group = 'display', key = 'alignExtent', subkey = 'enabled'))
        self.widget.Hide()
        self.widget.SetToolTip(wx.ToolTip (_("Align region extent based on display "
                                             "size from center point. "
                                             "Default value for new map displays can "
                                             "be set up in 'User GUI settings' dialog.")))      
        
class SbResolution(SbItem):
    """!Checkbox to select used display resolution.
    
    Requires MapFrame.OnRender method. 
    """
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'resolution'
        self.label = _("Display resolution")
        
        self.widget = wx.CheckBox(parent = self.statusbar, id = wx.ID_ANY,
                                  label = _("Constrain display resolution to computational settings"))
        
        self.widget.SetValue(UserSettings.Get(group = 'display', key = 'compResolution', subkey = 'enabled'))
        self.widget.Hide()
        self.widget.SetToolTip(wx.ToolTip (_("Constrain display resolution "
                                             "to computational region settings. "
                                             "Default value for new map displays can "
                                             "be set up in 'User GUI settings' dialog.")))
                                            
        self.widget.Bind(wx.EVT_CHECKBOX, self.OnToggleUpdateMap)
        
    def OnToggleUpdateMap(self, event):
        """!Update display when toggle display mode
        """
        # redraw map if auto-rendering is enabled
        if self.mapFrame.IsAutoRendered():
            self.mapFrame.OnRender(None)


class SbMapScale(SbItem):
    """!Editable combobox to get/set current map scale.
    
    Requires MapFrame.GetMapScale, MapFrame.SetMapScale
    and MapFrame.GetWindow (and GetWindow().UpdateMap()).
    """
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'mapscale'
        self.label = _("Map scale")
        
        self.widget = wx.ComboBox(parent = self.statusbar, id = wx.ID_ANY,
                                                    style = wx.TE_PROCESS_ENTER,
                                                    size = (150, -1))
        
        self.widget.SetItems(['1:1000',
                              '1:5000',
                              '1:10000',
                              '1:25000',
                              '1:50000',
                              '1:100000',
                              '1:1000000'])
        self.widget.Hide()
        self.widget.SetToolTip(wx.ToolTip (_("As everyone's monitors and resolutions "
                                            "are set differently these values are not "
                                            "true map scales, but should get you into "
                                            "the right neighborhood.")))
                                            
        self.widget.Bind(wx.EVT_TEXT_ENTER, self.OnChangeMapScale)
        self.widget.Bind(wx.EVT_COMBOBOX, self.OnChangeMapScale)
        
        self.lastMapScale = None

    def Update(self):
        scale = self.mapFrame.GetMapScale()
        self.statusbar.SetStatusText("")
        try:
            self.SetValue("1:%ld" % (scale + 0.5))
        except TypeError:
            pass # FIXME, why this should happen?
        
        self.lastMapScale = scale
        self.Show()

        # disable long help
        self.mapFrame.StatusbarEnableLongHelp(False)

    def OnChangeMapScale(self, event):
        """!Map scale changed by user
        """
        scale = event.GetString()

        try:
            if scale[:2] != '1:':
                raise ValueError
            value = int(scale[2:])
        except ValueError:
            self.SetValue('1:%ld' % int(self.lastMapScale))
            return
        
        self.mapFrame.SetMapScale(value)
        
        # redraw a map
        self.mapFrame.GetWindow().UpdateMap()
        self.GetWidget().SetFocus()
        
        
class SbGoTo(SbItem):
    """!Textctrl to set coordinates which to focus on.
    
    Requires MapFrame.GetWindow, MapWindow.GoTo method.
    """
    
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'goto'
        self.label = _("Go to")
        
        self.widget = wx.TextCtrl(parent = self.statusbar, id = wx.ID_ANY,
                                                value = "", style = wx.TE_PROCESS_ENTER,
                                                size = (300, -1))
        
        self.widget.Hide()
        
        self.widget.Bind(wx.EVT_TEXT_ENTER, self.OnGoTo)
    
    def ReprojectENToMap(self, e, n, useDefinedProjection):
        """!Reproject east, north from user defined projection
        
        @param e,n coordinate (for DMS string, else float or string)
        @param useDefinedProjection projection defined by user in settings dialog
        
        @throws SbException if useDefinedProjection is True and projection is not defined in UserSettings
        """
        if useDefinedProjection:
            settings = UserSettings.Get(group = 'projection', key = 'statusbar', subkey = 'proj4')
            if not settings:
                raise SbException(_("Projection not defined (check the settings)"))
            else:
                # reproject values
                projIn = settings
                projOut = RunCommand('g.proj',
                                     flags = 'jf',
                                     read = True)
                proj = projIn.split(' ')[0].split('=')[1]
                if proj in ('ll', 'latlong', 'longlat'):
                    e, n = utils.DMS2Deg(e, n)
                    proj, coord1 = utils.ReprojectCoordinates(coord = (e, n),
                                                              projIn = projIn,
                                                              projOut = projOut, flags = 'd')
                    e, n = coord1
                else:
                    e, n = float(e), float(n)
                    proj, coord1 = utils.ReprojectCoordinates(coord = (e, n),
                                                              projIn = projIn,
                                                              projOut = projOut, flags = 'd')
                    e, n = coord1
        elif self.mapFrame.GetMap().projinfo['proj'] == 'll':
            e, n = utils.DMS2Deg(e, n)
        else: 
            e, n = float(e), float(n)
        return e, n

    def OnGoTo(self, event):
        """!Go to position
        """
        try:
            e, n = self.GetValue().split(';')
            e, n = self.ReprojectENToMap(e, n, self.mapFrame.GetProperty('projection'))
            self.mapFrame.GetWindow().GoTo(e, n)
            self.widget.SetFocus()
        except ValueError:
            # FIXME: move this code to MapWindow/BufferedWindow/MapFrame
            region = self.mapFrame.GetMap().GetCurrentRegion()
            precision = int(UserSettings.Get(group = 'projection', key = 'format',
                                             subkey = 'precision'))
            format = UserSettings.Get(group = 'projection', key = 'format',
                                      subkey = 'll')
            if self.mapFrame.GetMap().projinfo['proj'] == 'll' and format == 'DMS':
                self.SetValue("%s" % utils.Deg2DMS(region['center_easting'], 
                                                                            region['center_northing'],
                                                                            precision = precision))
            else:
                self.SetValue("%.*f; %.*f" % \
                               (precision, region['center_easting'],
                                precision, region['center_northing']))
        except SbException, e:
            # FIXME: this may be useless since statusbar update checks user defined projection and this exception raises when user def proj does not exists
            self.statusbar.SetStatusText(str(e), 0)

    def GetCenterString(self, map):
        """!Get current map center in appropriate format"""
        region = map.GetCurrentRegion()
        precision = int(UserSettings.Get(group = 'projection', key = 'format',
                                         subkey = 'precision'))
        format = UserSettings.Get(group = 'projection', key = 'format',
                                  subkey = 'll')
        projection = UserSettings.Get(group='projection', key='statusbar', subkey='proj4')
        
        if self.mapFrame.GetProperty('projection'):
            if not projection:
                raise SbException(_("Projection not defined (check the settings)"))
            else:
                proj, coord  = utils.ReprojectCoordinates(coord = (region['center_easting'],
                                                                   region['center_northing']),
                                                          projOut = projection,
                                                          flags = 'd')
                if coord:
                    if proj in ('ll', 'latlong', 'longlat') and format == 'DMS':
                        return "%s" % utils.Deg2DMS(coord[0],
                                                                                coord[1],
                                                                                precision = precision)
                    else:
                        return "%.*f; %.*f" % (precision, coord[0], precision, coord[1])
                else:
                    raise SbException(_("Error in projection (check the settings)"))
        else:
            if self.mapFrame.GetMap().projinfo['proj'] == 'll' and format == 'DMS':
                return "%s" % utils.Deg2DMS(region['center_easting'], region['center_northing'],
                                                                      precision = precision)
            else:
                return "%.*f; %.*f" % (precision, region['center_easting'], precision, region['center_northing'])


    def SetCenter(self):
        """!Set current map center as item value"""
        center = self.GetCenterString(self.mapFrame.GetMap())
        self.SetValue(center)
        
    def Update(self):
        self.statusbar.SetStatusText("")
        
        try:
            self.SetCenter()
            self.Show()
        except SbException, e:
            self.statusbar.SetStatusText(str(e), 0)
                        
        # disable long help
        self.mapFrame.StatusbarEnableLongHelp(False)
        

class SbProjection(SbItem):
    """!Checkbox to enable user defined projection (can be set in settings)"""
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'projection'
        self.label = _("Projection")
        
        self.defaultLabel = _("Use defined projection")
        
        self.widget = wx.CheckBox(parent = self.statusbar, id = wx.ID_ANY,
                                  label = self.defaultLabel)
        
        self.widget.SetValue(False)
        
        # necessary?
        size = self.widget.GetSize()
        self.widget.SetMinSize((size[0] + 150, size[1]))
        
        self.widget.Hide()
        self.widget.SetToolTip(wx.ToolTip (_("Reproject coordinates displayed "
                                             "in the statusbar. Projection can be "
                                             "defined in GUI preferences dialog "
                                             "(tab 'Projection')")))
                                            
    def Update(self):
        self.statusbar.SetStatusText("")
        epsg = UserSettings.Get(group = 'projection', key = 'statusbar', subkey = 'epsg')
        if epsg:
            label = '%s (EPSG: %s)' % (self.defaultLabel, epsg)
            self.widget.SetLabel(label)
        else:
            self.widget.SetLabel(self.defaultLabel)
        self.Show()
        
        # disable long help
        self.mapFrame.StatusbarEnableLongHelp(False)
        

class SbMask(SbItem):
    """!StaticText to show whether mask is activated."""
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'mask'
        
        self.widget = wx.StaticText(parent = self.statusbar, id = wx.ID_ANY, label = _('MASK'))
        self.widget.SetForegroundColour(wx.Colour(255, 0, 0))
        self.widget.Hide()
        
    def Update(self):
        if grass.find_file(name = 'MASK', element = 'cell',
                           mapset = grass.gisenv()['MAPSET'])['name']:
            self.Show()
        else:
            self.Hide()
        
class SbTextItem(SbItem):
    """!Base class for items without widgets.
    
    Only sets statusbar text.
    """
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        
        self.text = None
        
    def Show(self):
        self.statusbar.SetStatusText(self.GetValue(), self.position)
        
    def Hide(self):
        self.statusbar.SetStatusText("", self.position)
        
    def SetValue(self, value):
        self.text = value
    
    def GetValue(self):
        return self.text
            
    def GetWidget(self):
        return None
    
    def Update(self):
        self._update(longHelp = True)

class SbDisplayGeometry(SbTextItem):
    """!Show current display resolution."""
    def __init__(self, mapframe, statusbar, position = 0):
        SbTextItem.__init__(self, mapframe, statusbar, position)
        self.name = 'displayGeometry'
        self.label = _("Display geometry")
        
    def Show(self):
        region = copy.copy(self.mapFrame.GetMap().GetCurrentRegion())
        if self.mapFrame.GetProperty('resolution'):
            compRegion = self.mapFrame.GetMap().GetRegion(add3d = False)
            region['rows'] = abs(int((region['n'] - region['s']) / compRegion['nsres']) + 0.5)
            region['cols'] = abs(int((region['e'] - region['w']) / compRegion['ewres']) + 0.5)
            region['nsres'] = compRegion['nsres']
            region['ewres'] = compRegion['ewres']
        self.SetValue("rows=%d; cols=%d; nsres=%.2f; ewres=%.2f" %
                     (region["rows"], region["cols"],
                      region["nsres"], region["ewres"]))
        SbTextItem.Show(self)

class SbCoordinates(SbTextItem):
    """!Show map coordinates when mouse moves.
    
    Requires MapWindow.GetLastEN method."""
    def __init__(self, mapframe, statusbar, position = 0):
        SbTextItem.__init__(self, mapframe, statusbar, position)
        self.name = 'coordinates'
        self.label = _("Coordinates")
        
    def Show(self):
        precision = int(UserSettings.Get(group = 'projection', key = 'format',
                             subkey = 'precision'))
        format = UserSettings.Get(group = 'projection', key = 'format',
                                       subkey = 'll')
        projection = self.mapFrame.GetProperty('projection')
        try:
            e, n = self.mapFrame.GetWindow().GetLastEN()
            self.SetValue(self.ReprojectENFromMap(e, n, projection, precision, format))
        except SbException, e:
            self.SetValue(e)
        except TypeError, e:
            self.SetValue("")
        except AttributeError:
            self.SetValue("") # during initialization MapFrame has no MapWindow
        SbTextItem.Show(self)
        
    def ReprojectENFromMap(self, e, n, useDefinedProjection, precision, format):
        """!Reproject east, north to user defined projection.
        
        @param e,n coordinate
        
        @throws SbException if useDefinedProjection is True and projection is not defined in UserSettings
        """
        if useDefinedProjection:
            settings = UserSettings.Get(group = 'projection', key = 'statusbar', subkey = 'proj4')
            if not settings:
                raise SbException(_("Projection not defined (check the settings)"))
            else:
                # reproject values
                proj, coord  = utils.ReprojectCoordinates(coord = (e, n),
                                                          projOut = settings,
                                                          flags = 'd')
                if coord:
                    e, n = coord
                    if proj in ('ll', 'latlong', 'longlat') and format == 'DMS':
                        return utils.Deg2DMS(e, n, precision = precision)
                    else:
                        return "%.*f; %.*f" % (precision, e, precision, n)
                else:
                    raise SbException(_("Error in projection (check the settings)"))
        else:
            if self.mapFrame.GetMap().projinfo['proj'] == 'll' and format == 'DMS':
                return utils.Deg2DMS(e, n, precision = precision)
            else:
                return "%.*f; %.*f" % (precision, e, precision, n)
        
class SbRegionExtent(SbTextItem):
    """!Shows current display region"""
    def __init__(self, mapframe, statusbar, position = 0):
        SbTextItem.__init__(self, mapframe, statusbar, position)
        self.name = 'displayRegion'
        self.label = _("Extent")
        
    def Show(self):
        precision = int(UserSettings.Get(group = 'projection', key = 'format',
                             subkey = 'precision'))
        format = UserSettings.Get(group = 'projection', key = 'format',
                                       subkey = 'll')
        projection = self.mapFrame.GetProperty('projection')        
        region = self._getRegion()
        try:
            regionReprojected = self.ReprojectRegionFromMap(region, projection, precision, format)
            self.SetValue(regionReprojected)
        except SbException, e:
            self.SetValue(e)
        SbTextItem.Show(self)
    
    def _getRegion(self):
        """!Get current display region"""
        return self.mapFrame.GetMap().GetCurrentRegion() # display region
        
    def _formatRegion(self, w, e, s, n, nsres, ewres, precision = None):
        """!Format display region string for statusbar

        @param nsres,ewres unused
        """
        if precision is not None:
            return "%.*f - %.*f, %.*f - %.*f" % (precision, w, precision, e,
                                                 precision, s, precision, n)
        else:
            return "%s - %s, %s - %s" % (w, e, s, n)
         
           
    def ReprojectRegionFromMap(self, region, useDefinedProjection, precision, format):
        """!Reproject region values
        
        @todo reorganize this method to remove code useful only for derived class SbCompRegionExtent
        """
        if useDefinedProjection:
            settings = UserSettings.Get(group = 'projection', key = 'statusbar', subkey = 'proj4')
            
            if not settings:
                raise SbException(_("Projection not defined (check the settings)"))
            else:
                projOut = settings
                proj, coord1 = utils.ReprojectCoordinates(coord = (region["w"], region["s"]),
                                                          projOut = projOut, flags = 'd')
                proj, coord2 = utils.ReprojectCoordinates(coord = (region["e"], region["n"]),
                                                          projOut = projOut, flags = 'd')
                # useless, used in derived class
                proj, coord3 = utils.ReprojectCoordinates(coord = (0.0, 0.0),
                                                          projOut = projOut, flags = 'd')
                proj, coord4 = utils.ReprojectCoordinates(coord = (region["ewres"], region["nsres"]),
                                                          projOut = projOut, flags = 'd')
                if coord1 and coord2:
                    if proj in ('ll', 'latlong', 'longlat') and format == 'DMS':
                        w, s = utils.Deg2DMS(coord1[0], coord1[1], string = False,
                                             precision = precision)
                        e, n = utils.Deg2DMS(coord2[0], coord2[1], string = False,
                                             precision = precision)
                        ewres, nsres = utils.Deg2DMS(abs(coord3[0]) - abs(coord4[0]),
                                                         abs(coord3[1]) - abs(coord4[1]),
                                                         string = False, hemisphere = False,
                                                         precision = precision)
                        return self._formatRegion(w = w, s = s, e = e, n = n, ewres = ewres, nsres = nsres)
                    else:
                        w, s = coord1
                        e, n = coord2
                        ewres, nsres = coord3
                        return self._formatRegion(w = w, s = s, e = e, n = n, ewres = ewres,
                                                  nsres = nsres, precision = precision)
                else:
                    raise SbException(_("Error in projection (check the settings)"))
                
        else:
            if self.mapFrame.GetMap().projinfo['proj'] == 'll' and format == 'DMS':
                w, s = utils.Deg2DMS(region["w"], region["s"],
                                     string = False, precision = precision)
                e, n = utils.Deg2DMS(region["e"], region["n"],
                                     string = False, precision = precision)
                ewres, nsres = utils.Deg2DMS(region['ewres'], region['nsres'],
                                             string = False, precision = precision)
                return self._formatRegion(w = w, s = s, e = e, n = n, ewres = ewres, nsres = nsres)
            else:
                w, s = region["w"], region["s"]
                e, n = region["e"], region["n"]
                ewres, nsres = region['ewres'], region['nsres']
                return self._formatRegion(w = w, s = s, e = e, n = n, ewres = ewres,
                                          nsres = nsres, precision = precision)
                                
                                
class SbCompRegionExtent(SbRegionExtent):
    """!Shows computational region."""
    def __init__(self, mapframe, statusbar, position = 0):
        SbRegionExtent.__init__(self, mapframe, statusbar, position)
        self.name = 'computationalRegion'
        self.label = _("Computational region")
        
    def _formatRegion(self, w, e, s, n, ewres, nsres, precision = None):
        """!Format computational region string for statusbar"""
        if precision is not None:
            return "%.*f - %.*f, %.*f - %.*f (%.*f, %.*f)" % (precision, w, precision, e,
                                                              precision, s, precision, n,
                                                              precision, ewres, precision, nsres)
        else:
            return "%s - %s, %s - %s (%s, %s)" % (w, e, s, n, ewres, nsres)
        
    def _getRegion(self):
        """!Returns computational region."""
        return self.mapFrame.GetMap().GetRegion() # computational region
        
        
class SbProgress(SbItem):
    """!General progress bar to show progress.
    
    Underlaying widget is wx.Gauge.
    """
    def __init__(self, mapframe, statusbar, sbManager, position = 0):
        self.progressShown = Signal('SbProgress.progressShown')
        self.progressHidden = Signal('SbProgress.progressHidden')
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'progress'
        self.sbManager = sbManager
        # on-render gauge
        self.widget = wx.Gauge(parent = self.statusbar, id = wx.ID_ANY,
                               range = 0, style = wx.GA_HORIZONTAL)
        self.Hide()
        
        
    def GetRange(self):
        """!Returns progress range."""
        return self.widget.GetRange()
    
    def SetRange(self, range):
        """!Sets progress range."""
        if range > 0:        
            if self.GetRange() != range:
                self.widget.SetRange(range)
            self.Show()
        else:
            self.Hide()
    
    def Show(self):
        if not self.IsShown():
            self.progressShown.emit()
            self.widget.Show()

    def Hide(self):
        if self.IsShown():
            self.progressHidden.emit()
            self.widget.Hide()

    def IsShown(self):
        """!Is progress bar shown
        """
        return self.widget.IsShown()

    def SetValue(self, value):
        """!Sets value of progressbar.
        
        Calls wx.Yield which allows
        to update gui for displaying progress.
        """
        self.SafeSetValue(self, value)
        wx.Yield()

    def SafeSetValue(self, value):
        """! Thread save SetValue method.
        
        Needed for wxNVIZ.
        """
        if value > self.GetRange():
            return
        self.widget.SetValue(value)
        if value == self.GetRange():
            self.Hide()


    def GetWidget(self):
        """!Returns underlaying winget.
        
        @return widget or None if doesn't exist
        """
        return self.widget

    def Update(self):
        pass
    
class SbGoToGCP(SbItem):
    """!SpinCtrl to select GCP to focus on
    
    Requires MapFrame.GetSrcWindow, MapFrame.GetTgtWindow, MapFrame.GetListCtrl,
    MapFrame.GetMapCoordList.
    """
    
    def __init__(self, mapframe, statusbar, position = 0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'gotoGCP'
        self.label = _("Go to GCP No.")

        self.widget = wx.SpinCtrl(parent = self.statusbar, id = wx.ID_ANY,
                                                value = "", min = 0)
        self.widget.Hide()
        
        self.widget.Bind(wx.EVT_TEXT_ENTER, self.OnGoToGCP)
        self.widget.Bind(wx.EVT_SPINCTRL, self.OnGoToGCP)
    
    def OnGoToGCP(self, event):
        """!Zooms to given GCP."""
        GCPNo = self.GetValue()
        mapCoords = self.mapFrame.GetMapCoordList()
        
        if GCPNo < 0 or GCPNo > len(mapCoords): # always false, spin checks it
            GMessage(parent=self,
                     message="%s 1 - %s." % (_("Valid Range:"),
                                             len(mapCoords)))
            return

        if GCPNo == 0:
            return
            
        listCtrl = self.mapFrame.GetListCtrl()
        
        listCtrl.selectedkey = GCPNo
        listCtrl.selected = listCtrl.FindItemData(-1, GCPNo)
        listCtrl.render = False
        listCtrl.SetItemState(listCtrl.selected,
                          wx.LIST_STATE_SELECTED,
                          wx.LIST_STATE_SELECTED)
        listCtrl.render = True
        
        listCtrl.EnsureVisible(listCtrl.selected)

        srcWin = self.mapFrame.GetSrcWindow()
        tgtWin = self.mapFrame.GetTgtWindow()
        
        # Source MapWindow:
        begin = (mapCoords[GCPNo][1], mapCoords[GCPNo][2])
        begin = srcWin.Cell2Pixel(begin)
        end = begin
        srcWin.Zoom(begin, end, 0)

        # redraw map
        srcWin.UpdateMap()

        if self.mapFrame.GetShowTarget():
            # Target MapWindow:
            begin = (mapCoords[GCPNo][3], mapCoords[GCPNo][4])
            begin = tgtWin.Cell2Pixel(begin)
            end = begin
            tgtWin.Zoom(begin, end, 0)

            # redraw map
            tgtWin.UpdateMap()

        self.GetWidget().SetFocus()
    
    def Update(self):
        self.statusbar.SetStatusText("")
        max = self.mapFrame.GetListCtrl().GetItemCount()
        if max < 1:
            max = 1
        self.widget.SetRange(0, max)
        self.Show()
                        
        # disable long help
        self.mapFrame.StatusbarEnableLongHelp(False)
        
class SbRMSError(SbTextItem):
    """!Shows RMS error.
    
    Requires MapFrame.GetFwdError, MapFrame.GetBkwError.
    """
    def __init__(self, mapframe, statusbar, position = 0):
        SbTextItem.__init__(self, mapframe, statusbar, position)
        self.name = 'RMSError'
        self.label = _("RMS error")
        
    def Show(self):
        self.SetValue(_("Forward: %(forw)s, Backward: %(back)s") %
                                   { 'forw' : self.mapFrame.GetFwdError(),
                                     'back' : self.mapFrame.GetBkwError() })
        SbTextItem.Show(self)
        
