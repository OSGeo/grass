"""
@package jupyter_notebook.utils

@brief wxGUI Jupyter utils

Functions:
- `is_jupyter_notebook_installed()`: Check if Jupyter Notebook is installed on the system and functional.
- `is_wx_html2_available()`: Check if wx.html2 module is available.
- `is_webview2_available()`: Check if wx.html2.WebView uses Microsoft Edge WebView2 on Windows.

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import sys
import subprocess


def is_jupyter_notebook_installed() -> bool:
    """Check if Jupyter Notebook is installed and functional.

    :return: True if Jupyter Notebook is installed and available, False otherwise
    """
    try:
        subprocess.run(
            [sys.executable, "-m", "notebook", "--version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=True,
        )
        return True
    except (FileNotFoundError, subprocess.CalledProcessError, OSError):
        return False


def is_wx_html2_available() -> bool:
    """Check whether wx.html2 (WebView) support is available.

    This can be missing on some platforms or distributions (e.g. Gentoo)
    when wxPython or the underlying wxWidgets library is built without
    HTML2/WebView support.

    :return: True if wxPython/wxWidgets html2 module is available, False otherwise
    """
    try:
        __import__("wx.html2")
        return True
    except (ImportError, ModuleNotFoundError):
        return False


def is_webview2_available() -> bool:
    """Check whether wx.html2.WebView uses Microsoft Edge WebView2 on Windows.

    On Windows, wx.html2.WebView can fall back to the Internet Explorer (MSHTML)
    engine if WebView2 runtime is not installed, which is not compatible with
    modern Jupyter Notebook/Lab interfaces. On Linux/macOS, the backend is
    assumed to be usable (WebKit-based).

    :return: True if WebView2 (Edge) is available or non-Windows platform,
             False otherwise
    """
    if sys.platform.startswith("win"):
        try:
            import wx.html2 as html

            return html.WebView.IsBackendAvailable(html.WebViewBackendEdge)
        except Exception:
            return False
    return True
