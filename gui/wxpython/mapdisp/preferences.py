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
from gui_core.wrap import Button


class ChBItem:
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

    def GetWidget(self):
        """Returns underlaying widget.

        :return: widget or None if doesn't exist
        """
        return self.widget

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

        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)
        self._connect()

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
        ChBItem.__init__(self, mapWindowProperties)
        self.name = "alignExtent"
        self.widget = wx.CheckBox(
            parent=parent,
            id=wx.ID_ANY,
            label=_("Align region extent based on display size"),
        )
        self.widget.SetValue(self.mapWindowProperty)
        self.widget.SetToolTip(
            wx.ToolTip(
                _(
                    "Align region extent based on display "
                    "size from center point. "
                    "Default value for new map displays can "
                    "be set up in 'User GUI settings' dialog."
                )
            )
        )

        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)
        self._connect()

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
        self.name = "resolution"
        self.widget = wx.CheckBox(
            parent=parent,
            id=wx.ID_ANY,
            label=_("Constrain display resolution to computational settings"),
        )
        self.widget.SetValue(self.mapWindowProperty)
        self.widget.SetToolTip(
            wx.ToolTip(
                _(
                    "Constrain display resolution "
                    "to computational region settings. "
                    "Default value for new map displays can "
                    "be set up in 'User GUI settings' dialog."
                )
            )
        )

        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)
        self._connect()

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
        self.widget = wx.CheckBox(
            parent=parent, id=wx.ID_ANY, label=_("Show computational extent")
        )
        self.widget.SetValue(self.mapWindowProperty)
        self.widget.SetToolTip(
            wx.ToolTip(
                _(
                    "Show/hide computational "
                    "region extent (set with g.region). "
                    "Display region drawn as a blue box inside the "
                    "computational region, "
                    "computational region inside a display region "
                    "as a red box)."
                )
            )
        )
        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggleCheckBox)
        self._connect()

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
    """Map Display preference dialog"""

    def __init__(
        self,
        parent,
        giface,
        properties,
        title=_("Map Display Settings"),
        size=(-1, 500),
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
    ):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=title, style=style)

        self.parent = parent
        self.title = title
        self.size = size
        self.giface = giface
        self.mapWindowProperties = properties

        # notebook
        self.notebook = wx.Notebook(parent=self, id=wx.ID_ANY, style=wx.BK_DEFAULT)
        # create notebook pages
        self._createDisplayPage(parent=self.notebook)

        self.btnCancel = Button(self, wx.ID_CANCEL, label="Close")
        self.btnCancel.SetToolTip(_("Close dialog"))

        self._layout()

    def _layout(self):
        """Layout window"""
        # sizers
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnCancel)
        btnStdSizer.Realize()

        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(self.notebook, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(btnStdSizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        self.SetSizer(mainSizer)
        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)

    def _createDisplayPage(self, parent):
        """Create notebook page for display settings"""

        panel = SP.ScrolledPanel(parent=parent, id=wx.ID_ANY)
        panel.SetupScrolling(scroll_x=False, scroll_y=True)
        parent.AddPage(page=panel, text=_("General"))

        border = wx.BoxSizer(wx.VERTICAL)

        # General settings
        gridSizer = wx.GridBagSizer(hgap=4, vgap=4)

        # Auto-rendering
        self.autoRendering = ChBRender(panel, self.mapWindowProperties)
        gridSizer.Add(self.autoRendering.GetWidget(), pos=(0, 0), span=(1, 2))

        # Align extent to display size
        self.alignExtent = ChBAlignExtent(panel, self.mapWindowProperties)
        gridSizer.Add(self.alignExtent.GetWidget(), pos=(1, 0), span=(1, 2))

        # Use computation resolution
        self.compResolution = ChBResolution(
            panel, self.giface, self.mapWindowProperties
        )
        gridSizer.Add(self.compResolution.GetWidget(), pos=(2, 0), span=(1, 2))

        # Show computation extent
        self.showCompExtent = ChBShowRegion(
            panel, self.giface, self.mapWindowProperties
        )
        gridSizer.Add(self.showCompExtent.GetWidget(), pos=(3, 0), span=(1, 2))

        gridSizer.AddGrowableCol(0)
        border.Add(gridSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=3)

        panel.SetSizer(border)

        return panel
