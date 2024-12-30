"""
@package mapdisp.statusbar

@brief Classes for statusbar management

Classes:
 - statusbar::SbException
 - statusbar::SbManager
 - statusbar::SbItem
 - statusbar::SbRender
 - statusbar::SbMapScale
 - statusbar::SbGoTo
 - statusbar::SbTextItem
 - statusbar::SbDisplayGeometry
 - statusbar::SbCoordinates
 - statusbar::SbRegionExtent
 - statusbar::SbCompRegionExtent
 - statusbar::SbProgress

(C) 2006-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import copy
import wx

from core import utils
from core.gcmd import RunCommand
from core.settings import UserSettings
from gui_core.wrap import TextCtrl, Menu, NewId

from grass.pydispatch.signal import Signal


class SbException(Exception):
    """Exception class used in SbManager and SbItems"""

    def __init__(self, message):
        # Exception.__init__(self, message)
        self.message = message

    def __str__(self):
        return self.message


class SbManager:
    """Statusbar manager for wx.Statusbar and SbItems.

    Statusbar manager manages items added by AddStatusbarItem method.
    Provides progress bar (SbProgress).
    Items with position 0 are shown according to selection in Map Display settings
    dialog. Only one item of the same class is supposed to be in statusbar. Manager
    user have to create statusbar on his own, add items to manager and call Update
    method to show particular widgets.
    User settings (group = 'display', key = 'statusbarMode', subkey = 'selection')
    are taken into account.

    .. todo::
        generalize access to UserSettings (specify group, etc.)

    .. todo::
        add GetMode method using name instead of index
    """

    def __init__(self, mapframe, statusbar):
        """Connects manager to statusbar

        Creates progress bar.
        """
        self.mapFrame = mapframe
        self.statusbar = statusbar

        self.statusbarItems = {}

        self._postInitialized = False
        self._modeIndexSet = False
        self._mode = 0

        self.progressbar = SbProgress(self.mapFrame, self.statusbar, self)
        self.progressbar.progressShown.connect(self._progressShown)
        self.progressbar.progressHidden.connect(self._progressHidden)

        self.statusbar.Bind(wx.EVT_CONTEXT_MENU, self.OnContextMenu)
        self.mapFrame.mapWindowProperties.sbItemChanged.connect(self.SetMode)

        self._oldStatus = ""

        self.disabledItems = {}

    def SetProperty(self, name, value):
        """Sets property represented by one of contained SbItems

        :param name: name of SbItem (from name attribute)
        :param value: value to be set
        """
        self.statusbarItems[name].SetValue(value)

    def GetProperty(self, name):
        """Returns property represented by one of contained SbItems

        :param name: name of SbItem (from name attribute)
        """
        return self.statusbarItems[name].GetValue()

    def HasProperty(self, name) -> bool:
        """Checks whether property is represented by one of contained SbItems

        :param name: name of SbItem (from name attribute)

        :return: True if particular SbItem is contained, False otherwise
        """
        return name in self.statusbarItems

    def AddStatusbarItem(self, item):
        """Adds item to statusbar"""
        self.statusbarItems[item.name] = item

    def AddStatusbarItemsByClass(self, itemClasses, **kwargs):
        """Adds items to statusbar

        :param list itemClasses: list of classes of items to be add
        :param kwargs: SbItem constructor parameters

        :func:`AddStatusbarItem`
        """
        for Item in itemClasses:
            item = Item(**kwargs)
            self.AddStatusbarItem(item)

    def DisableStatusbarItemsByClass(self, itemClasses):
        """Fill list of item indexes that are disabled.

        :param itemClasses list of classes of items to be disabled
        """
        for itemClass in itemClasses:
            for i in range(len(self.statusbarItems.values())):
                item = list(self.statusbarItems.values())[i]
                if item.__class__ == itemClass:
                    self.disabledItems[i] = item

    def GetItemLabels(self):
        """Get list of item labels"""
        return [
            value.label
            for value in self.statusbarItems.values()
            if value.GetPosition() == 0
        ]

    def GetDisabledItemLabels(self):
        """Get list of disabled item labels"""
        return [
            value.label
            for value in self.disabledItems.values()
            if value.GetPosition() == 0
        ]

    def ShowItem(self, itemName):
        """Invokes showing of particular item

        :func:`Update`
        """
        if (
            self.statusbarItems[itemName].GetPosition() != 0
            or not self.progressbar.IsShown()
        ):
            self.statusbarItems[itemName].Show()

    def _postInit(self):
        """Post-initialization method

        It sets internal user settings,
        set selection (from map display settings) and does reposition.
        It is called automatically.
        """
        UserSettings.Set(
            group="display",
            key="statusbarMode",
            subkey="choices",
            value=self.GetItemLabels(),
            settings_type="internal",
        )
        if not self._modeIndexSet:
            self.SetMode(
                UserSettings.Get(
                    group="display", key="statusbarMode", subkey="selection"
                )
            )
            self.mapFrame.mapWindowProperties.sbItem = UserSettings.Get(
                group="display", key="statusbarMode", subkey="selection"
            )
        self.Reposition()

        self._postInitialized = True

    def Update(self):
        """Updates statusbar"""
        self.progressbar.Update()

        if not self._postInitialized:
            self._postInit()
        for item in self.statusbarItems.values():
            if item.GetPosition() == 0:
                if not self.progressbar.IsShown():
                    item.Hide()
            else:
                item.Update()  # render

        if self.progressbar.IsShown():
            pass
        else:
            item = list(self.statusbarItems.values())[self.GetMode()]
            item.Update()

    def Reposition(self):
        """Reposition items in statusbar

        Set positions to all items managed by statusbar manager.
        It should not be necessary to call it manually.
        """

        widgets = []
        for item in self.statusbarItems.values():
            widgets.append((item.GetPosition(), item.GetWidget()))

        widgets.append((1, self.progressbar.GetWidget()))

        for idx, win in widgets:
            if not win:
                continue
            rect = self.statusbar.GetFieldRect(idx)
            if idx == 0:  # show region / mapscale / process bar
                # -> size
                wWin, hWin = win.GetBestSize()
                # -> position
                # if win == self.statusbarWin['region']:
                # x, y = rect.x + rect.width - wWin, rect.y - 1
                # align left
                # else:
                x, y = rect.x + 3, rect.y - 1
                w, h = wWin, rect.height + 2
            else:  # auto-rendering
                x, y = rect.x, rect.y
                w, h = rect.width, rect.height + 1
                if win == self.progressbar.GetWidget():
                    wWin = rect.width - 6
                if idx == 2:  # render
                    x += 5
            win.SetPosition((x, y))
            win.SetSize((w, h))

    def GetProgressBar(self):
        """Returns progress bar"""
        return self.progressbar

    def _progressShown(self):
        self._oldStatus = self.statusbar.GetStatusText(0)

    def _progressHidden(self):
        self.statusbar.SetStatusText(self._oldStatus, 0)

    def SetMode(self, mode):
        """Sets current mode and updates statusbar

        Mode is usually driven by user through map display settings.
        """
        self._mode = mode
        self._modeIndexSet = True
        self.Update()

    def GetMode(self):
        """Returns current mode"""
        return self._mode

    def SetProgress(self, range, value, text):
        """Update progress."""
        self.progressbar.SetRange(range)
        self.progressbar.SetValue(value)
        if text:
            self.statusbar.SetStatusText(text)

    def OnContextMenu(self, event):
        """Popup context menu enabling to choose a widget that will be shown in
        statusbar."""

        def setSbItemProperty(idx):
            self.mapFrame.mapWindowProperties.sbItem = idx

        def getSbItemProperty():
            return self.mapFrame.mapWindowProperties.sbItem

        menu = Menu()
        for i, label in enumerate(self.GetItemLabels()):
            wxid = NewId()
            self.statusbar.Bind(
                wx.EVT_MENU,
                lambda evt, idx=i: setSbItemProperty(idx),
                id=wxid,
            )
            menu.Append(wxid, label, kind=wx.ITEM_RADIO)
            item = menu.FindItem(wxid)[0]
            if i == getSbItemProperty():
                item.Check(item.IsChecked() is False)
            if label in (self.GetDisabledItemLabels()):
                item.Enable(enable=False)

        # show the popup menu
        self.statusbar.PopupMenu(menu)
        menu.Destroy()
        event.Skip()


class SbItem:
    """Base class for statusbar items.

    Each item represents functionality (or action) controlled by statusbar
    and related to MapFrame.
    One item is usually connected with one widget but it is not necessary.
    Item can represent property (depends on manager).
    Items are not widgets but can provide interface to them.
    Items usually has requirements to MapFrame instance
    (specified as MapFrame.methodname or MapWindow.methodname).

    .. todo::
        consider externalizing position (see SbProgress use in SbManager)
    """

    def __init__(self, mapframe, statusbar, position=0):
        """

        :param mapframe: instance of class with MapFrame interface
        :param statusbar: statusbar instance (wx.Statusbar)
        :param position: item position in statusbar

        .. todo::
            rewrite Update also in derived classes to take in account item position
        """
        self.mapFrame = mapframe
        self.statusbar = statusbar
        self.position = position

    def Show(self):
        """Invokes showing of underlying widget.

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
        """Returns underlying widget.

        :return: widget or None if doesn't exist
        """
        return self.widget

    def _update(self, longHelp):
        """Default implementation for Update method.

        :param longHelp: True to enable long help (help from toolbars)
        """
        self.statusbar.SetStatusText("", 0)
        self.Show()
        self.mapFrame.StatusbarEnableLongHelp(longHelp)

    def Update(self):
        """Called when statusbar action is activated (e.g. through Map Display
        settings)."""
        self._update(longHelp=False)


class SbRender(SbItem):
    """Checkbox to enable and disable auto-rendering.

    Requires MapFrame.OnRender method.
    """

    def __init__(self, mapframe, statusbar, position=0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = "render"
        self._properties = mapframe.mapWindowProperties
        self.widget = wx.CheckBox(
            parent=self.statusbar, id=wx.ID_ANY, label=_("Render")
        )

        self.widget.SetValue(self._properties.autoRender)
        self.widget.Hide()
        self.widget.SetToolTip(wx.ToolTip(_("Enable/disable auto-rendering")))

        self._connectAutoRender()
        self.widget.Bind(wx.EVT_CHECKBOX, self._onCheckbox)

    def _setValue(self, value):
        self.widget.SetValue(value)

    def _connectAutoRender(self):
        self._properties.autoRenderChanged.connect(self._setValue)

    def _disconnectAutoRender(self):
        self._properties.autoRenderChanged.disconnect(self._setValue)

    def _onCheckbox(self, event):
        self._disconnectAutoRender()
        self._properties.autoRender = self.widget.GetValue()
        self._connectAutoRender()

    def Update(self):
        self.Show()


class SbMapScale(SbItem):
    """Editable combobox to get/set current map scale.

    Requires MapFrame.GetMapScale, MapFrame.SetMapScale
    and MapFrame.GetWindow (and GetWindow().UpdateMap()).
    """

    def __init__(self, mapframe, statusbar, position=0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = "mapscale"
        self.label = _("Map scale")

        self.widget = wx.ComboBox(
            parent=self.statusbar,
            id=wx.ID_ANY,
            style=wx.TE_PROCESS_ENTER,
            size=(150, -1),
        )

        self.widget.SetItems(
            [
                "1:1000",
                "1:5000",
                "1:10000",
                "1:25000",
                "1:50000",
                "1:100000",
                "1:1000000",
            ]
        )
        self.widget.Hide()
        self.widget.SetToolTip(
            wx.ToolTip(
                _(
                    "As everyone's monitors and resolutions "
                    "are set differently these values are not "
                    "true map scales, but should get you into "
                    "the right neighborhood."
                )
            )
        )

        self.widget.Bind(wx.EVT_TEXT_ENTER, self.OnChangeMapScale)
        self.widget.Bind(wx.EVT_COMBOBOX, self.OnChangeMapScale)

        self.lastMapScale = None

    def Update(self):
        scale = self.mapFrame.GetMapScale()
        self.statusbar.SetStatusText("")
        try:
            self.SetValue("1:%ld" % (scale + 0.5))
        except TypeError:
            pass  # FIXME, why this should happen?

        self.lastMapScale = scale
        self.Show()

        # disable long help
        self.mapFrame.StatusbarEnableLongHelp(False)

    def OnChangeMapScale(self, event):
        """Map scale changed by user"""
        scale = event.GetString()

        try:
            if scale[:2] != "1:":
                raise ValueError
            value = int(scale[2:])
        except ValueError:
            self.SetValue("1:%ld" % int(self.lastMapScale))
            return

        self.mapFrame.SetMapScale(value)

        # redraw a map
        self.mapFrame.GetWindow().UpdateMap()
        self.GetWidget().SetFocus()


class SbGoTo(SbItem):
    """Textctrl to set coordinates which to focus on.

    Requires MapFrame.GetWindow, MapWindow.GoTo method.
    """

    def __init__(self, mapframe, statusbar, position=0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = "goto"
        self.label = _("Go to XY coordinates")

        self.widget = TextCtrl(
            parent=self.statusbar,
            id=wx.ID_ANY,
            value="",
            style=wx.TE_PROCESS_ENTER,
            size=(300, -1),
        )

        self.widget.Hide()

        self.widget.Bind(wx.EVT_TEXT_ENTER, self.OnGoTo)

    def ReprojectENToMap(self, e, n, useDefinedProjection):
        """Reproject east, north from user defined projection

        :param e,n: coordinate (for DMS string, else float or string)
        :param useDefinedProjection: projection defined by user in settings dialog

        @throws SbException if useDefinedProjection is True and projection is not
                defined in UserSettings
        """
        if useDefinedProjection:
            settings = UserSettings.Get(
                group="projection", key="statusbar", subkey="proj4"
            )
            if not settings:
                raise SbException(_("Projection not defined (check the settings)"))
            # reproject values
            projIn = settings
            projOut = RunCommand("g.proj", flags="jf", read=True)
            proj = projIn.split(" ")[0].split("=")[1]
            if proj in {"ll", "latlong", "longlat"}:
                e, n = utils.DMS2Deg(e, n)
                proj, coord1 = utils.ReprojectCoordinates(
                    coord=(e, n), projIn=projIn, projOut=projOut, flags="d"
                )
                e, n = coord1
            else:
                e, n = float(e), float(n)
                proj, coord1 = utils.ReprojectCoordinates(
                    coord=(e, n), projIn=projIn, projOut=projOut, flags="d"
                )
                e, n = coord1
        elif self.mapFrame.GetMap().projinfo["proj"] == "ll":
            e, n = utils.DMS2Deg(e, n)
        else:
            e, n = float(e), float(n)
        return e, n

    def OnGoTo(self, event):
        """Go to position"""
        try:
            e, n = self.GetValue().split(";")
            e, n = self.ReprojectENToMap(
                e, n, self.mapFrame.GetProperty("useDefinedProjection")
            )
            self.mapFrame.GetWindow().GoTo(e, n)
            self.widget.SetFocus()
        except ValueError:
            # FIXME: move this code to MapWindow/BufferedWindow/MapFrame
            region = self.mapFrame.GetMap().GetCurrentRegion()
            precision = int(
                UserSettings.Get(group="projection", key="format", subkey="precision")
            )
            format = UserSettings.Get(group="projection", key="format", subkey="ll")
            if self.mapFrame.GetMap().projinfo["proj"] == "ll" and format == "DMS":
                self.SetValue(
                    "%s"
                    % utils.Deg2DMS(
                        region["center_easting"],
                        region["center_northing"],
                        precision=precision,
                    )
                )
            else:
                self.SetValue(
                    "%.*f; %.*f"
                    % (
                        precision,
                        region["center_easting"],
                        precision,
                        region["center_northing"],
                    )
                )
        except SbException as e:
            # FIXME: this may be useless since statusbar update checks user
            # defined projection and this exception raises when user def proj
            # does not exists
            self.statusbar.SetStatusText(str(e), 0)

    def GetCenterString(self, map):
        """Get current map center in appropriate format"""
        region = map.GetCurrentRegion()
        precision = int(
            UserSettings.Get(group="projection", key="format", subkey="precision")
        )
        format = UserSettings.Get(group="projection", key="format", subkey="ll")
        projection = UserSettings.Get(
            group="projection", key="statusbar", subkey="proj4"
        )

        if self.mapFrame.GetProperty("useDefinedProjection"):
            if not projection:
                raise SbException(_("Projection not defined (check the settings)"))
            proj, coord = utils.ReprojectCoordinates(
                coord=(region["center_easting"], region["center_northing"]),
                projOut=projection,
                flags="d",
            )
            if coord:
                if proj in {"ll", "latlong", "longlat"} and format == "DMS":
                    return "%s" % utils.Deg2DMS(coord[0], coord[1], precision=precision)
                return "%.*f; %.*f" % (precision, coord[0], precision, coord[1])
            raise SbException(_("Error in projection (check the settings)"))
        if self.mapFrame.GetMap().projinfo["proj"] == "ll" and format == "DMS":
            return "%s" % utils.Deg2DMS(
                region["center_easting"],
                region["center_northing"],
                precision=precision,
            )
        return "%.*f; %.*f" % (
            precision,
            region["center_easting"],
            precision,
            region["center_northing"],
        )

    def SetCenter(self):
        """Set current map center as item value"""
        center = self.GetCenterString(self.mapFrame.GetMap())
        self.SetValue(center)

    def Update(self):
        self.statusbar.SetStatusText("")

        try:
            self.SetCenter()
            self.Show()
        except SbException as e:
            self.statusbar.SetStatusText(str(e), 0)

        # disable long help
        self.mapFrame.StatusbarEnableLongHelp(False)


class SbTextItem(SbItem):
    """Base class for items without widgets.

    Only sets statusbar text.
    """

    def __init__(self, mapframe, statusbar, position=0):
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
        self._update(longHelp=True)


class SbDisplayGeometry(SbTextItem):
    """Show current display resolution."""

    def __init__(self, mapframe, statusbar, position=0):
        SbTextItem.__init__(self, mapframe, statusbar, position)
        self.name = "displayGeometry"
        self.label = _("Display geometry")

    def Show(self):
        region = copy.copy(self.mapFrame.GetMap().GetCurrentRegion())
        if self.mapFrame.mapWindowProperties.resolution:
            compRegion = self.mapFrame.GetMap().GetRegion(add3d=False)
            region["rows"] = abs(
                int((region["n"] - region["s"]) / compRegion["nsres"]) + 0.5
            )
            region["cols"] = abs(
                int((region["e"] - region["w"]) / compRegion["ewres"]) + 0.5
            )
            region["nsres"] = compRegion["nsres"]
            region["ewres"] = compRegion["ewres"]
        self.SetValue(
            "rows=%d; cols=%d; nsres=%.2f; ewres=%.2f"
            % (region["rows"], region["cols"], region["nsres"], region["ewres"])
        )
        SbTextItem.Show(self)


class SbCoordinates(SbTextItem):
    """Show map coordinates when mouse moves.

    Requires MapWindow.GetLastEN method."""

    def __init__(self, mapframe, statusbar, position=0):
        SbTextItem.__init__(self, mapframe, statusbar, position)
        self.name = "coordinates"
        self.label = _("Coordinates")
        self._additionalInfo = None
        self._basicValue = None

    def Show(self):
        """Show the last map window coordinates.

        .. todo::
            remove last EN call and use coordinates coming from signal
        """
        precision = int(
            UserSettings.Get(group="projection", key="format", subkey="precision")
        )
        format = UserSettings.Get(group="projection", key="format", subkey="ll")
        projection = self.mapFrame.GetProperty("useDefinedProjection")
        try:
            e, n = self.mapFrame.GetWindow().GetLastEN()
            self._basicValue = self.ReprojectENFromMap(
                e, n, projection, precision, format
            )
            if self._additionalInfo:
                value = "{coords} ({additionalInfo})".format(
                    coords=self._basicValue, additionalInfo=self._additionalInfo
                )
            else:
                value = self._basicValue
            self.SetValue(value)
        except SbException as e:
            self.SetValue(e.message)
        # TODO: remove these excepts, they just hide errors, solve problems
        # differently
        except TypeError:
            self.SetValue("")
        except AttributeError:
            # during initialization MapFrame has no MapWindow
            self.SetValue("")
        SbTextItem.Show(self)

    def SetAdditionalInfo(self, text):
        """Sets additional info to be shown together with coordinates.

        The format is translation dependent but the default is
        "coordinates (additional info)"

        It does not show the changed text immediately, it waits for the Show()
        method to be called.

        :param text: string to be shown
        """
        self._additionalInfo = text

    def ReprojectENFromMap(self, e, n, useDefinedProjection, precision, format):
        """Reproject east, north to user defined projection.

        :param e,n: coordinate

        @throws SbException if useDefinedProjection is True and projection is not
                defined in UserSettings
        """
        if useDefinedProjection:
            settings = UserSettings.Get(
                group="projection", key="statusbar", subkey="proj4"
            )
            if not settings:
                raise SbException(_("Projection not defined (check the settings)"))
            # reproject values
            proj, coord = utils.ReprojectCoordinates(
                coord=(e, n), projOut=settings, flags="d"
            )
            if coord:
                e, n = coord
                if proj in {"ll", "latlong", "longlat"} and format == "DMS":
                    return utils.Deg2DMS(e, n, precision=precision)
                return "%.*f; %.*f" % (precision, e, precision, n)
            raise SbException(_("Error in projection (check the settings)"))
        if self.mapFrame.GetMap().projinfo["proj"] == "ll" and format == "DMS":
            return utils.Deg2DMS(e, n, precision=precision)
        return "%.*f; %.*f" % (precision, e, precision, n)


class SbRegionExtent(SbTextItem):
    """Shows current display region"""

    def __init__(self, mapframe, statusbar, position=0):
        SbTextItem.__init__(self, mapframe, statusbar, position)
        self.name = "displayRegion"
        self.label = _("Display extent")

    def Show(self):
        precision = int(
            UserSettings.Get(group="projection", key="format", subkey="precision")
        )
        format = UserSettings.Get(group="projection", key="format", subkey="ll")
        projection = self.mapFrame.GetProperty("useDefinedProjection")
        region = self._getRegion()
        try:
            regionReprojected = self.ReprojectRegionFromMap(
                region, projection, precision, format
            )
            self.SetValue(regionReprojected)
        except SbException as e:
            self.SetValue(e.message)
        SbTextItem.Show(self)

    def _getRegion(self):
        """Get current display region"""
        return self.mapFrame.GetMap().GetCurrentRegion()  # display region

    def _formatRegion(self, w, e, s, n, nsres, ewres, precision=None):
        """Format display region string for statusbar

        :param nsres,ewres: unused
        """
        if precision is not None:
            return "%.*f - %.*f, %.*f - %.*f" % (
                precision,
                w,
                precision,
                e,
                precision,
                s,
                precision,
                n,
            )
        return "%s - %s, %s - %s" % (w, e, s, n)

    def ReprojectRegionFromMap(self, region, useDefinedProjection, precision, format):
        """Reproject region values

        .. todo::
            reorganize this method to remove code useful only for derived class
            SbCompRegionExtent
        """
        if useDefinedProjection:
            settings = UserSettings.Get(
                group="projection", key="statusbar", subkey="proj4"
            )

            if not settings:
                raise SbException(_("Projection not defined (check the settings)"))
            projOut = settings
            proj, coord1 = utils.ReprojectCoordinates(
                coord=(region["w"], region["s"]), projOut=projOut, flags="d"
            )
            proj, coord2 = utils.ReprojectCoordinates(
                coord=(region["e"], region["n"]), projOut=projOut, flags="d"
            )
            # useless, used in derived class
            proj, coord3 = utils.ReprojectCoordinates(
                coord=(0.0, 0.0), projOut=projOut, flags="d"
            )
            proj, coord4 = utils.ReprojectCoordinates(
                coord=(region["ewres"], region["nsres"]), projOut=projOut, flags="d"
            )
            if coord1 and coord2:
                if proj in {"ll", "latlong", "longlat"} and format == "DMS":
                    w, s = utils.Deg2DMS(
                        coord1[0], coord1[1], string=False, precision=precision
                    )
                    e, n = utils.Deg2DMS(
                        coord2[0], coord2[1], string=False, precision=precision
                    )
                    ewres, nsres = utils.Deg2DMS(
                        abs(coord3[0]) - abs(coord4[0]),
                        abs(coord3[1]) - abs(coord4[1]),
                        string=False,
                        hemisphere=False,
                        precision=precision,
                    )
                    return self._formatRegion(
                        w=w, s=s, e=e, n=n, ewres=ewres, nsres=nsres
                    )
                w, s = coord1
                e, n = coord2
                ewres, nsres = coord3
                return self._formatRegion(
                    w=w,
                    s=s,
                    e=e,
                    n=n,
                    ewres=ewres,
                    nsres=nsres,
                    precision=precision,
                )
            raise SbException(_("Error in projection (check the settings)"))

        if self.mapFrame.GetMap().projinfo["proj"] == "ll" and format == "DMS":
            w, s = utils.Deg2DMS(
                region["w"], region["s"], string=False, precision=precision
            )
            e, n = utils.Deg2DMS(
                region["e"], region["n"], string=False, precision=precision
            )
            ewres, nsres = utils.Deg2DMS(
                region["ewres"], region["nsres"], string=False, precision=precision
            )
            return self._formatRegion(w=w, s=s, e=e, n=n, ewres=ewres, nsres=nsres)
        w, s = region["w"], region["s"]
        e, n = region["e"], region["n"]
        ewres, nsres = region["ewres"], region["nsres"]
        return self._formatRegion(
            w=w, s=s, e=e, n=n, ewres=ewres, nsres=nsres, precision=precision
        )


class SbCompRegionExtent(SbRegionExtent):
    """Shows computational region."""

    def __init__(self, mapframe, statusbar, position=0):
        SbRegionExtent.__init__(self, mapframe, statusbar, position)
        self.name = "computationalRegion"
        self.label = _("Computational region")

    def _formatRegion(self, w, e, s, n, ewres, nsres, precision=None):
        """Format computational region string for statusbar"""
        if precision is not None:
            return "%.*f - %.*f, %.*f - %.*f (%.*f, %.*f)" % (
                precision,
                w,
                precision,
                e,
                precision,
                s,
                precision,
                n,
                precision,
                ewres,
                precision,
                nsres,
            )
        return "%s - %s, %s - %s (%s, %s)" % (w, e, s, n, ewres, nsres)

    def _getRegion(self):
        """Returns computational region."""
        return self.mapFrame.GetMap().GetRegion()  # computational region


class SbProgress(SbItem):
    """General progress bar to show progress.

    Underlying widget is wx.Gauge.
    """

    def __init__(self, mapframe, statusbar, sbManager, position=0):
        self.progressShown = Signal("SbProgress.progressShown")
        self.progressHidden = Signal("SbProgress.progressHidden")
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = "progress"
        self.sbManager = sbManager
        # on-render gauge
        self.widget = wx.Gauge(
            parent=self.statusbar, id=wx.ID_ANY, range=0, style=wx.GA_HORIZONTAL
        )
        self.Hide()

    def GetRange(self):
        """Returns progress range."""
        return self.widget.GetRange()

    def SetRange(self, range):
        """Sets progress range."""
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
        """Is progress bar shown"""
        return self.widget.IsShown()

    def SetValue(self, value):
        """Sets value of progressbar."""
        if value > self.GetRange():
            self.Hide()
            return

        self.widget.SetValue(value)
        if value == self.GetRange():
            self.Hide()

    def GetWidget(self):
        """Returns underlying winget.

        :return: widget or None if doesn't exist
        """
        return self.widget

    def Update(self):
        pass
