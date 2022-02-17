"""
@package frame.statusbar

@brief Classes for main window statusbar management

Classes:
 - statusbar::SbMapDisplay
 - statusbar::SbException
 - statusbar::SbItem
 - statusbar::SbTextItem
 - statusbar::SbCoordinates

(C) 2022 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Kladivova <lindakladivova gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
"""

import wx

from core import utils
from mapdisp.properties import ChBRender


class SbMapDisplay:
    """Statusbar for map display window."""

    def __init__(self, parent, giface):
        self.parent = parent
        self.giface = giface
        self.statusbar = wx.StatusBar(self.parent, id=wx.ID_ANY)
        self.statusbar.SetMinHeight(24)
        self.statusbar.SetFieldsCount(3)
        self.statusbar.SetStatusWidths([200, -1, 100])
        self._init_items()
        self.statusbar.Bind(wx.EVT_SIZE, self.OnSize)

        self._repositionStatusbar()

    def _init_items(self):
        """"Init all items that can be shown in statusbar."""
        self.coordinates = SbCoordinates(self.widget, self.giface)
        self.render = ChBRender(self.widget, self.properties)

    def GetStatusbar(self):
        return self.statusbar

    def _repositionStatusbar(self):
        """Reposition widgets in main window statusbar"""
        rect1 = self.GetStatusbar().GetFieldRect(1)
        rect1.x += 1
        rect1.y += 1
        self.coordinates.GetItem().SetRect(rect1)
        rect1.x += 1
        rect1.y += 1
        self.render.GetItem().SetRect(rect1)

    def OnSize(self, event):
        """Adjust main window statusbar on changing size"""
        self._repositionStatusbar()

    def SetStatusText(self, *args):
        """Overide wx.StatusBar method"""
        self.GetStatusbar().SetStatusText(*args)


class SbException(Exception):
    """Exception class used in SbMapDisplay and SbItems"""

    def __init__(self, message):
        self.message = message

    def __str__(self):
        return self.message


class SbItem:
    """Base class for statusbar items."""

    def __init__(self, statusbar):
        self.statusbar = statusbar

    def Show(self):
        """Invokes showing of underlying item.

        In derived classes it can do what is appropriate for it,
        e.g. showing text on statusbar (only).
        """
        self.item.Show()

    def Hide(self):
        self.item.Hide()

    def SetValue(self, value):
        self.item.SetValue(value)

    def GetValue(self):
        return self.item.GetValue()

    def GetItem(self):
        """Returns underlying widget.

        :return: widget or None if doesn't exist
        """
        return self.item

    def _update(self):
        """Default implementation for Update method.

        :param longHelp: True to enable long help (help from toolbars)
        """
        self.statusbar.SetStatusText("", 0)
        self.Show()


class SbTextItem(SbItem):
    """Base class for items without widgets.

    Only sets statusbar text.
    """

    def __init__(self, statusbar):
        SbItem.__init__(self, statusbar)

        self.text = None

    def Show(self):
        self.statusbar.SetStatusText(self.GetValue())

    def Hide(self):
        self.statusbar.SetStatusText("")

    def SetValue(self, value):
        self.text = value

    def GetValue(self):
        return self.text

    def GetWidget(self):
        return None


class SbCoordinates(SbTextItem):
    """Show map coordinates when mouse moves.

    Requires MapWindow.GetLastEN method."""

    def __init__(self, statusbar, giface, mapDisplayProperties):
        SbTextItem.__init__(self, statusbar)
        self.name = "coordinates"
        self.label = _("Coordinates")
        self._properties = mapDisplayProperties
        self._additionalInfo = None
        self._basicValue = None

    def Show(self):
        """Show the last map window coordinates.

        .. todo::
            remove last EN call and use coordinates coming from signal
        """
        try:
            e, n = self.giface.GetMapWindow().GetLastEN()
            self._basicValue = self.ReprojectENFromMap(e, n)
            if self._additionalInfo:
                value = _("{coords} ({additionalInfo})").format(
                    coords=self._basicValue, additionalInfo=self._additionalInfo
                )
            else:
                value = self._basicValue
            self.SetValue(value)
        except SbException as e:
            self.SetValue(e.message)
        except TypeError:
            self.SetValue("")
        except AttributeError:
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

    def ReprojectENFromMap(self, e, n):
        """Reproject east, north to user defined projection.

        :param e,n: coordinate

        @throws SbException if useDefinedProjection is True and projection is not defined in UserSettings
        """
        useDefinedProjection = self._properties.useDefinedProjection
        projectionSettings = self._properties.projectionSettings

        if useDefinedProjection:
            if projectionSettings["proj4"]:
                raise SbException(_("Projection not defined (check the settings)"))
            else:
                # reproject values
                proj, coord = utils.ReprojectCoordinates(
                    coord=(e, n), projOut=projectionSettings["proj4"], flags="d"
                )
                if coord:
                    e, n = coord
                    if proj in ("ll", "latlong", "longlat") and projectionSettings["format"] == "DMS":
                        return utils.Deg2DMS(e, n, precision=projectionSettings["precision"])
                    else:
                        return "%.*f; %.*f" % (projectionSettings["precision"], e, projectionSettings["precision"], n)
                else:
                    raise SbException(_("Error in projection (check the settings)"))
        else:
            if self.giface.GetMapDisplay().GetMap().projinfo["proj"] == "ll" and projectionSettings["format"] == "DMS":
                return utils.Deg2DMS(e, n, precision=projectionSettings["precision"])
            else:
                return "%.*f; %.*f" % (projectionSettings["precision"], e, projectionSettings["precision"], n)


#class SbProgress(SbItem):
#    """General progress bar to show progress.
#
#    Underlaying widget is wx.Gauge.
#    """
#
#    def __init__(self, mapframe, statusbar, sbManager, position=0):
#        self.progressShown = Signal("SbProgress.progressShown")
#        self.progressHidden = Signal("SbProgress.progressHidden")
#        SbItem.__init__(self, mapframe, statusbar, position)
#        self.name = "progress"
#        self.sbManager = sbManager
#        # on-render gauge
#        self.widget = wx.Gauge(
#            parent=self.statusbar, id=wx.ID_ANY, range=0, style=wx.GA_HORIZONTAL
#        )
#        self.Hide()
#
#        self.progressShown.connect(self._progressShown)
#        self.progressHidden.connect(self._progressHidden)
#
#    def GetRange(self):
#        """Returns progress range."""
#        return self.widget.GetRange()
#
#    def SetRange(self, range):
#        """Sets progress range."""
#        if range > 0:
#            if self.GetRange() != range:
#                self.widget.SetRange(range)
#            self.Show()
#        else:
#            self.Hide()
#
#    def Show(self):
#        if not self.IsShown():
#            self.progressShown.emit()
#            self.widget.Show()
#
#    def Hide(self):
#        if self.IsShown():
#            self.progressHidden.emit()
#            self.widget.Hide()
#
#    def IsShown(self):
#        """Is progress bar shown"""
#        return self.widget.IsShown()
#
#    def SetValue(self, value):
#        """Sets value of progressbar."""
#        if value > self.GetRange():
#            self.Hide()
#            return
#
#        self.widget.SetValue(value)
#        if value == self.GetRange():
#            self.Hide()
#
#    def GetWidget(self):
#        """Returns underlaying winget.
#
#        :return: widget or None if doesn't exist
#        """
#        return self.widget
#
#    def Update(self):
#        pass
#
#    def GetProgressBar(self):
#        """Returns progress bar"""
#        return self.progressbar
#
#    def _progressShown(self):
#        self._oldStatus = self.statusbar.GetStatusText(0)
#        self.choice.GetClientData(self.choice.GetSelection()).Hide()
#
#    def _progressHidden(self):
#        self.statusbar.SetStatusText(self._oldStatus, 0)
#        self.choice.GetClientData(self.choice.GetSelection()).Show()
#
#    def SetProgress(self, range, value, text):
#        """Update progress."""
#        self.progressbar.SetRange(range)
#        self.progressbar.SetValue(value)
#        if text:
#            self.statusbar.SetStatusText(text)
#




#class SbMapScale(SbItem):
#    """Editable combobox to get/set current map scale."""
#
#    def __init__(self, parent, mapWindowProperties, giface):
#        SbItem.__init__(self, mapWindowProperties)
#        self.name = "mapscale"
#        self.widget = wx.ComboBox(
#            parent=parent,
#            id=wx.ID_ANY,
#            label = _("Map scale"),
#            style=wx.TE_PROCESS_ENTER,
#            size=(150, -1),
#        )
#        self.widget.SetItems(
#            [
#                "1:1000",
#                "1:5000",
#                "1:10000",
#                "1:25000",
#                "1:50000",
#                "1:100000",
#                "1:1000000",
#            ]
#        )
#        self.widget.SetToolTip(
#            wx.ToolTip(
#                _(
#                    "As everyone's monitors and resolutions "
#                    "are set differently these values are not "
#                    "true map scales, but should get you into "
#                    "the right neighborhood."
#                )
#            )
#        )
#
#        self.widget.Bind(wx.EVT_TEXT_ENTER, self.OnChangeMapScale)
#        self.widget.Bind(wx.EVT_COMBOBOX, self.OnChangeMapScale)
#
#        self.lastMapScale = None
#
#    def Update(self):
#        scale = self._properties.mapScale
#        self.parent.SetStatusText("")
#        try:
#            self.SetValue("1:%ld" % (scale + 0.5))
#        except TypeError:
#            pass  # FIXME, why this should happen?
#
#        self.lastMapScale = scale
#
#
#    def OnChangeMapScale(self, event):
#        """Map scale changed by user"""
#        scale = event.GetString()
#
#        try:
#            if scale[:2] != "1:":
#                raise ValueError
#            value = int(scale[2:])
#        except ValueError:
#            self.SetValue("1:%ld" % int(self._properties.mapScale))
#            return
#
#        self._properties.mapScale = value
#
#        # redraw map if auto-rendering is enabled
#        if self._properties.autoRender:
#            self.giface.updateMap.emit()


#class SbGoTo(SbItem):
#    """Textctrl to set coordinates which to focus on.
#
#    Requires MapFrame.GetWindow, MapWindow.GoTo method.
#    """
#
#    def __init__(self, mapframe, statusbar, position=0):
#        SbItem.__init__(self, mapframe, statusbar, position)
#        self.name = "goto"
#        self.label = _("Go to")
#
#        self.widget = TextCtrl(
#            parent=self.statusbar,
#            id=wx.ID_ANY,
#            value="",
#            style=wx.TE_PROCESS_ENTER,
#            size=(300, -1),
#        )
#
#        self.widget.Hide()
#
#        self.widget.Bind(wx.EVT_TEXT_ENTER, self.OnGoTo)
#
#    def ReprojectENToMap(self, e, n, useDefinedProjection):
#        """Reproject east, north from user defined projection
#
#        :param e,n: coordinate (for DMS string, else float or string)
#        :param useDefinedProjection: projection defined by user in settings dialog
#
#        @throws SbException if useDefinedProjection is True and projection is not defined in UserSettings
#        """
#        if useDefinedProjection:
#            settings = UserSettings.Get(
#                group="projection", key="statusbar", subkey="proj4"
#            )
#            if not settings:
#                raise SbException(_("Projection not defined (check the settings)"))
#            else:
#                # reproject values
#                projIn = settings
#                projOut = RunCommand("g.proj", flags="jf", read=True)
#                proj = projIn.split(" ")[0].split("=")[1]
#                if proj in ("ll", "latlong", "longlat"):
#                    e, n = utils.DMS2Deg(e, n)
#                    proj, coord1 = utils.ReprojectCoordinates(
#                        coord=(e, n), projIn=projIn, projOut=projOut, flags="d"
#                    )
#                    e, n = coord1
#                else:
#                    e, n = float(e), float(n)
#                    proj, coord1 = utils.ReprojectCoordinates(
#                        coord=(e, n), projIn=projIn, projOut=projOut, flags="d"
#                    )
#                    e, n = coord1
#        elif self.mapFrame.GetMap().projinfo["proj"] == "ll":
#            e, n = utils.DMS2Deg(e, n)
#        else:
#            e, n = float(e), float(n)
#        return e, n
#
#    def OnGoTo(self, event):
#        """Go to position"""
#        try:
#            e, n = self.GetValue().split(";")
#            e, n = self.ReprojectENToMap(e, n, self.mapFrame.GetProperty("projection"))
#            self.mapFrame.GetWindow().GoTo(e, n)
#            self.widget.SetFocus()
#        except ValueError:
#            # FIXME: move this code to MapWindow/BufferedWindow/MapFrame
#            region = self.mapFrame.GetMap().GetCurrentRegion()
#            precision = int(
#                UserSettings.Get(group="projection", key="format", subkey="precision")
#            )
#            format = UserSettings.Get(group="projection", key="format", subkey="ll")
#            if self.mapFrame.GetMap().projinfo["proj"] == "ll" and format == "DMS":
#                self.SetValue(
#                    "%s"
#                    % utils.Deg2DMS(
#                        region["center_easting"],
#                        region["center_northing"],
#                        precision=precision,
#                    )
#                )
#            else:
#                self.SetValue(
#                    "%.*f; %.*f"
#                    % (
#                        precision,
#                        region["center_easting"],
#                        precision,
#                        region["center_northing"],
#                    )
#                )
#        except SbException as e:
#            # FIXME: this may be useless since statusbar update checks user
#            # defined projection and this exception raises when user def proj
#            # does not exists
#            self.statusbar.SetStatusText(str(e), 0)
#
#    def GetCenterString(self, map):
#        """Get current map center in appropriate format"""
#        region = map.GetCurrentRegion()
#        precision = int(
#            UserSettings.Get(group="projection", key="format", subkey="precision")
#        )
#        format = UserSettings.Get(group="projection", key="format", subkey="ll")
#        projection = UserSettings.Get(
#            group="projection", key="statusbar", subkey="proj4"
#        )
#
#        if self.mapFrame.GetProperty("projection"):
#            if not projection:
#                raise SbException(_("Projection not defined (check the settings)"))
#            else:
#                proj, coord = utils.ReprojectCoordinates(
#                    coord=(region["center_easting"], region["center_northing"]),
#                    projOut=projection,
#                    flags="d",
#                )
#                if coord:
#                    if proj in ("ll", "latlong", "longlat") and format == "DMS":
#                        return "%s" % utils.Deg2DMS(
#                            coord[0], coord[1], precision=precision
#                        )
#                    else:
#                        return "%.*f; %.*f" % (precision, coord[0], precision, coord[1])
#                else:
#                    raise SbException(_("Error in projection (check the settings)"))
#        else:
#            if self.mapFrame.GetMap().projinfo["proj"] == "ll" and format == "DMS":
#                return "%s" % utils.Deg2DMS(
#                    region["center_easting"],
#                    region["center_northing"],
#                    precision=precision,
#                )
#            else:
#                return "%.*f; %.*f" % (
#                    precision,
#                    region["center_easting"],
#                    precision,
#                    region["center_northing"],
#                )
#
#    def SetCenter(self):
#        """Set current map center as item value"""
#        center = self.GetCenterString(self.mapFrame.GetMap())
#        self.SetValue(center)
#
#    def Update(self):
#        self.statusbar.SetStatusText("")
#
#        try:
#            self.SetCenter()
#            self.Show()
#        except SbException as e:
#            self.statusbar.SetStatusText(str(e), 0)
#
#        # disable long help
#        self.mapFrame.StatusbarEnableLongHelp(False)

#
#
#
#class SbDisplayGeometry(SbTextItem):
#    """Show current display resolution."""
#
#    def __init__(self, mapframe, statusbar, position=0):
#        SbTextItem.__init__(self, mapframe, statusbar, position)
#        self.name = "displayGeometry"
#        self.label = _("Display geometry")
#
#    def Show(self):
#        region = copy.copy(self.mapFrame.GetMap().GetCurrentRegion())
#        if self.mapFrame.mapWindowProperties.resolution:
#            compRegion = self.mapFrame.GetMap().GetRegion(add3d=False)
#            region["rows"] = abs(
#                int((region["n"] - region["s"]) / compRegion["nsres"]) + 0.5
#            )
#            region["cols"] = abs(
#                int((region["e"] - region["w"]) / compRegion["ewres"]) + 0.5
#            )
#            region["nsres"] = compRegion["nsres"]
#            region["ewres"] = compRegion["ewres"]
#        self.SetValue(
#            "rows=%d; cols=%d; nsres=%.2f; ewres=%.2f"
#            % (region["rows"], region["cols"], region["nsres"], region["ewres"])
#        )
#        SbTextItem.Show(self)




#class SbRegionExtent(SbTextItem):
#    """Shows current display region"""
#
#    def __init__(self, mapframe, statusbar, position=0):
#        SbTextItem.__init__(self, mapframe, statusbar, position)
#        self.name = "displayRegion"
#        self.label = _("Extent")
#
#    def Show(self):
#        precision = int(
#            UserSettings.Get(group="projection", key="format", subkey="precision")
#        )
#        format = UserSettings.Get(group="projection", key="format", subkey="ll")
#        projection = self.mapFrame.GetProperty("projection")
#        region = self._getRegion()
#        try:
#            regionReprojected = self.ReprojectRegionFromMap(
#                region, projection, precision, format
#            )
#            self.SetValue(regionReprojected)
#        except SbException as e:
#            self.SetValue(e.message)
#        SbTextItem.Show(self)
#
#    def _getRegion(self):
#        """Get current display region"""
#        return self.mapFrame.GetMap().GetCurrentRegion()  # display region
#
#    def _formatRegion(self, w, e, s, n, nsres, ewres, precision=None):
#        """Format display region string for statusbar
#
#        :param nsres,ewres: unused
#        """
#        if precision is not None:
#            return "%.*f - %.*f, %.*f - %.*f" % (
#                precision,
#                w,
#                precision,
#                e,
#                precision,
#                s,
#                precision,
#                n,
#            )
#        else:
#            return "%s - %s, %s - %s" % (w, e, s, n)
#
#    def ReprojectRegionFromMap(self, region, useDefinedProjection, precision, format):
#        """Reproject region values
#
#        .. todo::
#            reorganize this method to remove code useful only for derived class SbCompRegionExtent
#        """
#        if useDefinedProjection:
#            settings = UserSettings.Get(
#                group="projection", key="statusbar", subkey="proj4"
#            )
#
#            if not settings:
#                raise SbException(_("Projection not defined (check the settings)"))
#            else:
#                projOut = settings
#                proj, coord1 = utils.ReprojectCoordinates(
#                    coord=(region["w"], region["s"]), projOut=projOut, flags="d"
#                )
#                proj, coord2 = utils.ReprojectCoordinates(
#                    coord=(region["e"], region["n"]), projOut=projOut, flags="d"
#                )
#                # useless, used in derived class
#                proj, coord3 = utils.ReprojectCoordinates(
#                    coord=(0.0, 0.0), projOut=projOut, flags="d"
#                )
#                proj, coord4 = utils.ReprojectCoordinates(
#                    coord=(region["ewres"], region["nsres"]), projOut=projOut, flags="d"
#                )
#                if coord1 and coord2:
#                    if proj in ("ll", "latlong", "longlat") and format == "DMS":
#                        w, s = utils.Deg2DMS(
#                            coord1[0], coord1[1], string=False, precision=precision
#                        )
#                        e, n = utils.Deg2DMS(
#                            coord2[0], coord2[1], string=False, precision=precision
#                        )
#                        ewres, nsres = utils.Deg2DMS(
#                            abs(coord3[0]) - abs(coord4[0]),
#                            abs(coord3[1]) - abs(coord4[1]),
#                            string=False,
#                            hemisphere=False,
#                            precision=precision,
#                        )
#                        return self._formatRegion(
#                            w=w, s=s, e=e, n=n, ewres=ewres, nsres=nsres
#                        )
#                    else:
#                        w, s = coord1
#                        e, n = coord2
#                        ewres, nsres = coord3
#                        return self._formatRegion(
#                            w=w,
#                            s=s,
#                            e=e,
#                            n=n,
#                            ewres=ewres,
#                            nsres=nsres,
#                            precision=precision,
#                        )
#                else:
#                    raise SbException(_("Error in projection (check the settings)"))
#
#        else:
#            if self.mapFrame.GetMap().projinfo["proj"] == "ll" and format == "DMS":
#                w, s = utils.Deg2DMS(
#                    region["w"], region["s"], string=False, precision=precision
#                )
#                e, n = utils.Deg2DMS(
#                    region["e"], region["n"], string=False, precision=precision
#                )
#                ewres, nsres = utils.Deg2DMS(
#                    region["ewres"], region["nsres"], string=False, precision=precision
#                )
#                return self._formatRegion(w=w, s=s, e=e, n=n, ewres=ewres, nsres=nsres)
#            else:
#                w, s = region["w"], region["s"]
#                e, n = region["e"], region["n"]
#                ewres, nsres = region["ewres"], region["nsres"]
#                return self._formatRegion(
#                    w=w, s=s, e=e, n=n, ewres=ewres, nsres=nsres, precision=precision
#                )
#
#
#class SbCompRegionExtent(SbRegionExtent):
#    """Shows computational region."""
#
#    def __init__(self, mapframe, statusbar, position=0):
#        SbRegionExtent.__init__(self, mapframe, statusbar, position)
#        self.name = "computationalRegion"
#        self.label = _("Computational region")
#
#    def _formatRegion(self, w, e, s, n, ewres, nsres, precision=None):
#        """Format computational region string for statusbar"""
#        if precision is not None:
#            return "%.*f - %.*f, %.*f - %.*f (%.*f, %.*f)" % (
#                precision,
#                w,
#                precision,
#                e,
#                precision,
#                s,
#                precision,
#                n,
#                precision,
#                ewres,
#                precision,
#                nsres,
#            )
#        else:
#            return "%s - %s, %s - %s (%s, %s)" % (w, e, s, n, ewres, nsres)
#
#    def _getRegion(self):
#        """Returns computational region."""
#        return self.mapFrame.GetMap().GetRegion()  # computational region


