"""!
@package animation.anim

@brief Animation class controls frame order

Classes:
 - anim::Animation

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx
from utils import Orientation, ReplayMode
from core.utils import _

class Animation(wx.EvtHandler):
    """!Animation class specifies which frame to show at which instance."""
    def __init__(self):
        wx.EvtHandler.__init__(self)

        self.currentIndex = 0
        self.frames = []
        # states
        self.orientation = Orientation.FORWARD
        self.replayMode = ReplayMode.ONESHOT
        
        self.callbackUpdateFrame = None
        self.callbackEndAnimation = None
        self.callbackOrientationChanged = None

        self.isActive = False

    def IsActive(self):
        """!Returns if the animation is active or not"""
        return self.isActive

    def SetActive(self, active):
        self.isActive = active

    def SetFrames(self, frames):
        """!Sets animation frames.

        @param frames list of strings
        """
        self.frames = frames

    def GetFrame(self, index):
        """!Returns animation frame"""
        return self.frames[index]

    def GetCount(self):
        """!Get frame count."""
        return len(self.frames)

    count = property(fget = GetCount)

    def GetReplayMode(self):
        """!Returns replay mode (loop)."""
        return self._replayMode

    def SetReplayMode(self, mode):
        self._replayMode = mode

    replayMode = property(fset = SetReplayMode, fget = GetReplayMode)

    def GetOrientation(self):
        return self._orientation

    def SetOrientation(self, mode):
        self._orientation = mode

    orientation = property(fset = SetOrientation, fget = GetOrientation)

    def SetCallbackUpdateFrame(self, callback):
        """!Sets function to be called when updating frame."""
        self.callbackUpdateFrame = callback

    def SetCallbackEndAnimation(self, callback):
        """!Sets function to be called when animation ends."""
        self.callbackEndAnimation = callback

    def SetCallbackOrientationChanged(self, callback):
        """!Sets function to be called when orientation changes."""
        self.callbackOrientationChanged = callback

    def Start(self):
        if not self.IsActive():
            return 

    def Pause(self, paused):
        if not self.IsActive():
            return

    def Stop(self):
        if not self.IsActive():
            return
        self.currentIndex = 0
        self.callbackEndAnimation(self.currentIndex, self.frames[self.currentIndex])

    def _arrivedToEnd(self):
        """!Decides which action to do after animation end (stop, repeat)."""
        if not self.IsActive():
            return
        if self.replayMode == ReplayMode.ONESHOT:
            self.Stop()

        if self.orientation == Orientation.FORWARD:
            if self.replayMode == ReplayMode.REPEAT:
                self.currentIndex = 0
            elif self.replayMode == ReplayMode.REVERSE:
                self.orientation = Orientation.BACKWARD
                self.currentIndex = self.count - 2 # -1
                self.callbackOrientationChanged(Orientation.BACKWARD)
        else:
            if self.replayMode == ReplayMode.REPEAT:
                self.currentIndex = self.count - 1
            elif self.replayMode == ReplayMode.REVERSE:
                self.orientation = Orientation.FORWARD
                self.currentIndex = 1 # 0
                self.callbackOrientationChanged(Orientation.FORWARD)

    def Update(self):
        """!Updates frame."""
        if not self.IsActive():
            return

        self.callbackUpdateFrame(self.currentIndex, self.frames[self.currentIndex])
        if self.orientation == Orientation.FORWARD:
            self.currentIndex += 1
            if self.currentIndex == self.count:
                self._arrivedToEnd()
        else:
            self.currentIndex -= 1
            if self.currentIndex == -1:
                self._arrivedToEnd()
                
    def FrameChangedFromOutside(self, index):
        """!Let the animation know that frame was changed from outside."""
        if not self.IsActive():
            return
        self.currentIndex = index
        self.callbackUpdateFrame(self.currentIndex, self.frames[self.currentIndex])

    def PreviousFrameIndex(self):
        if not self.IsActive():
            return
        if self.orientation == Orientation.FORWARD:
            self.currentIndex -= 1
            if self.currentIndex == -1:
                self.currentIndex = 0
        else:
            self.currentIndex += 1
            if self.currentIndex == self.count:
                self.currentIndex = self.count - 1

    def NextFrameIndex(self):
        if not self.IsActive():
            return
        if self.orientation == Orientation.FORWARD:
            self.currentIndex += 1
            if self.currentIndex == self.count:
                self.currentIndex = self.count - 1
        else:
            self.currentIndex -= 1
            if self.currentIndex == -1:
                self.currentIndex = 0

#def test():
#    import wx
#    app = wx.PySimpleApp()
#    a = Animation()
#
#
#    frame = wx.Frame(None)
#    frame.Show()
#
#    a.SetReplayMode(ReplayMode.REVERSE)
#    a.Start()
#    app.MainLoop()
#
#
#if __name__ == '__main__':
#    test()


