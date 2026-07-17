"""
@package ps.menudata

@brief wxPsMap - menu entries

Classes:
 - menudata::PsMapMenuData

SPDX-FileCopyrightText: 2011 Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os

from core import globalvar
from core.menutree import MenuTreeModelBuilder


class PsMapMenuData(MenuTreeModelBuilder):
    def __init__(self, path=None):
        """Menu for Cartographic Composer (psmap.py)

        :param path: path to XML to be read (None for menudata_psmap.xml)
        """
        if not path:
            path = os.path.join(globalvar.WXGUIDIR, "xml", "menudata_psmap.xml")

        MenuTreeModelBuilder.__init__(self, path)
