"""!
@package mapdisp.main

@brief Start Map Display as standalone application

Classes:
 - mapdisp::MapApp

Usage:
python mapdisp/main.py monitor-identifier /path/to/map/file /path/to/command/file /path/to/env/file

(C) 2006-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (MapFrameBase)
@author Anna Kratochvilova <kratochanna gmail.com> (MapFrameBase)
"""

import os
import sys

if __name__ == "__main__":
    sys.path.append(os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython'))
from core          import globalvar
import wx

from core.gcmd     import RunCommand
from core.render   import Map
from mapdisp.frame import MapFrame
from grass.script  import core as grass

# for standalone app
monFile = { 'cmd' : None,
            'map' : None,
            'env' : None,
            }
monName = None
monSize = list(globalvar.MAP_WINDOW_SIZE)


class MapApp(wx.App):
    def OnInit(self):
        wx.InitAllImageHandlers()
        if __name__ == "__main__":
            self.cmdTimeStamp = os.path.getmtime(monFile['cmd'])
            self.Map = Map(cmdfile = monFile['cmd'], mapfile = monFile['map'],
                           envfile = monFile['env'], monitor = monName)
        else:
            self.Map = None

        self.mapFrm = MapFrame(parent = None, id = wx.ID_ANY, Map = self.Map,
                               size = monSize)
        # self.SetTopWindow(Map)
        self.mapFrm.Show()
        
        if __name__ == "__main__":
            self.timer = wx.PyTimer(self.watcher)
            #check each 0.5s
            global mtime
            mtime = 500
            self.timer.Start(mtime)
            
        return True
    
    def OnExit(self):
        if __name__ == "__main__":
            # stop the timer
            # self.timer.Stop()
            # terminate thread
            for f in monFile.itervalues():
                grass.try_remove(f)
            
    def watcher(self):
        """!Redraw, if new layer appears (check's timestamp of
        cmdfile)
        """
        # todo: events
        if os.path.getmtime(monFile['cmd']) > self.cmdTimeStamp:
            self.timer.Stop()
            self.cmdTimeStamp = os.path.getmtime(monFile['cmd'])
            self.mapFrm.OnDraw(None)
            self.mapFrm.GetMap().GetLayersFromCmdFile()
            self.timer.Start(mtime)

if __name__ == "__main__":
    # set command variable
    if len(sys.argv) < 5:
        print __doc__
        sys.exit(1)
    
    monName = sys.argv[1]
    monFile = { 'map' : sys.argv[2],
                'cmd' : sys.argv[3],
                'env' : sys.argv[4],
                }
    if len(sys.argv) >= 6:
        try:
            monSize[0] = int(sys.argv[5])
        except ValueError:
            pass
    
    if len(sys.argv) == 7:
        try:
            monSize[1] = int(sys.argv[6])
        except ValueError:
            pass

    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    
    grass.verbose(_("Starting map display <%s>...") % (monName))

    RunCommand('g.gisenv',
               set = 'MONITOR_%s_PID=%d' % (monName, os.getpid()))
    
    gmMap = MapApp(0)
    # set title
    gmMap.mapFrm.SetTitle(_("GRASS GIS Map Display: " +
                            monName + 
                            " - Location: " + grass.gisenv()["LOCATION_NAME"]))
    
    gmMap.MainLoop()
    
    grass.verbose(_("Stopping map display <%s>...") % (monName))

    # clean up GRASS env variables
    env = grass.gisenv()
    env_name = 'MONITOR_%s' % monName
    for key in env.keys():
        if key.find(env_name) == 0:
            RunCommand('g.gisenv',
                       set = '%s=' % key)
        if key == 'MONITOR' and env[key] == monName:
            RunCommand('g.gisenv',
                       set = '%s=' % key)
    
    sys.exit(0)
