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

import sys
if sys.version_info.major == 2:
    import Queue
else:
    import queue as Queue

from core.gconsole import EVT_CMD_DONE, wxCmdDone


class gThread(threading.Thread, wx.EvtHandler):
    """Thread for various backends"""
    requestId = 0

    def __init__(self, requestQ=None, resultQ=None, **kwds):
        wx.EvtHandler.__init__(self)
        self.terminate = False

        threading.Thread.__init__(self, **kwds)

        if requestQ is None:
            self.requestQ = Queue.Queue()
        else:
            self.requestQ = requestQ

        if resultQ is None:
            self.resultQ = Queue.Queue()
        else:
            self.resultQ = resultQ

        self.setDaemon(True)

        self.Bind(EVT_CMD_DONE, self.OnDone)
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
        while True:
            requestId, args, kwds = self.requestQ.get()
            for key in ('callable', 'ondone', 'userdata'):
                if key in kwds:
                    vars()[key] = kwds[key]
                    del kwds[key]
                else:
                    vars()[key] = None

            requestTime = time.time()

            ret = None
            exception = None
            time.sleep(.01)

            if self.terminate:
                return

            ret = vars()['callable'](*args, **kwds)

            if self.terminate:
                return
            # except Exception as e:
            #    exception  = e;

            self.resultQ.put((requestId, ret))

            event = wxCmdDone(ondone=vars()['ondone'],
                              kwds=kwds,
                              args=args,  # TODO expand args to kwds
                              ret=ret,
                              exception=exception,
                              userdata=vars()['userdata'],
                              pid=requestId)

            # send event
            wx.PostEvent(self, event)

    def OnDone(self, event):
        if event.ondone:
            event.ondone(event)

    def Terminate(self, terminate=True):
        """Abort command(s)"""
        self.terminate = terminate
