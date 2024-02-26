"""
@package startup.locdownload

@brief GRASS Location Download Management

Classes:
 - LocationDownloadPanel
 - LocationDownloadDialog
 - DownloadError

(C) 2017 by Vaclav Petras the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail com>
"""

import os
import sys
import shutil
import textwrap
import time

import wx
from wx.lib.newevent import NewEvent

from grass.pydispatch.signal import Signal
from grass.script.utils import try_rmdir, legalize_vector_name
from grass.utils.download import download_and_extract, name_from_url, DownloadError
from grass.grassdb.checks import is_location_valid
from grass.script.setup import set_gui_path

set_gui_path()

# flake8: noqa: E402
from core.debug import Debug
from core.gthread import gThread
from gui_core.wrap import Button, StaticText

# flakes8: qa


# TODO: labels (and descriptions) translatable?
LOCATIONS = [
    {
        "label": "Complete North Carolina dataset",
        "url": "https://grass.osgeo.org/sampledata/north_carolina/nc_spm_08_grass7.tar.gz",  # noqa: E501
    },
    {
        "label": "Basic North Carolina dataset",
        "url": "https://grass.osgeo.org/sampledata/north_carolina/nc_basic_spm_grass7.tar.gz",  # noqa: E501
    },
    {
        "label": "World dataset in LatLong/WGS84",
        "url": "https://grass.osgeo.org/sampledata/worldlocation.tar.gz",
    },
    {
        "label": "Spearfish (SD) dataset",
        "url": "https://grass.osgeo.org/sampledata/spearfish_grass70data-0.3.tar.gz",
    },
    {
        "label": "Piemonte, Italy dataset",
        "url": "https://grass.osgeo.org/sampledata/grassdata_piemonte_utm32n_wgs84_grass7.tar.gz",  # noqa: E501
    },
    {
        "label": "Slovakia 3D precipitation voxel dataset",
        "url": "https://grass.osgeo.org/sampledata/slovakia3d_grass7.tar.gz",
    },
    {
        "label": "Fire simulation sample data",
        "url": "https://grass.osgeo.org/sampledata/fire_grass6data.tar.gz",
    },
    {
        "label": "GISMentors dataset, Czech Republic",
        "url": "https://www.training.gismentors.cz/geodata/grass/gismentors.zip",
    },
    {
        "label": "Natural Earth Dataset in WGS84",
        "url": "https://zenodo.org/records/13370131/files/natural_earth_dataset.zip",
        "size": "121.3 MB",
        "epsg": "4326",
        "license": "ODC Public Domain Dedication and License 1.0",
        "maintainer": "Brendan Harmon (brendan.harmon@gmail.com)",
    },
]


class RedirectText:
    def __init__(self, window):
        self.window = window

    def write(self, string):
        try:
            if self.window.message:
                string = self._wrap_string(string)
                height = self._get_height(string)
                wx.CallAfter(self.window.message.SetLabel, string)
                # Show Data Catalog info bar messsage
                wx.CallAfter(self.window.ShowInfoBarMessage, string)
                self._resize(height)
        except (RuntimeError, AttributeError):
            pass

    def flush(self):
        pass

    def _wrap_string(self, string, width=40):
        """Wrap string

        :param str string: input string
        :param int width: maximum length allowed of the wrapped lines

        :return str: newline-separated string
        """
        wrapper = textwrap.TextWrapper(width=width)
        return wrapper.fill(text=string)

    def _get_height(self, string):
        """Get widget new height

        :param str string: input string

        :return int: widget height
        """
        n_lines = string.count("\n")
        attr = self.window.message.GetClassDefaultAttributes()
        font_size = attr.font.GetPointSize()
        return int((n_lines + 2) * font_size // 0.75)  # 1 px = 0.75 pt

    def _resize(self, height=-1):
        """Resize widget height

        :param int height: widget height
        """
        wx.CallAfter(self.window.message.GetParent().SetMinSize, (-1, -1))
        wx.CallAfter(self.window.message.SetMinSize, (-1, height))
        wx.CallAfter(
            self.window.message.GetParent().parent.sizer.Fit,
            self.window.message.GetParent().parent,
        )


# based on
# https://blog.shichao.io/2012/10/04/
# progress_speed_indicator_for_urlretrieve_in_python.html
def reporthook(count, block_size, total_size):
    global start_time
    if count == 0:
        start_time = time.time()
        sys.stdout.write(
            _("Download in progress, wait until it is finished 0%"),
        )
        return
    if count % 100 != 0:  # be less verbose
        return
    duration = time.time() - start_time
    progress_size = int(count * block_size)
    speed = int(progress_size / (1024 * duration))
    percent = int(count * block_size * 100 / total_size)
    sys.stdout.write(
        _(
            "Download in progress, wait until it is finished "
            "{0}%, {1} MB, {2} KB/s, {3:.0f} seconds passed"
        ).format(
            percent,
            progress_size / (1024 * 1024),
            speed,
            duration,
        ),
    )


def download_location(url, name, database):
    """Wrapper to return DownloadError by value

    It also moves the location directory to the database.
    """
    try:
        # TODO: the unpacking could go right to the path (but less
        # robust) or replace copytree here with move
        directory = download_and_extract(source=url, reporthook=reporthook)
        destination = os.path.join(database, name)
        if not is_location_valid(directory):
            return _("Downloaded project is not valid")
        shutil.copytree(src=directory, dst=destination)
        try_rmdir(directory)
    except DownloadError as error:
        return error
    return None


def location_name_from_url(url):
    """Create location name from URL"""
    return legalize_vector_name(name_from_url(url))


DownloadDoneEvent, EVT_DOWNLOAD_DONE = NewEvent()


class LocationDownloadPanel(wx.Panel):
    """Panel to select and initiate downloads of locations.

    Has a place to report errors to user and also any potential problems
    before the user hits the button.

    In the future, it can potentially show also some details about what
    will be downloaded. The choice widget can be also replaced.

    For the future, there can be multiple panels with different methods
    or sources, e.g. direct input of URL. These can be in separate tabs
    of one panel (perhaps sharing the common background download and
    message logic).
    """

    def __init__(self, parent, database, locations=LOCATIONS):
        """

        :param database: directory with G database to download to
        :param locations: list of dictionaries with label and url
        """
        wx.Panel.__init__(self, parent=parent)

        self.parent = parent
        self._last_downloaded_location_name = None
        self._download_in_progress = False
        self.database = database
        self.locations = locations
        self._abort_btn_label = _("Abort")
        self._abort_btn_tooltip = _("Abort download location")
        self._infobar_message_btn_id = wx.NewIdRef()
        self._infobar_message_btns = {
            "addBtn": None,
            "removeBtn": None,
        }

        self.label = StaticText(
            parent=self, label=_("Select sample project to download:")
        )

        choices = [item["label"] for item in self.locations]
        self.choice = wx.Choice(parent=self, choices=choices)

        self.choice.Bind(wx.EVT_CHOICE, self.OnChangeChoice)
        if hasattr(self.parent, "download_button"):
            self.parent.download_button.Bind(wx.EVT_BUTTON, self.OnDownload)
        # TODO: add button for a link to an associated website?
        # TODO: add thumbnail for each location?

        # TODO: messages copied from gis_set.py, need this as API?
        self.message = StaticText(parent=self, size=(-1, 50))
        sys.stdout = RedirectText(window=self)

        # It is not clear if all wx versions supports color, so try-except.
        # The color itself may not be correct for all platforms/system settings
        # but in
        # http://xoomer.virgilio.it/infinity77/wxPython/Widgets/wx.SystemSettings.html
        # there is no 'warning' color.
        try:
            self.message.SetForegroundColour(wx.Colour(255, 0, 0))
        except AttributeError:
            pass

        self._layout()

        default = 0
        self.choice.SetSelection(default)
        self.CheckItem(self.locations[default])

        self.thread = gThread()

    def _layout(self):
        """Create and layout sizers"""
        vertical = wx.BoxSizer(wx.VERTICAL)
        self.sizer = vertical

        vertical.Add(
            self.label,
            proportion=0,
            flag=wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT,
            border=10,
        )
        vertical.Add(
            self.choice,
            proportion=0,
            flag=wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT,
            border=10,
        )
        vertical.AddStretchSpacer()
        vertical.Add(
            self.message,
            proportion=0,
            flag=wx.ALIGN_LEFT | wx.ALL | wx.EXPAND,
            border=10,
        )

        self.SetSizer(vertical)
        vertical.Fit(self)
        self.Layout()
        self.SetMinSize(self.GetBestSize())

    def _change_download_btn_label(
        self, label=_("Do&wnload"), tooltip=_("Download selected project")
    ):
        """Change download button label/tooltip"""
        if self.parent.download_button:
            self.parent.download_button.SetLabel(label)
            self.parent.download_button.SetToolTip(tooltip)

    def _terminateDownloadCallback(self, event):
        """Terminate download callback

        :param object event: event object
        """
        self._infobar_message_btns["addBtn"] = True
        if self._download_in_progress:
            self.thread.Terminate()
            # Clean up after urllib urlretrieve which is used internally
            # in grass.utils.
            from urllib import request  # pylint: disable=import-outside-toplevel

            self._download_in_progress = False
            request.urlcleanup()
            sys.stdout.write("Download aborted")
            self.thread = gThread()
            self._change_download_btn_label()
            self.parent.Show(True)
            self._infobar_message_btns.update(
                dict(zip(("addBtn", "removeBtn"), (False, True))),
            )

    def OnAbort(self, event):
        """Info bar widget abort button event handler

        :param object event: event object
        """
        self._terminateDownloadCallback(event)

    def OnDownload(self, event):
        """Handle user-initiated action of download"""
        self._infobar_message_btns.update(
            dict(zip(("addBtn", "removeBtn"), (True, False))),
        )
        button_label = self.parent.download_button.GetLabel()
        if button_label in {_("Download"), _("Do&wnload")}:
            self._change_download_btn_label(
                label=self._abort_btn_label,
                tooltip=self._abort_btn_tooltip,
            )
            Debug.msg(1, "OnDownload")
            if self._download_in_progress:
                self._warning(_("Download in progress, wait until it is finished"))
            index = self.choice.GetSelection()
            self.DownloadItem(self.locations[index])
        else:
            self.parent.OnCancel()

    def ShowInfoBarMessage(self, message, buttons=None):
        """Show progress of downloading new location Data Catalog info
        message

        :param str message: text message
        :param list buttons: list of info bar widget buttons
                             [(button_label, button_click_event_handler),...]
        """
        if hasattr(self.parent, "showInfoBarMessage"):
            self.parent.showInfoBarMessage.emit(
                message=message,
                buttons=[
                    {
                        "id": self._infobar_message_btn_id,
                        "label": self._abort_btn_label,
                        "handler": self.OnAbort,
                        "btnsAction": self._infobar_message_btns,
                    }
                ]
                if buttons is None
                else buttons,
            )
            self._infobar_message_btns["addBtn"] = False

    def DownloadItem(self, item):
        """Download the selected item"""
        Debug.msg(1, "DownloadItem: %s" % item)
        # similar code as in CheckItem
        url = item["url"]
        dirname = location_name_from_url(url)
        destination = os.path.join(self.database, dirname)
        if os.path.exists(destination):
            self._error(
                _(
                    "Project name {name} already exists in {path}, download canceled"
                ).format(name=dirname, path=self.database)
            )
            self._infobar_message_btns.update(
                dict(zip(("addBtn", "removeBtn"), (False, True))),
            )
            self._change_download_btn_label()
            return

        def download_complete_callback(event):
            self._download_in_progress = False
            self._infobar_message_btns["addBtn"] = True
            errors = event.ret
            if errors:
                self._error(_("Download failed: %s") % errors)
            else:
                self._last_downloaded_location_name = dirname
                self._warning(
                    _(
                        "Download completed. The downloaded sample data is listed "
                        "in the location/mapset tree."
                    )
                )
                self.parent.newLocationIsDownloaded.emit()
            self._infobar_message_btns.update(
                dict(zip(("addBtn", "removeBtn"), (False, True))),
            )
            self._change_download_btn_label()
            if errors:
                self.parent.Show(True)

        self._download_in_progress = True
        self._warning(_("Download in progress, wait until it is finished"))
        self.thread.Run(
            callable=download_location,
            url=url,
            name=dirname,
            database=self.database,
            ondone=download_complete_callback,
            onterminate=self._terminateDownloadCallback,
        )
        wx.CallLater(1000, self.parent.Show, False)

    def OnChangeChoice(self, event):
        """React to user changing the selection"""
        index = self.choice.GetSelection()
        self.CheckItem(self.locations[index])

    def CheckItem(self, item):
        """Check what user selected and report potential issues"""
        # similar code as in DownloadItem
        self._infobar_message_btns["addBtn"] = True
        url = item["url"]
        dirname = location_name_from_url(url)
        destination = os.path.join(self.database, dirname)
        if os.path.exists(destination):
            self._warning(
                _("Project named {name} already exists, rename it first").format(
                    name=dirname
                )
            )
            self._infobar_message_btns["addBtn"] = False
            if hasattr(self.parent, "download_button"):
                self.parent.download_button.SetLabel(label=_("Download"))
            return
        else:
            if hasattr(self.parent, "dismissShowInfoBarMessage"):
                self.parent.dismissShowInfoBarMessage.emit()
            self._clearMessage()

    def GetLocation(self):
        """Get the name of the last location downloaded by the user"""
        return self._last_downloaded_location_name

    def _warning(self, text):
        """Displays a warning, hint or info message to the user.

        This function can be used for all kinds of messages except for
        error messages.

        .. note::
            There is no cleaning procedure. You should call
            _clearMessage() when you know that there is everything
            correct.
        """
        sys.stdout.write(text)
        self.sizer.Layout()

    def _error(self, text):
        """Displays a error message to the user.

        This function should be used only when something serious and unexpected
        happens, otherwise _showWarning should be used.

        .. note::
            There is no cleaning procedure. You should call
            _clearMessage() when you know that there is everything
            correct.
        """
        sys.stdout.write(_("Error: {text}").format(text=text))
        self.sizer.Layout()

    def _clearMessage(self):
        """Clears/hides the error message."""
        # we do no hide widget
        # because we do not want the dialog to change the size
        self.message.SetLabel("")
        self.sizer.Layout()


class LocationDownloadDialog(wx.Dialog):
    """Dialog for download of locations

    Contains the panel and Cancel button.
    """

    def __init__(self, parent, database, title=_("Dataset Download")):
        """
        :param database: database to download the project (location) to
        :param title: window title if the default is not appropriate
        """
        wx.Dialog.__init__(self, parent=parent, title=title)

        self.newLocationIsDownloaded = Signal(
            "LocationDownloadDialog.newLocationIsDownloaded"
        )
        self.showInfoBarMessage = Signal("LocationDownloadDialog.showInfoBarMessage")
        self.dismissShowInfoBarMessage = Signal(
            "LocationDownloadDialog.dismissShowInfoBarMessage"
        )

        self.cancel_button = Button(self, id=wx.ID_CANCEL)
        self.download_button = Button(parent=self, id=wx.ID_ANY, label=_("Do&wnload"))
        self.download_button.SetToolTip(_("Download selected dataset"))
        self.panel = LocationDownloadPanel(parent=self, database=database)
        self.cancel_button.Bind(wx.EVT_BUTTON, self.OnCancel)
        self.Bind(wx.EVT_CLOSE, self.OnCancel)

        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer.Add(self.panel, proportion=1, flag=wx.EXPAND)

        button_sizer = wx.StdDialogButtonSizer()
        button_sizer.Add(
            self.cancel_button,
            proportion=0,
            flag=wx.EXPAND | wx.LEFT | wx.RIGHT,
            border=5,
        )
        button_sizer.Add(
            self.download_button,
            proportion=0,
            flag=wx.EXPAND | wx.LEFT | wx.RIGHT,
            border=5,
        )
        button_sizer.Realize()

        self.sizer.Add(
            button_sizer,
            proportion=0,
            flag=wx.ALIGN_RIGHT | wx.TOP | wx.BOTTOM,
            border=10,
        )
        self.SetSizer(self.sizer)
        self.sizer.Fit(self)

        self.Layout()

    def GetLocation(self):
        """Get the name of the last location downloaded by the user"""
        return self.panel.GetLocation()

    def OnCancel(self, event=None):
        if self.panel._download_in_progress:
            # running thread
            dlg = wx.MessageDialog(
                parent=self,
                message=_("Do you want to cancel dataset download?"),
                caption=_("Abort download"),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE,
            )

            ret = dlg.ShowModal()
            dlg.Destroy()

            if ret == wx.ID_NO:
                return
            self.panel.thread.Terminate()
            self.panel._change_download_btn_label()

        if event:
            self.dismissShowInfoBarMessage.emit()
            self.Destroy()


def main():
    """Tests the download dialog"""
    if len(sys.argv) < 2:
        sys.exit("Provide a test directory")
    database = sys.argv[1]

    app = wx.App()

    if len(sys.argv) == 2 or sys.argv[2] == "dialog":

        def new_location_is_downloaded():
            # Reset stdout
            sys.stdout = sys.__stdout__
            print(window.GetLocation())

        def on_close(event):
            parent_window.Destroy()

        parent_window = wx.Dialog(parent=None)
        window = LocationDownloadDialog(parent=parent_window, database=database)
        window.Bind(wx.EVT_CLOSE, on_close)
        window.cancel_button.Bind(wx.EVT_BUTTON, on_close)
        window.newLocationIsDownloaded.connect(new_location_is_downloaded)
        window.Show()
    elif sys.argv[2] == "panel":
        window = wx.Dialog(parent=None)
        panel = LocationDownloadPanel(parent=window, database=database)
        window.ShowModal()
        location = panel.GetLocation()
        if location:
            print(location)
        window.Destroy()
    else:
        print("Unknown settings: try dialog or panel")

    app.MainLoop()


if __name__ == "__main__":
    main()
