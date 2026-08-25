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
import weakref

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

    _shared_observer = None
    _instances = weakref.WeakSet()

    def __init__(self, elements_dirs, evt_handler, giface, patterns="*"):
        self._elements_dirs = elements_dirs
        self._evt_handler = evt_handler
        self._patterns = patterns
        self._giface = giface
        self._scheduled_mapset = None
        MapsetWatchdog._instances.add(self)

    def ScheduleWatchCurrentMapset(self):
        """Using watchdog library, sets up watching of current mapset folder
        to detect changes not captured by other means (e.g. from command line).
        If watchdog observers are active, it restarts the shared observer
        in current mapset, scheduling watches for all registered instances.
        """
        global watchdog_used
        if not watchdog_used:
            return

        gisenv = grass.gisenv()
        mapset_path = os.path.join(
            gisenv["GISDBASE"], gisenv["LOCATION_NAME"], gisenv["MAPSET"]
        )

        if (
            MapsetWatchdog._shared_observer
            and MapsetWatchdog._shared_observer.is_alive()
            and self._scheduled_mapset == mapset_path
        ):
            return

        if (
            MapsetWatchdog._shared_observer
            and MapsetWatchdog._shared_observer.is_alive()
        ):
            MapsetWatchdog._shared_observer.stop()
            MapsetWatchdog._shared_observer.join()
            MapsetWatchdog._shared_observer.unschedule_all()
        MapsetWatchdog._shared_observer = Observer()

        rcfile = os.environ["GISRC"]

        for instance in MapsetWatchdog._instances:
            instance._scheduled_mapset = mapset_path
            MapsetWatchdog._shared_observer.schedule(
                CurrentMapsetWatch(rcfile, mapset_path, instance._evt_handler),
                os.path.dirname(rcfile),
                recursive=False,
            )
            for element, directory in instance._elements_dirs:
                path = os.path.join(mapset_path, directory)
                if not Path(path).exists():
                    try:
                        Path(path).mkdir()
                    except OSError:
                        pass
                if Path(path).exists():
                    MapsetWatchdog._shared_observer.schedule(
                        MapWatch(instance._patterns, element, instance._evt_handler),
                        path=path,
                        recursive=False,
                    )
        try:
            MapsetWatchdog._shared_observer.start()
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
