# -*- coding: utf-8 -*-
"""
Created on Mon Nov 26 11:48:03 2012

@author: lucadelu
"""
import wx
import os
from grass.script import core as grass


def checkValue(value):
    if value == '':
        wx.FindWindowById(wx.ID_FORWARD).Enable(False)
    else:
        wx.FindWindowById(wx.ID_FORWARD).Enable(True)


def retRLiPath():
    major_version = int(grass.version()['version'].split('.', 1)[0])
    rlipath = os.path.join(os.environ['HOME'], '.grass%d' % major_version,
                           'r.li')
    if os.path.exists(rlipath):
        return rlipath
    else:
        os.mkdir(rlipath)
        return rlipath