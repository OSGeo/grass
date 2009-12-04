"""
@package mapdisp.py

@brief Command line useg of GIS map display canvas.view).

Classes:
 - Command

(C) 2006-2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Jachym Cepicky
"""

import sys
import time

from debug import Debug
from threading import Thread

class Command(Thread):
    """
    Creates thread which will observe the command file and see, if
    there is new command to be executed
    """
    def __init__ (self, parent, Map, cmdfile):
        Thread.__init__(self)

        global cmdfilename

        self.parent = parent
        self.map = Map
        self.cmdfile = open(cmdfile, "r")

    def run(self):
        """
        Run this in thread
        """
        dispcmd = []
        while 1:
            self.parent.redraw = False
            line = self.cmdfile.readline().strip()
            if line == "quit":
                break

            if line:
                try:
                    Debug.msg (3, "Command.run(): cmd=%s" % (line))

                    cmd = line.split(" ")
                    opacity = 1
                    if " opacity=" in line:
                        cmd2 = cmd
                        cmd = []
                        for c in cmd2:
                            if c.find("opacity=") == 0:
                                opacity = float(c.split("=")[1]) / 100
                            else:
                                cmd.append(c)

                    self.map.AddLayer(type="raster",
                                      name='',
                                      command=cmd,
                                      l_opacity=opacity)

                    self.parent.redraw =True

                except Exception, e:
                    print "Command Thread: ",e

            time.sleep(0.1)

        sys.exit()
