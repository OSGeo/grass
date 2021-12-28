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


class ChBItem:
    """Base class for checkbox settings items"""

    def __init__(self, parent, mapWindowProperties):
        self._properties = mapWindowProperties
        self.checkbox = wx.CheckBox(parent=parent, id=wx.ID_ANY, label=self.label)
        self.checkbox.SetValue(self.mapWindowProperty)
        self.checkbox.SetToolTip(wx.ToolTip(self.tooltip))

        self._connect()
        self.checkbox.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)

    @property
    def name(self):
        pass

    @property
    def label(self):
        pass

    @property
    def tooltip(self):
        pass

    @property
    def mapWindowProperty(self):
        pass

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        pass

    @property
    def _mapWindowPropertyChanged(self):
        pass

    def SetValue(self, value):
        self.checkbox.SetValue(value)

    def GetValue(self):
        return self.checkbox.GetValue()

    def _connect(self):
        self._mapWindowPropertyChanged.connect(self.SetValue)

    def _disconnect(self):
        self._mapWindowPropertyChanged.disconnect(self.SetValue)

    def _onToggleCheckBox(self, event):
        self._disconnect()
        self.mapWindowProperty = self.GetValue()
        self._connect()


class ChBRender(ChBItem):
    """Checkbox to enable and disable auto-rendering.
    """

    @property
    def name(self):
        return "render"

    @property
    def label(self):
        return _("Render")

    @property
    def tooltip(self):
        return _("Enable/disable auto-rendering")

    @property
    def mapWindowProperty(self):
        return self._properties.autoRender

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.autoRender = value

    @property
    def _mapWindowPropertyChanged(self):
        return self._properties.autoRenderChanged


class ChBAlignExtent(ChBItem):
    """Checkbox to select zoom behavior.

    Used by BufferedWindow (through MapFrame property).
    See tooltip for explanation.
    """

    @property
    def name(self):
        return "alignExtent"

    @property
    def label(self):
        return _("Align region extent based on display size")

    @property
    def tooltip(self):
        return _(
            "Align region extent based on display "
            "size from center point. "
            "Default value for new map displays can "
            "be set up in 'User GUI settings' dialog."
        )

    @property
    def mapWindowProperty(self):
        return self._properties.alignExtent

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.alignExtent = value

    @property
    def _mapWindowPropertyChanged(self):
        return self._properties.alignExtentChanged


class ChBResolution(ChBItem):
    """Checkbox to select used display resolution.
    """

    def __init__(self, parent, giface, mapWindowProperties):
        ChBItem.__init__(self, parent, mapWindowProperties)
        self.giface = giface

    @property
    def name(self):
        return "resolution"

    @property
    def label(self):
        return _("Constrain display resolution to computational settings")

    @property
    def tooltip(self):
        return _(
            "Constrain display resolution "
            "to computational region settings. "
            "Default value for new map displays can "
            "be set up in 'User GUI settings' dialog."
        )

    @property
    def mapWindowProperty(self):
        return self._properties.resolution

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.resolution = value

    @property
    def _mapWindowPropertyChanged(self):
        return self._properties.resolutionChanged

    def _onToggleCheckBox(self, event):
        """Update display when toggle display mode"""
        super()._onToggleCheckBox(event)

        # redraw map if auto-rendering is enabled
        if self._properties.autoRender:
            self.giface.updateMap.emit()


class ChBShowRegion(ChBItem):
    """Checkbox to enable and disable showing of computational region.
    """

    def __init__(self, parent, giface, mapWindowProperties):
        ChBItem.__init__(self, parent, mapWindowProperties)
        self.giface = giface

    @property
    def name(self):
        return "region"

    @property
    def label(self):
        return _("Show computational extent")

    @property
    def tooltip(self):
        return _(
            "Show/hide computational "
            "region extent (set with g.region). "
            "Display region drawn as a blue box inside the "
            "computational region, "
            "computational region inside a display region "
            "as a red box)."
        )

    @property
    def mapWindowProperty(self):
        return self._properties.showRegion

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.showRegion = value

    @property
    def _mapWindowPropertyChanged(self):
        return self._properties.showRegionChanged

    def _onToggleCheckBox(self, event):
        """Shows/Hides extent (comp. region) in map canvas.

        Shows or hides according to checkbox value.
        """
        super()._onToggleCheckBox(event)

        # redraw map if auto-rendering is enabled
        if self._properties.autoRender:
            self.giface.updateMap.emit(render=False)

    def SetValue(self, value):
        self._disconnect()
        self.mapWindowProperty(value)
        ChBItem.SetValue(self, value)
        self._connect()


class MapDisplayPreferencesDialog(PreferencesBaseDialog):
    """Map Display preferences dialog"""

    def __init__(
        self, parent, giface, properties, title=_("Map Display Settings"), settings=UserSettings
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

        gridSizer = wx.GridBagSizer(hgap=3, vgap=3)

        # Align extent to display size
        alignExtent = ChBAlignExtent(panel, self.mapWindowProperties)
        gridSizer.Add(alignExtent.checkbox, pos=(0, 0), span=(1, 2))

        # Use computation resolution
        compResolution = ChBResolution(panel, self.giface, self.mapWindowProperties)
        gridSizer.Add(compResolution.checkbox, pos=(1, 0), span=(1, 2))

        # Show computation extent
        showCompExtent = ChBShowRegion(panel, self.giface, self.mapWindowProperties)
        gridSizer.Add(showCompExtent.checkbox, pos=(2, 0), span=(1, 2))

        gridSizer.AddGrowableCol(0)
        sizer.Add(gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)

        panel.SetSizer(border)

        return panel
