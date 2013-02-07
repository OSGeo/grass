"""!
@package mapdisp.overlays

@brief Map display overlays - barscale and legend

Classes:
 - overlays::OverlayController
 - overlays::BarscaleController
 - overlays::LegendController

(C) 2006-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

class OverlayController(object):
    """!Base class for decorations (barscale, legend) controller."""
    def __init__(self, renderer):
        self._renderer = renderer
        self._overlay = None
        self._coords = [0, 0]
        self._pdcType = 'image'
        self._propwin = None
        self._defaultAt = ''
        self._cmd = None  # to be set by user
        self._name = None  # to be defined by subclass
        self._id = None  # to be defined by subclass

    def SetCmd(self, cmd):
        hasAt = False
        for i in cmd:
            if i.startswith("at="):
                hasAt = True
                break
        if not hasAt:
            cmd.append(self._defaultAt)
        self._cmd = cmd

    def GetCmd(self):
        return self._cmd

    cmd = property(fset = SetCmd, fget = GetCmd)

    def SetCoords(self, coords):
        self._coords = list(coords)

    def GetCoords(self):
        return self._coords

    coords = property(fset = SetCoords, fget = GetCoords)

    def GetPdcType(self):
        return self._pdcType

    pdcType = property(fget = GetPdcType)

    def GetName(self):
        return self._name

    name = property(fget = GetName)

    def GetId(self):
        return self._id

    id = property(fget = GetId)

    def GetPropwin(self):
        return self._propwin

    def SetPropwin(self, win):
        self._propwin = win

    propwin = property(fget = GetPropwin, fset = SetPropwin)

    def GetLayer(self):
        return self._overlay

    layer = property(fget = GetLayer)

    def IsShown(self):
        if self._overlay and self._overlay.IsActive():
            return True
        return False

    def Show(self, show = True):
        """!Activate or deactivate overlay."""
        if show:
            if not self._overlay:
                self._add()
            self._overlay.SetActive(True)
            self._update()
        else:
            self.Hide()

    def Hide(self):
        if self._overlay:
            self._overlay.SetActive(False)

    def _add(self):
        self._overlay = self._renderer.AddOverlay(id = self._id, ltype = self._name,
                                                  command = self.cmd, active = False,
                                                  render = False, hidden = True)
        # check if successfull

    def _update(self):
        self._renderer.ChangeOverlay(id = self._id, command = self._cmd,
                                     render = False)


class BarscaleController(OverlayController):
    def __init__(self, renderer):
        OverlayController.__init__(self, renderer)
        self._id = 0
        self._name = 'barscale'
        self._defaultAt = 'at=0,95'
        self._cmd = ['d.barscale', self._defaultAt]


class LegendController(OverlayController):
    def __init__(self, renderer):
        OverlayController.__init__(self, renderer)
        self._id = 1
        self._name = 'legend'
        # TODO: synchronize with d.legend?
        self._defaultAt = 'at=5,50,2,5'
        self._cmd = ['d.legend', self._defaultAt]

    def ResizeLegend(self, begin, end, screenSize):
        """!Resize legend according to given bbox coordinates."""
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

        self._coords = [0, 0]
        self.Show()
