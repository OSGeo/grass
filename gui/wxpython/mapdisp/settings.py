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

    def __init__(self, parent, mapframe):
        self.mapFrame = mapframe
        self._properties = self.mapFrame.mapWindowProperties
        self.checkbox = wx.CheckBox(parent=parent, id=wx.ID_ANY, label=self.label)
        self.checkbox.SetValue(self._property)
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
    def _property(self):
        pass

    @_property.setter
    def _property(self, value):
        pass

    @property
    def _propertyChanged(self):
        pass

    def SetValue(self, value):
        self.checkbox.SetValue(value)

    def GetValue(self):
        return self.checkbox.GetValue()

    def _connect(self):
        self._propertyChanged.connect(self.SetValue)

    def _disconnect(self):
        self._propertyChanged.disconnect(self.SetValue)

    def _onToggleCheckBox(self, event):
        self._disconnect()
        self._property = self.GetValue()
        self._connect()


class ChBRender(ChBItem):
    """Checkbox to enable and disable auto-rendering.

    Requires MapFrame.OnRender method.
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
    def _property(self):
        return self._properties.autoRender

    @_property.setter
    def _property(self, value):
        self._properties.autoRender = value

    @property
    def _propertyChanged(self):
        return self._properties.autoRenderChanged


class ChBShowRegion(ChBItem):
    """Checkbox to enable and disable showing of computational region.

    Requires MapFrame.OnRender, MapFrame.IsAutoRendered, MapFrame.GetWindow.
    """

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
    def _property(self):
        return self._properties.showRegion

    @_property.setter
    def _property(self, value):
        self._properties.showRegion = value

    @property
    def _propertyChanged(self):
        return self._properties.showRegionChanged

    def _onToggleCheckBox(self, event):
        """Shows/Hides extent (comp. region) in map canvas.

        Shows or hides according to checkbox value.
        """
        super()._onToggleCheckBox(event)

        # redraw map if auto-rendering is enabled
        if self.mapFrame.IsAutoRendered():
            self.mapFrame.GetWindow().UpdateMap(render=False)

    def SetValue(self, value):
        self._disconnect()
        self._property(value)
        ChBItem.SetValue(self, value)
        self._connect()


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
    def _property(self):
        return self._properties.alignExtent

    @_property.setter
    def _property(self, value):
        self._properties.alignExtent = value

    @property
    def _propertyChanged(self):
        return self._properties.alignExtentChanged


class ChBResolution(ChBItem):
    """Checkbox to select used display resolution.

    Requires MapFrame.OnRender method.
    """

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
    def _property(self):
        return self._properties.resolution

    @_property.setter
    def _property(self, value):
        self._properties.resolution = value

    @property
    def _propertyChanged(self):
        return self._properties.resolutionChanged

    def _onToggleCheckBox(self, event):
        """Update display when toggle display mode"""
        super()._onToggleCheckBox(event)

        # redraw map if auto-rendering is enabled
        if self.mapFrame.IsAutoRendered():
            self.mapFrame.GetWindow().UpdateMap()


class MapDisplayPreferencesDialog(PreferencesBaseDialog):
    """Map Display preferences dialog"""

    def __init__(
        self, parent, giface, title=_("Map Display Settings"), settings=UserSettings
    ):
        PreferencesBaseDialog.__init__(
            self, parent=parent, giface=giface, title=title, settings=settings
        )
        self.parent = parent

        # create notebook pages
        self.displayPage = self._createDisplayPage(self.notebook)

        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)

    def _createDisplayPage(self, notebook):
        """Create notebook page for display settings"""

        panel = SP.ScrolledPanel(parent=notebook, id=wx.ID_ANY)
        panel.SetupScrolling(scroll_x=False, scroll_y=True)
        notebook.AddPage(page=panel, text=_("General"))

        border = wx.BoxSizer(wx.VERTICAL)

        # General settings
        box = StaticBox(
            parent=panel, id=wx.ID_ANY, label=" %s " % _("Customize Map Display")
        )
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer(hgap=3, vgap=3)

        # Align extent to display size
        alignExtent = ChBAlignExtent(parent=panel, mapframe=self.parent)
        gridSizer.Add(alignExtent.checkbox, pos=(0, 0), span=(1, 2))

        # Use computation resolution
        compResolution = ChBResolution(parent=panel, mapframe=self.parent)
        gridSizer.Add(compResolution.checkbox, pos=(1, 0), span=(1, 2))

        # Show computation extent
        showCompExtent = ChBShowRegion(parent=panel, mapframe=self.parent)
        gridSizer.Add(showCompExtent.checkbox, pos=(2, 0), span=(1, 2))

        gridSizer.AddGrowableCol(0)
        sizer.Add(gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        border.Add(sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)

        panel.SetSizer(border)

        return panel
