"""
@package core.gthread

@brief Threading

Classes:
 - gthread::gThread

(C) 2013-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
"""

import threading
import time

import wx
from wx.lib.newevent import NewEvent

import sys

import queue as Queue

from core.gconsole import EVT_CMD_DONE, wxCmdDone

wxThdTerminate, EVT_THD_TERMINATE = NewEvent()


class gThread(threading.Thread, wx.EvtHandler):
    """Thread for various backends

    terminating thread:
    https://www.geeksforgeeks.org/python-different-ways-to-kill-a-thread/
    """

    requestId = 0

    def __init__(self, requestQ=None, resultQ=None, **kwds):
        wx.EvtHandler.__init__(self)
        self.terminate = False
        self._terminate_evt = None

        threading.Thread.__init__(self, **kwds)

        if requestQ is None:
            self.requestQ = Queue.Queue()
        else:
            self.requestQ = requestQ

        if resultQ is None:
            self.resultQ = Queue.Queue()
        else:
            self.resultQ = resultQ

        self.daemon = True

        self.Bind(EVT_CMD_DONE, self.OnDone)
        self.Bind(EVT_THD_TERMINATE, self.OnTerminate)
        self.start()

    def Run(self, *args, **kwds):
        """Run command in queue

        :param args: unnamed command arguments
        :param kwds: named command arguments, keyword 'callable'
                     represents function to be run, keyword 'ondone'
                     represents function to be called after the
                     callable is done

        :return: request id in queue
        """
        gThread.requestId += 1
        self.requestQ.put((gThread.requestId, args, kwds))

        return gThread.requestId

    def GetId(self):
        """Get id for next command"""
        return gThread.requestId + 1

    def SetId(self, id):
        """Set starting id"""
        gThread.requestId = id

    def run(self):
        variables = {
            "callable": None,
            "ondone": None,
            "userdata": None,
            "onterminate": None,
        }
        while True:
            requestId, args, kwds = self.requestQ.get()
            for key in ("callable", "ondone", "userdata", "onterminate"):
                if key in kwds:
                    variables[key] = kwds[key]
                    del kwds[key]

            ret = None
            exception = None
            time.sleep(0.01)

            self._terminate_evt = wxThdTerminate(
                onterminate=variables["onterminate"],
                kwds=kwds,
                args=args,
                pid=requestId,
            )

            if self.terminate:
                return

            ret = variables["callable"](*args, **kwds)

            if self.terminate:
                return
            # except Exception as e:
            #    exception  = e;

            self.resultQ.put((requestId, ret))

            event = wxCmdDone(
                ondone=variables["ondone"],
                kwds=kwds,
                args=args,  # TODO expand args to kwds
                ret=ret,
                exception=exception,
                userdata=variables["userdata"],
                pid=requestId,
            )

            # send event
            wx.PostEvent(self, event)

    def OnDone(self, event):
        if event.ondone:
            event.ondone(event)

    def Terminate(self, terminate=True):
        """Abort command(s)"""
        self.terminate = terminate

    def start(self):
        self.__run_backup = self.run
        self.run = self.__run
        threading.Thread.start(self)

    def __run(self):
        sys.settrace(self.globaltrace)
        self.__run_backup()
        self.run = self.__run_backup

    def globaltrace(self, frame, event, arg):
        if event == "call":
            return self.localtrace
        return None

    def localtrace(self, frame, event, arg):
        if self.terminate:
            if event == "line":
                # Send event
                wx.PostEvent(self, self._terminate_evt)
                raise SystemExit
        return self.localtrace

    def OnTerminate(self, event):
        if event.onterminate:
            event.onterminate(event)
