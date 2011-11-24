"""
@package gui_core.task

@brief Construct simple wxPython GUI from a GRASS command interface
description.

Classes:
 - GUI

(C) 2000-2011 by the GRASS Development Team
This program is free software under the GPL(>=v2) Read the file
COPYING coming with GRASS for details.

@author Jan-Oliver Wagner <jan@intevation.de>
@author Bernhard Reiter <bernhard@intevation.de>
@author Michael Barton, Arizona State University
@author Daniel Calvelo <dca.gis@gmail.com>
@author Martin Landa <landa.martin@gmail.com>
@author Luca Delucchi <lucadeluge@gmail.com>
"""

import sys
import re
import os
import time
import locale
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

from core import globalvar
import wx

from grass.script import core as grass
from grass.script import task as gtask
from core.gcmd    import GException, GError

class GUI:
    def __init__(self, parent = None, show = True, modal = False,
                 centreOnParent = False, checkError = False):
        """!Parses GRASS commands when module is imported and used from
        Layer Manager.
        """
        self.parent = parent
        self.show   = show
        self.modal  = modal
        self.centreOnParent = centreOnParent
        self.checkError     = checkError
        
        self.grass_task = None
        self.cmd = list()
        
        global _blackList
        if self.parent:
            _blackList['enabled'] = True
        else:
            _blackList['enabled'] = False
        
    def GetCmd(self):
        """!Get validated command"""
        return self.cmd
    
    def ParseCommand(self, cmd, gmpath = None, completed = None):
        """!Parse command
        
        Note: cmd is given as list
        
        If command is given with options, return validated cmd list:
         - add key name for first parameter if not given
         - change mapname to mapname@mapset
        """
        start = time.time()
        dcmd_params = {}
        if completed == None:
            get_dcmd = None
            layer = None
            dcmd_params = None
        else:
            get_dcmd = completed[0]
            layer = completed[1]
            if completed[2]:
                dcmd_params.update(completed[2])
        
        # parse the interface decription
        try:
            global _blackList
            self.grass_task = gtask.parse_interface(cmd[0],
                                                    blackList = _blackList)
        except ValueError, e: #grass.ScriptError, e:
            GError(e.value)
            return
        
        # if layer parameters previously set, re-insert them into dialog
        if completed is not None:
            if 'params' in dcmd_params:
                self.grass_task.params = dcmd_params['params']
            if 'flags' in dcmd_params:
                self.grass_task.flags = dcmd_params['flags']
        
        err = list()
        # update parameters if needed && validate command
        if len(cmd) > 1:
            i = 0
            cmd_validated = [cmd[0]]
            for option in cmd[1:]:
                if option[0] ==  '-': # flag
                    if option[1] ==  '-':
                        self.grass_task.set_flag(option[2:], True)
                    else:
                        self.grass_task.set_flag(option[1], True)
                    cmd_validated.append(option)
                else: # parameter
                    try:
                        key, value = option.split('=', 1)
                    except:
                        if self.grass_task.firstParam:
                            if i == 0: # add key name of first parameter if not given
                                key = self.grass_task.firstParam
                                value = option
                            else:
                                raise GException, _("Unable to parse command '%s'") % ' '.join(cmd)
                        else:
                            continue
                    
                    element = self.grass_task.get_param(key, raiseError = False)
                    if not element:
                        err.append(_("%(cmd)s: parameter '%(key)s' not available") % \
                                       { 'cmd' : cmd[0],
                                         'key' : key })
                        continue
                    element = element['element']
                    
                    if element in ['cell', 'vector']:
                        # mapname -> mapname@mapset
                        try:
                            name, mapset = value.split('@')
                        except ValueError:
                            mapset = grass.find_file(value, element)['mapset']
                            curr_mapset = grass.gisenv()['MAPSET']
                            if mapset and mapset !=  curr_mapset:
                                value = value + '@' + mapset
                    
                    self.grass_task.set_param(key, value)
                    cmd_validated.append(key + '=' + value)
                    i += 1
            
            # update original command list
            cmd = cmd_validated
        
        if self.show is not None:
            self.mf = mainFrame(parent = self.parent, ID = wx.ID_ANY,
                                task_description = self.grass_task,
                                get_dcmd = get_dcmd, layer = layer)
        else:
            self.mf = None
        
        if get_dcmd is not None:
            # update only propwin reference
            get_dcmd(dcmd = None, layer = layer, params = None,
                     propwin = self.mf)
        
        if self.show is not None:
            self.mf.notebookpanel.OnUpdateSelection(None)
            if self.show is True:
                if self.parent and self.centreOnParent:
                    self.mf.CentreOnParent()
                else:
                    self.mf.CenterOnScreen()
                self.mf.Show(self.show)
                self.mf.MakeModal(self.modal)
            else:
                self.mf.OnApply(None)
        
        self.cmd = cmd
        
        if self.checkError:
            return self.grass_task, err
        else:
            return self.grass_task
    
    def GetCommandInputMapParamKey(self, cmd):
        """!Get parameter key for input raster/vector map
        
        @param cmd module name
        
        @return parameter key
        @return None on failure
        """
        # parse the interface decription
        if not self.grass_task:
            enc = locale.getdefaultlocale()[1]
            if enc and enc.lower() == "cp932":
                p = re.compile('encoding="' + enc + '"', re.IGNORECASE)
                tree = etree.fromstring(p.sub('encoding="utf-8"',
                                              gtask.get_interface_description(cmd).decode(enc).encode('utf-8')))
            else:
                tree = etree.fromstring(gtask.get_interface_description(cmd))
            self.grass_task = gtask.processTask(tree).get_task()
            
            for p in self.grass_task.params:
                if p.get('name', '') in ('input', 'map'):
                    age = p.get('age', '')
                    prompt = p.get('prompt', '')
                    element = p.get('element', '') 
                    if age ==  'old' and \
                            element in ('cell', 'grid3', 'vector') and \
                            prompt in ('raster', '3d-raster', 'vector'):
                        return p.get('name', None)
        return None
