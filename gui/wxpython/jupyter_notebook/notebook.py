"""
@package jupyter_notebook.notebook

@brief Manages the jupyter notebook widget.

Classes:
 - page::JupyterAuiNotebook

(C) 2025 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import wx
from wx.lib.agw import aui

try:
    import wx.html2 as html  # wx.html2 is available in wxPython 4.0 and later
except ImportError as e:
    raise RuntimeError(_("wx.html2 is required for Jupyter integration.")) from e

from gui_core.wrap import SimpleTabArt


class JupyterAuiNotebook(aui.AuiNotebook):
    def __init__(
        self,
        parent,
        agwStyle=aui.AUI_NB_DEFAULT_STYLE
        | aui.AUI_NB_CLOSE_ON_ACTIVE_TAB
        | aui.AUI_NB_TAB_EXTERNAL_MOVE
        | aui.AUI_NB_BOTTOM
        | wx.NO_BORDER,
    ):
        """
        Wrapper for the notebook widget that manages notebook pages.
        """
        self.parent = parent
        self.webview = None

        super().__init__(parent=self.parent, id=wx.ID_ANY, agwStyle=agwStyle)

        self.SetArtProvider(SimpleTabArt())

        self.Bind(aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.OnPageClose)

    def _inject_javascript(self, event):
        """
        Inject JavaScript into the Jupyter notebook page to hide top UI bars.

        Works for:
        - Jupyter Notebook 6 and older (classic interface)
        - Jupyter Notebook 7+ (Jupyter Lab interface)

        This is called once the WebView has fully loaded the Jupyter page.
        """
        webview = event.GetEventObject()
        js = """
        var interval = setInterval(function() {
            // --- Jupyter Notebook 7+ (new UI) ---
            var topPanel = document.getElementById('top-panel-wrapper');
            var menuPanel = document.getElementById('menu-panel-wrapper');
            if (topPanel) topPanel.style.display = 'none';
            if (menuPanel) menuPanel.style.display = 'none';

            // --- Jupyter Notebook 6 and older (classic UI) ---
            var headerContainer = document.getElementById('header-container');
            var menubar = document.getElementById('menubar');
            if (headerContainer) headerContainer.style.display = 'none';
            if (menubar) menubar.style.display = 'none';

            // --- Stop once everything is hidden ---
            if ((topPanel || headerContainer) && (menuPanel || menubar)) {
                clearInterval(interval);
            }
        }, 500);
        """
        webview.RunScript(js)

    def AddPage(self, url, title):
        """
        Add a new aui notebook page with a Jupyter WebView.
        :param url: URL of the Jupyter file (str).
        :param title: Tab title (str).
        """
        browser = html.WebView.New(self)
        wx.CallAfter(browser.LoadURL, url)
        wx.CallAfter(browser.Bind, html.EVT_WEBVIEW_LOADED, self._inject_javascript)
        super().AddPage(browser, title)

    def OnPageClose(self, event):
        """Close the aui notebook page with confirmation dialog."""
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
