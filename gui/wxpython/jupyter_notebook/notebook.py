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

    def _hide_top_ui(self, event):
        """
        Inject CSS via JS into the Jupyter page to hide top UI elements.

        Works for both:
        - Jupyter Notebook 6 and older (classic interface)
        - Jupyter Notebook 7+ (Lab interface)

        This method is called once the WebView has fully loaded the page.
        Some UI elements may be created dynamically after page load,
        so the script ensures the CSS/JS is applied once elements exist.
        Duplicate injection is prevented by checking for a unique style ID.
        """
        webview = event.GetEventObject()

        # CSS rules to hide panels and interface switcher
        css = """
        /* Notebook 7+ (Lab UI) */
        #top-panel-wrapper { display: none !important; }
        #menu-panel-wrapper { display: none !important; }

        /* Interface switcher ("Open in...") */
        .jp-InterfaceSwitcher { display: none !important; }

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

    def AddPage(self, url, title):
        """
        Add a new aui notebook page with a Jupyter WebView.
        :param url: URL of the Jupyter file (str).
        :param title: Tab title (str).
        """
        browser = html.WebView.New(self)
        wx.CallAfter(browser.LoadURL, url)
        wx.CallAfter(browser.Bind, html.EVT_WEBVIEW_LOADED, self._hide_top_ui)
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
