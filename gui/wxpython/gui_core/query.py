"""
@package gui_core.query

@brief wxGUI query dialog

Classes:
 - query::QueryDialog

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import json
import os
import threading
import urllib.error
import urllib.request

import wx

from gui_core.treeview import TreeListView
from gui_core.wrap import Button, StaticText, Menu, NewId
from core.treemodel import TreeModel, DictNode

from grass.pydispatch.signal import Signal


def _format_query_data_as_text(data):
    """Convert query result data (list of dicts) into a plain-text summary.

    The output is suitable for inclusion in a prompt sent to an LLM.
    """
    lines = []
    for part in data:
        for key, value in part.items():
            if isinstance(value, dict):
                lines.append(f"{key}:")
                for sub_key, sub_value in value.items():
                    lines.append(f"  {sub_key}: {sub_value}")
            else:
                lines.append(f"{key}: {value}")
    return "\n".join(lines)


def _call_llm_api(prompt):
    """Send a prompt to an LLM API and return the response text.

    Tries Google Gemini first (GEMINI_API_KEY), then OpenAI (OPENAI_API_KEY).
    Uses only Python standard library modules to avoid external dependencies.

    :param prompt: the text prompt to send
    :returns: response text from the LLM
    :raises OSError: if no API key is configured
    :raises RuntimeError: if the API request fails
    """
    gemini_key = os.environ.get("GEMINI_API_KEY")
    openai_key = os.environ.get("OPENAI_API_KEY")

    if gemini_key:
        return _call_gemini(prompt, gemini_key)
    if openai_key:
        return _call_openai(prompt, openai_key)

    msg = (
        "No LLM API key found. Set GEMINI_API_KEY or OPENAI_API_KEY "
        "in your environment before starting GRASS."
    )
    raise OSError(msg)


def _call_gemini(prompt, api_key):
    """Call the Google Gemini REST API.

    :param prompt: text prompt
    :param api_key: Gemini API key
    :returns: generated text
    """
    url = (
        "https://generativelanguage.googleapis.com/v1beta/models/"
        f"gemini-2.0-flash:generateContent?key={api_key}"
    )
    payload = {"contents": [{"parts": [{"text": prompt}]}]}
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url, data=data, headers={"Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            body = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        msg = f"Gemini API returned HTTP {exc.code}: {exc.read().decode('utf-8', errors='replace')}"
        raise RuntimeError(msg) from exc
    except urllib.error.URLError as exc:
        msg = f"Network error contacting Gemini API: {exc.reason}"
        raise RuntimeError(msg) from exc

    try:
        return body["candidates"][0]["content"]["parts"][0]["text"]
    except (KeyError, IndexError) as exc:
        msg = f"Unexpected Gemini API response structure: {body}"
        raise RuntimeError(msg) from exc


def _call_openai(prompt, api_key):
    """Call the OpenAI chat completions REST API.

    :param prompt: text prompt
    :param api_key: OpenAI API key
    :returns: generated text
    """
    url = "https://api.openai.com/v1/chat/completions"
    payload = {
        "model": "gpt-4o-mini",
        "messages": [{"role": "user", "content": prompt}],
        "max_tokens": 1024,
    }
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Bearer {api_key}",
        },
    )
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            body = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        msg = f"OpenAI API returned HTTP {exc.code}: {exc.read().decode('utf-8', errors='replace')}"
        raise RuntimeError(msg) from exc
    except urllib.error.URLError as exc:
        msg = f"Network error contacting OpenAI API: {exc.reason}"
        raise RuntimeError(msg) from exc

    try:
        return body["choices"][0]["message"]["content"]
    except (KeyError, IndexError) as exc:
        msg = f"Unexpected OpenAI API response structure: {body}"
        raise RuntimeError(msg) from exc


class QueryDialog(wx.Dialog):
    def __init__(self, parent, data=None):
        wx.Dialog.__init__(
            self,
            parent,
            id=wx.ID_ANY,
            title=_("Query results"),
            size=(520, 480),
            style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        )
        # send query output to console
        self.redirectOutput = Signal("QueryDialog.redirectOutput")

        self.data = data

        self.panel = wx.Panel(self, id=wx.ID_ANY)
        self.mainSizer = wx.BoxSizer(wx.VERTICAL)

        # --- Notebook with two tabs ---
        self.notebook = wx.Notebook(self.panel, id=wx.ID_ANY)

        # Tab 1: Query Results (original tree view)
        self._queryPage = wx.Panel(self.notebook)
        querySizer = wx.BoxSizer(wx.VERTICAL)

        helpText = StaticText(
            self._queryPage,
            wx.ID_ANY,
            label=_("Right click to copy selected values to clipboard."),
        )
        helpText.SetForegroundColour(
            wx.SystemSettings.GetColour(wx.SYS_COLOUR_GRAYTEXT)
        )
        querySizer.Add(helpText, proportion=0, flag=wx.ALL, border=5)

        self._colNames = [_("Feature"), _("Value")]
        self._model = QueryTreeBuilder(self.data, column=self._colNames[1])
        self.tree = TreeListView(
            model=self._model,
            parent=self._queryPage,
            columns=self._colNames,
            style=wx.TR_DEFAULT_STYLE
            | wx.TR_HIDE_ROOT
            | wx.TR_FULL_ROW_HIGHLIGHT
            | wx.TR_MULTIPLE,
        )

        self.tree.SetColumnWidth(0, 220)
        self.tree.SetColumnWidth(1, 1000)
        self.tree.ExpandAll()
        self.tree.RefreshItems()
        self.tree.contextMenu.connect(self.ShowContextMenu)
        querySizer.Add(self.tree, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)

        self._queryPage.SetSizer(querySizer)
        self.notebook.AddPage(self._queryPage, _("Query Results"))

        # Tab 2: AI Terrain Agent
        self._aiPage = wx.Panel(self.notebook)
        aiSizer = wx.BoxSizer(wx.VERTICAL)

        aiDesc = StaticText(
            self._aiPage,
            wx.ID_ANY,
            label=_(
                "Generate a human-readable terrain description from the "
                "queried map data using an AI language model."
            ),
        )
        aiDesc.Wrap(460)
        aiDesc.SetForegroundColour(
            wx.SystemSettings.GetColour(wx.SYS_COLOUR_GRAYTEXT)
        )
        aiSizer.Add(aiDesc, proportion=0, flag=wx.ALL, border=5)

        self._aiStatus = StaticText(self._aiPage, wx.ID_ANY, label="")
        aiSizer.Add(self._aiStatus, proportion=0, flag=wx.LEFT | wx.RIGHT, border=5)

        self._aiText = wx.TextCtrl(
            self._aiPage,
            wx.ID_ANY,
            style=wx.TE_MULTILINE | wx.TE_READONLY | wx.TE_WORDWRAP,
        )
        aiSizer.Add(self._aiText, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)

        self._aiButton = Button(
            self._aiPage, id=wx.ID_ANY, label=_("Describe Terrain")
        )
        self._aiButton.Bind(wx.EVT_BUTTON, self.OnGenerateAI)

        aiButtonSizer = wx.BoxSizer(wx.HORIZONTAL)
        aiButtonSizer.AddStretchSpacer(1)
        aiButtonSizer.Add(
            self._aiButton, proportion=0, flag=wx.EXPAND | wx.ALL, border=0
        )
        aiSizer.Add(aiButtonSizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        self._aiPage.SetSizer(aiSizer)
        self.notebook.AddPage(self._aiPage, _("AI Terrain Agent"))

        self.mainSizer.Add(
            self.notebook, proportion=1, flag=wx.EXPAND | wx.ALL, border=5
        )

        # --- Bottom bar (shared across tabs) ---
        close = Button(self.panel, id=wx.ID_CLOSE)
        close.Bind(wx.EVT_BUTTON, lambda event: self.Close())
        copy = Button(self.panel, id=wx.ID_ANY, label=_("Copy all to clipboard"))
        copy.Bind(wx.EVT_BUTTON, self.Copy)
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.redirect = wx.CheckBox(self.panel, label=_("Redirect to console"))
        self.redirect.SetValue(False)
        self.redirect.Bind(
            wx.EVT_CHECKBOX, lambda evt: self._onRedirect(evt.IsChecked())
        )

        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(self.redirect, proportion=0, flag=wx.EXPAND | wx.RIGHT, border=5)
        hbox.AddStretchSpacer(1)
        hbox.Add(copy, proportion=0, flag=wx.EXPAND | wx.RIGHT, border=5)
        hbox.Add(close, proportion=0, flag=wx.EXPAND | wx.ALL, border=0)

        self.mainSizer.Add(hbox, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)
        self.panel.SetSizer(self.mainSizer)
        self.mainSizer.Fit(self.panel)
        # for Windows
        self.SendSizeEvent()

    def SetData(self, data):
        state = self.tree.GetExpansionState()
        self.data = data
        self._model = QueryTreeBuilder(self.data, column=self._colNames[1])
        self.tree.SetModel(self._model)
        self.tree.SetExpansionState(state)

        if self.redirect.IsChecked():
            self.redirectOutput.emit(output=self._textToRedirect())

        # Reset AI tab for the new query location.
        self._aiText.SetValue("")
        self._aiStatus.SetLabel("")
        self._aiButton.Enable()

    def Copy(self, event):
        text = printResults(self._model, self._colNames[1])
        self._copyText(text)

    def ShowContextMenu(self, node):
        """Show context menu.

        Menu for copying distinguishes single and multiple selection.
        """
        nodes = self.tree.GetSelected()
        if not nodes:
            return

        menu = Menu()
        texts = []
        if len(nodes) > 1:
            values = [
                (node.label, node.data[self._colNames[1]] if node.data else "")
                for node in nodes
            ]
            col1 = "\n".join([val[1] for val in values if val[1]])
            col2 = "\n".join([val[0] for val in values if val[0]])
            table = "\n".join([val[0] + ": " + val[1] for val in values])
            texts.extend(
                (
                    (_("Copy from '%s' column") % self._colNames[1], col1),
                    (_("Copy from '%s' column") % self._colNames[0], col2),
                    (_("Copy selected lines"), table),
                )
            )
        else:
            label1 = nodes[0].label
            texts.append((_("Copy '%s'") % self._cutLabel(label1), label1))
            col1 = self._colNames[1]
            if nodes[0].data and col1 in nodes[0].data and nodes[0].data[col1]:
                label2 = nodes[0].data[col1]
                texts.insert(0, (_("Copy '%s'") % self._cutLabel(label2), label2))
                texts.append((_("Copy line"), label1 + ": " + label2))

        ids = []
        for text in texts:
            id = NewId()
            ids.append(id)
            self.Bind(
                wx.EVT_MENU,
                lambda evt, t=text[1], id=id: self._copyText(t),  # noqa: A006
                id=id,
            )

            menu.Append(id, text[0])

        # show the popup menu
        self.PopupMenu(menu)
        menu.Destroy()
        for id in ids:
            self.Unbind(wx.EVT_MENU, id=id)

    def OnGenerateAI(self, event):
        """Request an AI-generated terrain description in a background thread."""
        if not self.data:
            self._aiStatus.SetLabel(_("No query data available."))
            return

        self._aiButton.Disable()
        self._aiText.SetValue("")
        self._aiStatus.SetLabel(_("Contacting AI service..."))

        query_text = _format_query_data_as_text(self.data)
        prompt = (
            "You are a geospatial terrain analyst. A user queried a location "
            "in GRASS GIS and received the following map data:\n\n"
            f"{query_text}\n\n"
            "Provide a concise, informative, human-readable description of "
            "the terrain at this location. Explain what the values mean in "
            "practical terms (e.g. slope steepness, land cover type, "
            "elevation context). Keep the response under 200 words."
        )

        thread = threading.Thread(target=self._queryAI, args=(prompt,), daemon=True)
        thread.start()

    def _queryAI(self, prompt):
        """Run the LLM API call in a background thread.

        Results are posted back to the GUI via wx.CallAfter.
        """
        try:
            result = _call_llm_api(prompt)
            wx.CallAfter(self._onAIDone, result, error=None)
        except Exception as exc:
            wx.CallAfter(self._onAIDone, None, error=str(exc))

    def _onAIDone(self, result, error):
        """Handle the completed AI response on the main GUI thread."""
        self._aiButton.Enable()
        if error:
            self._aiStatus.SetLabel(_("Error"))
            self._aiText.SetValue(error)
        else:
            self._aiStatus.SetLabel(_("Description generated successfully."))
            self._aiText.SetValue(result)

    def _onRedirect(self, redirect):
        """Emits instructions to redirect query results.

        :param redirect: True to start redirecting, False to stop
        """
        if redirect:
            self.redirectOutput.emit(output=_("Query results:"), style="cmd")
            self.redirectOutput.emit(output=self._textToRedirect())
        else:
            self.redirectOutput.emit(output=_(" "), style="cmd")

    def _textToRedirect(self):
        text = printResults(self._model, self._colNames[1])
        text += "\n" + "-" * 50 + "\n"
        return text

    def _cutLabel(self, label):
        limit = 15
        if len(label) > limit:
            return label[:limit] + "..."

        return label

    def _copyText(self, text):
        """Helper function for copying"""
        if wx.TheClipboard.Open():
            do = wx.TextDataObject()
            do.SetText(text)
            wx.TheClipboard.SetData(do)
            wx.TheClipboard.Close()

    def OnClose(self, event):
        if self.redirect.IsChecked():
            self._onRedirect(False)
        self.Destroy()
        event.Skip()


def QueryTreeBuilder(data, column):
    """Builds tree model from query results.

    :param data: query results as a dictionary
    :param column: column name

    :return: tree model
    """

    def addNode(parent, data, model):
        for k, v in data.items():
            if isinstance(v, dict):
                node = model.AppendNode(parent=parent, data={"label": k})
                addNode(parent=node, data=v, model=model)
            else:
                if not isinstance(v, str):
                    v = str(v)
                node = model.AppendNode(parent=parent, data={"label": k, column: v})

    model = TreeModel(DictNode)
    for part in data:
        addNode(parent=model.root, data=part, model=model)

    return model


def printResults(model, valueCol):
    """Print all results to string.

    :param model: results tree model
    :param valueCol: column name with value to be printed
    """

    def printTree(node, textList, valueCol, indent=0):
        if node.data.get(valueCol, "") or node.children:
            textList.append(
                indent * " " + node.label + ": " + node.data.get(valueCol, "")
            )
        for child in node.children:
            printTree(
                node=child, textList=textList, valueCol=valueCol, indent=indent + 2
            )

    textList = []
    for child in model.root.children:
        printTree(node=child, textList=textList, valueCol=valueCol)
    return "\n".join(textList)


def PrepareQueryResults(coordinates, result):
    """Prepare query results as a Query dialog input.

    Adds coordinates, improves vector results tree structure.
    """
    data = [{_("east, north"): ", ".join(map(str, coordinates))}]
    for part in result:
        if "Map" in part:
            itemText = part["Map"]
            if "Mapset" in part:
                itemText += "@" + part["Mapset"]
                del part["Mapset"]
            del part["Map"]
            if part:
                data.append({itemText: part})
            else:
                data.append({itemText: _("Nothing found")})
        else:
            # remove often empty raster label and color pixel info
            for key in part:
                for empty_keys in ("label", "color"):
                    if empty_keys in part[key] and not part[key][empty_keys]:
                        del part[key][empty_keys]
            data.append(part)
    return data


def test():
    app = wx.App()
    from grass.script import vector as gvect
    from grass.script import raster as grast

    testdata1 = grast.raster_what(
        map=("elevation_shade@PERMANENT", "landclass96"),
        coord=[(638509.051416, 224742.348346)],
        localized=True,
    )

    testdata2 = gvect.vector_what(
        map=("firestations", "bridges"),
        coord=(633177.897487, 221352.921257),
        distance=10,
    )

    testdata = testdata1 + testdata2
    data = PrepareQueryResults(
        coordinates=(638509.051416, 224742.348346), result=testdata
    )
    frame = QueryDialog(parent=None, data=data)
    frame.ShowModal()
    frame.Destroy()
    app.MainLoop()


if __name__ == "__main__":
    test()
