"""!
@package nviz.animation

@brief Nviz (3D view) animation

Classes:
 - animation::Animation

(C) 2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com> 
"""

import os
import copy

import wx

from grass.pydispatch.signal import Signal

class Animation:
    """!Class represents animation as a sequence of states (views).
    It enables to record, replay the sequence and finally generate
    all image files. Recording and replaying is based on timer events.
    There is no frame interpolation like in the Tcl/Tk based Nviz.
    """
    def __init__(self, mapWindow, timer):
        """!Animation constructor
        
        Signals:
            animationFinished - emitted when animation finished
                              - attribute 'mode'
            animationUpdateIndex - emitted during animation to update gui
                                 - attributes 'index' and 'mode'
        
        @param mapWindow glWindow where rendering takes place
        @param timer timer for recording and replaying
        """
        self.animationFinished = Signal('Animation.animationFinished')
        self.animationUpdateIndex = Signal('Animation.animationUpdateIndex')
        
        self.animationList = []         # view states
        self.timer = timer
        self.mapWindow = mapWindow
        self.actions = {'record': self.Record,
                        'play': self.Play}
        self.formats = ['ppm', 'tif']   # currently supported formats
        self.mode = 'record'            # current mode (record, play, save)
        self.paused = False             # recording/replaying paused
        self.currentFrame = 0           # index of current frame
        self.fps = 24 # user settings   # Frames per second
        
        self.stopSaving = False         # stop during saving images
        self.animationSaved = False     # current animation saved or not
        
    def Start(self):
        """!Start recording/playing"""
        self.timer.Start(self.GetInterval())
        
    def Pause(self):
        """!Pause recording/playing"""
        self.timer.Stop()
        
    def Stop(self):
        """!Stop recording/playing"""
        self.timer.Stop()
        self.PostFinishedEvent()
        
    def Update(self):
        """!Record/play next view state (on timer event)"""
        self.actions[self.mode]()
    
    def Record(self):
        """!Record new view state"""
        self.animationList.append({'view' : copy.deepcopy(self.mapWindow.view),
                                   'iview': copy.deepcopy(self.mapWindow.iview)})
        self.currentFrame += 1
        self.PostUpdateIndexEvent(index = self.currentFrame)
        self.animationSaved = False
        
    def Play(self):
        """!Render next frame"""
        if not self.animationList:
            self.Stop()
            return
        try:
            self.IterAnimation()
        except IndexError:
            # no more frames
            self.Stop()
            
    def IterAnimation(self):
        params = self.animationList[self.currentFrame]
        self.UpdateView(params)
        self.currentFrame += 1
        
        self.PostUpdateIndexEvent(index = self.currentFrame)
        
    def UpdateView(self, params):
        """!Update view data in map window and render"""
        toolWin = self.mapWindow.GetToolWin()
        toolWin.UpdateState(view = params['view'], iview = params['iview'])
        
        self.mapWindow.UpdateView()
        
        self.mapWindow.render['quick'] = True
        self.mapWindow.Refresh(False)
        
    def IsRunning(self):
        """!Test if timer is running"""
        return self.timer.IsRunning()
        
    def SetMode(self, mode):
        """!Start animation mode
        
        @param mode animation mode (record, play, save)
        """
        self.mode = mode
        
    def GetMode(self):
        """!Get animation mode (record, play, save)"""
        return self.mode
        
    def IsPaused(self):
        """!Test if animation is paused"""
        return self.paused
        
    def SetPause(self, pause):
        self.paused = pause
        
    def Exists(self):
        """!Returns if an animation has been recorded"""
        return bool(self.animationList)
        
    def GetFrameCount(self):
        """!Return number of recorded frames"""
        return len(self.animationList)
        
    def Clear(self):
        """!Clear all records"""
        self.animationList = []
        self.currentFrame = 0
        
    def GoToFrame(self, index):
        """!Render frame of given index"""
        if index >= len(self.animationList):
            return
            
        self.currentFrame = index
        params = self.animationList[self.currentFrame]
        self.UpdateView(params)
        
    def PostFinishedEvent(self):
        """!Animation ends"""
        self.animationFinished.emit(mode=self.mode)
        
    def PostUpdateIndexEvent(self, index):
        """!Frame index changed, update tool window"""
        self.animationUpdateIndex(index=index, mode=self.mode)
        
    def StopSaving(self):
        """!Abort image files generation"""
        self.stopSaving = True
        
    def IsSaved(self):
        """"!Test if animation has been saved (to images)"""
        return self.animationSaved
        
    def SaveAnimationFile(self, path, prefix, format):
        """!Generate image files
        
        @param path path to direcory
        @param prefix file prefix
        @param format index of image file format
        """
        w, h = self.mapWindow.GetClientSizeTuple()
        toolWin = self.mapWindow.GetToolWin()
        
        self.currentFrame = 0
        self.mode = 'save'
        for params in self.animationList:
            if not self.stopSaving:
                self.UpdateView(params)
                filename = prefix + "_" + str(self.currentFrame) + '.' + self.formats[format]
                filepath = os.path.join(path, filename)
                self.mapWindow.SaveToFile(FileName = filepath, FileType = self.formats[format],
                                                  width = w, height = h)
                self.currentFrame += 1
                
                wx.Yield()
                toolWin.UpdateFrameIndex(index = self.currentFrame, goToFrame = False)
            else:
                self.stopSaving = False
                break
        self.animationSaved = True
        self.PostFinishedEvent()

    def SetFPS(self, fps):
        """!Set Frames Per Second value
        @param fps frames per second
        """
        self.fps = fps
    
    def GetInterval(self):
        """!Return timer interval in ms"""
        return 1000. / self.fps
