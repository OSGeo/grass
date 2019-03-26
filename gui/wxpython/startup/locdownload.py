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

from __future__ import print_function

import os
import sys
import tempfile
import shutil
import time

try:
    from urllib2 import HTTPError, URLError
    from urllib import urlopen, urlretrieve
except ImportError:
    # there is also HTTPException, perhaps change to list
    from urllib.error import HTTPError, URLError
    from urllib.request import urlopen, urlretrieve

import wx
from wx.lib.newevent import NewEvent

from grass.script import debug
from grass.script.utils import try_rmdir

from grass.script.setup import set_gui_path
set_gui_path()

from core.debug import Debug
from core.gthread import gThread
from gui_core.wrap import Button, StaticText


# TODO: labels (and descriptions) translatable?
LOCATIONS = [
    {
        "label": "Complete NC location",
        "url": "https://grass.osgeo.org/sampledata/north_carolina/nc_spm_08_grass7.tar.gz",
    },
    {
        "label": "Basic NC location",
        "url": "https://grass.osgeo.org/sampledata/north_carolina/nc_basic_spm_grass7.tar.gz",
    },
    {
        "label": "World location in LatLong/WGS84",
        "url": "https://grass.osgeo.org/sampledata/worldlocation.tar.gz",
    },
    {
        "label": "Spearfish (SD) location",
        "url": "https://grass.osgeo.org/sampledata/spearfish_grass70data-0.3.tar.gz",
    },
    {
        "label": "Piemonte, Italy data set",
        "url": "http://geodati.fmach.it/gfoss_geodata/libro_gfoss/grassdata_piemonte_utm32n_wgs84_grass7.tar.gz",
    },
    {
        "label": "Slovakia 3D precipitation voxel data set",
        "url": "https://grass.osgeo.org/uploads/grass/sampledata/slovakia3d_grass7.tar.gz",
    },
    {
        "label": "Fire simulation sample data",
        "url": "https://grass.osgeo.org/sampledata/fire_grass6data.tar.gz",
    },
    {
        "label": "GISMentors location, Czech Republic",
        "url": "http://training.gismentors.eu/geodata/grass/gismentors.zip",
    },
]


class DownloadError(Exception):
    """Error happened during download or when processing the file"""
    pass

class RedirectText(object):
    def __init__(self, window):
        self.out = window
 
    def write(self, string):
        try:
            wx.CallAfter(self.out.SetLabel, string)
        except:
            # window closed -> PyDeadObjectError
            pass

# copy from g.extension, potentially move to library
def move_extracted_files(extract_dir, target_dir, files):
    """Fix state of extracted file by moving them to different diretcory

    When extracting, it is not clear what will be the root directory
    or if there will be one at all. So this function moves the files to
    a different directory in the way that if there was one directory extracted,
    the contained files are moved.
    """
    debug("move_extracted_files({0})".format(locals()))
    if len(files) == 1:
        shutil.copytree(os.path.join(extract_dir, files[0]), target_dir)
    else:
        if not os.path.exists(target_dir):
            os.mkdir(target_dir)
        for file_name in files:
            actual_file = os.path.join(extract_dir, file_name)
            if os.path.isdir(actual_file):
                # copy_tree() from distutils failed to create
                # directories before copying files time to time
                # (when copying to recently deleted directory)
                shutil.copytree(actual_file,
                                os.path.join(target_dir, file_name))
            else:
                shutil.copy(actual_file, os.path.join(target_dir, file_name))


# copy from g.extension, potentially move to library
def extract_zip(name, directory, tmpdir):
    """Extract a ZIP file into a directory"""
    debug("extract_zip(name={name}, directory={directory},"
          " tmpdir={tmpdir})".format(name=name, directory=directory,
                                     tmpdir=tmpdir), 3)
    try:
        import zipfile
        zip_file = zipfile.ZipFile(name, mode='r')
        file_list = zip_file.namelist()
        # we suppose we can write to parent of the given dir
        # (supposing a tmp dir)
        extract_dir = os.path.join(tmpdir, 'extract_dir')
        os.mkdir(extract_dir)
        for subfile in file_list:
            # this should be safe in Python 2.7.4
            zip_file.extract(subfile, extract_dir)
        files = os.listdir(extract_dir)
        move_extracted_files(extract_dir=extract_dir,
                             target_dir=directory, files=files)
    except zipfile.BadZipfile as error:
        raise DownloadError(_("ZIP file is unreadable: {0}").format(error))


# copy from g.extension, potentially move to library
def extract_tar(name, directory, tmpdir):
    """Extract a TAR or a similar file into a directory"""
    debug("extract_tar(name={name}, directory={directory},"
          " tmpdir={tmpdir})".format(name=name, directory=directory,
                                     tmpdir=tmpdir), 3)
    try:
        import tarfile  # we don't need it anywhere else
        tar = tarfile.open(name)
        extract_dir = os.path.join(tmpdir, 'extract_dir')
        os.mkdir(extract_dir)
        tar.extractall(path=extract_dir)
        files = os.listdir(extract_dir)
        move_extracted_files(extract_dir=extract_dir,
                             target_dir=directory, files=files)
    except tarfile.TarError as error:
        raise DownloadError(_("Archive file is unreadable: {0}").format(error))

extract_tar.supported_formats = ['tar.gz', 'gz', 'bz2', 'tar', 'gzip', 'targz']

# based on https://blog.shichao.io/2012/10/04/progress_speed_indicator_for_urlretrieve_in_python.html
def reporthook(count, block_size, total_size):
    global start_time
    if count == 0:
        start_time = time.time()
        sys.stdout.write("Download in progress, wait until it is finished\n0%")
        return
    if count % 100 != 0: # be less verbose
        return
    duration = time.time() - start_time
    progress_size = int(count * block_size)
    speed = int(progress_size / (1024 * duration))
    percent = int(count * block_size * 100 / total_size)
    sys.stdout.write("Download in progress, wait until it is finished\n{0}%, {1} MB, {2} KB/s, {3:.0f} seconds passed".format(
        percent, progress_size / (1024 * 1024), speed, duration
    ))
    
# based on g.extension, potentially move to library
def download_and_extract(source):
    """Download a file (archive) from URL and uncompress it"""
    tmpdir = tempfile.mkdtemp()
    Debug.msg(1, 'Tmpdir: {}'.format(tmpdir))
    directory = os.path.join(tmpdir, 'location')
    if source.endswith('.zip'):
        archive_name = os.path.join(tmpdir, 'location.zip')
        filename, headers = urlretrieve(source, archive_name, reporthook)
        if headers.get('content-type', '') != 'application/zip':
            raise DownloadError(
                _("Download of <{url}> failed"
                  " or file <{name}> is not a ZIP file").format(
                      url=source, name=filename))
        extract_zip(name=archive_name, directory=directory, tmpdir=tmpdir)
    elif (source.endswith(".tar.gz") or
          source.rsplit('.', 1)[1] in extract_tar.supported_formats):
        if source.endswith(".tar.gz"):
            ext = "tar.gz"
        else:
            ext = source.rsplit('.', 1)[1]
        archive_name = os.path.join(tmpdir, 'location.' + ext)
        urlretrieve(source, archive_name, reporthook)
        # TODO: error handling for urlretrieve
        extract_tar(name=archive_name, directory=directory, tmpdir=tmpdir)
    else:
        # probably programmer error
        raise DownloadError(_("Unknown format '{0}'.").format(source))
    assert os.path.isdir(directory)
    return directory


def download_location(url, name, database):
    """Wrapper to return DownloadError by value

    It also moves the location directory to the database.
    """
    try:
        # TODO: the unpacking could go right to the path (but less
        # robust) or replace copytree here with move
        directory = download_and_extract(source=url)
        destination = os.path.join(database, name)
        if not is_location_valid(directory):
            return _("Downloaded location is not valid")
        shutil.copytree(src=directory, dst=destination)
        try_rmdir(directory)
    except DownloadError as error:
        return error
    return None


# based on grass.py (to be moved to future "grass.init")
def is_location_valid(location):
    """Return True if GRASS Location is valid

    :param location: path of a Location
    """
    # DEFAULT_WIND file should not be required until you do something
    # that actually uses them. The check is just a heuristic; a directory
    # containing a PERMANENT/DEFAULT_WIND file is probably a GRASS
    # location, while a directory lacking it probably isn't.
    # TODO: perhaps we can relax this and require only permanent
    return os.access(os.path.join(location,
                                  "PERMANENT", "DEFAULT_WIND"), os.F_OK)


def location_name_from_url(url):
    """Create location name from URL"""
    return url.rsplit('/', 1)[1].split('.', 1)[0].replace("-", "_").replace(" ", "_")


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

        self._last_downloaded_location_name = None
        self._download_in_progress = False
        self.database = database
        self.locations = locations

        self.label = StaticText(
            parent=self,
            label=_("Select sample location to download:"))

        choices = []
        for item in self.locations:
            choices.append(item['label'])
        self.choice = wx.Choice(parent=self, choices=choices)

        self.choice.Bind(wx.EVT_CHOICE, self.OnChangeChoice)

        self.download_button = Button(parent=self, id=wx.ID_ANY,
                                      label=_("Do&wnload"))
        self.download_button.SetToolTip(_("Download selected location"))
        self.download_button.Bind(wx.EVT_BUTTON, self.OnDownload)
        # TODO: add button for a link to an associated website?
        # TODO: add thumbnail for each location?

        # TODO: messages copied from gis_set.py, need this as API?
        self.message = StaticText(parent=self, size=(-1, 50))
        sys.stdout = RedirectText(self.message)

        # It is not clear if all wx versions supports color, so try-except.
        # The color itself may not be correct for all platforms/system settings
        # but in http://xoomer.virgilio.it/infinity77/wxPython/Widgets/wx.SystemSettings.html
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

        vertical.Add(self.label, proportion=0,
                     flag=wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, border=10)
        vertical.Add(self.choice, proportion=0,
                     flag=wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, border=10)

        button_sizer = wx.BoxSizer(wx.HORIZONTAL)
        button_sizer.AddStretchSpacer()
        button_sizer.Add(self.download_button, proportion=0)

        vertical.Add(button_sizer, proportion=0,
                     flag=wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT | wx.ALIGN_RIGHT, border=10)
        vertical.AddStretchSpacer()
        vertical.Add(self.message, proportion=0,
                     flag=wx.ALIGN_CENTER_VERTICAL |
                     wx.ALIGN_LEFT | wx.ALL | wx.EXPAND, border=10)

        self.SetSizer(vertical)
        vertical.Fit(self)
        self.Layout()
        self.SetMinSize(self.GetBestSize())

    def OnDownload(self, event):
        """Handle user-initiated action of download"""
        Debug.msg(1, "OnDownload")
        if self._download_in_progress:
            self._warning(_("Download in progress, wait until it is finished"))
        index = self.choice.GetSelection()
        self.DownloadItem(self.locations[index])
        self.download_button.Enable(False)

    def DownloadItem(self, item):
        """Download the selected item"""
        Debug.msg(1, "DownloadItem: %s" % item)
        # similar code as in CheckItem
        url = item['url']
        dirname = location_name_from_url(url)
        destination = os.path.join(self.database, dirname)
        if os.path.exists(destination):
            self._error(_("Location named <%s> already exists,"
                          " download canceled") % dirname)
            return

        def download_complete_callback(event):
            self._download_in_progress = False
            errors = event.ret
            if errors:
                self._error(_("Download failed: %s") % errors)
            else:
                self._last_downloaded_location_name = dirname
                self._warning(_("Download completed. The downloaded sample data is listed "
                                "in the location/mapset tabs upon closing of this window")
                )

        self._download_in_progress = True
        self._warning(_("Download in progress, wait until it is finished"))
        self.thread.Run(callable=download_location,
                        url=url, name=dirname, database=self.database,
                        ondone=download_complete_callback)

    def OnChangeChoice(self, event):
        """React to user changing the selection"""
        index = self.choice.GetSelection()
        self.CheckItem(self.locations[index])

    def CheckItem(self, item):
        """Check what user selected and report potential issues"""
        # similar code as in DownloadItem
        url = item['url']
        dirname = location_name_from_url(url)
        destination = os.path.join(self.database, dirname)
        if os.path.exists(destination):
            self._warning(_("Location named <%s> already exists,"
                            " rename it first") % dirname)
            return
        else:
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
        self.message.SetLabel(text)
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
        self.message.SetLabel(_("Error: {text}").format(text=text))
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
    def __init__(self, parent, database,
                 title=_("GRASS GIS Location Download")):
        """
        :param database: database to download the location to
        :param title: window title if the default is not appropriate
        """
        wx.Dialog.__init__(self, parent=parent, title=title)
        self.panel = LocationDownloadPanel(parent=self, database=database)
        close_button = Button(self, id=wx.ID_CLOSE)
        # TODO: terminate download process
        close_button.Bind(wx.EVT_BUTTON, self.OnClose)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.panel, proportion=1, flag=wx.EXPAND)

        button_sizer = wx.StdDialogButtonSizer()
        button_sizer.Add(close_button)
        button_sizer.Realize()

        sizer.Add(button_sizer, proportion=0,
                  flag=wx.ALIGN_RIGHT | wx.BOTTOM, border=10)
        self.SetSizer(sizer)
        sizer.Fit(self)

        self.Layout()

    def GetLocation(self):
        """Get the name of the last location downloaded by the user"""
        return self.panel.GetLocation()

    def OnClose(self, event):
        if self.panel._download_in_progress:
            # running thread
            dlg = wx.MessageDialog(parent=self,
                                   message=_("Do you want to cancel location download?"),
                                   caption=_("Abort download"),
                                   style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE
            )
            
            ret = dlg.ShowModal()
            dlg.Destroy()

            # TODO: terminate download process on wx.ID_YES
            if ret == wx.ID_NO:
                return
    
        self.Close()

def main():
    """Tests the download dialog"""
    if len(sys.argv) < 2:
        sys.exit("Provide a test directory")
    database = sys.argv[1]

    app = wx.App()

    if len(sys.argv) == 2 or sys.argv[2] == 'dialog':
        window = LocationDownloadDialog(parent=None, database=database)
        window.ShowModal()
        location = window.GetLocation()
        if location:
            print(location)
        window.Destroy()
    elif sys.argv[2] == 'panel':
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


if __name__ == '__main__':
    main()
