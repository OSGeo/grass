"""
MODULE:    r_li_setup_GUI.py

AUTHOR(S): Anne Ghisla <a.ghisla AT gmail.com>

PURPOSE:   Dedicated GUI for r.li.setup, translated and reshaped from original TclTk.

DEPENDS:   []

COPYRIGHT: (C) 2010 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""
# bulk import from location_wizard.py. 
import os
import shutil
import re
import string
import sys
import locale
import platform

import wx
import wx.lib.mixins.listctrl as listmix
import wx.wizard as wiz
import wx.lib.scrolledpanel as scrolled
import time

import gcmd
import globalvar
import utils
from grass.script import core as grass

#@TODO create wizard instead of progressively increasing window
