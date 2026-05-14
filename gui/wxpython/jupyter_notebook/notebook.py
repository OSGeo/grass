"""
@package jupyter_notebook.notebook

@brief Notebook for integration of Jupyter Notebook to GUI.

Classes:
 - notebook::JupyterAuiNotebook

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import wx
from wx.lib.agw import aui

# Try to import html2, but it might not be available
try:
    import wx.html2 as html

    WX_HTML2_AVAILABLE = True
except ImportError:
    WX_HTML2_AVAILABLE = False
    html = None

from gui_core.wrap import SimpleTabArt


class JupyterAuiNotebook(aui.AuiNotebook):
    """AUI Notebook for managing Jupyter notebook tabs with embedded WebView.

    Note: This class requires wx.html2.WebView to be available and functional.
    If wx.html2 is not available or WebView.New() raises NotImplementedError,
    the AddPage method will raise an exception.
    """

    def __init__(
        self,
        parent: wx.Window,
        agwStyle: int = aui.AUI_NB_DEFAULT_STYLE
        | aui.AUI_NB_CLOSE_ON_ACTIVE_TAB
        | aui.AUI_NB_TAB_EXTERNAL_MOVE
        | aui.AUI_NB_BOTTOM
        | wx.NO_BORDER,
    ) -> None:
        """Initialize AUI notebook for Jupyter with WebView tabs.

        :param parent: Parent window
        :param agwStyle: AUI notebook style flags
        :raises ImportError: If wx.html2 is not available
        """
        if not WX_HTML2_AVAILABLE:
            msg = "wx.html2 is not available"
            raise ImportError(msg)

        self.parent = parent

        super().__init__(parent=self.parent, id=wx.ID_ANY, agwStyle=agwStyle)

        self.SetArtProvider(SimpleTabArt())

        self.Bind(aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.OnPageClose)

    def _hide_top_ui(self, event: html.WebViewEvent) -> None:
        """Inject CSS via JS into the Jupyter page to hide top UI elements.

        Works for both:
        - Jupyter Notebook 6 and older (classic interface)
        - Jupyter Notebook 7+ (Lab interface)

        This method is called once the WebView has fully loaded the page.
        Some UI elements may be created dynamically after page load,
        so the script ensures the CSS/JS is applied once elements exist.
        Duplicate injection is prevented by checking for a unique style ID.

        :param event: WebView loaded event
        """
        webview = event.GetEventObject()

        # CSS rules to hide panels and interface switcher
        css = """
        /* Notebook 7+ (Lab UI) */
        #top-panel-wrapper { display: none !important; }
        #menu-panel-wrapper { display: none !important; }

        /* Interface switcher - hides any interface switching UI elements
        (e.g., dropdown menu "Open In..." when nbclassic is installed,
        JupyterLab button when only jupyter lab is installed, or other
        interface options that may be added in future Jupyter versions) */
        [data-jp-item-name="interfaceSwitcher"] { display: none !important; }

        /* remove top spacing left by hidden panels */
        .lm-Panel { top: 0 !important; }

        /* Notebook 6 and older (classic UI) */
        #header-container { display: none !important; }
        #menubar { display: none !important; }
        """

        # JavaScript that injects the CSS once
        js = f"""
        (function() {{
            if (document.getElementById('grass-hide-ui')) {{
                return;
            }}

            var style = document.createElement('style');
            style.id = 'grass-hide-ui';
            style.innerHTML = `{css}`;
            document.head.appendChild(style);
        }})();
        """

        webview.RunScript(js)

    def _on_navigating(self, event):
        """Handle navigation events - open external links in system browser.

        :param event: WebView navigating event
        """
        import webbrowser
        from urllib.parse import urlparse

        url = event.GetURL()
        parsed = urlparse(url)

        # Allow navigation within Jupyter (localhost)
        if parsed.hostname in {"localhost", "127.0.0.1", None}:
            event.Skip()
            return

        # Open external links in system browser (new window)
        event.Veto()
        webbrowser.open_new(url)

    def _on_new_window(self, event):
        """Handle links that try to open in new window.

        :param event: WebView new window event
        """
        import webbrowser

        url = event.GetURL()
        event.Veto()  # Don't open new WebView window
        webbrowser.open_new(url)

    def AddPage(self, url: str, title: str) -> None:
        """Add a new AUI notebook page with a Jupyter WebView.

        :param url: URL of the Jupyter file
        :param title: Tab title
        :raises NotImplementedError: If wx.html2.WebView is not functional on this system
        """
        browser = html.WebView.New(self)
        wx.CallAfter(browser.LoadURL, url)
        wx.CallAfter(browser.Bind, html.EVT_WEBVIEW_LOADED, self._hide_top_ui)
        wx.CallAfter(browser.Bind, html.EVT_WEBVIEW_NAVIGATING, self._on_navigating)
        wx.CallAfter(browser.Bind, html.EVT_WEBVIEW_NEWWINDOW, self._on_new_window)
        super().AddPage(browser, title)

    def OnPageClose(self, event: aui.AuiNotebookEvent) -> None:
        """Close the AUI notebook page with confirmation dialog.

        :param event: Notebook page close event
        """
        index = event.GetSelection()
        title = self.GetPageText(index)

        dlg = wx.MessageDialog(
            self,
            message=_("Really close page '{}'?").format(title),
            caption=_("Close page"),
            style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
        )

        if dlg.ShowModal() != wx.ID_YES:
            event.Veto()

        dlg.Destroy()
