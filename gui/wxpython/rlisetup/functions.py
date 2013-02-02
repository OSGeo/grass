# -*- coding: utf-8 -*-
"""
Created on Mon Nov 26 11:48:03 2012

@author: lucadelu
"""
import wx
import os
import sys


def checkValue(value):
    if value == '':
        wx.FindWindowById(wx.ID_FORWARD).Enable(False)
    else:
        wx.FindWindowById(wx.ID_FORWARD).Enable(True)


def retRLiPath():
    # configuration directory
    if sys.platform == 'win32':
        grass_config_dirname = "GRASS7"
        grass_config_dir = os.path.join(os.getenv('APPDATA'),
                                        grass_config_dirname)
    else:
        grass_config_dirname = ".grass7"
        grass_config_dir = os.path.join(os.getenv('HOME'),
                                        grass_config_dirname)

    rlipath = os.path.join(grass_config_dir, 'r.li')
    if os.path.exists(rlipath):
        return rlipath
    else:
        os.mkdir(rlipath)
        return rlipath
