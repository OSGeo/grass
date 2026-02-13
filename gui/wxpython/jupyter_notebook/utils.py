"""
@package jupyter_notebook.utils

@brief wxGUI Jupyter utils

Functions:
- `is_jupyter_installed()`: Check if Jupyter Notebook is installed on the system and functional.
- `is_wx_html2_available()`: Check if wx.html2 module is available.

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import shutil
import subprocess


def is_jupyter_installed():
    """Check if Jupyter Notebook is installed and functional.

    Uses shutil.which() to check if 'jupyter' command is available in PATH.
    Works on all platforms (Windows, Linux, macOS).

    :return: True if Jupyter Notebook is installed and available, False otherwise.
    """
    # Check if 'jupyter' CLI exists
    jupyter_cmd = shutil.which("jupyter")
    if not jupyter_cmd:
        return False

    # Check if 'jupyter notebook' subcommand works
    try:
        subprocess.run(
            [jupyter_cmd, "notebook", "--version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=True,
        )
        return True
    except Exception:
        return False


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
