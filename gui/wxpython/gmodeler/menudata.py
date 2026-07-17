"""
@package gmodeler.menudata

@brief wxGUI Graphical Modeler - menu data

Classes:
 - menudata::ModelerMenuData

SPDX-FileCopyrightText: 2010-2011 Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later

@author Martin Landa <landa.martin gmail.com>
"""

import os

from core import globalvar
from core.menutree import MenuTreeModelBuilder


class ModelerMenuData(MenuTreeModelBuilder):
    def __init__(self, filename=None):
        if not filename:
            filename = os.path.join(globalvar.WXGUIDIR, "xml", "menudata_modeler.xml")

        MenuTreeModelBuilder.__init__(self, filename)
