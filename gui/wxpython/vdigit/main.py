"""
@package vdigit.main

@brief wxGUI vector digitizer

Classes:
 - main::VDigit

SPDX-FileCopyrightText: 2007-2012 Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later

@author Martin Landa <landa.martin gmail.com>
"""

try:
    from vdigit.wxdigit import IVDigit, GV_LINES  # noqa: F401

    haveVDigit = True
    errorMsg = ""
except (ImportError, NameError) as err:
    haveVDigit = False
    errorMsg = err
    GV_LINES = -1

    class IVDigit:
        def __init__(self, giface, mapwindow):
            pass


class VDigit(IVDigit):
    def __init__(self, giface, mapwindow):
        """Base class of vector digitizer

        :param giface: reference to a grass interface instance
        :param mapwindow: reference to a map window instance
        """
        IVDigit.__init__(self, giface, mapwindow)
