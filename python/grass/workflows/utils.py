#
# AUTHOR(S): Linda Karlovska <linda.karlovska@seznam.cz>
#
# PURPOSE:   Provides utils related to launching and managing
#            a local Jupyter server.
#
# COPYRIGHT: (C) 2025 by Linda Karlovska and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""
This module provides utils for launching and managing
a local Jupyter server.

Functions:
- `is_jupyter_installed()`: Check if Jupyter Notebook is installed on the system.
- `is_wx_html2_available()`: Check if wx.html2 module is available.

Designed for use within GRASS GUI tools or scripting environments.
"""

import shutil


def is_jupyter_installed():
    """Check if Jupyter Notebook is installed.

    Uses shutil.which() to check if 'jupyter' command is available in PATH.
    Works on all platforms (Windows, Linux, macOS).

    :return: True if Jupyter Notebook is installed and available, False otherwise.
    """
    return shutil.which("jupyter") is not None


def is_wx_html2_available():
    """Check whether wx.html2 (WebView) support is available.

    This can be missing on some platforms or distributions (e.g. Gentoo)
    when wxPython or the underlying wxWidgets library is built without
    HTML2/WebView support.

    :return: True if wxPython/wxWidgets html2 module is available, False otherwise.
    """
    try:
        __import__("wx.html2")
        return True
    except (ImportError, ModuleNotFoundError):
        return False
