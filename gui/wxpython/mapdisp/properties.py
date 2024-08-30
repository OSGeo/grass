"""
@package mapdisp.properties

@brief Classes for map display properties management

Classes:
 - properties::PropertyItem
 - properties::ChBRender
 - properties::ChBShowRegion
 - properties::ChBAlignExtent
 - properties::ChBResolution
 - properties::ChBProjection
 - properties::RBShowInStatusbar
 - properties::MapDisplayPropertiesDialog

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


class PropertyItem:
    """Base class for Map Display properties widgets that use property signals"""

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

    def _setValue(self, value):
        self.widget.SetValue(value)

    def GetValue(self):
        return self.widget.GetValue()

    def GetWidget(self):
        """Returns underlying widget.

        :return: widget or None if doesn't exist
        """
        return self.widget

    def _connect(self):
        self.mapWindowPropertyChanged().connect(self._setValue)

    def _disconnect(self):
        self.mapWindowPropertyChanged().disconnect(self._setValue)

    def _onToggle(self, event):
        self._disconnect()
        self.mapWindowProperty = self.GetValue()
        self._connect()


class ChBRender(PropertyItem):
    """Checkbox to enable and disable auto-rendering."""

    def __init__(self, parent, mapWindowProperties):
        PropertyItem.__init__(self, mapWindowProperties)
        self.name = "render"
        self.widget = wx.CheckBox(
            parent=parent, id=wx.ID_ANY, label=_("Enable auto-rendering")
        )
        self.widget.SetValue(self.mapWindowProperty)
        self.widget.SetToolTip(wx.ToolTip(_("Enable/disable auto-rendering")))

        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggle)
        self._connect()

    @property
    def mapWindowProperty(self):
        return self._properties.autoRender

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.autoRender = value

    def mapWindowPropertyChanged(self):
        return self._properties.autoRenderChanged


class ChBAlignExtent(PropertyItem):
    """Checkbox to select zoom behavior.

    Used by BufferedWindow (through MapFrame property).
    See tooltip for explanation.
    """

    def __init__(self, parent, mapWindowProperties):
        PropertyItem.__init__(self, mapWindowProperties)
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
        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggle)
        self._connect()

    @property
    def mapWindowProperty(self):
        return self._properties.alignExtent

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.alignExtent = value

    def mapWindowPropertyChanged(self):
        return self._properties.alignExtentChanged


class ChBResolution(PropertyItem):
    """Checkbox to select used display resolution."""

    def __init__(self, parent, mapframe, mapWindowProperties):
        PropertyItem.__init__(self, mapWindowProperties)
        self.mapFrame = mapframe
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
        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggle)
        self._connect()

    @property
    def mapWindowProperty(self):
        return self._properties.resolution

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.resolution = value

    def mapWindowPropertyChanged(self):
        return self._properties.resolutionChanged

    def _onToggle(self, event):
        """Update display when toggle display mode"""
        super()._onToggle(event)

        # redraw map if auto-rendering is enabled
        if self._properties.autoRender:
            self.mapFrame.GetWindow().UpdateMap()


class ChBShowRegion(PropertyItem):
    """Checkbox to enable and disable showing of computational region."""

    def __init__(self, parent, mapframe, mapWindowProperties):
        PropertyItem.__init__(self, mapWindowProperties)
        self.mapFrame = mapframe
        self.name = "showRegion"
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
        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggle)
        self._connect()

    @property
    def mapWindowProperty(self):
        return self._properties.showRegion

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.showRegion = value

    def mapWindowPropertyChanged(self):
        return self._properties.showRegionChanged

    def _onToggle(self, event):
        """Shows/Hides extent (comp. region) in map canvas.

        Shows or hides according to checkbox value.
        """
        super()._onToggle(event)

        # redraw map if auto-rendering is enabled
        if self._properties.autoRender:
            self.mapFrame.GetWindow().UpdateMap(render=False)


class ChBProjection(PropertyItem):
    """Checkbox to enable user defined projection"""

    def __init__(self, parent, mapWindowProperties):
        PropertyItem.__init__(self, mapWindowProperties)
        self.name = "useDefinedProjection"
        self.defaultLabel = _("Display coordinates in different CRS")
        self.widget = wx.CheckBox(parent=parent, id=wx.ID_ANY, label=self.defaultLabel)
        self.widget.SetValue(self.mapWindowProperty)
        self.widget.SetToolTip(
            wx.ToolTip(
                _(
                    "Reproject coordinates displayed "
                    "in the statusbar. Coordinate reference system can be "
                    "specified in GUI preferences dialog "
                    "(tab 'Projection')"
                )
            )
        )
        self.widget.Bind(wx.EVT_CHECKBOX, self._onToggle)
        self._connect()

    @property
    def mapWindowProperty(self):
        return self._properties.useDefinedProjection

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.useDefinedProjection = value

    def mapWindowPropertyChanged(self):
        return self._properties.useDefinedProjectionChanged

    def _onToggle(self, event):
        super()._onToggle(event)
        epsg = self._properties.epsg
        if epsg:
            label = _("{label} (EPSG: {epsg})").format(
                label=self.defaultLabel, epsg=epsg
            )
            self.widget.SetLabel(label)
        else:
            self.widget.SetLabel(self.defaultLabel)


class RBShowInStatusbar(PropertyItem):
    """Radiobox managing widgets in statusbar."""

    def __init__(self, parent, mapWindowProperties, sbmanager):
        PropertyItem.__init__(self, mapWindowProperties)
        self.name = "showInStatusbar"
        self.statusbarManager = sbmanager

        choices = self.statusbarManager.GetItemLabels()
        self.widget = wx.RadioBox(
            parent=parent,
            id=wx.ID_ANY,
            label="Displayed content",
            choices=choices,
            majorDimension=1,
            style=wx.RA_SPECIFY_COLS,
        )
        self._setValue(self.mapWindowProperty)
        self._disableItems()

        self.widget.Bind(wx.EVT_RADIOBOX, self._onToggle)
        self._connect()

    def _setValue(self, mode):
        self.widget.SetSelection(mode)

    def GetValue(self):
        return self.widget.GetSelection()

    def _disableItems(self):
        """Disables a radiobox options"""
        for item in self.statusbarManager.disabledItems.keys():
            self.widget.EnableItem(n=item, enable=False)

    @property
    def mapWindowProperty(self):
        return self._properties.sbItem

    @mapWindowProperty.setter
    def mapWindowProperty(self, value):
        self._properties.sbItem = value

    def mapWindowPropertyChanged(self):
        return self._properties.sbItemChanged


class MapDisplayPropertiesDialog(wx.Dialog):
    """Map Display properties dialog"""

    def __init__(
        self,
        parent,
        mapframe,
        properties,
        sbmanager,
        title=_("Map Display Settings"),
        size=(-1, 300),
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
    ):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=title, style=style)

        self.parent = parent
        self.title = title
        self.size = size
        self.mapframe = mapframe
        self.mapWindowProperties = properties
        self.statusbarManager = sbmanager

        # notebook
        self.notebook = wx.Notebook(parent=self, id=wx.ID_ANY, style=wx.BK_DEFAULT)
        # create notebook pages
        self._createDisplayPage(parent=self.notebook)
        self._createStatusBarPage(parent=self.notebook)

        self.btnClose = Button(self, wx.ID_CLOSE)
        self.SetEscapeId(wx.ID_CLOSE)

        self._layout()

    def _layout(self):
        """Layout window"""
        # sizers
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnClose)
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

        # General settings
        sizer = wx.BoxSizer(wx.VERTICAL)

        # Auto-rendering
        self.autoRendering = ChBRender(panel, self.mapWindowProperties)
        sizer.Add(
            self.autoRendering.GetWidget(),
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=3,
        )

        # Align extent to display size
        self.alignExtent = ChBAlignExtent(panel, self.mapWindowProperties)
        sizer.Add(
            self.alignExtent.GetWidget(),
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=3,
        )

        # Use computation resolution
        self.compResolution = ChBResolution(
            panel, self.mapframe, self.mapWindowProperties
        )
        sizer.Add(
            self.compResolution.GetWidget(),
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=3,
        )

        # Show computation extent
        self.showCompExtent = ChBShowRegion(
            panel, self.mapframe, self.mapWindowProperties
        )
        sizer.Add(
            self.showCompExtent.GetWidget(),
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=3,
        )

        panel.SetSizer(sizer)

    def _createStatusBarPage(self, parent):
        """Create notebook page for statusbar settings"""

        panel = SP.ScrolledPanel(parent=parent, id=wx.ID_ANY)
        panel.SetupScrolling(scroll_x=False, scroll_y=True)
        parent.AddPage(page=panel, text=_("Status bar"))

        sizer = wx.BoxSizer(wx.VERTICAL)

        self.shownInStatusbar = RBShowInStatusbar(
            parent=panel,
            mapWindowProperties=self.mapWindowProperties,
            sbmanager=self.statusbarManager,
        )
        sizer.Add(
            self.shownInStatusbar.GetWidget(),
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=3,
        )

        # Display coordinates in different CRS
        self.projection = ChBProjection(panel, self.mapWindowProperties)
        sizer.Add(
            self.projection.GetWidget(),
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=3,
        )

        panel.SetSizer(sizer)
