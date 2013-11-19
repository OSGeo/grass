"""!
@package mapwin.decorations

@brief Map display decorations (overlays) - text, barscale and legend

Classes:
 - decorations::OverlayController
 - decorations::BarscaleController
 - decorations::ArrowController
 - decorations::LegendController
 - decorations::TextLayerDialog

(C) 2006-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx
from core.utils import _

from grass.pydispatch.signal import Signal
try:
    from PIL import Image
    hasPIL = True
except ImportError:
    hasPIL = False


class OverlayController(object):

    """!Base class for decorations (barscale, legend) controller."""

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
        self._id = None    # to be defined by subclass
        self._dialog = None

        # signals that overlay or its visibility changed
        self.overlayChanged = Signal('OverlayController::overlayChanged')

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

    cmd = property(fset=SetCmd, fget=GetCmd)

    def SetCoords(self, coords):
        self._coords = list(coords)

    def GetCoords(self):
        if self._coords is None:  # initial position
            x, y = self.GetPlacement((self._renderer.width, self._renderer.height))
            self._coords = [x, y]
        return self._coords

    coords = property(fset=SetCoords, fget=GetCoords)

    def GetPdcType(self):
        return self._pdcType

    pdcType = property(fget=GetPdcType)

    def GetName(self):
        return self._name

    name = property(fget=GetName)

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
        if self._overlay and self._overlay.IsActive():
            return True
        return False

    def Show(self, show=True):
        """!Activate or deactivate overlay."""
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

    def GetOptData(self, dcmd, layer, params, propwin):
        """!Called after options are set through module dialog.

        @param dcmd resulting command
        @param layer not used
        @param params module parameters (not used)
        @param propwin dialog window
        """
        if not dcmd:
            return

        self._cmd = dcmd
        self._dialog = propwin

        self.Show()

    def _add(self):
        self._overlay = self._renderer.AddOverlay(id=self._id, ltype=self._name,
                                                  command=self.cmd, active=False,
                                                  render=False, hidden=True)
        # check if successful

    def _update(self):
        self._renderer.ChangeOverlay(id=self._id, command=self._cmd,
                                     render=False)

    def CmdIsValid(self):
        """!If command is valid"""
        return True

    def GetPlacement(self, screensize):
        """!Get coordinates where to place overlay in a reasonable way

        @param screensize sreen size
        """
        if not hasPIL:
            self._giface.WriteWarning(_("Please install Python Imaging Library (PIL)\n"
                                        "for better control of legend and other decorations."))
            return 0, 0
        for param in self._cmd:
            if not param.startswith('at'):
                continue
            x, y = [float(number) for number in param.split('=')[1].split(',')]
            x = int((x / 100.) * screensize[0])
            y = int((1 - y / 100.) * screensize[1])

            return x, y


class BarscaleController(OverlayController):

    def __init__(self, renderer, giface):
        OverlayController.__init__(self, renderer, giface)
        self._id = 1
        self._name = 'barscale'
        # different from default because the reference point is not in the middle
        self._defaultAt = 'at=0,98'
        self._cmd = ['d.barscale', self._defaultAt]


class ArrowController(OverlayController):

    def __init__(self, renderer, giface):
        OverlayController.__init__(self, renderer, giface)
        self._id = 2
        self._name = 'arrow'
        # different from default because the reference point is not in the middle
        self._defaultAt = 'at=85.0,25.0'
        self._cmd = ['d.northarrow', self._defaultAt]


class LegendController(OverlayController):

    def __init__(self, renderer, giface):
        OverlayController.__init__(self, renderer, giface)
        self._id = 0
        self._name = 'legend'
        # TODO: synchronize with d.legend?
        self._defaultAt = 'at=5,50,2,5'
        self._cmd = ['d.legend', self._defaultAt]

    def GetPlacement(self, screensize):
        if not hasPIL:
            self._giface.WriteWarning(_("Please install Python Imaging Library (PIL)\n"
                                        "for better control of legend and other decorations."))
            return 0, 0
        for param in self._cmd:
            if not param.startswith('at'):
                continue
            b, t, l, r = [float(number) for number in param.split('=')[1].split(',')]  # pylint: disable-msg=W0612
            x = int((l / 100.) * screensize[0])
            y = int((1 - t / 100.) * screensize[1])

            return x, y

    def CmdIsValid(self):
        for param in self._cmd:
            param = param.split('=')
            if param[0] == 'map' and len(param) == 2:
                return True
        return False

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

        self._coords = None
        self.Show()

    def StartResizing(self):
        """!Tool in toolbar or button itself were pressed"""
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
        screenSize = window.GetClientSizeTuple()
        self.ResizeLegend(window.mouse["begin"], window.mouse["end"], screenSize)
        self._giface.GetMapDisplay().GetMapToolbar().SelectDefault()
        # redraw
        self.overlayChanged.emit()


class TextLayerDialog(wx.Dialog):

    """
    Controls setting options and displaying/hiding map overlay decorations
    """

    def __init__(self, parent, ovlId, title, name='text',
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE):

        wx.Dialog.__init__(self, parent, wx.ID_ANY, title, pos, size, style)
        from wx.lib.expando import ExpandoTextCtrl, EVT_ETC_LAYOUT_NEEDED

        self.ovlId = ovlId
        self.parent = parent

        if self.ovlId in self.parent.MapWindow.textdict.keys():
            self.currText = self.parent.MapWindow.textdict[self.ovlId]['text']
            self.currFont = self.parent.MapWindow.textdict[self.ovlId]['font']
            self.currClr = self.parent.MapWindow.textdict[self.ovlId]['color']
            self.currRot = self.parent.MapWindow.textdict[self.ovlId]['rotation']
            self.currCoords = self.parent.MapWindow.textdict[self.ovlId]['coords']
            self.currBB = self.parent.MapWindow.textdict[self.ovlId]['bbox']
        else:
            self.currClr = wx.BLACK
            self.currText = ''
            self.currFont = self.GetFont()
            self.currRot = 0.0
            self.currCoords = [10, 10]
            self.currBB = wx.Rect()

        self.sizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.GridBagSizer(vgap=5, hgap=5)

        # show/hide
        self.chkbox = wx.CheckBox(parent=self, id=wx.ID_ANY,
                                  label=_('Show text object'))
        if self.parent.Map.GetOverlay(self.ovlId) is None:
            self.chkbox.SetValue(True)
        else:
            self.chkbox.SetValue(self.parent.MapWindow.overlays[self.ovlId]['layer'].IsActive())
        box.Add(item=self.chkbox, span=(1, 2),
                flag=wx.ALIGN_LEFT | wx.ALL, border=5,
                pos=(0, 0))

        # text entry
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Enter text:"))
        box.Add(item=label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(1, 0))

        self.textentry = ExpandoTextCtrl(
            parent=self, id=wx.ID_ANY, value="", size=(300, -1))
        self.textentry.SetFont(self.currFont)
        self.textentry.SetForegroundColour(self.currClr)
        self.textentry.SetValue(self.currText)
        # get rid of unneeded scrollbar when text box first opened
        self.textentry.SetClientSize((300, -1))

        box.Add(item=self.textentry,
                pos=(1, 1))

        # rotation
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Rotation:"))
        box.Add(item=label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(2, 0))
        self.rotation = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="", pos=(30, 50),
                                    size=(75, -1), style=wx.SP_ARROW_KEYS)
        self.rotation.SetRange(-360, 360)
        self.rotation.SetValue(int(self.currRot))
        box.Add(item=self.rotation,
                flag=wx.ALIGN_RIGHT,
                pos=(2, 1))

        # font
        fontbtn = wx.Button(parent=self, id=wx.ID_ANY, label=_("Set font"))
        box.Add(item=fontbtn,
                flag=wx.ALIGN_RIGHT,
                pos=(3, 1))

        self.sizer.Add(item=box, proportion=1,
                       flag=wx.ALL, border=10)

        # note
        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label=_("Drag text with mouse in pointer mode "
                                      "to position.\nDouble-click to change options"))
        box.Add(item=label, proportion=0,
                flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
        self.sizer.Add(item=box, proportion=0,
                       flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER | wx.ALL, border=5)

        line = wx.StaticLine(parent=self, id=wx.ID_ANY,
                             size=(20, -1), style=wx.LI_HORIZONTAL)
        self.sizer.Add(item=line, proportion=0,
                       flag=wx.EXPAND | wx.ALIGN_CENTRE | wx.ALL, border=5)

        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(parent=self, id=wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(parent=self, id=wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        self.sizer.Add(item=btnsizer, proportion=0,
                       flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(self.sizer)
        self.sizer.Fit(self)

        # bindings
        self.Bind(EVT_ETC_LAYOUT_NEEDED, self.OnRefit, self.textentry)
        self.Bind(wx.EVT_BUTTON,     self.OnSelectFont, fontbtn)
        self.Bind(wx.EVT_TEXT,       self.OnText,       self.textentry)
        self.Bind(wx.EVT_SPINCTRL,   self.OnRotation,   self.rotation)

    def OnRefit(self, event):
        """!Resize text entry to match text"""
        self.sizer.Fit(self)

    def OnText(self, event):
        """!Change text string"""
        self.currText = event.GetString()

    def OnRotation(self, event):
        """!Change rotation"""
        self.currRot = event.GetInt()

        event.Skip()

    def OnSelectFont(self, event):
        """!Change font"""
        data = wx.FontData()
        data.EnableEffects(True)
        data.SetColour(self.currClr)         # set colour
        data.SetInitialFont(self.currFont)

        dlg = wx.FontDialog(self, data)

        if dlg.ShowModal() == wx.ID_OK:
            data = dlg.GetFontData()
            self.currFont = data.GetChosenFont()
            self.currClr = data.GetColour()

            self.textentry.SetFont(self.currFont)
            self.textentry.SetForegroundColour(self.currClr)

            self.Layout()

        dlg.Destroy()

    def GetValues(self):
        """!Get text properties"""
        return {'text': self.currText,
                'font': self.currFont,
                'color': self.currClr,
                'rotation': self.currRot,
                'coords': self.currCoords,
                'active': self.chkbox.IsChecked()}
