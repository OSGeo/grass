"""
@package jupyter_notebook.utils

@brief wxGUI Jupyter utils

Functions:
- `is_notebook_module_available()`: Check if the current Python can run the notebook module.
- `is_wx_html2_available()`: Check if wx.html2 module is available.
- `is_webview2_available()`: Check if wx.html2.WebView uses Microsoft Edge WebView2 on Windows.
- `get_wxpython_version()`: Return current wxPython version.
- `force_reinstall_wxpython()`: Force reinstall current wxPython version via pip.
- `ensure_notebook_module_available()`: Show install dialog and ensure notebook module.
- `ensure_webview2_backend_available()`: Show reinstall dialog and ensure WebView2 backend.

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import sys
import subprocess


def is_notebook_module_available() -> bool:
    """Check if the current Python can run the notebook module.

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


def install_notebook_package() -> None:
    """Install notebook package in the current Python environment.

    Uses the same interpreter as GRASS to avoid environment mismatch issues.
    """
    subprocess.check_call([sys.executable, "-m", "pip", "install", "notebook"])


def get_wxpython_version() -> str | None:
    """Get current wxPython version string.

    :return: Installed wxPython version, or None if version is not exposed
    """
    import wx

    try:
        return str(wx.__version__)
    except AttributeError:
        return None


def force_reinstall_wxpython(version: str) -> None:
    """Force reinstall wxPython with a specific version in current Python.

    :param version: wxPython version (e.g. "4.2.2")
    """
    subprocess.check_call(
        [
            sys.executable,
            "-m",
            "pip",
            "install",
            "--force-reinstall",
            "wxpython=={}".format(version),
        ]
    )


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


def ensure_notebook_module_available(parent, report_error, report_info) -> bool:
    """Ensure notebook module is available, offering interactive installation.

    :param parent: Parent wx window for dialogs
    :param report_error: Error reporting callback (e.g. GError)
    :param report_info: Info reporting callback (e.g. GMessage)
    :return: True when notebook module is available, False otherwise
    """
    import wx

    if is_notebook_module_available():
        return True

    dlg = wx.MessageDialog(
        parent=parent,
        message=(
            "To use notebooks in GRASS, you need to have the Jupyter Notebook "
            "package installed.\n\n"
            "Would you like to install it now?"
        ),
        caption="Jupyter Notebook not available",
        style=wx.YES_NO | wx.ICON_INFORMATION,
    )
    if hasattr(dlg, "SetYesNoLabels"):
        dlg.SetYesNoLabels("Install", "Cancel")

    response = dlg.ShowModal()
    dlg.Destroy()

    if response != wx.ID_YES:
        return False

    busy = wx.BusyInfo("Installing Jupyter Notebook package...", parent=parent)
    try:
        install_notebook_package()
    except Exception as error:
        report_error(
            parent=parent,
            message=(
                "Automatic installation failed.\n\n"
                "Please run this command manually in the same Python environment:\n"
                "{command}\n\n"
                "Details: {error}"
            ).format(
                command="{} -m pip install notebook".format(sys.executable),
                error=error,
            ),
        )
        return False
    finally:
        del busy

    if not is_notebook_module_available():
        report_error(
            parent=parent,
            message=(
                "Installation finished, but notebook module is still not available "
                "in the current Python environment. Please restart GRASS and try again."
            ),
        )
        return False

    report_info(
        parent=parent,
        message="Jupyter Notebook dependency was installed successfully.",
    )
    return True


def ensure_webview2_backend_available(parent, report_error, report_info) -> str:
    """Ensure WebView2 backend is available on Windows.

    Offers wxPython force-reinstall with currently installed wxPython version.

    :param parent: Parent wx window for dialogs
    :param report_error: Error reporting callback (e.g. GError)
    :param report_info: Info reporting callback (e.g. GMessage)
    :return: "available" when backend is ready now,
             "restart-required" when reinstall succeeded but GRASS restart is needed,
             "unavailable" otherwise
    """
    import wx

    # WebView2 backend is Windows-specific; non-Windows platforms are unaffected.
    if not sys.platform.startswith("win"):
        return "available"

    if is_webview2_available():
        return "available"

    version = get_wxpython_version()
    if not version:
        report_error(
            parent=parent,
            message=(
                "Failed to detect wxPython version. "
                "Cannot run automatic reinstall for WebView2 support."
            ),
        )
        return "unavailable"

    command = "{} -m pip install --force-reinstall wxpython=={}".format(
        sys.executable, version
    )
    dlg = wx.MessageDialog(
        parent=parent,
        message=(
            "Integrated mode requires Microsoft Edge WebView2 backend in wxPython.\n\n"
            "Your current wxPython build may come without this support "
            "(common in some OSGeo4W builds).\n\n"
            "Would you like to run this command now in the current Python environment?\n"
            "{command}"
        ).format(command=command),
        caption="WebView2 Backend Not Available",
        style=wx.YES_NO | wx.ICON_WARNING,
    )
    if hasattr(dlg, "SetYesNoLabels"):
        dlg.SetYesNoLabels("Reinstall", "Skip")

    response = dlg.ShowModal()
    dlg.Destroy()

    if response != wx.ID_YES:
        return "unavailable"

    busy = wx.BusyInfo(
        "Reinstalling wxPython (this may take a while)...", parent=parent
    )
    try:
        force_reinstall_wxpython(version)
    except Exception as error:
        report_error(
            parent=parent,
            message=(
                "Automatic wxPython reinstall failed.\n\n"
                "Please run this command manually in the same Python environment:\n"
                "{command}\n\n"
                "Details: {error}"
            ).format(command=command, error=error),
        )
        return "unavailable"
    finally:
        del busy

    report_info(
        parent=parent,
        message=(
            "wxPython reinstall finished successfully, but the current GRASS session "
            "is still using the old wxPython build.\n\n"
            "Please restart GRASS and try integrated mode again."
        ),
    )
    return "restart-required"
