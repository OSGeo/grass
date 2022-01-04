"""
@package mapdisp.settings

@brief Classes for map display settings management

Classes:
 - settings::ChBItem
 - settings::ChBRender
 - settings::ChBShowRegion
 - settings::ChBAlignExtent
 - settings::ChBResolution
 - settings::MapDisplayPreferencesDialog

(C) 2021 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
@author Linda Kladivova <lindakladivova gmail.com>
"""

import wx
import wx.lib.scrolledpanel as SP
from gui_core.wrap import StaticBox

from core.settings import UserSettings
from gui_core.preferences import PreferencesBaseDialog


class ChBItem():
    """Base class for Map Display settings widgets that use property signals"""

    def __init__(self, mapWindowProperties):
        self._properties = mapWindowProperties

    @property
    def mapWindowProperty(self):
        pass

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        pass

    def mapWindowPropertyChanged(self):
        """Returns signal from MapWindowProperties."""
        pass

    def _setValue(self, value):
        self.widget.SetValue(value)

    def SetValue(self, value):
        self._disconnect()
        self.mapWindowProperty = value
        self.widget.SetValue(self, value)
        self._connect()

    def _connect(self):
        self.mapWindowPropertyChanged().connect(self._setValue)

    def _disconnect(self):
        self.mapWindowPropertyChanged().disconnect(self._setValue)

    def _onToggleCheckBox(self, event):
        self._disconnect()
        self.mapWindowProperty = self.widget.GetValue()
        self._connect()


class ChBRender(ChBItem):
    """Checkbox to enable and disable auto-rendering."""

    def __init__(self, parent, mapWindowProperties):
        ChBItem.__init__(self, mapWindowProperties)
        self.name = "render"
        self.widget = wx.CheckBox(
            parent=parent, id=wx.ID_ANY, label=_("Enable auto-rendering")
        )
        self.widget.SetValue(self.mapWindowProperty)
        self.widget.SetToolTip(wx.ToolTip(_("Enable/disable auto-rendering")))

        self._connect()
        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)

    @property
    def mapWindowProperty(self):
        return self._properties.autoRender

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.autoRender = value

    def mapWindowPropertyChanged(self):
        return self._properties.autoRenderChanged


class ChBAlignExtent(ChBItem):
    """Checkbox to select zoom behavior.

    Used by BufferedWindow (through MapFrame property).
    See tooltip for explanation.
    """

    def __init__(self, parent, mapWindowProperties):
        ChBItem.__init__(self, parent, mapWindowProperties)
        self.name = "alignExtent"
        self.widget = wx.CheckBox(
            parent=parent, id=wx.ID_ANY, label=_("Align region extent based on display size")
        )
        self.widget.SetValue(self.mapWindowProperty)
        self.widget.SetToolTip(wx.ToolTip(_(
            "Align region extent based on display "
            "size from center point. "
            "Default value for new map displays can "
            "be set up in 'User GUI settings' dialog."
        )))

        self._connect()
        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)

    @property
    def mapWindowProperty(self):
        return self._properties.alignExtent

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.alignExtent = value

    def mapWindowPropertyChanged(self):
        return self._properties.alignExtentChanged


class ChBResolution(ChBItem):
    """Checkbox to select used display resolution."""

    def __init__(self, parent, giface, mapWindowProperties):
        ChBItem.__init__(self, mapWindowProperties)
        self.giface = giface
        self.SetLabel = _("Constrain display resolution to computational settings")
        self.widget.SetToolTip(wx.ToolTip(_(
            "Constrain display resolution "
            "to computational region settings. "
            "Default value for new map displays can "
            "be set up in 'User GUI settings' dialog."
        )))

        self._connect()
        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)

    @property
    def mapWindowProperty(self):
        return self._properties.resolution

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.resolution = value

    def mapWindowPropertyChanged(self):
        return self._properties.resolutionChanged

    def _onToggleCheckBox(self, event):
        """Update display when toggle display mode"""
        super()._onToggleCheckBox(event)

        # redraw map if auto-rendering is enabled
        if self._properties.autoRender:
            self.giface.updateMap.emit()


class ChBShowRegion(ChBItem):
    """Checkbox to enable and disable showing of computational region."""

    def __init__(self, parent, giface, mapWindowProperties):
        ChBItem.__init__(self, mapWindowProperties)
        self.giface = giface
        self.name = "region"
        self.label = _("Show comp. extent")
        self.widget = wx.CheckBox(
            parent=self.statusbar, id=wx.ID_ANY, label=_("Show computational extent")
        )
        self.widget.SetValue(self.mapWindowProperty)
        self.widget.SetToolTip(wx.ToolTip(_(
            "Show/hide computational "
            "region extent (set with g.region). "
            "Display region drawn as a blue box inside the "
            "computational region, "
            "computational region inside a display region "
            "as a red box)."
        )))

        self._connect()
        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)

    @property
    def mapWindowProperty(self):
        return self._properties.showRegion

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.showRegion = value

    def mapWindowPropertyChanged(self):
        return self._properties.showRegionChanged

    def _onToggleCheckBox(self, event):
        """Shows/Hides extent (comp. region) in map canvas.

        Shows or hides according to checkbox value.
        """
        super()._onToggleCheckBox(event)

        # redraw map if auto-rendering is enabled
        if self._properties.autoRender:
            self.giface.updateMap.emit(render=False)


class MapDisplayPreferencesDialog(wx.Dialog):
    """Model properties dialog"""

    def __init__(
        self,
        parent,
        id=wx.ID_ANY,
        title=_("Model properties"),
        size=(350, 400),
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
    ):
        wx.Dialog.__init__(self, parent, id, title, size=size, style=style)

        self.metaBox = StaticBox(
            parent=self, id=wx.ID_ANY, label=" %s " % _("Metadata")
        )
        self.cmdBox = StaticBox(parent=self, id=wx.ID_ANY, label=" %s " % _("Commands"))

        self.name = TextCtrl(parent=self, id=wx.ID_ANY, size=(300, 25))
        self.desc = TextCtrl(
            parent=self, id=wx.ID_ANY, style=wx.TE_MULTILINE, size=(300, 50)
        )
        self.author = TextCtrl(parent=self, id=wx.ID_ANY, size=(300, 25))

        # commands
        self.overwrite = wx.CheckBox(
            parent=self,
            id=wx.ID_ANY,
            label=_("Allow output files to overwrite existing files"),
        )
        self.overwrite.SetValue(
            UserSettings.Get(group="cmd", key="overwrite", subkey="enabled")
        )

        # buttons
        self.btnOk = Button(self, wx.ID_OK)
        self.btnCancel = Button(self, wx.ID_CANCEL)
        self.btnOk.SetDefault()

        self.btnOk.SetToolTip(_("Apply properties"))
        self.btnOk.SetDefault()
        self.btnCancel.SetToolTip(_("Close dialog and ignore changes"))

        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        self._layout()

class MapDisplayPreferencesDialog(PreferencesBaseDialog):
    """Map Display preferences dialog"""

    def __init__(
        self,
        parent,
        giface,
        properties,
        title=_("Map Display Settings"),
        settings=UserSettings,
    ):
        PreferencesBaseDialog.__init__(
            self, parent=parent, giface=giface, title=title, settings=settings
        )
        self.giface = giface
        self.mapWindowProperties = properties

        # create notebook pages
        self.displayPage = self._createDisplayPage(parent=self.notebook)

        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)

    def _createDisplayPage(self, parent):
        """Create notebook page for display settings"""

        panel = SP.ScrolledPanel(parent=parent, id=wx.ID_ANY)
        panel.SetupScrolling(scroll_x=False, scroll_y=True)
        parent.AddPage(page=panel, text=_("General"))

        border = wx.BoxSizer(wx.VERTICAL)

        # General settings
        box = StaticBox(
            parent=panel, id=wx.ID_ANY, label=" %s " % _("Customize Map Display")
        )
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer(hgap=4, vgap=4)

        # Auto-rendering
        autoRendering = ChBRender(panel, self.mapWindowProperties)
        #self.winId["display:autoRendering:enabled"] = autoRendering.GetId()
        gridSizer.Add(autoRendering.widget, pos=(0, 0), span=(1, 2))

        # Align extent to display size
        alignExtent = ChBAlignExtent(panel, self.mapWindowProperties)
        #self.winId["display:alignExtent:enabled"] = alignExtent.GetId()
        gridSizer.Add(alignExtent.widget, pos=(1, 0), span=(1, 2))

        # Use computation resolution
        compResolution = ChBResolution(panel, self.giface, self.mapWindowProperties)
        #self.winId["display:compResolution:enabled"] = compResolution.GetId()
        gridSizer.Add(compResolution.widget, pos=(2, 0), span=(1, 2))

        # Show computation extent
        showCompExtent = ChBShowRegion(panel, self.giface, self.mapWindowProperties)
        #self.winId["display:showCompExtent:enabled"] = showCompExtent.GetId()
        gridSizer.Add(showCompExtent.widget, pos=(3, 0), span=(1, 2))

        gridSizer.AddGrowableCol(0)
        sizer.Add(gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)

        panel.SetSizer(border)

        return panel
