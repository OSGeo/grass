"""
@package mapdisp.settings

@brief Classes for map display settings management

Classes:
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


class ChBItem(wx.CheckBox):
    """Base class for checkbox settings items"""

    def __init__(self, parent, mapWindowProperties, label, tooltip):
        wx.CheckBox.__init__(self, parent=parent,
                            id=wx.ID_ANY,
                            label=label,
                            name="IsChecked")
        self._properties = mapWindowProperties
        self._setValue(self.mapWindowProperty)
        self.SetToolTip(wx.ToolTip(tooltip))

        self._connect()
        self.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)

    @property
    def mapWindowProperty(self):
        pass

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        pass

    @property
    def mapWindowPropertyChanged(self):
        pass

    def _setValue(self, value):
        self.SetValue(value)

    def SetValue(self, value):
        self._disconnect()
        self.mapWindowProperty = value
        wx.CheckBox.SetValue(self, value)
        self._connect()

    def _connect(self):
        self.mapWindowPropertyChanged.connect(self._setValue)

    def _disconnect(self):
        self.mapWindowPropertyChanged.disconnect(self._setValue)

    def _onToggleCheckBox(self, event):
        self._disconnect()
        self.mapWindowProperty = self.GetValue()
        self._connect()


class ChBRender(ChBItem):
    """Checkbox to enable and disable auto-rendering."""

    def __init__(self, parent, mapWindowProperties):
        label = _("Enable auto-rendering")
        tooltip = _("Enable/disable auto-rendering")
        ChBItem.__init__(self, parent, mapWindowProperties, label , tooltip)

    @property
    def mapWindowProperty(self):
        return self._properties.autoRender

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.autoRender = value

    @property
    def mapWindowPropertyChanged(self):
        return self._properties.autoRenderChanged


class ChBAlignExtent(ChBItem):
    """Checkbox to select zoom behavior.

    Used by BufferedWindow (through MapFrame property).
    See tooltip for explanation.
    """
    def __init__(self, parent, mapWindowProperties):
        label = _("Align region extent based on display size")
        tooltip = _(
                "Align region extent based on display "
                "size from center point. "
                "Default value for new map displays can "
                "be set up in 'User GUI settings' dialog.")
        ChBItem.__init__(self, parent, mapWindowProperties, label , tooltip)

    @property
    def mapWindowProperty(self):
        return self._properties.alignExtent

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.alignExtent = value

    @property
    def mapWindowPropertyChanged(self):
        return self._properties.alignExtentChanged


class ChBResolution(ChBItem):
    """Checkbox to select used display resolution."""
    def __init__(self, parent, giface, mapWindowProperties):
        self.giface = giface
        label = _("Constrain display resolution to computational settings")
        tooltip = _(
                "Constrain display resolution "
                "to computational region settings. "
                "Default value for new map displays can "
                "be set up in 'User GUI settings' dialog."
                )
        ChBItem.__init__(self, parent, mapWindowProperties, label , tooltip)

    @property
    def mapWindowProperty(self):
        return self._properties.resolution

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.resolution = value

    @property
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
        self.giface = giface
        label = _("Show computational extent")
        tooltip = _(
                "Show/hide computational "
                "region extent (set with g.region). "
                "Display region drawn as a blue box inside the "
                "computational region, "
                "computational region inside a display region "
                "as a red box)."
                )
        ChBItem.__init__(self, parent, mapWindowProperties, label , tooltip)

    @property
    def mapWindowProperty(self):
        return self._properties.showRegion

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.showRegion = value

    @property
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
        self.winId["display:autoRendering:enabled"] = autoRendering.GetId()
        gridSizer.Add(autoRendering, pos=(0, 0), span=(1, 2))

        # Align extent to display size
        alignExtent = ChBAlignExtent(panel, self.mapWindowProperties)
        self.winId["display:alignExtent:enabled"] = alignExtent.GetId()
        gridSizer.Add(alignExtent, pos=(1, 0), span=(1, 2))

        # Use computation resolution
        compResolution = ChBResolution(panel, self.giface, self.mapWindowProperties)
        self.winId["display:compResolution:enabled"] = compResolution.GetId()
        gridSizer.Add(compResolution, pos=(2, 0), span=(1, 2))

        # Show computation extent
        showCompExtent = ChBShowRegion(panel, self.giface, self.mapWindowProperties)
        self.winId["display:showCompExtent:enabled"] = showCompExtent.GetId()
        gridSizer.Add(showCompExtent, pos=(3, 0), span=(1, 2))

        gridSizer.AddGrowableCol(0)
        sizer.Add(gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)

        panel.SetSizer(border)

        return panel
