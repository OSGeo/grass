"""
@package gui_core.ghelp

@brief Help/about window, menu tree, search module tree

Classes:
 - ghelp::AboutWindow
 - ghelp::HelpWindow
 - ghelp::HelpPanel

(C) 2008-2019 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import codecs
import platform
import textwrap
import sys

from pathlib import Path
from html.parser import HTMLParser
from urllib.parse import urljoin, urlparse

import wx
from wx.html import HtmlWindow
from operator import itemgetter

try:
    from wx.lib.agw.hyperlink import HyperLinkCtrl
except ImportError:
    from wx.lib.hyperlink import HyperLinkCtrl
try:
    from wx.adv import AboutDialogInfo
    from wx.adv import AboutBox
except ImportError:
    from wx import AboutDialogInfo
    from wx import AboutBox

import grass.script as gs
from grass.exceptions import CalledModuleError

# needed just for testing
if __name__ == "__main__":
    from grass.script.setup import set_gui_path

    set_gui_path()

from core import globalvar
from core.gcmd import GError, DecodeString
from core.settings import UserSettings
from gui_core.widgets import FormNotebook, ScrolledPanel
from gui_core.wrap import Button, StaticText, TextCtrl
from core.debug import Debug


class AboutWindow(wx.Frame):
    """Create custom About Window"""

    def __init__(self, parent, size=(770, 460), title=_("About GRASS GIS"), **kwargs):
        wx.Frame.__init__(
            self, parent=parent, id=wx.ID_ANY, title=title, size=size, **kwargs
        )

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        # icon
        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
        )

        # notebook
        self.aboutNotebook = FormNotebook(self.panel, style=wx.BK_LEFT)

        for title, win in (
            (_("Info"), self._pageInfo()),
            (_("Copyright"), self._pageCopyright()),
            (_("License"), self._pageLicense()),
            (_("Citation"), self._pageCitation()),
            (_("Authors"), self._pageCredit()),
            (_("Contributors"), self._pageContributors()),
            (_("Extra contributors"), self._pageContributors(extra=True)),
            (_("Translators"), self._pageTranslators()),
            (_("Translation status"), self._pageStats()),
        ):
            self.aboutNotebook.AddPage(page=win, text=title)
        wx.CallAfter(self.aboutNotebook.SetSelection, 0)
        wx.CallAfter(self.aboutNotebook.Refresh)

        # buttons
        self.btnClose = Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btnClose.Bind(wx.EVT_BUTTON, self.OnCloseWindow)

        self._doLayout()

    def _doLayout(self):
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btnClose, proportion=0, flag=wx.ALL, border=5)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.aboutNotebook, proportion=1, flag=wx.EXPAND | wx.ALL, border=1)
        sizer.Add(btnSizer, proportion=0, flag=wx.ALL | wx.ALIGN_RIGHT, border=1)

        self.SetMinSize((400, 400))

        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)

        self.Layout()

    def _pageInfo(self):
        """Info page"""
        # get version and web site
        vInfo = gs.version()
        if not vInfo:
            sys.stderr.write(_("Unable to get GRASS version\n"))

        infoTxt = ScrolledPanel(self.aboutNotebook)
        infoTxt.SetupScrolling()
        infoSizer = wx.BoxSizer(wx.VERTICAL)
        infoGridSizer = wx.GridBagSizer(vgap=5, hgap=5)
        logo = os.path.join(globalvar.ICONDIR, "grass-64x64.png")
        logoBitmap = wx.StaticBitmap(
            infoTxt, wx.ID_ANY, wx.Bitmap(name=logo, type=wx.BITMAP_TYPE_PNG)
        )
        infoSizer.Add(
            logoBitmap, proportion=0, flag=wx.ALL | wx.ALIGN_CENTER, border=20
        )

        infoLabel = "GRASS GIS %s" % vInfo.get("version", _("unknown version"))
        if "x86_64" in vInfo.get("build_platform", ""):
            infoLabel += " (64bit)"
        info = StaticText(parent=infoTxt, id=wx.ID_ANY, label=infoLabel + os.linesep)
        info.SetFont(wx.Font(13, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
        info.SetForegroundColour(wx.Colour(35, 142, 35))
        infoSizer.Add(info, proportion=0, flag=wx.BOTTOM | wx.ALIGN_CENTER, border=1)

        team = StaticText(parent=infoTxt, label=_grassDevTeam(1999) + "\n")
        infoSizer.Add(team, proportion=0, flag=wx.BOTTOM | wx.ALIGN_CENTER, border=1)

        row = 0
        infoGridSizer.Add(
            StaticText(parent=infoTxt, id=wx.ID_ANY, label=_("Official GRASS site:")),
            pos=(row, 0),
            flag=wx.ALIGN_RIGHT,
        )

        infoGridSizer.Add(
            HyperLinkCtrl(
                parent=infoTxt, id=wx.ID_ANY, label="https://grass.osgeo.org"
            ),
            pos=(row, 1),
            flag=wx.ALIGN_LEFT,
        )

        row += 2
        infoGridSizer.Add(
            StaticText(parent=infoTxt, id=wx.ID_ANY, label="%s:" % _("Code Revision")),
            pos=(row, 0),
            flag=wx.ALIGN_RIGHT,
        )

        infoGridSizer.Add(
            HyperLinkCtrl(
                parent=infoTxt,
                id=wx.ID_ANY,
                label=vInfo.get("revision", "?"),
                URL="https://github.com/OSGeo/grass.git",
            ),
            pos=(row, 1),
            flag=wx.ALIGN_LEFT,
        )

        row += 1
        infoGridSizer.Add(
            StaticText(parent=infoTxt, id=wx.ID_ANY, label="%s:" % _("Build Date")),
            pos=(row, 0),
            flag=wx.ALIGN_RIGHT,
        )

        infoGridSizer.Add(
            StaticText(
                parent=infoTxt, id=wx.ID_ANY, label=vInfo.get("build_date", "?")
            ),
            pos=(row, 1),
            flag=wx.ALIGN_LEFT,
        )

        # show only basic info
        # row += 1
        # infoGridSizer.Add(
        #     item=wx.StaticText(
        #         parent=infoTxt, id=wx.ID_ANY, label="%s:" % _("GIS Library Revision")
        #     ),
        #     pos=(row, 0),
        #     flag=wx.ALIGN_RIGHT,
        # )
        #
        # infoGridSizer.Add(
        #     item=wx.StaticText(
        #         parent=infoTxt,
        #         id=wx.ID_ANY,
        #         label=vInfo["libgis_revision"]
        #         + " ("
        #         + vInfo["libgis_date"].split(" ")[0]
        #         + ")",
        #     ),
        #     pos=(row, 1),
        #     flag=wx.ALIGN_LEFT,
        # )

        row += 2
        infoGridSizer.Add(
            StaticText(parent=infoTxt, id=wx.ID_ANY, label="Python:"),
            pos=(row, 0),
            flag=wx.ALIGN_RIGHT,
        )

        infoGridSizer.Add(
            StaticText(parent=infoTxt, id=wx.ID_ANY, label=platform.python_version()),
            pos=(row, 1),
            flag=wx.ALIGN_LEFT,
        )

        row += 1
        infoGridSizer.Add(
            StaticText(parent=infoTxt, id=wx.ID_ANY, label="wxPython:"),
            pos=(row, 0),
            flag=wx.ALIGN_RIGHT,
        )

        infoGridSizer.Add(
            StaticText(parent=infoTxt, id=wx.ID_ANY, label=wx.__version__),
            pos=(row, 1),
            flag=wx.ALIGN_LEFT,
        )

        infoGridSizer.AddGrowableCol(0)
        infoGridSizer.AddGrowableCol(1)
        infoSizer.Add(infoGridSizer, proportion=1, flag=wx.EXPAND)

        row += 2
        infoGridSizer.Add(
            StaticText(parent=infoTxt, id=wx.ID_ANY, label="%s:" % _("Language")),
            pos=(row, 0),
            flag=wx.ALIGN_RIGHT,
        )
        self.langUsed = gs.gisenv().get("LANG", None)
        if not self.langUsed:
            import locale

            try:
                # Python >= 3.11
                loc = locale.getlocale()
            except AttributeError:
                loc = locale.getdefaultlocale()
            if loc == (None, None):
                self.langUsed = _("unknown")
            else:
                self.langUsed = "%s.%s" % (loc[0], loc[1])
        infoGridSizer.Add(
            StaticText(parent=infoTxt, id=wx.ID_ANY, label=self.langUsed),
            pos=(row, 1),
            flag=wx.ALIGN_LEFT,
        )

        infoTxt.SetSizer(infoSizer)
        infoSizer.Fit(infoTxt)

        return infoTxt

    def _pageCopyright(self):
        """Copyright information"""
        copyfile = os.path.join(os.getenv("GISBASE"), "COPYING")
        if os.path.exists(copyfile):
            copytext = Path(copyfile).read_text()
        else:
            copytext = _("%s file missing") % "COPYING"

        # put text into a scrolling panel
        copyrightwin = ScrolledPanel(self.aboutNotebook)
        copyrighttxt = TextCtrl(
            copyrightwin,
            id=wx.ID_ANY,
            value=copytext,
            style=wx.TE_MULTILINE | wx.TE_READONLY,
        )
        copyrightwin.SetAutoLayout(True)
        copyrightwin.sizer = wx.BoxSizer(wx.VERTICAL)
        copyrightwin.sizer.Add(
            copyrighttxt, proportion=1, flag=wx.EXPAND | wx.ALL, border=3
        )
        copyrightwin.SetSizer(copyrightwin.sizer)
        copyrightwin.Layout()
        copyrightwin.SetupScrolling()

        return copyrightwin

    def _pageLicense(self):
        """Licence about"""
        licfile = os.path.join(os.getenv("GISBASE"), "GPL.TXT")
        if os.path.exists(licfile):
            with open(licfile) as licenceFile:
                license = "".join(licenceFile.readlines())
        else:
            license = _("%s file missing") % "GPL.TXT"
        # put text into a scrolling panel
        licensewin = ScrolledPanel(self.aboutNotebook)
        licensetxt = TextCtrl(
            licensewin,
            id=wx.ID_ANY,
            value=license,
            style=wx.TE_MULTILINE | wx.TE_READONLY,
        )
        licensewin.SetAutoLayout(True)
        licensewin.sizer = wx.BoxSizer(wx.VERTICAL)
        licensewin.sizer.Add(
            licensetxt, proportion=1, flag=wx.EXPAND | wx.ALL, border=3
        )
        licensewin.SetSizer(licensewin.sizer)
        licensewin.Layout()
        licensewin.SetupScrolling()

        return licensewin

    def _pageCitation(self):
        """Citation information"""
        try:
            text = gs.read_command("g.version", flags="x")
        except CalledModuleError as error:
            text = _(
                "Unable to provide citation suggestion,"
                " see GRASS GIS website instead."
                " The error was: {0}"
            ).format(error)

        # put text into a scrolling panel
        window = ScrolledPanel(self.aboutNotebook)
        stat_text = TextCtrl(
            window, id=wx.ID_ANY, value=text, style=wx.TE_MULTILINE | wx.TE_READONLY
        )
        window.SetAutoLayout(True)
        window.sizer = wx.BoxSizer(wx.VERTICAL)
        window.sizer.Add(stat_text, proportion=1, flag=wx.EXPAND | wx.ALL, border=3)
        window.SetSizer(window.sizer)
        window.Layout()
        window.SetupScrolling()

        return window

    def _pageCredit(self):
        """Credit about"""
        # credits
        authfile = os.path.join(os.getenv("GISBASE"), "AUTHORS")
        if os.path.exists(authfile):
            with codecs.open(authfile, encoding="utf-8", mode="r") as authorsFile:
                authors = "".join(authorsFile.readlines())
        else:
            authors = _("%s file missing") % "AUTHORS"
        authorwin = ScrolledPanel(self.aboutNotebook)
        authortxt = TextCtrl(
            authorwin,
            id=wx.ID_ANY,
            value=authors,
            style=wx.TE_MULTILINE | wx.TE_READONLY,
        )
        authorwin.SetAutoLayout(True)
        authorwin.SetupScrolling()
        authorwin.sizer = wx.BoxSizer(wx.VERTICAL)
        authorwin.sizer.Add(authortxt, proportion=1, flag=wx.EXPAND | wx.ALL, border=3)
        authorwin.SetSizer(authorwin.sizer)
        authorwin.Layout()

        return authorwin

    def _pageContributors(self, extra=False):
        """Contributors info"""
        if extra:
            contribfile = os.path.join(os.getenv("GISBASE"), "contributors_extra.csv")
        else:
            contribfile = os.path.join(os.getenv("GISBASE"), "contributors.csv")
        if os.path.exists(contribfile):
            contribs = []
            errLines = []
            with codecs.open(contribfile, encoding="utf-8", mode="r") as contribFile:
                for line in contribFile.readlines()[1:]:
                    line = line.rstrip("\n")
                    try:
                        if extra:
                            name, email, country, rfc2_agreed = line.split(",")
                        else:
                            (
                                cvs_id,
                                name,
                                email,
                                country,
                                osgeo_id,
                                rfc2_agreed,
                                orcid,
                            ) = line.split(",")
                    except ValueError:
                        errLines.append(line)
                        continue
                    if extra:
                        contribs.append((name, email, country))
                    else:
                        contribs.append((name, email, country, osgeo_id, orcid))

            if errLines:
                GError(
                    parent=self,
                    message=_("Error when reading file '%s'.") % contribfile
                    + "\n\n"
                    + _("Lines:")
                    + " %s" % os.linesep.join(map(DecodeString, errLines)),
                )
        else:
            contribs = None

        contribwin = ScrolledPanel(self.aboutNotebook)
        contribwin.SetAutoLayout(True)
        contribwin.SetupScrolling()
        contribwin.sizer = wx.BoxSizer(wx.VERTICAL)

        if not contribs:
            contribtxt = StaticText(
                contribwin, id=wx.ID_ANY, label=_("%s file missing") % contribfile
            )
            contribwin.sizer.Add(
                contribtxt, proportion=1, flag=wx.EXPAND | wx.ALL, border=3
            )
        else:
            if extra:
                items = (_("Name"), _("E-mail"), _("Country"))
            else:
                items = (
                    _("Name"),
                    _("E-mail"),
                    _("Country"),
                    _("OSGeo_ID"),
                    _("ORCID"),
                )
            contribBox = wx.FlexGridSizer(cols=len(items), vgap=5, hgap=5)
            for item in items:
                text = StaticText(parent=contribwin, id=wx.ID_ANY, label=item)
                text.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
                contribBox.Add(text)
            for vals in sorted(contribs, key=itemgetter(0)):
                for item in vals:
                    contribBox.Add(
                        StaticText(parent=contribwin, id=wx.ID_ANY, label=item)
                    )
            contribwin.sizer.Add(
                contribBox, proportion=1, flag=wx.EXPAND | wx.ALL, border=3
            )

        contribwin.SetSizer(contribwin.sizer)
        contribwin.Layout()

        return contribwin

    def _pageTranslators(self):
        """Translators info"""
        translatorsfile = os.path.join(os.getenv("GISBASE"), "translators.csv")
        if os.path.exists(translatorsfile):
            translators = {}
            errLines = []
            with codecs.open(translatorsfile, encoding="utf-8", mode="r") as fd:
                for line in fd.readlines()[1:]:
                    line = line.rstrip("\n")
                    try:
                        name, email, languages = line.split(",")
                    except ValueError:
                        errLines.append(line)
                        continue
                    for language in languages.split(" "):
                        if language not in translators:
                            translators[language] = []
                        translators[language].append((name, email))

            if errLines:
                GError(
                    parent=self,
                    message=_("Error when reading file '%s'.") % translatorsfile
                    + "\n\n"
                    + _("Lines:")
                    + " %s" % os.linesep.join(map(DecodeString, errLines)),
                )
        else:
            translators = None

        translatorswin = ScrolledPanel(self.aboutNotebook)
        translatorswin.SetAutoLayout(True)
        translatorswin.SetupScrolling()
        translatorswin.sizer = wx.BoxSizer(wx.VERTICAL)

        if not translators:
            translatorstxt = StaticText(
                translatorswin,
                id=wx.ID_ANY,
                label=_("%s file missing") % "translators.csv",
            )
            translatorswin.sizer.Add(
                translatorstxt, proportion=1, flag=wx.EXPAND | wx.ALL, border=3
            )
        else:
            translatorsBox = wx.FlexGridSizer(cols=4, vgap=5, hgap=5)
            languages = sorted(translators.keys())
            tname = StaticText(parent=translatorswin, id=wx.ID_ANY, label=_("Name"))
            tname.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
            translatorsBox.Add(tname)
            temail = StaticText(parent=translatorswin, id=wx.ID_ANY, label=_("E-mail"))
            temail.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
            translatorsBox.Add(temail)
            tlang = StaticText(parent=translatorswin, id=wx.ID_ANY, label=_("Language"))
            tlang.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
            translatorsBox.Add(tlang)
            tnat = StaticText(parent=translatorswin, id=wx.ID_ANY, label=_("Nation"))
            tnat.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
            translatorsBox.Add(tnat)
            for lang in languages:
                for translator in translators[lang]:
                    name, email = translator
                    translatorsBox.Add(
                        StaticText(parent=translatorswin, id=wx.ID_ANY, label=name)
                    )
                    translatorsBox.Add(
                        StaticText(parent=translatorswin, id=wx.ID_ANY, label=email)
                    )
                    translatorsBox.Add(
                        StaticText(parent=translatorswin, id=wx.ID_ANY, label=lang)
                    )
                    flag = os.path.join(
                        globalvar.ICONDIR, "flags", "%s.png" % lang.lower()
                    )
                    if os.path.exists(flag):
                        flagBitmap = wx.StaticBitmap(
                            translatorswin,
                            wx.ID_ANY,
                            wx.Bitmap(name=flag, type=wx.BITMAP_TYPE_PNG),
                        )
                        translatorsBox.Add(flagBitmap)
                    else:
                        translatorsBox.Add(
                            StaticText(parent=translatorswin, id=wx.ID_ANY, label=lang)
                        )

            translatorswin.sizer.Add(
                translatorsBox, proportion=1, flag=wx.EXPAND | wx.ALL, border=3
            )

        translatorswin.SetSizer(translatorswin.sizer)
        translatorswin.Layout()

        return translatorswin

    def _langString(self, k, v):
        """Return string for the status of translation"""
        allStr = "%s :" % k.upper()
        try:
            allStr += _("   %d translated") % v["good"]
        except KeyError:
            pass
        try:
            allStr += _("   %d fuzzy") % v["fuzzy"]
        except KeyError:
            pass
        try:
            allStr += _("   %d untranslated") % v["bad"]
        except KeyError:
            pass
        return allStr

    def _langBox(self, par, k, v):
        """Return box"""
        langBox = wx.FlexGridSizer(cols=4, vgap=5, hgap=5)
        tkey = StaticText(parent=par, id=wx.ID_ANY, label=k.upper())
        langBox.Add(tkey)
        try:
            tgood = StaticText(
                parent=par, id=wx.ID_ANY, label=_("%d translated") % v["good"]
            )
            tgood.SetForegroundColour(wx.Colour(35, 142, 35))
            langBox.Add(tgood)
        except KeyError:
            tgood = StaticText(parent=par, id=wx.ID_ANY, label="")
            langBox.Add(tgood)
        try:
            tfuzzy = StaticText(
                parent=par, id=wx.ID_ANY, label=_("   %d fuzzy") % v["fuzzy"]
            )
            tfuzzy.SetForegroundColour(wx.Colour(255, 142, 0))
            langBox.Add(tfuzzy)
        except KeyError:
            tfuzzy = StaticText(parent=par, id=wx.ID_ANY, label="")
            langBox.Add(tfuzzy)
        try:
            tbad = StaticText(
                parent=par, id=wx.ID_ANY, label=_("   %d untranslated") % v["bad"]
            )
            tbad.SetForegroundColour(wx.Colour(255, 0, 0))
            langBox.Add(tbad)
        except KeyError:
            tbad = StaticText(parent=par, id=wx.ID_ANY, label="")
            langBox.Add(tbad)
        return langBox

    def _langPanel(self, lang, js):
        """Create panel for each languages"""
        text = self._langString(lang, js["total"])
        panel = wx.CollapsiblePane(
            self.statswin,
            -1,
            label=text,
            style=wx.CP_DEFAULT_STYLE | wx.CP_NO_TLW_RESIZE,
        )
        panel.Bind(wx.EVT_COLLAPSIBLEPANE_CHANGED, self.OnPaneChanged)
        win = panel.GetPane()
        # TODO IT DOESN'T WORK
        # TO ADD ONLY WHEN TAB IS OPENED
        # if lang == self.langUsed.split('_')[0]:
        # panel.Collapse(False)
        # else:
        # panel.Collapse(True)
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        for k, v in js.items():
            if k not in {"total", "name"}:
                box = self._langBox(win, k, v)
                pageSizer.Add(box, proportion=1, flag=wx.EXPAND | wx.ALL, border=3)

        win.SetSizer(pageSizer)
        pageSizer.SetSizeHints(win)

        return panel

    def OnPaneChanged(self, evt):
        """Redo the layout"""
        # TODO better to test on Windows
        self.statswin.SetupScrolling(scrollToTop=False)

    def _pageStats(self):
        """Translation statistics info"""
        fname = "translation_status.json"
        statsfile = os.path.join(os.getenv("GISBASE"), fname)
        if os.path.exists(statsfile):
            import json

            with open(statsfile) as statsFile:
                jsStats = json.load(statsFile)
        else:
            jsStats = None
        self.statswin = ScrolledPanel(self.aboutNotebook)
        self.statswin.SetAutoLayout(True)

        if not jsStats:
            Debug.msg(5, _("File <%s> not found") % fname)
            statsSizer = wx.BoxSizer(wx.VERTICAL)
            statstext = StaticText(
                self.statswin, id=wx.ID_ANY, label=_("%s file missing") % fname
            )
            statsSizer.Add(statstext, proportion=1, flag=wx.EXPAND | wx.ALL, border=3)
        else:
            languages = sorted(jsStats["langs"].keys())

            statsSizer = wx.BoxSizer(wx.VERTICAL)
            for lang in languages:
                v = jsStats["langs"][lang]
                panel = self._langPanel(lang, v)
                statsSizer.Add(panel)

        self.statswin.SetSizer(statsSizer)
        self.statswin.SetupScrolling(scroll_x=False, scroll_y=True)
        self.statswin.Layout()
        self.statswin.Fit()
        return self.statswin

    def OnCloseWindow(self, event):
        """Close window"""
        self.Close()


def extract_md_content(html_string, base_url):
    """Extract only relevant part of the mkdocs generated html as string.

    Relevant content is in a div with class="md-content".
    """

    class SimpleDivParser(HTMLParser):
        def __init__(self):
            super().__init__()
            self.recording = False
            self.depth = 0
            self.extracted_data = []
            self.base_url = base_url
            self.target_class = "md-content"

        def handle_starttag(self, tag, attrs):
            attr_dict = dict(attrs)

            # Convert relative URLs to absolute
            # to be able to display images and use links
            if tag in ["img", "a"]:
                if "src" in attr_dict and not bool(urlparse(attr_dict["src"]).netloc):
                    attr_dict["src"] = urljoin(self.base_url, attr_dict["src"])
                if "href" in attr_dict and not bool(urlparse(attr_dict["href"]).netloc):
                    attr_dict["href"] = urljoin(self.base_url, attr_dict["href"])

            attr_str = " ".join(f'{k}="{v}"' for k, v in attr_dict.items())

            if tag == "div" and attr_dict.get("class") == self.target_class:
                self.recording = True
                self.extracted_data.append(f"<div {attr_str}>")
                self.depth = 1
            elif self.recording:
                self.extracted_data.append(
                    f"<{tag} {attr_str}>" if attr_str else f"<{tag}>"
                )
                if tag == "div":
                    self.depth += 1

        def handle_endtag(self, tag):
            if self.recording:
                self.extracted_data.append(f"</{tag}>")
                if tag == "div":
                    self.depth -= 1
                    if self.depth == 0:
                        self.recording = False

        def handle_data(self, data):
            if self.recording:
                self.extracted_data.append(data)

    parser = SimpleDivParser()
    parser.feed(html_string)
    return "".join(parser.extracted_data)


class HelpWindow(HtmlWindow):
    """This panel holds the text from GRASS docs.

    GISBASE must be set in the environment to find the html docs dir.
    """

    def __init__(self, parent, command, text, **kwargs):
        """If command is given, the corresponding HTML help
        file will be presented, with all links pointing to absolute
        paths of local files.

        If 'text' is given, it must be the HTML text to be presented
        in the Panel.
        """
        self.parent = parent
        HtmlWindow.__init__(self, parent=parent, **kwargs)

        self.loaded = False
        self.history = []
        self.historyIdx = 0
        self.markdown = True
        # check if mkdocs is used (add slash at the end)
        self.fspath = os.path.join(os.getenv("GISBASE"), "docs", "mkdocs", "site", "")
        if not os.path.exists(self.fspath):
            self.markdown = False
            self.fspath = os.path.join(os.getenv("GISBASE"), "docs", "html", "")

        self._setFont()
        self.SetBorders(10)

        self.loaded = False
        if text:
            self.SetPage(text)
            self.loaded = True

    def _setFont(self):
        """Set font size/face"""
        font_face_name = UserSettings.Get(
            group="appearance",
            key="manualPageFont",
            subkey="faceName",
        )
        font_size = UserSettings.Get(
            group="appearance",
            key="manualPageFont",
            subkey="pointSize",
        )
        if font_size:
            self.SetStandardFonts(
                size=font_size,
                normal_face=font_face_name,
                fixed_face=font_face_name,
            )

    def OnLinkClicked(self, linkinfo):
        url = linkinfo.GetHref()
        if url[:4] != "http":
            url = os.path.join(self.fspath, url)
        self.history.append(url)
        self.historyIdx += 1
        self.parent.OnHistory()

        super().OnLinkClicked(linkinfo)

    def OnOpeningURL(self, type, url):
        """A workaround reloading the extracted mkdocs tool page content"""
        if self.markdown and url.startswith(self.fspath) and url.endswith(".html"):
            wx.CallAfter(self.LoadPage, url)
        return (wx.html.HTML_OPEN, url)

    def LoadPage(self, path):
        if not self.markdown:
            super().LoadPage(path)
            return
        try:
            # extract only certain div content
            html = Path(path).read_text(encoding="utf-8")
            self.SetPage(extract_md_content(html, self.fspath))
            self.loaded = True
        except OSError:
            pass


class HelpPanel(wx.Panel):
    def __init__(self, parent, command="index", text=None, **kwargs):
        self.command = command
        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY)

        self.content = HelpWindow(self, command, text)

        self.btnNext = Button(parent=self, id=wx.ID_ANY, label=_("&Next"))
        self.btnNext.Enable(False)
        self.btnPrev = Button(parent=self, id=wx.ID_ANY, label=_("&Previous"))
        self.btnPrev.Enable(False)

        self.btnNext.Bind(wx.EVT_BUTTON, self.OnNext)
        self.btnPrev.Bind(wx.EVT_BUTTON, self.OnPrev)

        self._layout()

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)

        btnSizer.Add(self.btnPrev, proportion=0, flag=wx.ALL, border=5)
        btnSizer.Add(wx.Size(1, 1), proportion=1)
        btnSizer.Add(self.btnNext, proportion=0, flag=wx.ALL, border=5)

        sizer.Add(self.content, proportion=1, flag=wx.EXPAND)
        sizer.Add(btnSizer, proportion=0, flag=wx.EXPAND)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def LoadPage(self, path=None):
        """Load page"""
        if not path:
            path = self.GetFile()
        self.content.history.append(path)
        self.content.LoadPage(path)

    def GetFile(self):
        """Get HTML file"""
        fMan = os.path.join(self.content.fspath, self.command + ".html")
        if os.path.isfile(fMan):
            return fMan

        # check also addons
        faMan = os.path.join(
            os.getenv("GRASS_ADDON_BASE"), "docs", "html", self.command + ".html"
        )
        if os.getenv("GRASS_ADDON_BASE") and os.path.isfile(faMan):
            return faMan

        return None

    def IsLoaded(self):
        return self.content.loaded

    def OnHistory(self):
        """Update buttons"""
        nH = len(self.content.history)
        iH = self.content.historyIdx
        if iH == nH - 1:
            self.btnNext.Enable(False)
        elif iH > -1:
            self.btnNext.Enable(True)
        if iH < 1:
            self.btnPrev.Enable(False)
        else:
            self.btnPrev.Enable(True)

    def OnNext(self, event):
        """Load next page"""
        self.content.historyIdx += 1
        idx = self.content.historyIdx
        path = self.content.history[idx]
        self.content.LoadPage(path)
        self.OnHistory()

        event.Skip()

    def OnPrev(self, event):
        """Load previous page"""
        self.content.historyIdx -= 1
        idx = self.content.historyIdx
        path = self.content.history[idx]
        self.content.LoadPage(path)
        self.OnHistory()

        event.Skip()


def ShowAboutDialog(prgName, startYear):
    """Displays About window.

    :param prgName: name of the program
    :param startYear: the first year of existence of the program
    """
    info = AboutDialogInfo()

    info.SetIcon(
        wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
    )
    info.SetName(prgName)
    info.SetWebSite("https://grass.osgeo.org")
    info.SetDescription(
        _grassDevTeam(startYear)
        + "\n\n"
        + "\n".join(
            textwrap.wrap(
                "This program is free software under the GNU General Public License"
                "(>=v2). Read the file COPYING that comes with GRASS for details.",
                75,
            )
        )
    )

    AboutBox(info)


def _grassDevTeam(start):
    try:
        end = gs.version()["date"]
    except KeyError:
        sys.stderr.write(_("Unable to get GRASS version\n"))

        from datetime import date

        end = date.today().year

    return "%(c)s %(start)s-%(end)s by the GRASS Development Team" % {
        "c": chr(169),
        "start": start,
        "end": end,
    }
