"""
@package mapwin.decorations

@brief Map display decorations (overlays) - text, barscale and legend

Classes:
 - decorations::OverlayController
 - decorations::BarscaleController
 - decorations::ArrowController
 - decorations::LegendController
 - decorations::TextLayerDialog

(C) 2006-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os

import wx

from grass.pydispatch.signal import Signal
try:
    from PIL import Image
    hasPIL = True
except ImportError:
    hasPIL = False
from gui_core.wrap import NewId


class OverlayController(object):

    """Base class for decorations (barscale, legend) controller."""

    def __init__(self, renderer, giface):
        self._giface = giface
        self._renderer = renderer
        self._overlay = None
        self._coords = None
        self._pdcType = 'image'
        self._propwin = None
        self._defaultAt = ''
        self._cmd = None   # to be set by user
        self._name = None  # to be defined by subclass
        self._removeLabel = None  # to be defined by subclass
        self._id = NewId()
        self._dialog = None

        # signals that overlay or its visibility changed
        self.overlayChanged = Signal('OverlayController::overlayChanged')

    def SetCmd(self, cmd):
        hasAt = False
        for i in cmd:
            if i.startswith("at="):
                hasAt = True
                # reset coordinates, 'at' values will be used, see GetCoords
                self._coords = None
                break
        if not hasAt:
            cmd.append(self._defaultAt)
        self._cmd = cmd

    def GetCmd(self):
        return self._cmd

    cmd = property(fset=SetCmd, fget=GetCmd)

    def SetCoords(self, coords):
        self._coords = list(coords)

    def GetCoords(self):
        if self._coords is None:  # initial position
            x, y = self.GetPlacement(
                (self._renderer.width, self._renderer.height))
            self._coords = [x, y]
        return self._coords

    coords = property(fset=SetCoords, fget=GetCoords)

    def GetPdcType(self):
        return self._pdcType

    pdcType = property(fget=GetPdcType)

    def GetName(self):
        return self._name

    name = property(fget=GetName)

    def GetRemoveLabel(self):
        return self._removeLabel

    removeLabel = property(fget=GetRemoveLabel)

    def GetId(self):
        return self._id

    id = property(fget=GetId)

    def GetPropwin(self):
        return self._propwin

    def SetPropwin(self, win):
        self._propwin = win

    propwin = property(fget=GetPropwin, fset=SetPropwin)

    def GetLayer(self):
        return self._overlay

    layer = property(fget=GetLayer)

    def GetDialog(self):
        return self._dialog

    def SetDialog(self, win):
        self._dialog = win

    dialog = property(fget=GetDialog, fset=SetDialog)

    def IsShown(self):
        if self._overlay and self._overlay.IsActive() and self._overlay.IsRendered():
            return True
        return False

    def Show(self, show=True):
        """Activate or deactivate overlay."""
        if show:
            if not self._overlay:
                self._add()
            self._overlay.SetActive(True)
            self._update()
        else:
            self.Hide()

        self.overlayChanged.emit()

    def Hide(self):
        if self._overlay:
            self._overlay.SetActive(False)
        self.overlayChanged.emit()

    def Remove(self):
        if self._dialog:
            self._dialog.Destroy()
        self._renderer.DeleteOverlay(self._overlay)
        self.overlayChanged.emit()

    def _add(self):
        self._overlay = self._renderer.AddOverlay(
            id=self._id,
            ltype=self._name,
            command=self.cmd,
            active=False,
            render=True,
            hidden=True)
        # check if successful

    def _update(self):
        self._renderer.ChangeOverlay(id=self._id, command=self._cmd)

    def CmdIsValid(self):
        """If command is valid"""
        return True

    def GetPlacement(self, screensize):
        """Get coordinates where to place overlay in a reasonable way

        :param screensize: screen size
        """
        if not hasPIL:
            self._giface.WriteWarning(
                _(
                    "Please install Python Imaging Library (PIL)\n"
                    "for better control of legend and other decorations."))
            return 0, 0

        for param in self._cmd:
            if not param.startswith('at'):
                continue
            x, y = [float(number) for number in param.split('=')[1].split(',')]
            x = int((x / 100.) * screensize[0])
            y = int((1 - y / 100.) * screensize[1])

            return x, y


class DtextController(OverlayController):

    def __init__(self, renderer, giface):
        OverlayController.__init__(self, renderer, giface)
        self._name = 'text'
        self._removeLabel = _("Remove text")
        self._defaultAt = 'at=50,50'
        self._cmd = ['d.text', self._defaultAt]

    def CmdIsValid(self):
        inputs = 0
        for param in self._cmd[1:]:
            param = param.split('=')
            if len(param) == 1:
                inputs += 1
            else:
                if param[0] == 'text' and len(param) == 2:
                    inputs += 1
        if inputs >= 1:
            return True
        return False


class BarscaleController(OverlayController):

    def __init__(self, renderer, giface):
        OverlayController.__init__(self, renderer, giface)
        self._name = 'barscale'
        self._removeLabel = _("Remove scale bar")
        # different from default because the reference point is not in the
        # middle
        self._defaultAt = 'at=0,98'
        self._cmd = ['d.barscale', self._defaultAt]


class ArrowController(OverlayController):

    def __init__(self, renderer, giface):
        OverlayController.__init__(self, renderer, giface)
        self._name = 'arrow'
        self._removeLabel = _("Remove north arrow")
        # different from default because the reference point is not in the
        # middle
        self._defaultAt = 'at=85.0,25.0'
        self._cmd = ['d.northarrow', self._defaultAt]


class LegendVectController(OverlayController):

    def __init__(self, renderer, giface):
        OverlayController.__init__(self, renderer, giface)
        self._name = 'vectleg'
        self._removeLabel = _("Remove vector legend")
        # different from default because the reference point is not in the
        # middle
        self._defaultAt = 'at=20.0,80.0'
        self._cmd = ['d.legend.vect', self._defaultAt]


class LegendController(OverlayController):

    def __init__(self, renderer, giface):
        OverlayController.__init__(self, renderer, giface)
        self._name = 'legend'
        self._removeLabel = _("Remove legend")
        # default is in the center to avoid trimmed legend on the edge
        self._defaultAt = 'at=5,50,47,50'
        self._cmd = ['d.legend', self._defaultAt]

    def GetPlacement(self, screensize):
        if not hasPIL:
            self._giface.WriteWarning(
                _(
                    "Please install Python Imaging Library (PIL)\n"
                    "for better control of legend and other decorations."))
            return 0, 0
        for param in self._cmd:
            if not param.startswith('at'):
                continue
            # if the at= is the default, we will move the legend from the center to bottom left
            if param == self._defaultAt:
                b, t, l, r = 5, 50, 7, 10
            else:
                b, t, l, r = [float(number) for number in param.split(
                    '=')[1].split(',')]  # pylint: disable-msg=W0612
            x = int((l / 100.) * screensize[0])
            y = int((1 - t / 100.) * screensize[1])

            return x, y

    def CmdIsValid(self):
        inputs = 0
        for param in self._cmd[1:]:
            param = param.split('=')
            if len(param) == 1:
                inputs += 1
            else:
                if param[0] == 'raster' and len(param) == 2:
                    inputs += 1
                elif param[0] == 'raster_3d' and len(param) == 2:
                    inputs += 1
        if inputs == 1:
            return True
        return False

    def ResizeLegend(self, begin, end, screenSize):
        """Resize legend according to given bbox coordinates."""
        w = abs(begin[0] - end[0])
        h = abs(begin[1] - end[1])
        if begin[0] < end[0]:
            x = begin[0]
        else:
            x = end[0]
        if begin[1] < end[1]:
            y = begin[1]
        else:
            y = end[1]

        at = [(screenSize[1] - (y + h)) / float(screenSize[1]) * 100,
              (screenSize[1] - y) / float(screenSize[1]) * 100,
              x / float(screenSize[0]) * 100,
              (x + w) / float(screenSize[0]) * 100]
        atStr = "at=%d,%d,%d,%d" % (at[0], at[1], at[2], at[3])

        for i, subcmd in enumerate(self._cmd):
            if subcmd.startswith('at='):
                self._cmd[i] = atStr
                break

        self._coords = None
        self.Show()

    def StartResizing(self):
        """Tool in toolbar or button itself were pressed"""
        # prepare for resizing
        window = self._giface.GetMapWindow()
        window.SetNamedCursor('cross')
        window.mouse['use'] = None
        window.mouse['box'] = 'box'
        window.pen = wx.Pen(colour='Black', width=2, style=wx.SHORT_DASH)
        window.mouseLeftUp.connect(self._finishResizing)

    def _finishResizing(self):
        window = self._giface.GetMapWindow()
        window.mouseLeftUp.disconnect(self._finishResizing)
        screenSize = window.GetClientSize()
        self.ResizeLegend(
            window.mouse["begin"],
            window.mouse["end"],
            screenSize)
        self._giface.GetMapDisplay().GetMapToolbar().SelectDefault()
        # redraw
        self.overlayChanged.emit()
