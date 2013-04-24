"""!
@package gui_core.preferences

@brief User preferences dialog

Sets default display font, etc.  If you want to add some value to
settings you have to add default value to defaultSettings and set
constraints in internalSettings in Settings class. Everything can be
used in PreferencesDialog.

Classes:
 - preferences::PreferencesBaseDialog
 - preferences::PreferencesDialog
 - preferences::DefaultFontDialog
 - preferences::MapsetAccess
 - preferences::CheckListMapset

(C) 2007-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
@author Luca Delucchi <lucadeluge gmail.com> (language choice)
"""

import os
import sys
import copy
try:
    import pwd
    havePwd = True
except ImportError:
    havePwd = False

import wx
import wx.lib.colourselect    as csel
import wx.lib.mixins.listctrl as listmix
import wx.lib.scrolledpanel as SP

from grass.pydispatch.signal import Signal

from grass.script import core as grass

from core          import globalvar
from core.gcmd     import RunCommand, GError
from core.utils    import ListOfMapsets, GetColorTables, ReadEpsgCodes, StoreEnvVariable
from core.settings import UserSettings
from gui_core.dialogs import SymbolDialog
from gui_core.widgets import IntegerValidator

class PreferencesBaseDialog(wx.Dialog):
    """!Base preferences dialog"""
    def __init__(self, parent, giface, settings, title = _("User settings"),
                 size = (500, 475),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        self.parent = parent # ModelerFrame
        self.title  = title
        self.size   = size
        self.settings = settings
        self._giface = giface
        
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title,
                           style = style)
        
        self.settingsChanged = Signal('PreferencesBaseDialog.settingsChanged')
        
        # notebook
        self.notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)
        
        # dict for window ids
        self.winId = {}
        
        # create notebook pages
        
        # buttons
        self.btnDefault = wx.Button(self, wx.ID_ANY, _("Set to default"))
        self.btnSave = wx.Button(self, wx.ID_SAVE)
        self.btnApply = wx.Button(self, wx.ID_APPLY)
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnSave.SetDefault()
        
        # bindigs
        self.btnDefault.Bind(wx.EVT_BUTTON, self.OnDefault)
        self.btnDefault.SetToolTipString(_("Revert settings to default and apply changes"))
        self.btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        self.btnApply.SetToolTipString(_("Apply changes for the current session"))
        self.btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        self.btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))
        self.btnSave.SetDefault()
        self.btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        self.btnCancel.SetToolTipString(_("Close dialog and ignore changes"))

        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        self._layout()
        
    def _layout(self):
        """!Layout window"""
        # sizers
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = self.btnDefault, proportion = 1,
                     flag = wx.ALL, border = 5)
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnCancel)
        btnStdSizer.AddButton(self.btnSave)
        btnStdSizer.AddButton(self.btnApply)
        btnStdSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = self.notebook, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND, border = 0)
        mainSizer.Add(item = btnStdSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT, border = 5)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        
    def OnDefault(self, event):
        """!Button 'Set to default' pressed"""
        self.settings.userSettings = copy.deepcopy(self.settings.defaultSettings)
        
        # update widgets
        for gks in self.winId.keys():
            try:
                group, key, subkey = gks.split(':')
                value = self.settings.Get(group, key, subkey)
            except ValueError:
                group, key, subkey, subkey1 = gks.split(':')
                value = self.settings.Get(group, key, [subkey, subkey1])
            win = self.FindWindowById(self.winId[gks])
            
            if win.GetName() in ('GetValue', 'IsChecked'):
                value = win.SetValue(value)
            elif win.GetName() == 'GetSelection':
                value = win.SetSelection(value)
            elif win.GetName() == 'GetStringSelection':
                value = win.SetStringSelection(value)
            elif win.GetName() == 'GetLabel':
                value = win.SetLabel(value)
            else:
                value = win.SetValue(value)
        
    def OnApply(self, event):
        """!Button 'Apply' pressed
        Emits signal settingsChanged.
        """
        if self._updateSettings():
            self._giface.WriteLog(_('Settings applied to current session but not saved'))
            self.settingsChanged.emit()
            self.Close()

    def OnCloseWindow(self, event):
        self.Hide()
        
    def OnCancel(self, event):
        """!Button 'Cancel' pressed"""
        self.Close()
    
    def OnSave(self, event):
        """!Button 'Save' pressed
        Emits signal settingsChanged.
        """
        if self._updateSettings():
            lang = self.settings.Get(group = 'language', key = 'locale', subkey = 'lc_all')
            if lang == 'system':
                # Most fool proof way to use system locale is to not provide any locale info at all
                self.settings.Set(group = 'language', key = 'locale', subkey = 'lc_all', value = None)
                lang = None
            if lang == 'en':
                # GRASS doesn't ship EN translation, default texts have to be used instead
                self.settings.Set(group = 'language', key = 'locale', subkey = 'lc_all', value = 'C')
                lang = 'C'
            self.settings.SaveToFile()
            self._giface.WriteLog(_('Settings saved to file \'%s\'.') % self.settings.filePath)
            if lang:
                StoreEnvVariable(key = 'LANG', value = lang)
            else:
                StoreEnvVariable(key = 'LANG')
            self.settingsChanged.emit()
            self.Close()

    def _updateSettings(self):
        """!Update user settings"""
        for item in self.winId.keys():
            try:
                group, key, subkey = item.split(':')
                subkey1 = None
            except ValueError:
                group, key, subkey, subkey1 = item.split(':')
            
            id = self.winId[item]
            win = self.FindWindowById(id)
            if win.GetName() == 'GetValue':
                value = win.GetValue()
            elif win.GetName() == 'GetSelection':
                value = win.GetSelection()
            elif win.GetName() == 'IsChecked':
                value = win.IsChecked()
            elif win.GetName() == 'GetStringSelection':
                value = win.GetStringSelection()
            elif win.GetName() == 'GetLabel':
                value = win.GetLabel()
            elif win.GetName() == 'GetColour':
                value = tuple(win.GetValue())
            else:
                value = win.GetValue()

            if key == 'keycolumn' and value == '':
                wx.MessageBox(parent = self,
                              message = _("Key column cannot be empty string."),
                              caption = _("Error"), style = wx.OK | wx.ICON_ERROR)
                win.SetValue(self.settings.Get(group = 'atm', key = 'keycolumn', subkey = 'value'))
                return False
            if key == 'fieldSeparator':
                try:
                    value = str(value)
                except UnicodeEncodeError:
                    GError(parent = self, message = _("Field separator must be ASCII character "
                                                      "not <%s> (use e.g. ';', '&', '#')") % value)
                    return False
                if value == '':
                    value = self.settings.Get(group = 'atm', key = 'fieldSeparator', subkey = 'value')
            if subkey1:
                self.settings.Set(group, value, key, [subkey, subkey1])
            else:
                self.settings.Set(group, value, key, subkey)
        
        if self.parent.GetName() == 'Modeler':
            return True
        
        #
        # update default window dimension
        #
        if self.settings.Get(group = 'general', key = 'defWindowPos', subkey = 'enabled') is True:
            dim = ''
            # layer manager
            pos = self.parent.GetPosition()
            size = self.parent.GetSize()
            dim = '%d,%d,%d,%d' % (pos[0], pos[1], size[0], size[1])
            # opened displays
            for mapdisp in self._giface.GetAllMapDisplays():
                pos  = mapdisp.GetPosition()
                size = mapdisp.GetSize()

                dim += ',%d,%d,%d,%d' % (pos[0], pos[1], size[0], size[1])

            self.settings.Set(group = 'general', key = 'defWindowPos', subkey = 'dim', value = dim)
        else:
            self.settings.Set(group = 'general', key = 'defWindowPos', subkey = 'dim', value = '')

        return True

class PreferencesDialog(PreferencesBaseDialog):
    """!User preferences dialog"""
    def __init__(self, parent, giface, title = _("GUI Settings"),
                 settings = UserSettings):
        PreferencesBaseDialog.__init__(self, parent = parent, giface = giface, title = title,
                                       settings = settings)
        
        # create notebook pages
        self._createGeneralPage(self.notebook)
        self._createAppearancePage(self.notebook)
        self._createDisplayPage(self.notebook)
        self._createCmdPage(self.notebook)
        self._createLayersPage(self.notebook)
        self._createAttributeManagerPage(self.notebook)
        self._createProjectionPage(self.notebook)
        
        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)
        
    def _createGeneralPage(self, notebook):
        """!Create notebook page for general settings"""
        panel = SP.ScrolledPanel(parent = notebook, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False, scroll_y = True)
        notebook.AddPage(page = panel, text = _("General"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        #
        # Layer Manager settings
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Layer Manager settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        
        #
        # ask when removing map layer from layer tree
        #
        row = 0
        askOnRemoveLayer = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                       label = _("Ask when removing map layer from layer tree"),
                                       name = 'IsChecked')
        askOnRemoveLayer.SetValue(self.settings.Get(group = 'manager', key = 'askOnRemoveLayer', subkey = 'enabled'))
        self.winId['manager:askOnRemoveLayer:enabled'] = askOnRemoveLayer.GetId()
        
        gridSizer.Add(item = askOnRemoveLayer,
                      pos = (row, 0), span = (1, 2))
        
        row += 1
        askOnQuit = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                label = _("Ask when quiting wxGUI or closing display"),
                                name = 'IsChecked')
        askOnQuit.SetValue(self.settings.Get(group = 'manager', key = 'askOnQuit', subkey = 'enabled'))
        self.winId['manager:askOnQuit:enabled'] = askOnQuit.GetId()
        
        gridSizer.Add(item = askOnQuit,
                      pos = (row, 0), span = (1, 2))

        row += 1
        hideSearch = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                 label = _("Hide '%s' tab (requires GUI restart)") % _("Search module"),
                                 name = 'IsChecked')
        hideSearch.SetValue(self.settings.Get(group = 'manager', key = 'hideTabs', subkey = 'search'))
        self.winId['manager:hideTabs:search'] = hideSearch.GetId()
        
        gridSizer.Add(item = hideSearch,
                      pos = (row, 0), span = (1, 2))
        
        row += 1
        hidePyShell = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                  label = _("Hide '%s' tab (requires GUI restart)") % _("Python shell"),
                                  name = 'IsChecked')
        hidePyShell.SetValue(self.settings.Get(group = 'manager', key = 'hideTabs', subkey = 'pyshell'))
        self.winId['manager:hideTabs:pyshell'] = hidePyShell.GetId()
        
        gridSizer.Add(item = hidePyShell,
                      pos = (row, 0), span = (1, 2))
        
        #
        # Selected text is copied to clipboard
        #
        row += 1
        copySelectedTextToClipboard = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                                  label = _("Automatically copy selected text to clipboard (in Command console)"),
                                                  name = 'IsChecked')
        copySelectedTextToClipboard.SetValue(self.settings.Get(group = 'manager', key = 'copySelectedTextToClipboard', subkey = 'enabled'))
        self.winId['manager:copySelectedTextToClipboard:enabled'] = copySelectedTextToClipboard.GetId()
        
        gridSizer.Add(item = copySelectedTextToClipboard,
                      pos = (row, 0), span = (1, 2))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)
        
        #
        # workspace
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Workspace settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        
        row = 0
        posDisplay = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                 label = _("Suppress positioning Map Display Window(s)"),
                                 name = 'IsChecked')
        posDisplay.SetValue(self.settings.Get(group = 'general', key = 'workspace',
                                              subkey = ['posDisplay', 'enabled']))
        self.winId['general:workspace:posDisplay:enabled'] = posDisplay.GetId()
        
        gridSizer.Add(item = posDisplay,
                      pos = (row, 0), span = (1, 2))
        
        row += 1 
        
        posManager = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                 label = _("Suppress positioning Layer Manager window"),
                                 name = 'IsChecked')
        posManager.SetValue(self.settings.Get(group = 'general', key = 'workspace',
                                              subkey = ['posManager', 'enabled']))
        self.winId['general:workspace:posManager:enabled'] = posManager.GetId()
        
        gridSizer.Add(item = posManager,
                      pos = (row, 0), span = (1, 2))
        
        row += 1
        defaultPos = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                 label = _("Save current window layout as default"),
                                 name = 'IsChecked')
        defaultPos.SetValue(self.settings.Get(group = 'general', key = 'defWindowPos', subkey = 'enabled'))
        defaultPos.SetToolTip(wx.ToolTip (_("Save current position and size of Layer Manager window and opened "
                                            "Map Display window(s) and use as default for next sessions.")))
        self.winId['general:defWindowPos:enabled'] = defaultPos.GetId()
        
        gridSizer.Add(item = defaultPos,
                      pos = (row, 0), span = (1, 2))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)
        
        panel.SetSizer(border)
        
        return panel

    def _createAppearancePage(self, notebook):
        """!Create notebook page for display settings"""
        panel = SP.ScrolledPanel(parent = notebook, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False, scroll_y = True)
        notebook.AddPage(page = panel, text = _("Appearance"))

        border = wx.BoxSizer(wx.VERTICAL)

        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Font settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)

        #
        # font settings
        #
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)

        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Font for command output:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        outfontButton = wx.Button(parent = panel, id = wx.ID_ANY,
                                  label = _("Set font"))
        gridSizer.Add(item = outfontButton,
                      flag = wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        gridSizer.AddGrowableCol(0)

        #
        # languages
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Language settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)

        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Choose language (requires to save and GRASS restart):")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        locales = self.settings.Get(group = 'language', key = 'locale', 
                                         subkey = 'choices', internal = True)
        loc = self.settings.Get(group = 'language', key = 'locale', subkey = 'lc_all')
        elementList = wx.Choice(parent = panel, id = wx.ID_ANY, size = (325, -1),
                                choices = locales, name = "GetStringSelection")
        if loc in locales:
            elementList.SetStringSelection(loc)
        if loc == 'C':
            elementList.SetStringSelection('en')
        self.winId['language:locale:lc_all'] = elementList.GetId()

        gridSizer.Add(item = elementList,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        gridSizer.AddGrowableCol(0)
        #
        # appearence
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Appearance settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer  =  wx.GridBagSizer (hgap = 3, vgap = 3)

        #
        # element list
        #
        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Element list:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        elementList = wx.Choice(parent = panel, id = wx.ID_ANY, size = (325, -1),
                                choices = self.settings.Get(group = 'appearance', key = 'elementListExpand',
                                                            subkey = 'choices', internal = True),
                                name = "GetSelection")
        elementList.SetSelection(self.settings.Get(group = 'appearance', key = 'elementListExpand',
                                                   subkey = 'selection'))
        self.winId['appearance:elementListExpand:selection'] = elementList.GetId()

        gridSizer.Add(item = elementList,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        
        #
        # menu style
        #
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Menu style (requires to save and GUI restart):")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        listOfStyles = self.settings.Get(group = 'appearance', key = 'menustyle',
                                         subkey = 'choices', internal = True)
        
        menuItemText = wx.Choice(parent = panel, id = wx.ID_ANY, size = (325, -1),
                                 choices = listOfStyles,
                                 name = "GetSelection")
        menuItemText.SetSelection(self.settings.Get(group = 'appearance', key = 'menustyle', subkey = 'selection'))
        
        self.winId['appearance:menustyle:selection'] = menuItemText.GetId()
        
        gridSizer.Add(item = menuItemText,
                      flag = wx.ALIGN_RIGHT,
                      pos = (row, 1))
        
        #
        # gselect.TreeCtrlComboPopup height
        #
        row += 1
        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Height of map selection popup window (in pixels):")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        min = self.settings.Get(group = 'appearance', key = 'gSelectPopupHeight', subkey = 'min', internal = True)
        max = self.settings.Get(group = 'appearance', key = 'gSelectPopupHeight', subkey = 'max', internal = True)
        value = self.settings.Get(group = 'appearance', key = 'gSelectPopupHeight', subkey = 'value')
        
        popupHeightSpin = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (100, -1))
        popupHeightSpin.SetRange(min,max)
        popupHeightSpin.SetValue(value)
        
        self.winId['appearance:gSelectPopupHeight:value'] = popupHeightSpin.GetId()
        
        gridSizer.Add(item = popupHeightSpin,
                      flag = wx.ALIGN_RIGHT,
                      pos = (row, 1))
        
        
        #
        # icon theme
        #
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Icon theme (requires GUI restart):")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        iconTheme = wx.Choice(parent = panel, id = wx.ID_ANY, size = (100, -1),
                              choices = self.settings.Get(group = 'appearance', key = 'iconTheme',
                                                        subkey = 'choices', internal = True),
                              name = "GetStringSelection")
        iconTheme.SetStringSelection(self.settings.Get(group = 'appearance', key = 'iconTheme', subkey = 'type'))
        self.winId['appearance:iconTheme:type'] = iconTheme.GetId()

        gridSizer.Add(item = iconTheme,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))
        #
        # command dialog style
        #
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Command dialog style:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        styleList = wx.Choice(parent = panel, id = wx.ID_ANY, size = (325, -1),
                                choices = self.settings.Get(group = 'appearance', key = 'commandNotebook',
                                                            subkey = 'choices', internal = True),
                                name = "GetSelection")
        styleList.SetSelection(self.settings.Get(group = 'appearance', key = 'commandNotebook',
                                                   subkey = 'selection'))
        self.winId['appearance:commandNotebook:selection'] = styleList.GetId()

        gridSizer.Add(item = styleList,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))

        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        
        panel.SetSizer(border)
                
        # bindings
        outfontButton.Bind(wx.EVT_BUTTON, self.OnSetOutputFont)
        
        return panel
    
    def _createDisplayPage(self, notebook):
        """!Create notebook page for display settings"""
   
        panel = SP.ScrolledPanel(parent = notebook, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False, scroll_y = True)
        notebook.AddPage(page = panel, text = _("Map Display"))

        border = wx.BoxSizer(wx.VERTICAL)

        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Font settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)

        #
        # font settings
        #
        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Default font for GRASS displays:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        fontButton = wx.Button(parent = panel, id = wx.ID_ANY,
                               label = _("Set font"))
        gridSizer.Add(item = fontButton,
                      flag = wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 1))

        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)

        #
        # display settings
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Default display settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        
        #
        # display driver
        #
        row = 0
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Display driver:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        listOfDrivers = self.settings.Get(group = 'display', key = 'driver', subkey = 'choices', internal = True)
        driver = wx.Choice(parent = panel, id = wx.ID_ANY, size = (150, -1),
                           choices = listOfDrivers,
                           name = "GetStringSelection")
        driver.SetStringSelection(self.settings.Get(group = 'display', key = 'driver', subkey = 'type'))
        self.winId['display:driver:type'] = driver.GetId()
        
        gridSizer.Add(item = driver,
                      flag = wx.ALIGN_RIGHT,
                      pos = (row, 1))
        
        #
        # Statusbar mode
        #
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Statusbar mode:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        listOfModes = self.settings.Get(group = 'display', key = 'statusbarMode', subkey = 'choices', internal = True)
        statusbarMode = wx.Choice(parent = panel, id = wx.ID_ANY, size = (150, -1),
                                  choices = listOfModes,
                                  name = "GetSelection")
        statusbarMode.SetSelection(self.settings.Get(group = 'display', key = 'statusbarMode', subkey = 'selection'))
        self.winId['display:statusbarMode:selection'] = statusbarMode.GetId()

        gridSizer.Add(item = statusbarMode,
                      flag = wx.ALIGN_RIGHT,
                      pos = (row, 1))

        #
        # Background color
        #
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Background color:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        bgColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                    colour = self.settings.Get(group = 'display', key = 'bgcolor', subkey = 'color'),
                                    size = globalvar.DIALOG_COLOR_SIZE)
        bgColor.SetName('GetColour')
        self.winId['display:bgcolor:color'] = bgColor.GetId()
        
        gridSizer.Add(item = bgColor,
                      flag = wx.ALIGN_RIGHT,
                      pos = (row, 1))
        
        #
        # Align extent to display size
        #
        row += 1
        alignExtent = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                  label = _("Align region extent based on display size"),
                                  name = "IsChecked")
        alignExtent.SetValue(self.settings.Get(group = 'display', key = 'alignExtent', subkey = 'enabled'))
        self.winId['display:alignExtent:enabled'] = alignExtent.GetId()

        gridSizer.Add(item = alignExtent,
                      pos = (row, 0), span = (1, 2))
        
        #
        # Use computation resolution
        #
        row += 1
        compResolution = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                     label = _("Constrain display resolution to computational settings"),
                                     name = "IsChecked")
        compResolution.SetValue(self.settings.Get(group = 'display', key = 'compResolution', subkey = 'enabled'))
        self.winId['display:compResolution:enabled'] = compResolution.GetId()

        gridSizer.Add(item = compResolution,
                      pos = (row, 0), span = (1, 2))

        #
        # auto-rendering
        #
        row += 1
        autoRendering = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                    label = _("Enable auto-rendering"),
                                    name = "IsChecked")
        autoRendering.SetValue(self.settings.Get(group = 'display', key = 'autoRendering', subkey = 'enabled'))
        self.winId['display:autoRendering:enabled'] = autoRendering.GetId()

        gridSizer.Add(item = autoRendering,
                      pos = (row, 0), span = (1, 2))
        
        #
        # auto-zoom
        #
        row += 1
        autoZooming = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                  label = _("Enable auto-zooming to selected map layer"),
                                  name = "IsChecked")
        autoZooming.SetValue(self.settings.Get(group = 'display', key = 'autoZooming', subkey = 'enabled'))
        self.winId['display:autoZooming:enabled'] = autoZooming.GetId()

        gridSizer.Add(item = autoZooming,
                      pos = (row, 0), span = (1, 2))
        
        #
        # mouse wheel zoom
        #
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                          label = _("Mouse wheel action:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        listOfModes = self.settings.Get(group = 'display', key = 'mouseWheelZoom', subkey = 'choices', internal = True)
        zoomAction = wx.Choice(parent = panel, id = wx.ID_ANY, size = (200, -1),
                               choices = listOfModes,
                               name = "GetSelection")
        zoomAction.SetSelection(self.settings.Get(group = 'display', key = 'mouseWheelZoom', subkey = 'selection'))
        self.winId['display:mouseWheelZoom:selection'] = zoomAction.GetId()
        gridSizer.Add(item = zoomAction,
                      flag = wx.ALIGN_RIGHT,
                      pos = (row, 1))
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                          label = _("Mouse scrolling direction:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        listOfModes = self.settings.Get(group = 'display', key = 'scrollDirection', subkey = 'choices', internal = True)
        scrollDir = wx.Choice(parent = panel, id = wx.ID_ANY, size = (200, -1),
                               choices = listOfModes,
                               name = "GetSelection")
        scrollDir.SetSelection(self.settings.Get(group = 'display', key = 'scrollDirection', subkey = 'selection'))
        self.winId['display:scrollDirection:selection'] = scrollDir.GetId()
        gridSizer.Add(item = scrollDir,
                      flag = wx.ALIGN_RIGHT,
                      pos = (row, 1))

        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        
        
        #
        # advanced
        #
        
        # see initialization of nviz GLWindow
        if globalvar.CheckWxVersion(version=[2, 8, 11]) and \
           sys.platform not in ('win32', 'darwin'):
            box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Advanced display settings"))
            sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
    
            gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
            row = 0
            gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                               label = _("3D view depth buffer (possible values are 16, 24, 32):")),
                          flag = wx.ALIGN_LEFT |
                          wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 0))
            value = self.settings.Get(group='display', key='nvizDepthBuffer', subkey='value')
            textCtrl = wx.TextCtrl(parent=panel, id=wx.ID_ANY, value=str(value), validator=IntegerValidator())
            self.winId['display:nvizDepthBuffer:value'] = textCtrl.GetId()
            gridSizer.Add(item = textCtrl,
                          flag = wx.ALIGN_RIGHT |
                          wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 1))
    
            gridSizer.AddGrowableCol(0)
            sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
            border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)
        
        panel.SetSizer(border)
                
        # bindings
        fontButton.Bind(wx.EVT_BUTTON, self.OnSetFont)
        zoomAction.Bind(wx.EVT_CHOICE, self.OnEnableWheelZoom)
        
        # enable/disable controls according to settings
        self.OnEnableWheelZoom(None)
        
        return panel

    def _createCmdPage(self, notebook):
        """!Create notebook page for commad dialog settings"""
        panel = SP.ScrolledPanel(parent = notebook, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False, scroll_y = True)
        notebook.AddPage(page = panel, text = _("Command"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Command dialog settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        
        #
        # command dialog settings
        #
        row = 0
        # overwrite
        overwrite = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                label = _("Allow output files to overwrite existing files"),
                                name = "IsChecked")
        overwrite.SetValue(self.settings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'))
        self.winId['cmd:overwrite:enabled'] = overwrite.GetId()
        
        gridSizer.Add(item = overwrite,
                      pos = (row, 0), span = (1, 2))
        row += 1
        # close
        close = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                            label = _("Close dialog when command is successfully finished"),
                            name = "IsChecked")
        close.SetValue(self.settings.Get(group = 'cmd', key = 'closeDlg', subkey = 'enabled'))
        self.winId['cmd:closeDlg:enabled'] = close.GetId()
        
        gridSizer.Add(item = close,
                      pos = (row, 0), span = (1, 2))
        row += 1
        # add layer
        add = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                          label = _("Add created map into layer tree"),
                          name = "IsChecked")
        add.SetValue(self.settings.Get(group = 'cmd', key = 'addNewLayer', subkey = 'enabled'))
        self.winId['cmd:addNewLayer:enabled'] = add.GetId()
    
        gridSizer.Add(item = add,
                      pos = (row, 0), span = (1, 2))
        
        row += 1
        # interactive input
        interactive = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                  label = _("Allow interactive input"),
                                  name = "IsChecked")
        interactive.SetValue(self.settings.Get(group = 'cmd', key = 'interactiveInput', subkey = 'enabled'))
        self.winId['cmd:interactiveInput:enabled'] = interactive.GetId()
        gridSizer.Add(item = interactive,
                      pos = (row, 0), span = (1, 2))
        
        row += 1
        # verbosity
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Verbosity level:")),
                      flag = wx.ALIGN_LEFT |
                      wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        verbosity = wx.Choice(parent = panel, id = wx.ID_ANY, size = (200, -1),
                              choices = self.settings.Get(group = 'cmd', key = 'verbosity', subkey = 'choices', internal = True),
                              name = "GetStringSelection")
        verbosity.SetStringSelection(self.settings.Get(group = 'cmd', key = 'verbosity', subkey = 'selection'))
        self.winId['cmd:verbosity:selection'] = verbosity.GetId()
        
        gridSizer.Add(item = verbosity,
                      pos = (row, 1), flag = wx.ALIGN_RIGHT)
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)
        
        panel.SetSizer(border)
        
        return panel

    def _createLayersPage(self, notebook):
        """!Create notebook page for layer settings"""
        panel = SP.ScrolledPanel(parent = notebook, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False, scroll_y = True)
        notebook.AddPage(page = panel, text = _("Layers"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        #
        # raster settings
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Default raster settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)
        
        #
        # raster overlay
        #
        row = 0
        rasterOpaque = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                    label = _("Make null cells opaque"),
                                    name = 'IsChecked')
        rasterOpaque.SetValue(self.settings.Get(group = 'rasterLayer', key = 'opaque', subkey = 'enabled'))
        self.winId['rasterLayer:opaque:enabled'] = rasterOpaque.GetId()
        
        gridSizer.Add(item = rasterOpaque,
                      pos = (row, 0), span = (1, 2))

        # default color table
        row += 1
        rasterCTCheck = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                    label = _("Default color table"),
                                    name = 'IsChecked')
        rasterCTCheck.SetValue(self.settings.Get(group = 'rasterLayer', key = 'colorTable', subkey = 'enabled'))
        self.winId['rasterLayer:colorTable:enabled'] = rasterCTCheck.GetId()
        rasterCTCheck.Bind(wx.EVT_CHECKBOX, self.OnCheckColorTable)
        
        gridSizer.Add(item = rasterCTCheck, flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 0))
        
        rasterCTName = wx.Choice(parent = panel, id = wx.ID_ANY, size = (200, -1),
                               choices = GetColorTables(),
                               name = "GetStringSelection")
        rasterCTName.SetStringSelection(self.settings.Get(group = 'rasterLayer', key = 'colorTable', subkey = 'selection'))
        self.winId['rasterLayer:colorTable:selection'] = rasterCTName.GetId()
        if not rasterCTCheck.IsChecked():
            rasterCTName.Enable(False)
        
        gridSizer.Add(item = rasterCTName,
                      pos = (row, 1))
        
        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        
        #
        # vector settings
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Default vector settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.FlexGridSizer (cols = 7, hgap = 10, vgap = 3)
        
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Display:")),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        for type in ('point', 'line', 'centroid', 'boundary',
                     'area', 'face'):
            chkbox = wx.CheckBox(parent = panel, label = type)
            checked = self.settings.Get(group = 'vectorLayer', key = 'showType',
                                        subkey = [type, 'enabled'])
            chkbox.SetValue(checked)
            self.winId['vectorLayer:showType:%s:enabled' % type] = chkbox.GetId()
            gridSizer.Add(item = chkbox)

        sizer.Add(item = gridSizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        row = col = 0
        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)

        # feature color
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Feature color:")),
                      flag = wx.ALIGN_CENTER_VERTICAL, pos = (row, 0))
        featureColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                         colour = self.settings.Get(group = 'vectorLayer',
                                                                    key = 'featureColor',
                                                                    subkey = 'color'),
                                         size = globalvar.DIALOG_COLOR_SIZE)
        featureColor.SetName('GetColour')
        self.winId['vectorLayer:featureColor:color'] = featureColor.GetId()
        gridSizer.Add(item = featureColor, pos = (row, col + 2), flag = wx.ALIGN_RIGHT)
        
        transpFeature = wx.CheckBox(parent  = panel, id = wx.ID_ANY,
                                    label = _("Transparent"), name = "IsChecked")
        transpFeature.SetValue(self.settings.Get(group = 'vectorLayer', key = 'featureColor',
                                                     subkey =  ['transparent', 'enabled']))
        self.winId['vectorLayer:featureColor:transparent:enabled'] = transpFeature.GetId()
        gridSizer.Add(item = transpFeature, pos = (row, col + 1), flag = wx.ALIGN_CENTER_VERTICAL)


        # area fill color
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Area fill color:")),
                      flag = wx.ALIGN_CENTER_VERTICAL, pos = (row, col))
        fillColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                      colour = self.settings.Get(group = 'vectorLayer',
                                                                 key = 'areaFillColor',
                                                                 subkey = 'color'),
                                      size = globalvar.DIALOG_COLOR_SIZE)
        fillColor.SetName('GetColour')
        self.winId['vectorLayer:areaFillColor:color'] = fillColor.GetId()
        gridSizer.Add(item = fillColor, pos = (row, col + 2), flag = wx.ALIGN_RIGHT)

        transpArea = wx.CheckBox(parent  = panel, id = wx.ID_ANY,
                                 label = _("Transparent"), name = "IsChecked")
        transpArea.SetValue(self.settings.Get(group = 'vectorLayer', key = 'areaFillColor',
                                              subkey = ['transparent', 'enabled']))
        self.winId['vectorLayer:areaFillColor:transparent:enabled'] = transpArea.GetId()
        gridSizer.Add(item = transpArea, pos = (row, col + 1), flag = wx.ALIGN_CENTER_VERTICAL)

        # line
        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Line width:")),
                      flag = wx.ALIGN_CENTER_VERTICAL, pos = (row, col))
        hlWidth = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (50, -1),
                              initial = self.settings.Get(group = 'vectorLayer', key = 'line', subkey = 'width'),
                              min = 1, max = 1e6, name = "GetValue")
        self.winId['vectorLayer:line:width'] = hlWidth.GetId()
        gridSizer.Add(item = hlWidth, pos = (row, col + 1), span = (1, 2), flag = wx.ALIGN_RIGHT)

        # symbol
        row = 0
        col = 4
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Symbol size:")),
                      flag = wx.ALIGN_CENTER_VERTICAL, pos = (row, col))
        ptSize = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (50, -1),
                              initial = self.settings.Get(group = 'vectorLayer', key = 'point', subkey = 'size'),
                              min = 1, max = 1e6, name = "GetValue")
        self.winId['vectorLayer:point:size'] = ptSize.GetId()
        gridSizer.Add(item = ptSize, pos = (row, col + 2), flag = wx.ALIGN_RIGHT)

        row += 1
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = _("Symbol:")),
                      flag = wx.ALIGN_CENTER_VERTICAL, pos = (row, col))
        symbolPath = self.settings.Get(group = 'vectorLayer', key = 'point', subkey = 'symbol')
        symbolLabel = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                    label = symbolPath, name = 'GetLabel')
        symbolLabel.SetMinSize((150, -1))
        self.winId['vectorLayer:point:symbol'] = symbolLabel.GetId()
        gridSizer.Add(item = symbolLabel, flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT, pos = (row, col + 1))

        bitmap = wx.Bitmap(os.path.join(globalvar.ETCSYMBOLDIR, symbolPath) + '.png')
        bb = wx.BitmapButton(parent = panel, id = wx.ID_ANY, bitmap = bitmap, name = "symbolButton")
        bb.Bind(wx.EVT_BUTTON, self.OnSetSymbol)
        gridSizer.Add(item = bb, pos = (row, col + 2))

        gridSizer.AddGrowableCol(0)
        gridSizer.AddGrowableCol(3)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        
        panel.SetSizer(border)
        
        return panel

    def _createAttributeManagerPage(self, notebook):
        """!Create notebook page for 'Attribute Table Manager' settings"""
        panel = SP.ScrolledPanel(parent = notebook, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False, scroll_y = True)
        notebook.AddPage(page = panel, text = _("Attributes"))

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        #
        # highlighting
        #
        highlightBox = wx.StaticBox(parent = panel, id = wx.ID_ANY,
                                    label = " %s " % _("Highlighting"))
        highlightSizer = wx.StaticBoxSizer(highlightBox, wx.VERTICAL)

        flexSizer = wx.FlexGridSizer (cols = 2, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)
        
        label = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Color:"))
        hlColor = csel.ColourSelect(parent = panel, id = wx.ID_ANY,
                                    colour = self.settings.Get(group = 'atm', key = 'highlight', subkey = 'color'),
                                    size = globalvar.DIALOG_COLOR_SIZE)
        hlColor.SetName('GetColour')
        self.winId['atm:highlight:color'] = hlColor.GetId()

        flexSizer.Add(label, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(hlColor, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        label = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Line width (in pixels):"))
        hlWidth = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (50, -1),
                              initial = self.settings.Get(group = 'atm', key = 'highlight',subkey = 'width'),
                              min = 1, max = 1e6)
        self.winId['atm:highlight:width'] = hlWidth.GetId()

        flexSizer.Add(label, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(hlWidth, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        highlightSizer.Add(item = flexSizer,
                           proportion = 0,
                           flag = wx.ALL | wx.EXPAND,
                           border = 5)

        pageSizer.Add(item = highlightSizer,
                      proportion = 0,
                      flag = wx.ALL | wx.EXPAND,
                      border = 5)

        #
        # data browser related settings
        #
        dataBrowserBox = wx.StaticBox(parent = panel, id = wx.ID_ANY,
                                    label = " %s " % _("Data browser"))
        dataBrowserSizer = wx.StaticBoxSizer(dataBrowserBox, wx.VERTICAL)

        flexSizer = wx.FlexGridSizer (cols = 2, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)
        label = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Left mouse double click:"))
        leftDbClick = wx.Choice(parent = panel, id = wx.ID_ANY,
                                choices = self.settings.Get(group = 'atm', key = 'leftDbClick', subkey = 'choices', internal = True),
                                name = "GetSelection")
        leftDbClick.SetSelection(self.settings.Get(group = 'atm', key = 'leftDbClick', subkey = 'selection'))
        self.winId['atm:leftDbClick:selection'] = leftDbClick.GetId()

        flexSizer.Add(label, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(leftDbClick, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        # encoding
        label = wx.StaticText(parent = panel, id = wx.ID_ANY,
                              label = _("Encoding (e.g. utf-8, ascii, iso8859-1, koi8-r):"))
        encoding = wx.TextCtrl(parent = panel, id = wx.ID_ANY,
                               value = self.settings.Get(group = 'atm', key = 'encoding', subkey = 'value'),
                               name = "GetValue", size = (200, -1))
        self.winId['atm:encoding:value'] = encoding.GetId()

        flexSizer.Add(label, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(encoding, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        # field separator
        label = wx.StaticText(parent = panel, id = wx.ID_ANY,
                              label = _("Field separator:"))
        separator = wx.TextCtrl(parent = panel, id = wx.ID_ANY,
                                value = self.settings.Get(group = 'atm', key = 'fieldSeparator', subkey = 'value'),
                                name = "GetValue", size = (200, -1))
        self.winId['atm:fieldSeparator:value'] = separator.GetId()

        flexSizer.Add(label, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(separator, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        # ask on delete record
        askOnDeleteRec = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                     label = _("Ask when deleting data record(s) from table"),
                                     name = 'IsChecked')
        askOnDeleteRec.SetValue(self.settings.Get(group = 'atm', key = 'askOnDeleteRec', subkey = 'enabled'))
        self.winId['atm:askOnDeleteRec:enabled'] = askOnDeleteRec.GetId()

        flexSizer.Add(askOnDeleteRec, proportion = 0)

        dataBrowserSizer.Add(item = flexSizer,
                           proportion = 0,
                           flag = wx.ALL | wx.EXPAND,
                           border = 5)

        pageSizer.Add(item = dataBrowserSizer,
                      proportion = 0,
                      flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
                      border = 3)

        #
        # create table
        #
        createTableBox = wx.StaticBox(parent = panel, id = wx.ID_ANY,
                                    label = " %s " % _("Create table"))
        createTableSizer = wx.StaticBoxSizer(createTableBox, wx.VERTICAL)

        flexSizer = wx.FlexGridSizer (cols = 2, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)

        label = wx.StaticText(parent = panel, id = wx.ID_ANY,
                              label = _("Key column:"))
        keyColumn = wx.TextCtrl(parent = panel, id = wx.ID_ANY,
                                size = (250, -1))
        keyColumn.SetValue(self.settings.Get(group = 'atm', key = 'keycolumn', subkey = 'value'))
        self.winId['atm:keycolumn:value'] = keyColumn.GetId()
        
        flexSizer.Add(label, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(keyColumn, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        createTableSizer.Add(item = flexSizer,
                             proportion = 0,
                             flag = wx.ALL | wx.EXPAND,
                             border = 5)

        pageSizer.Add(item = createTableSizer,
                      proportion = 0,
                      flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
                      border = 3)
        
        panel.SetSizer(pageSizer)

        return panel

    def _createProjectionPage(self, notebook):
        """!Create notebook page for workspace settings"""
        panel = SP.ScrolledPanel(parent = notebook, id = wx.ID_ANY)
        panel.SetupScrolling(scroll_x = False, scroll_y = True)
        notebook.AddPage(page = panel, text = _("Projection"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        
        #
        # projections statusbar settings
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Projection statusbar settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)

        # note for users expecting on-the-fly data reprojection
        row = 0
        note0 = wx.StaticText(parent = panel, id = wx.ID_ANY,
                             label = _("\nNote: This only controls the coordinates "
                                       "displayed in the lower-left of the Map "
                                       "Display\nwindow's status bar. It is purely "
                                       "cosmetic and does not affect the working "
                                       "location's\nprojection in any way. You will "
                                       "need to enable the Projection check box in "
                                       "the drop-down\nmenu located at the bottom "
                                       "of the Map Display window."))
        gridSizer.Add(item = note0,
                      span = (1, 2),
                      pos = (row, 0))

        # epsg
        row += 1
        label = wx.StaticText(parent = panel, id = wx.ID_ANY,
                              label = _("EPSG code:"))
        epsgCode = wx.ComboBox(parent = panel, id = wx.ID_ANY,
                               name = "GetValue",
                               size = (150, -1))
        self.epsgCodeDict = dict()
        epsgCode.SetValue(str(self.settings.Get(group = 'projection', key = 'statusbar', subkey = 'epsg')))
        self.winId['projection:statusbar:epsg'] = epsgCode.GetId()
        
        gridSizer.Add(item = label,
                      pos = (row, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = epsgCode,
                      pos = (row, 1), span = (1, 2))

        # proj
        row += 1
        label = wx.StaticText(parent = panel, id = wx.ID_ANY,
                              label = _("Proj.4 string (required):"))
        projString = wx.TextCtrl(parent = panel, id = wx.ID_ANY,
                                 value = self.settings.Get(group = 'projection', key = 'statusbar', subkey = 'proj4'),
                                 name = "GetValue", size = (400, -1))
        self.winId['projection:statusbar:proj4'] = projString.GetId()

        gridSizer.Add(item = label,
                      pos = (row, 0),
                      flag  =  wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = projString,
                      pos = (row, 1), span = (1, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        
        # epsg file
        row += 1
        label = wx.StaticText(parent = panel, id = wx.ID_ANY,
                              label = _("EPSG file:"))
        projFile = wx.TextCtrl(parent = panel, id = wx.ID_ANY,
                               value  =  self.settings.Get(group = 'projection', key = 'statusbar', subkey = 'projFile'),
                               name = "GetValue", size = (400, -1))
        self.winId['projection:statusbar:projFile'] = projFile.GetId()
        gridSizer.Add(item = label,
                      pos = (row, 0),
                      flag  =  wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = projFile,
                      pos = (row, 1),
                      flag  =  wx.ALIGN_CENTER_VERTICAL)
        
        # note + button
        row += 1
        note = wx.StaticText(parent = panel, id = wx.ID_ANY,
                             label = _("Load EPSG codes (be patient), enter EPSG code or "
                                       "insert Proj.4 string directly."))
        gridSizer.Add(item = note,
                      span = (1, 2),
                      pos = (row, 0))

        row += 1
        epsgLoad = wx.Button(parent = panel, id = wx.ID_ANY,
                             label = _("&Load EPSG codes"))
        gridSizer.Add(item = epsgLoad,
                      flag = wx.ALIGN_RIGHT,
                      pos = (row, 1))
        
        gridSizer.AddGrowableCol(1)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)

        #
        # format
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Coordinates format"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer (hgap = 3, vgap = 3)

        row = 0
        # ll format
        ll = wx.RadioBox(parent = panel, id = wx.ID_ANY,
                         label = " %s " % _("Lat/long projections"),
                         choices = ["DMS", "DEG"],
                         name = "GetStringSelection")
        self.winId['projection:format:ll'] = ll.GetId()
        if self.settings.Get(group = 'projection', key = 'format', subkey = 'll') == 'DMS':
            ll.SetSelection(0)
        else:
            ll.SetSelection(1)
        
        # precision
        precision =  wx.SpinCtrl(parent = panel, id = wx.ID_ANY,
                                 min = 0, max = 12,
                                 name = "GetValue")
        precision.SetValue(int(self.settings.Get(group = 'projection', key = 'format', subkey = 'precision')))
        self.winId['projection:format:precision'] = precision.GetId()
                
        gridSizer.Add(item = ll,
                      pos = (row, 0))
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                         label = _("Precision:")),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT,
                      border = 20,
                      pos = (row, 1))
        gridSizer.Add(item = precision,
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (row, 2))
        
        
        gridSizer.AddGrowableCol(2)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        
        panel.SetSizer(border)

        # bindings
        epsgLoad.Bind(wx.EVT_BUTTON, self.OnLoadEpsgCodes)
        epsgCode.Bind(wx.EVT_COMBOBOX, self.OnSetEpsgCode)
        epsgCode.Bind(wx.EVT_TEXT_ENTER, self.OnSetEpsgCode)
        
        return panel

    def OnCheckColorTable(self, event):
        """!Set/unset default color table"""
        win = self.FindWindowById(self.winId['rasterLayer:colorTable:selection'])
        if event.IsChecked():
            win.Enable()
        else:
            win.Enable(False)
        
    def OnLoadEpsgCodes(self, event):
        """!Load EPSG codes from the file"""
        win = self.FindWindowById(self.winId['projection:statusbar:projFile'])
        path = win.GetValue()

        self.epsgCodeDict = ReadEpsgCodes(path)
        list = self.FindWindowById(self.winId['projection:statusbar:epsg'])
        if type(self.epsgCodeDict) == type(''):
            wx.MessageBox(parent = self,
                          message = _("Unable to read EPSG codes: %s") % self.epsgCodeDict,
                          caption = _("Error"),  style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            self.epsgCodeDict = dict()
            list.SetItems([])
            list.SetValue('')
            self.FindWindowById(self.winId['projection:statusbar:proj4']).SetValue('')
            return
        
        choices = map(str, self.epsgCodeDict.keys())

        list.SetItems(choices)
        try:
            code = int(list.GetValue())
        except ValueError:
            code = -1
        win = self.FindWindowById(self.winId['projection:statusbar:proj4'])
        if code in self.epsgCodeDict:
            win.SetValue(self.epsgCodeDict[code][1])
        else:
            list.SetSelection(0)
            code = int(list.GetStringSelection())
            win.SetValue(self.epsgCodeDict[code][1])
    
    def OnSetEpsgCode(self, event):
        """!EPSG code selected"""
        winCode = self.FindWindowById(event.GetId())
        win = self.FindWindowById(self.winId['projection:statusbar:proj4'])
        if not self.epsgCodeDict:
            wx.MessageBox(parent = self,
                          message = _("EPSG code %s not found") % event.GetString(),
                          caption = _("Error"),  style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            winCode.SetValue('')
            win.SetValue('')
        
        try:
            code = int(event.GetString())
        except ValueError:
            wx.MessageBox(parent = self,
                          message = _("EPSG code %s not found") % str(code),
                          caption = _("Error"),  style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            winCode.SetValue('')
            win.SetValue('')
        
        try:
            win.SetValue(self.epsgCodeDict[code][1].replace('<>', '').strip())
        except KeyError:
            wx.MessageBox(parent = self,
                          message = _("EPSG code %s not found") % str(code),
                          caption = _("Error"),  style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            winCode.SetValue('')
            win.SetValue('')
        
    def OnSetFont(self, event):
        """'Set font' button pressed"""
        dlg = DefaultFontDialog(parent = self,
                                title = _('Select default display font'),
                                style = wx.DEFAULT_DIALOG_STYLE,
                                type = 'font')
        
        if dlg.ShowModal() == wx.ID_OK:
            # set default font and encoding environmental variables
            if dlg.font:
                os.environ["GRASS_FONT"] = dlg.font
                self.settings.Set(group = 'display', value = dlg.font,
                                  key = 'font', subkey = 'type')

            if dlg.encoding and \
                    dlg.encoding != "ISO-8859-1":
                os.environ["GRASS_ENCODING"] = dlg.encoding
                self.settings.Set(group = 'display', value = dlg.encoding,
                                  key = 'font', subkey = 'encoding')
                
        dlg.Destroy()
        
        event.Skip()

    def OnSetOutputFont(self, event):
        """'Set output font' button pressed
        """

        type = self.settings.Get(group = 'appearance', key = 'outputfont', subkey = 'type')   
                           
        size = self.settings.Get(group = 'appearance', key = 'outputfont', subkey = 'size')
        if size == None or size == 0: size = 11
        size = float(size)
        if type == None or type == '': type = 'Courier'
        
        outfont = wx.Font(size, wx.FONTFAMILY_MODERN, wx.NORMAL, 0, faceName = type)
        
        fontdata = wx.FontData()
        fontdata.EnableEffects(True)
        fontdata.SetColour('black')
        fontdata.SetInitialFont(outfont)
        
        dlg = wx.FontDialog(self, fontdata)
        
        'FIXME: native font dialog does not initialize with current font'

        if dlg.ShowModal() == wx.ID_OK:
            outdata = dlg.GetFontData()
            font = outdata.GetChosenFont()

            self.settings.Set(group = 'appearance', value = font.GetFaceName(),
                                  key = 'outputfont', subkey = 'type')
            self.settings.Set(group = 'appearance', value = font.GetPointSize(),
                                  key = 'outputfont', subkey = 'size')
        dlg.Destroy()

        event.Skip()

    def OnSetSymbol(self, event):
        """!Opens symbol dialog"""
        winId = self.winId['vectorLayer:point:symbol']
        label = self.FindWindowById(winId)
        bb = self.FindWindowByName('symbolButton')
        dlg = SymbolDialog(self, symbolPath = globalvar.ETCSYMBOLDIR,
                           currentSymbol = label.GetLabel())
        if dlg.ShowModal() == wx.ID_OK:
            img = dlg.GetSelectedSymbolPath()
            label.SetLabel(dlg.GetSelectedSymbolName())
            bb.SetBitmapLabel(wx.Bitmap(img + '.png'))

    def OnEnableWheelZoom(self, event):
        """!Enable/disable wheel zoom mode control"""
        choiceId = self.winId['display:mouseWheelZoom:selection']
        choice = self.FindWindowById(choiceId)
        if choice.GetSelection() == 2:
            enable = False
        else:
            enable = True
        scrollId = self.winId['display:scrollDirection:selection']
        self.FindWindowById(scrollId).Enable(enable)
        
class DefaultFontDialog(wx.Dialog):
    """
    Opens a file selection dialog to select default font
    to use in all GRASS displays
    """
    def __init__(self, parent, title, id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE |
                 wx.RESIZE_BORDER,
                 settings = UserSettings,
                 type = 'font'):
        
        self.settings = settings
        self.type = type
        
        wx.Dialog.__init__(self, parent, id, title, style = style)

        panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.fontlist = self.GetFonts()
        
        border = wx.BoxSizer(wx.VERTICAL)
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Font settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer (hgap = 5, vgap = 5)

        label = wx.StaticText(parent = panel, id = wx.ID_ANY,
                              label = _("Select font:"))
        gridSizer.Add(item = label,
                      flag = wx.ALIGN_TOP,
                      pos = (0,0))
        
        self.fontlb = wx.ListBox(parent = panel, id = wx.ID_ANY, pos = wx.DefaultPosition,
                                 choices = self.fontlist,
                                 style = wx.LB_SINGLE|wx.LB_SORT)
        self.Bind(wx.EVT_LISTBOX, self.EvtListBox, self.fontlb)
        self.Bind(wx.EVT_LISTBOX_DCLICK, self.EvtListBoxDClick, self.fontlb)

        gridSizer.Add(item = self.fontlb,
                flag = wx.EXPAND, pos = (1, 0))

        if self.type == 'font':
            if "GRASS_FONT" in os.environ:
                self.font = os.environ["GRASS_FONT"]
            else:
                self.font = self.settings.Get(group = 'display',
                                              key = 'font', subkey = 'type')
            self.encoding = self.settings.Get(group = 'display',
                                          key = 'font', subkey = 'encoding')

            label = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                  label = _("Character encoding:"))
            gridSizer.Add(item = label,
                          flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (2, 0))

            self.textentry = wx.TextCtrl(parent = panel, id = wx.ID_ANY,
                                         value = self.encoding)
            gridSizer.Add(item = self.textentry,
                    flag = wx.EXPAND, pos = (3, 0))

            self.textentry.Bind(wx.EVT_TEXT, self.OnEncoding)

        elif self.type == 'outputfont':
            self.font = self.settings.Get(group = 'appearance',
                                              key = 'outputfont', subkey = 'type')
            self.fontsize = self.settings.Get(group = 'appearance',
                                          key = 'outputfont', subkey = 'size')
            label = wx.StaticText(parent = panel, id = wx.ID_ANY,
                              label = _("Font size:"))
            gridSizer.Add(item = label,
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (2, 0))
                      
            self.spin = wx.SpinCtrl(parent = panel, id = wx.ID_ANY)
            if self.fontsize:
                self.spin.SetValue(int(self.fontsize))
            self.spin.Bind(wx.EVT_SPINCTRL, self.OnSizeSpin)
            self.spin.Bind(wx.EVT_TEXT, self.OnSizeSpin)
            gridSizer.Add(item = self.spin,
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (3, 0))

        else: 
            return

        if self.font:
            self.fontlb.SetStringSelection(self.font, True)

        gridSizer.AddGrowableCol(0)
        sizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.EXPAND | wx.ALL,
                  border = 5)

        border.Add(item = sizer, proportion = 1,
                   flag = wx.ALL | wx.EXPAND, border = 3)
        
        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(parent = panel, id = wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(parent = panel, id = wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        border.Add(item = btnsizer, proportion = 0,
                   flag = wx.EXPAND | wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        panel.SetAutoLayout(True)
        panel.SetSizer(border)
        border.Fit(self)
        
        self.Layout()

    def OnEncoding(self, event):
        self.encoding = event.GetString()

    def EvtListBox(self, event):
        self.font = event.GetString()
        event.Skip()

    def EvtListBoxDClick(self, event):
        self.font = event.GetString()
        event.Skip()
        
    def OnSizeSpin(self, event):
        self.fontsize = self.spin.GetValue()
        event.Skip()
    
    def GetFonts(self):
        """
        parses fonts directory or fretypecap file to get a list of fonts for the listbox
        """
        fontlist = []
        ret = RunCommand('d.font',
                         read = True,
                         flags = 'l')
        if not ret:
            return fontlist

        dfonts = ret.splitlines()
        dfonts.sort(lambda x,y: cmp(x.lower(), y.lower()))
        for item in range(len(dfonts)):
            # ignore duplicate fonts and those starting with #
            if not dfonts[item].startswith('#') and \
                  dfonts[item] != dfonts[item-1]:
                fontlist.append(dfonts[item])

        return fontlist

class MapsetAccess(wx.Dialog):
    """!Controls setting options and displaying/hiding map overlay
    decorations
    """
    def __init__(self, parent, id = wx.ID_ANY,
                 title = _('Manage access to mapsets'),
                 size  =  (350, 400),
                 style  =  wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        wx.Dialog.__init__(self, parent, id, title, size = size, style = style)

        self.all_mapsets_ordered = ListOfMapsets(get = 'ordered')
        self.accessible_mapsets  = ListOfMapsets(get = 'accessible')
        self.curr_mapset = grass.gisenv()['MAPSET']

        # make a checklistbox from available mapsets and check those that are active
        sizer = wx.BoxSizer(wx.VERTICAL)

        label = wx.StaticText(parent = self, id = wx.ID_ANY,
                              label = _("Check a mapset to make it accessible, uncheck it to hide it.\n"
                                        "  Notes:\n"
                                        "    - The current mapset is always accessible.\n"
                                        "    - You may only write to the current mapset.\n"
                                        "    - You may only write to mapsets which you own."))
        
        sizer.Add(item = label, proportion = 0,
                  flag = wx.ALL, border = 5)

        self.mapsetlb = CheckListMapset(parent = self)
        self.mapsetlb.LoadData()
        
        sizer.Add(item = self.mapsetlb, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 5)

        # check all accessible mapsets
        for mset in self.accessible_mapsets:
            self.mapsetlb.CheckItem(self.all_mapsets_ordered.index(mset), True)

        # FIXME (howto?): grey-out current mapset
        #self.mapsetlb.Enable(0, False)

        # dialog buttons
        line = wx.StaticLine(parent = self, id = wx.ID_ANY,
                             style = wx.LI_HORIZONTAL)
        sizer.Add(item = line, proportion = 0,
                  flag = wx.EXPAND | wx.ALIGN_CENTRE | wx.ALL, border = 5)

        btnsizer = wx.StdDialogButtonSizer()
        okbtn = wx.Button(self, wx.ID_OK)
        okbtn.SetDefault()
        btnsizer.AddButton(okbtn)

        cancelbtn = wx.Button(self, wx.ID_CANCEL)
        btnsizer.AddButton(cancelbtn)
        btnsizer.Realize()

        sizer.Add(item = btnsizer, proportion = 0,
                  flag = wx.EXPAND | wx.ALIGN_RIGHT | wx.ALL, border = 5)

        # do layout
        self.Layout()
        self.SetSizer(sizer)
        sizer.Fit(self)

        self.SetMinSize(size)
        
    def GetMapsets(self):
        """!Get list of checked mapsets"""
        ms = []
        i = 0
        for mset in self.all_mapsets_ordered:
            if self.mapsetlb.IsChecked(i):
                ms.append(mset)
            i += 1

        return ms

class CheckListMapset(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin, listmix.CheckListCtrlMixin):
    """!List of mapset/owner/group"""
    def __init__(self, parent, log = None):
        self.parent = parent
        
        wx.ListCtrl.__init__(self, parent, wx.ID_ANY,
                             style = wx.LC_REPORT)
        listmix.CheckListCtrlMixin.__init__(self)
        self.log = log

        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)

    def LoadData(self):
        """!Load data into list"""
        self.InsertColumn(0, _('Mapset'))
        self.InsertColumn(1, _('Owner'))
        ### self.InsertColumn(2, _('Group'))
        gisenv = grass.gisenv()
        locationPath = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'])

        for mapset in self.parent.all_mapsets_ordered:
            index = self.InsertStringItem(sys.maxint, mapset)
            mapsetPath = os.path.join(locationPath,
                                      mapset)
            stat_info = os.stat(mapsetPath)
            if havePwd:
                self.SetStringItem(index, 1, "%s" % pwd.getpwuid(stat_info.st_uid)[0])
                # FIXME: get group name
                ### self.SetStringItem(index, 2, "%-8s" % stat_info.st_gid) 
            else:
                # FIXME: no pwd under MS Windows (owner: 0, group: 0)
                self.SetStringItem(index, 1, "%-8s" % stat_info.st_uid)
                ### self.SetStringItem(index, 2, "%-8s" % stat_info.st_gid)
                
        self.SetColumnWidth(col = 0, width = wx.LIST_AUTOSIZE)
        ### self.SetColumnWidth(col = 1, width = wx.LIST_AUTOSIZE)
        
    def OnCheckItem(self, index, flag):
        """!Mapset checked/unchecked"""
        mapset = self.parent.all_mapsets_ordered[index]
        if mapset == self.parent.curr_mapset:
            self.CheckItem(index, True)
