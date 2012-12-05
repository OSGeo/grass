"""!
@package animation.toolbars

@brief Animation toolbars

Classes:
 - toolbars::MainToolbar(BaseToolbar):
 - toolbars::AnimationToolbar(BaseToolbar):
 - toolbars::MiscToolbar(BaseToolbar):


(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""
import wx
from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon

from anim import ReplayMode

ganimIcons = {
        'speed': MetaIcon(img = 'settings', label = _("Change animation speed")),
        'playForward': MetaIcon(img = 'execute', label = _("Play forward")),
        'playBack': MetaIcon(img = 'player-back', label = _("Play back")),
        'stop': MetaIcon(img = 'player-stop', label = _("Stop")),
        'pause': MetaIcon(img = 'player-pause', label = _("Pause")),
        'oneDirectionReplay': MetaIcon(img = 'redraw', label = _("Repeat")),
        'bothDirectionReplay': MetaIcon(img = 'player-repeat-back-forward',
                                        label = _("Play back and forward")),
        'addAnimation': MetaIcon(img = 'layer-add', label = _("Add new animation"),
                                 desc = _("Add new animation")),
        'editAnimation': MetaIcon(img = 'layer-more', label = _("Add, edit or remove animation"),
                                  desc = _("Add, edit or remove animation")),
        'exportAnimation': MetaIcon(img = 'layer-export', label = _("Export animation"),
                                    desc = _("Export animation"))
        }

class MainToolbar(BaseToolbar):
    """!Main toolbar (data management)
    """
    def __init__(self, parent):
        """!Main toolbar constructor
        """
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Returns toolbar data (name, icon, handler)"""
        # BaseIcons are a set of often used icons. It is possible
        # to reuse icons in ./trunk/gui/icons/grass or add new ones there.
        icons = ganimIcons
        return self._getToolbarData((("addAnimation", icons["addAnimation"],
                                      self.parent.OnAddAnimation),
                                     ("editAnimation", icons["editAnimation"],
                                      self.parent.OnEditAnimation),
                                     ("reload", BaseIcons["render"],
                                      self.parent.Reload),
                                     ("exportAnimation", icons["exportAnimation"],
                                      self.parent.OnExportAnimation),
                                    ))
class AnimationToolbar(BaseToolbar):
    """!Animation toolbar (to control animation)
    """
    def __init__(self, parent):
        """!Animation toolbar constructor
        """
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

        self.isPlayingForward = True
        self.EnableAnimTools(False)
        
        
    def _toolbarData(self):
        """!Returns toolbar data (name, icon, handler)"""
        # BaseIcons are a set of often used icons. It is possible
        # to reuse icons in ./trunk/gui/icons/grass or add new ones there.
        icons = ganimIcons
        return self._getToolbarData((("playBack", icons["playBack"],
                                      self.OnPlayBack),
                                     ("playForward", icons["playForward"],
                                      self.OnPlayForward),
                                     ("pause", icons["pause"],
                                      self.OnPause,
                                      wx.ITEM_CHECK),
                                     ("stop", icons["stop"],
                                      self.OnStop),
                                     (None, ),
                                     ("oneDirectionReplay", icons["oneDirectionReplay"],
                                      self.OnOneDirectionReplay,
                                      wx.ITEM_CHECK),
                                     ("bothDirectionReplay", icons["bothDirectionReplay"],
                                      self.OnBothDirectionReplay,
                                      wx.ITEM_CHECK),
                                     (None, ),
                                     ("adjustSpeed", icons['speed'],
                                       self.parent.OnAdjustSpeed)
                                    ))
    def OnPlayForward(self, event):
        self.PlayForward()
        self.parent.OnPlayForward(event)

    def PlayForward(self):
        self.EnableTool(self.playForward, False)
        self.EnableTool(self.playBack, True)
        self.EnableTool(self.pause, True)
        self.EnableTool(self.stop, True)
        self.ToggleTool(self.pause, False)
        self.isPlayingForward = True

    def OnPlayBack(self, event):
        self.PlayBack()
        self.parent.OnPlayBack(event)

    def PlayBack(self):
        self.EnableTool(self.playForward, True)
        self.EnableTool(self.playBack, False)
        self.EnableTool(self.pause, True)
        self.EnableTool(self.stop, True)
        self.ToggleTool(self.pause, False)
        self.isPlayingForward = False
        
    def OnPause(self, event):
        self.Pause()
        self.parent.OnPause(event)

    def Pause(self):
        if self.GetToolState(self.pause):
            self.EnableTool(self.playForward, True)
            self.EnableTool(self.playBack, True)
        else:
            self.EnableTool(self.playForward, not self.isPlayingForward)
            self.EnableTool(self.playBack, self.isPlayingForward)

    def OnStop(self, event):
        self.Stop()
        self.parent.OnStop(event)

    def Stop(self):
        self.EnableTool(self.playForward, True)
        self.EnableTool(self.playBack, True)
        self.EnableTool(self.pause, False)
        self.EnableTool(self.stop, False)
        self.ToggleTool(self.pause, False)

        # if not self.GetToolState(self.oneDirectionReplay) and \
        #    not self.GetToolState(self.bothDirectionReplay):
        #     self.EnableTool(self.playBack, False) # assuming that stop rewinds to the beginning

    def OnOneDirectionReplay(self, event):
        if event.IsChecked():
            self.ToggleTool(self.bothDirectionReplay, False)
        self.parent.OnOneDirectionReplay(event)

    def OnBothDirectionReplay(self, event):
        if event.IsChecked():
            self.ToggleTool(self.oneDirectionReplay, False)
        self.parent.OnBothDirectionReplay(event)

    def SetReplayMode(self, mode):
        one, both = False, False
        if mode == ReplayMode.REPEAT:
            one = True
        elif mode == ReplayMode.REVERSE:
            both = True

        self.ToggleTool(self.oneDirectionReplay, one)
        self.ToggleTool(self.bothDirectionReplay, both)

    def EnableAnimTools(self, enable):
        """!Enable or diable animation tools"""
        self.EnableTool(self.playForward, enable)
        self.EnableTool(self.playBack, enable)
        self.EnableTool(self.pause, enable)
        self.EnableTool(self.stop, enable)

class MiscToolbar(BaseToolbar):
    """!Toolbar with miscellaneous tools related to app
    """
    def __init__(self, parent):
        """!Toolbar constructor
        """
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        return self._getToolbarData((("help", BaseIcons['help'],
                                      self.parent.OnHelp),
                                    ("quit", BaseIcons['quit'],
                                      self.parent.OnCloseWindow),
                                     ))