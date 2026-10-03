"""
@package animation.frame

@brief Animation window for the standalone Animation Tool

Classes:
 - frame::AnimationFrame

SPDX-FileCopyrightText: 2013 GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later

@author Anna Petrasova <kratochanna gmail.com>
"""

import os

import wx

from core import globalvar

from animation.panels import AnimationToolPanel, MAX_COUNT

__all__ = ["MAX_COUNT", "AnimationFrame"]


class AnimationFrame(wx.Frame):
    """Window hosting the animation panel.

    Used by g.gui.animation, a separate process, and by multi-window mode,
    a separate window. Single-window mode puts the panel in its notebook.
    """

    def __init__(
        self, parent, giface, title=_("Animation Tool"), rasters=None, timeseries=None
    ):
        """
        :param parent: parent window
        :param giface: GRASS interface
        :param title: window title
        :param rasters: raster maps to animate
        :param timeseries: space time datasets to animate
        """
        wx.Frame.__init__(
            self, parent, title=title, style=wx.DEFAULT_FRAME_STYLE, size=(800, 600)
        )
        self.SetClientSize(self.GetSize())
        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
        )

        self.statusbar = self.CreateStatusBar(number=1, style=0)
        self.panel = AnimationToolPanel(
            parent=self,
            giface=giface,
            statusbar=self.statusbar,
            rasters=rasters,
            timeseries=timeseries,
        )

        self.SetName("AnimationFrame")
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

    def SetAnimations(self, layerLists):
        """Set animation data

        :param layerLists: list of layerLists
        """
        self.panel.SetAnimations(layerLists)

    def AnimateStds(self, name, stds_type):
        """Animate a space time dataset, replacing the current animations

        :param name: name of the dataset including the mapset
        :param stds_type: type of the dataset, 'strds' or 'stvds'
        """
        self.panel.AnimateStds(name, stds_type)

    def OnCloseWindow(self, event):
        """Clean up and close.

        The panel releases its resources and then destroys this window, so
        this must not destroy it as well.
        """
        self.panel.OnCloseWindow(event)
