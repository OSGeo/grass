"""
@package core.watchdog

@brief Current mapset and maps watchdog

Classes:
 - watchdog::CurrentMapsetWatch
 - watchdog::MapWatch
 - watchdog::MapsetWatchdog

(C) 2022 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
@author Tomas Zigo <tomas.zigo slovanet.sk>
"""

import os
import time

from pathlib import Path

import wx
from wx.lib.newevent import NewEvent

from grass.script import core as grass

watchdog_used = True
try:
    from watchdog.observers import Observer
    from watchdog.events import (
        PatternMatchingEventHandler,
        FileSystemEventHandler,
    )
except ImportError:
    watchdog_used = False
    PatternMatchingEventHandler = object
    FileSystemEventHandler = object

updateMapset, EVT_UPDATE_MAPSET = NewEvent()
currentMapsetChanged, EVT_CURRENT_MAPSET_CHANGED = NewEvent()


class CurrentMapsetWatch(FileSystemEventHandler):
    """Monitors rc file to check if mapset has been changed.
    In that case wx event is dispatched to event handler.
    Needs to check timestamp, because the modified event is sent twice.
    This assumes new instance of this class is started
    whenever mapset is changed."""

    def __init__(self, rcfile, mapset_path, event_handler):
        FileSystemEventHandler.__init__(self)
        self.event_handler = event_handler
        self.mapset_path = mapset_path
        self.rcfile_name = os.path.basename(rcfile)
        self.modified_time = 0

    def on_modified(self, event):
        if (
            not event.is_directory
            and os.path.basename(event.src_path) == self.rcfile_name
        ):
            timestamp = Path(event.src_path).stat().st_mtime
            if timestamp - self.modified_time < 0.5:
                return
            self.modified_time = timestamp
            # wait to make sure file writing is done
            time.sleep(0.1)
            with open(event.src_path) as f:
                gisrc = {}
                for line in f:
                    key, val = line.split(":")
                    gisrc[key.strip()] = val.strip()
                new = os.path.join(
                    gisrc["GISDBASE"], gisrc["LOCATION_NAME"], gisrc["MAPSET"]
                )
                if new != self.mapset_path:
                    evt = currentMapsetChanged()
                    wx.PostEvent(self.event_handler, evt)


class MapWatch(PatternMatchingEventHandler):
    """Monitors file events (create, delete, move files) using watchdog
    to inform about changes in current mapset. One instance monitors
    only one element (raster, vector, raster_3d).
    Patterns are not used/needed in this case, use just '*' for matching
    everything. When file/directory change is detected, wx event is dispatched
    to event handler (can't use Signals because this is different thread),
    containing info about the change."""

    def __init__(self, patterns, element, event_handler):
        PatternMatchingEventHandler.__init__(self, patterns=patterns)
        self.element = element
        self.event_handler = event_handler

    def on_created(self, event):
        if (self.element in {"vector", "raster_3d"}) and not event.is_directory:
            return
        evt = updateMapset(
            src_path=event.src_path,
            event_type=event.event_type,
            is_directory=event.is_directory,
            dest_path=None,
        )
        wx.PostEvent(self.event_handler, evt)

    def on_deleted(self, event):
        if (self.element in {"vector", "raster_3d"}) and not event.is_directory:
            return
        evt = updateMapset(
            src_path=event.src_path,
            event_type=event.event_type,
            is_directory=event.is_directory,
            dest_path=None,
        )
        wx.PostEvent(self.event_handler, evt)

    def on_moved(self, event):
        if (self.element in {"vector", "raster_3d"}) and not event.is_directory:
            return
        evt = updateMapset(
            src_path=event.src_path,
            event_type=event.event_type,
            is_directory=event.is_directory,
            dest_path=event.dest_path,
        )
        wx.PostEvent(self.event_handler, evt)


class MapsetWatchdog:
    """Current mapset and maps watchdog

    :param tuple elements_dir: tuple of element with dir tuples
                               (("raster", "cell"), ...)
    :param object instance evt_handler: event handler object instance of
                                        class
    :param object instance giface: object instance of giface class
    :param str patterns: map watchdog patterns with default value "*" all
    """

    def __init__(self, elements_dirs, evt_handler, giface, patterns="*"):
        self._elements_dirs = elements_dirs
        self._evt_handler = evt_handler
        self._patterns = patterns
        self._giface = giface
        self._observer = None

    def ScheduleWatchCurrentMapset(self):
        """Using watchdog library, sets up watching of current mapset folder
        to detect changes not captured by other means (e.g. from command line).
        Schedules 1 watches (raster).
        If watchdog observers are active, it restarts the observers in
        current mapset.
        """
        global watchdog_used
        if not watchdog_used:
            return

        if self._observer and self._observer.is_alive():
            self._observer.stop()
            self._observer.join()
            self._observer.unschedule_all()
        self._observer = Observer()

        gisenv = grass.gisenv()
        mapset_path = os.path.join(
            gisenv["GISDBASE"], gisenv["LOCATION_NAME"], gisenv["MAPSET"]
        )
        rcfile = os.environ["GISRC"]
        self._observer.schedule(
            CurrentMapsetWatch(rcfile, mapset_path, self._evt_handler),
            os.path.dirname(rcfile),
            recursive=False,
        )
        for element, directory in self._elements_dirs:
            path = os.path.join(mapset_path, directory)
            if not Path(path).exists():
                try:
                    Path(path).mkdir()
                except OSError:
                    pass
            if Path(path).exists():
                self._observer.schedule(
                    MapWatch(self._patterns, element, self._evt_handler),
                    path=path,
                    recursive=False,
                )
        try:
            self._observer.start()
        except OSError:
            # in case inotify on linux exceeds limits
            watchdog_used = False
            self._giface.WriteWarning(
                _(
                    "File size limit exceeded. The current mapset"
                    " and maps watchdog are disabled now."
                ),
            )
            return
