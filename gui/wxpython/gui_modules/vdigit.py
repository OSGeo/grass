"""
@package vdigit

@brief Vector digitizer extension

Progress:
 (1) v.edit called on the background (class VEdit)
 (2) Reimplentation of v.digit (VDigit)

Import:

  from vdigit import VDigit as VDigit

Classes:
 - AbstractDigit 
 - VEdit
 - VDigit
 - AbstractDisplayDriver
 - CDisplayDriver
 - VDigitSettingsDialog
 - VDigitCategoryDialog
 - VDigitZBulkDialog
 - VDigitDuplicatesDialog
 - VDigitVBuildDialog

(C) 2007-2008 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import string
import copy

import wx
import wx.lib.colourselect as csel
import wx.lib.mixins.listctrl as listmix

import gcmd
import dbm
from debug import Debug as Debug
import gselect
import globalvar
from preferences import globalSettings as UserSettings
try:
    digitPath = os.path.join(globalvar.ETCWXDIR, "vdigit")
    sys.path.append(digitPath)
    import grass7_wxvdigit as wxvdigit
    GV_LINES = wxvdigit.GV_LINES
    digitErr = ''
except ImportError, err:
    GV_LINES = None
    digitErr = err
    #    print >> sys.stderr, "%sWARNING: Digitization tool is disabled (%s). " \
    #          "Detailed information in README file." % \
    #          (os.linesep, err)

# which interface to use?
if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') == 'vedit' and GV_LINES is not None:
    print >> sys.stderr, "%sWARNING: Digitization tool uses v.edit interface. " \
        "This can significantly slow down some operations especially for " \
        "middle-large vector maps. "\
        "You can change the digitization interface in 'User preferences' " \
        "(menu 'Config'->'Preferences')." % \
          os.linesep

class AbstractDigit:
    """
    Abstract digitization class
    """
    def __init__(self, mapwindow):
        """Initialization

        @param mapwindow reference to mapwindow (MapFrame) instance
        @param settings  initial settings of digitization tool
        """
        self.map       = None
        self.mapWindow = mapwindow
        
        Debug.msg (3, "AbstractDigit.__init__(): map=%s" % \
                   self.map)

        #self.SetCategory()

        self.driver = CDisplayDriver(self, mapwindow)

    def SetCategoryNextToUse(self):
        """Find maximum category number in the map layer
        and update Digit.settings['category']

        @return 'True' on success, 'False' on failure
        """
        # vector map layer without categories, reset to '1'
        UserSettings.Set(group='vdigit', key='category', subkey='value', value=1)

        if self.map:
            if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') == 'vedit':
                categoryCmd = gcmd.Command(cmd=["v.category", "-g", "--q",
                                                "input=%s" % self.map, 
                                                "option=report"])

                if categoryCmd.returncode != 0:
                    return False
        
                for line in categoryCmd.ReadStdOutput():
                    if "all" in line:
                        if UserSettings.Get(group='vdigit', key='layer', subkey='value') != int(line.split(' ')[0]):
                            continue
                        try:
                            maxCat = int(line.split(' ')[-1]) + 1
                            UserSettings.Set(group='vdigit', key='category', subkey='value', value=maxCat)
                        except:
                            return False
                        return True
            else:
                cat = self.digit.GetCategory(UserSettings.Get(group='vdigit', key='layer', subkey='value'))
                UserSettings.Set(group='vdigit', key='category', subkey='value',
                                  value=cat + 1)
    
    def SetCategory(self):
        """Return category number to use (according Settings)"""
        if UserSettings.Get(group='vdigit', key="categoryMode", subkey='selection') == 0:
            self.SetCategoryNextToUse()

        return UserSettings.Get(group='vdigit', key="category", subkey='value')

    def SetMapName(self, map):
        """Set map name

        @param map map name to be set up or None (will close currently edited map)
        """
        Debug.msg (3, "AbstractDigit.SetMapName map=%s" % map)
        self.map = map

        try:
            ret = self.driver.Reset(self.map)
        except StandardError, e:
            raise gcmd.DigitError(parent=self.mapWindow.parent,
                                  message="%s %s (%s)" % (_('Unable to initialize display driver, '
                                                            'see README file for more information.\n\n'
                                                            'Details:'), e, digitErr))
        
        if map and ret == -1:
            raise gcmd.DigitError(parent=self.mapWindow.parent,
                                  message=_('Unable to open vector map <%s> for editing.\n\n'
                                            'Data are probably corrupted, '
                                            'try to run v.build to rebuild '
                                            'the topology (Vector->Develop vector map->'
                                            'Create/rebuild topology).') % map)
        if not map and ret != 0:
            raise gcmd.DigitError(parent=self.mapWindow.parent,
                                  message=_('Unable to open vector map <%s> for editing.\n\n'
                                            'Data are probably corrupted, '
                                            'try to run v.build to rebuild '
                                            'the topology (Vector->Develop vector map->'
                                            'Create/rebuild topology).') % map)
            
        if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') != 'v.edit':
            try:
                self.digit.InitCats()
            except:
                pass

        # avoid using current vector map as background map
        if self.map == UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value'):
            UserSettings.Set(group='vdigit', key='backgroundMap', subkey='value', value='')

    def SelectLinesByQueryThresh(self):
        """Generic method used for SelectLinesByQuery()
        -- to get threshold value"""
        thresh = 0.0
        if UserSettings.Get(group='vdigit', key='query', subkey='selection') == 0:
            thresh = UserSettings.Get(group='vdigit', key='queryLength', subkey='thresh')
            if UserSettings.Get(group='vdigit', key="queryLength", subkey='than-selection') == 0:
                thresh = -1 * thresh
        else:
            thresh = UserSettings.Get(group='vdigit', key='queryDangle', subkey='thresh')
            if UserSettings.Get(group='vdigit', key="queryDangle", subkey='than-selection') == 0:
                thresh = -1 * thresh

        return thresh

    def GetSelectType(self):
        """Get type(s) to be selected

        Used by SelectLinesByBox() and SelectLinesByPoint()"""

        type = 0
        for feature in (('Point', wxvdigit.GV_POINT),
                        ('Line', wxvdigit.GV_LINE),
                        ('Centroid', wxvdigit.GV_CENTROID),
                        ('Boundary', wxvdigit.GV_BOUNDARY)):
            if UserSettings.Get(group='vdigit', key='selectFeature'+feature[0], subkey='enabled') is True:
                type |= feature[1]

        return type

    def SelectLinesFromBackgroundMap(self, pos1, pos2):
        """Select features from background map

        @param pos1,pos2 bounding box defifinition
        """

        if UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value') == '':
            Debug.msg(4, "VEdit.SelectLinesFromBackgroundMap(): []")
            return []

        x1, y1 = pos1
        x2, y2 = pos2

        vEditCmd = gcmd.Command(['v.edit',
                                 '--q',
                                 'map=%s' % UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value'),
                                 'tool=select',
                                 'bbox=%f,%f,%f,%f' % (pos1[0], pos1[1], pos2[0], pos2[1])])
                                 #'polygon=%f,%f,%f,%f,%f,%f,%f,%f,%f,%f' % \
                                 #    (x1, y1, x2, y1, x2, y2, x1, y2, x1, y1)])
                                             
        try:
            output = vEditCmd.ReadStdOutput()[0] # first line
            ids = output.split(',') 
            ids = map(int, ids) # str -> int
        except:
            return []

        Debug.msg(4, "VEdit.SelectLinesFromBackgroundMap(): %s" % \
                      ",".join(["%d" % v for v in ids]))
        
        return ids

class VEdit(AbstractDigit):
    """
    Prototype of digitization class based on v.edit command

    Note: This should be replaced by VDigit class.
    """
    def __init__(self, mapwindow):
        """Initialization

        @param mapwindow reference to mapwindow (MapFrame) instance
        @param settings  initial settings of digitization tool
        """
        AbstractDigit.__init__(self, mapwindow)

    def AddPoint (self, map, point, x, y, z=None):
        """Add point/centroid

        @param map   map name
        @param point feature type (True for point, otherwise centroid)
        @param x,y,z coordinates
        """
        if point:
            key = "P"
        else:
            key = "C"

        if UserSettings.Get(group='vdigit', key="categoryMode", subkey='selection') == 2:
            layer = -1 # -> no category
            cat   = -1
        else:
            layer = UserSettings.Get(group='vdigit', key="layer", subkey='value')
            cat   = self.SetCategory()

        if layer > 0 and cat != "None":
            addstring =  "%s 1 1\n" % (key)
        else:
            addstring =  "%s 1\n" % (key)

        addstring += "%f %f\n" % (x, y)

        if layer > 0 and cat != "None":
            addstring += "%d %d\n" % (layer, cat)
            Debug.msg (3, "VEdit.AddPoint(): map=%s, type=%s, layer=%d, cat=%d, x=%f, y=%f" % \
                           (map, type, layer, cat, x, y))
        else:
            Debug.msg (3, "VEdit.AddPoint(): map=%s, type=%s, x=%f, y=%f" % \
                           (map, type, x, y))

        Debug.msg (4, "Vline.AddPoint(): input=%s" % addstring)
                
        self.__AddFeature (map=map, input=addstring)

    def AddLine (self, map, line, coords):
        """Add line/boundary

        @param map  map name
        @param line feature type (True for line, otherwise boundary)
        @param list of coordinates
        """
        if len(coords) < 2:
            return

        if UserSettings.Get(group='vdigit', key="categoryMode", subkey='selection') == 2:
            layer = -1 # -> no category
            cat   = -1
        else:
            layer = UserSettings.Get(group='vdigit', key="layer", subkey='value')
            cat   = self.SetCategory()

        if line:
            key = "L"
            flags = []
        else:
            key = "B"
            flags = ['-c'] # close boundaries
            
        if layer > 0 and cat != "None":
            addstring = "%s %d 1\n" % (key, len(coords))
        else:
            addstring = "%s %d\n" % (key, len(coords))

        for point in coords:
            addstring += "%f %f\n" % \
                (float(point[0]), float(point [1]))

        if layer > 0 and cat != "None":
            addstring += "%d %d\n" % (layer, cat)
            Debug.msg (3, "Vline.AddLine(): type=%s, layer=%d, cat=%d coords=%s" % \
                           (key, layer, cat, coords))
        else:
            Debug.msg (3, "Vline.AddLine(): type=%s, coords=%s" % \
                           (key, coords))

        Debug.msg (4, "VEdit.AddLine(): input=%s" % addstring)

        self.__AddFeature (map=map, input=addstring, flags=flags)

    def __AddFeature (self, map, input, flags=[]):
        """Generic method to add new vector feature

        @param map   map name
        @param input feature definition in GRASS ASCII format
        @param flags additional flags
        """
        if UserSettings.Get(group='vdigit', key='snapping', subkey='value') <= 0.0:
            snap = "no"
        else:
            if UserSettings.Get(group='vdigit', key='snapToVertex', subkey='enabled') is True:
                snap = "vertex"
            else:
                snap = "node"

        command = ["v.edit", "-n", "--q", 
                   "map=%s" % map,
                   "tool=add",
                   "thresh=%f,%f" % (self.driver.GetThreshold(type='selectThresh'), self.driver.GetThreshold(type='snapping')),
                   "snap=%s" % snap]

        if UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value') != '':
            command.append("bgmap=%s" % UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value'))

        # additional flags
        for flag in flags:
            command.append(flag)

        # run the command
        Debug.msg(4, "VEdit.AddFeature(): input=%s" % input)
        vedit = gcmd.Command(cmd=command, stdin=input, stderr=None)

        # reload map (needed for v.edit)
        self.driver.ReloadMap()
        
    def DeleteSelectedLines(self):
        """Delete selected features"""
        selected = self.driver.GetSelected() # grassId

        if len(selected) <= 0:
            return False

        ids = ",".join(["%d" % v for v in selected])

        Debug.msg(4, "Digit.DeleteSelectedLines(): ids=%s" % \
                      ids)

        # delete also attributes if requested
        if UserSettings.Get(group='vdigit', key='delRecord', subkey='enabled') is True:
            layerCommand = gcmd.Command(cmd=["v.db.connect",
                                             "-g", "--q",
                                             "map=%s" % self.map],
                                        rerr=None, stderr=None)
            if layerCommand.returncode == 0:
                layers = {}
                for line in layerCommand.ReadStdOutput():
                    lineList = line.split(' ')
                    layers[int(lineList[0])] = { "table"    : lineList[1],
                                                 "key"      : lineList[2],
                                                 "database" : lineList[3],
                                                 "driver"   : lineList[4] }
                for layer in layers.keys():
                    printCats = gcmd.Command(['v.category',
                                              '--q',
                                              'input=%s' % self.map,
                                              'layer=%d' % layer,
                                              'option=print',
                                              'id=%s' % ids])
                    sql = 'DELETE FROM %s WHERE' % layers[layer]['table']
                    n_cats = 0
                    for cat in printCats.ReadStdOutput():
                        for c in cat.split('/'):
                            sql += ' cat = %d or' % int(c)
                            n_cats += 1
                    sql = sql.rstrip(' or')
                    if n_cats > 0:
                        gcmd.Command(['db.execute',
                                      '--q',
                                      'driver=%s' % layers[layer]['driver'],
                                      'database=%s' % layers[layer]['database']],
                                     stdin=sql,
                                     rerr=None, stderr=None)

        command = [ "v.edit",
                    "map=%s" % self.map,
                    "tool=delete",
                    "ids=%s" % ids]

        # run the command
        vedit = gcmd.Command(cmd=command, stderr=None)

        # reload map (needed for v.edit)
        self.driver.ReloadMap()

        return True

    def MoveSelectedLines(self, move):
        """Move selected features

        @param move X,Y direction
        """
        return self.__MoveFeature("move", None, move)

    def MoveSelectedVertex(self, coords, move):
        """Move selected vertex

        Feature geometry is changed.

        @param coords click coordinates
        @param move   X,Y direction
        """
        return self.__MoveFeature("vertexmove", coords, move)

    def __MoveFeature(self, tool, coords, move):
        """Move selected vector feature (line, vertex)

        @param tool   tool for v.edit
        @param coords click coordinates
        @param move   direction (x, y)
        """
        selected = self.driver.GetSelected()

        if len(selected) <= 0:
            return False

        ids = ",".join(["%d" % v for v in selected])

        Debug.msg(4, "Digit.MoveSelectedLines(): ids=%s, move=%s" % \
                      (ids, move))

        if UserSettings.Get(group='vdigit', key='snapping', subkey='value') <= 0.0:
            snap = "no"
        else:
            if UserSettings.Get(group='vdigit', key='snapToVertex', subkey='enabled') is True:
                snap = "vertex"
            else:
                snap = "node"


        command = ["v.edit", "--q", 
                   "map=%s" % self.map,
                   "tool=%s" % tool,
                   "ids=%s" % ids,
                   "move=%f,%f" % (float(move[0]),float(move[1])),
                   "thresh=%f,%f" % (self.driver.GetThreshold(type='selectThresh'), self.driver.GetThreshold(type='snapping')),
                   "snap=%s" % snap]

        if tool == "vertexmove":
            command.append("coords=%f,%f" % (float(coords[0]), float(coords[1])))
            command.append("-1") # modify only first selected
                         
        if UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value') != '':
            command.append("bgmap=%s" % UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value'))
                    
        # run the command
        vedit = gcmd.Command(cmd=command, stderr=None)
        
        # reload map (needed for v.edit)
        self.driver.ReloadMap()

        return True

    def AddVertex(self, coords):
        """Add new vertex to the selected line/boundary on position 'coords'

        @param coords coordinates to add vertex
        """
        return self.__ModifyVertex(coords, "vertexadd")

    def RemoveVertex(self, coords):
        """Remove vertex from the selected line/boundary on position 'coords'

        @param coords coordinates to remove vertex
        """
        return self.__ModifyVertex(coords, "vertexdel")
    
    def __ModifyVertex(self, coords, action):
        """Generic method for vertex manipulation

        @param coords coordinates
        @param action operation to perform
        """
        try:
            line = self.driver.GetSelected()[0]
        except:
            return False

        command = ["v.edit", "--q",
                   "map=%s" % self.map,
                   "tool=%s" % action,
                   "ids=%s" % line,
                   "coords=%f,%f" % (float(coords[0]),float(coords[1])),
                   "thresh=%f,%f" % (self.driver.GetThreshold(type='selectThresh'), self.driver.GetThreshold(type='snapping'))]

        # run the command
        vedit = gcmd.Command(cmd=command, stderr=None)

        # reload map (needed for v.edit)
        self.driver.ReloadMap()
        
        return True

    def SplitLine(self, coords):
        """Split selected line/boundary on position 'coords'

        @param coords coordinates to split line
        """
        try:
            line = self.driver.GetSelected()[0]
        except:
            return False

        command = ["v.edit", "--q",
                   "map=%s" % self.map,
                   "tool=break",
                   "ids=%s" % line,
                   "coords=%f,%f" % (float(coords[0]),float(coords[1])),
                   "thresh=%f" % self.driver.GetThreshold(type='selectThresh')]

        # run the command
        vedit = gcmd.Command(cmd=command, stderr=None)

        # redraw map
        self.driver.ReloadMap()
        
        return True

    def EditLine(self, line, coords):
        """Edit existing line/boundary

        @param line id of line to be modified
        @param coords list of coordinates of modified line
        """
        # remove line
        vEditDelete = gcmd.Command(['v.edit',
                                   '--q',
                                   'map=%s' % self.map,
                                   'tool=delete',
                                   'ids=%s' % line], stderr=None)

        # add line
        if len(coords) > 0:
            self.AddLine(self.map, "line", coords)

        # reload map (needed for v.edit)
        self.driver.ReloadMap()

    def __ModifyLines(self, tool):
        """Generic method to modify selected lines/boundaries

        @param tool operation to be performed by v.edit
        """
        ids = self.driver.GetSelected()

        if len(ids) <= 0:
            return False

        vEdit = ['v.edit',
                 '--q',
                 'map=%s' % self.map,
                 'tool=%s' % tool,
                 'ids=%s' % ",".join(["%d" % v for v in ids])]

        if tool in ['snap', 'connect']:
            vEdit.append("thresh=%f,%f" % (self.driver.GetThreshold(type='selectThresh'), self.driver.GetThreshold(type='snapping')))

        runCmd = gcmd.Command(vEdit)

        # reload map (needed for v.edit)
        self.driver.ReloadMap()
                        
        return True

    def FlipLine(self):
        """Flip selected lines/boundaries"""
        return self.__ModifyLines('flip')

    def MergeLine(self):
        """Merge selected lines/boundaries"""
        return self.__ModifyLines('merge')

    def BreakLine(self):
        """Break selected lines/boundaries"""
        return self.__ModifyLines('break')

    def SnapLine(self):
        """Snap selected lines/boundaries"""
        return self.__ModifyLines('snap')

    def ConnectLine(self):
        """Connect selected lines/boundaries"""
        return self.__ModifyLines('connect')

    def TypeConvForSelectedLines(self):
        """Feature type conversion for selected objects.

        Supported conversions:
         - point <-> centroid
         - line <-> boundary
        """
        return self.__ModifyLines('chtype')

    def ZBulkLine(self, pos1, pos2, value, step):
        """Provide z bulk-labeling (automated assigment of z coordinate
        to 3d lines

        @param pos1,pos2 bounding box definition for selecting lines to be labeled
        @param value starting value
        @param step  step value
        """
        gcmd.Command(['v.edit',
                      '--q',
                      'map=%s' % self.map,
                      'tool=zbulk',
                      'bbox=%f,%f,%f,%f' % (pos1[0], pos1[1], pos2[0], pos2[1]),
                      'zbulk=%f,%f' % (value, step)])


    def CopyLine(self, ids=None):
        """Copy features from (background) vector map

        @param ids list of line ids to be copied
        """
        if not ids:
            ids = self.driver.GetSelected()

        if len(ids) <= 0:
            return False

        vEdit = ['v.edit',
                 '--q',
                 'map=%s' % self.map,
                 'tool=copy',
                 'ids=%s' % ",".join(["%d" % v for v in ids])]

        if UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value') != '':
            vEdit.append('bgmap=%s' % UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value'))

        runCmd = gcmd.Command(vEdit)

        # reload map (needed for v.edit)
        self.driver.ReloadMap()
                        
        return True

    def CopyCats(self, cats, ids):
        """Copy given categories to objects with id listed in ids

        @param cats list of cats to be copied
        @param ids  ids of lines to be modified
        """
        if len(cats) == 0 or len(ids) == 0:
            return False

        # collect cats
        gcmd.Command(['v.edit',
                     '--q',
                     'map=%s' % self.map,
                     'tool=catadd',
                     'cats=%s' % ",".join(["%d" % v for v in cats]),
                     'ids=%s' % ",".join(["%d" % v for v in ids])])
        
        # reload map (needed for v.edit)
        self.driver.ReloadMap()

        return True

    def SelectLinesByQuery(self, pos1, pos2):
        """Select features by query

        @param pos1, pos2 bounding box definition
        """
        thresh = self.SelectLinesByQueryThresh()
        
        w, n = pos1
        e, s = pos2

        if UserSettings.Get(group='vdigit', key='query', subkey='box') == False: # select globaly
            vInfo = gcmd.Command(['v.info',
                                  'map=%s' % self.map,
                                  '-g'])
            for item in vInfo.ReadStdOutput():
                if 'north' in item:
                    n = float(item.split('=')[1])
                elif 'south' in item:
                    s = float(item.split('=')[1])
                elif 'east' in item:
                    e = float(item.split('=')[1])
                elif 'west' in item:
                    w = float(item.split('=')[1])

        if UserSettings.Get(group='vdigit', key='query', subkey='selection') == 0:
            qtype = 'length'
        else:
            qtype = 'dangle'

        vEdit = (['v.edit',
                  '--q',
                  'map=%s' % self.map,
                  'tool=select',
                  'bbox=%f,%f,%f,%f' % (w, n, e, s),
                  'query=%s' % qtype,
                  'thresh=0,0,%f' % thresh])

        vEditCmd = gcmd.Command(vEdit)
        
        try:
            output = vEditCmd.ReadStdOutput()[0] # first line
            ids = output.split(',') 
            ids = map(int, ids) # str -> int
        except:
            return []

        Debug.msg(4, "VEdit.SelectLinesByQuery(): %s" % \
                      ",".join(["%d" % v for v in ids]))
        
        return ids

    def GetLayers(self):
        """Return list of layers"""
        layerCommand = gcmd.Command(cmd=["v.db.connect",
                                         "-g", "--q",
                                         "map=%s" % self.map],
                                    rerr=None, stderr=None)
        if layerCommand.returncode == 0:
            layers = []
            for line in layerCommand.ReadStdOutput():
                lineList = line.split(' ')
                layers.append(int(lineList[0]))
            return layers

        return [1,]

    def Undo(self, level=-1):
        """Undo not implemented here"""
        wx.MessageBox(parent=self.mapWindow, message=_("Undo is not implemented in vedit component. "
                                                    "Use vdigit instead."),
                      caption=_("Message"), style=wx.ID_OK | wx.ICON_INFORMATION | wx.CENTRE)

    def UpdateSettings(self):
        """Update digit settigs"""
        pass
    
class VDigit(AbstractDigit):
    """
    Prototype of digitization class based on v.digit reimplementation

    Under development (wxWidgets C/C++ background)
    """
    def __init__(self, mapwindow):
        """Initialization

        @param mapwindow reference to mapwindow (MapFrame) instance
        @param settings  initial settings of digitization tool
        """
        AbstractDigit.__init__(self, mapwindow)

        try:
            self.digit = wxvdigit.Digit(self.driver.GetDevice())
        except (ImportError, NameError):
            self.digit = None

        self.toolbar = mapwindow.parent.toolbars['vdigit']

        self.UpdateSettings()
        
    def __del__(self):
        del self.digit
        
    def AddPoint (self, map, point, x, y, z=None):
        """Add new point/centroid

        @param map   map name (unused, for compatability with VEdit)
        @param point feature type (if true point otherwise centroid)
        @param x,y,z coordinates
        """
        if UserSettings.Get(group='vdigit', key="categoryMode", subkey='selection') == 2:
            layer = -1 # -> no category
            cat   = -1
        else:
            layer = UserSettings.Get(group='vdigit', key="layer", subkey='value')
            cat   = self.SetCategory()

        if point:
            type = wxvdigit.GV_POINT 
        else:
            type = wxvdigit.GV_CENTROID 

        snap, thresh = self.__getSnapThreshold()

        if z:
            ret = self.digit.AddLine(type, [x, y, z], layer, cat,
                                     str(UserSettings.Get(group='vdigit', key="backgroundMap", subkey='value')), snap, thresh)
        else:
            ret = self.digit.AddLine(type, [x, y], layer, cat,
                                     str(UserSettings.Get(group='vdigit', key="backgroundMap", subkey='value')), snap, thresh)

        if ret == -1:
            raise gcmd.DigitError, _("Adding new feature to vector map <%s> failed.") % map

        self.toolbar.EnableUndo()
        
    def AddLine (self, map, line, coords):
        """Add line/boundary

        @param map    map name (unused, for compatability with VEdit)
        @param line   feature type (if True line, otherwise boundary)
        @param coords list of coordinates
        """
        if len(coords) < 2:
            return
        
        if UserSettings.Get(group='vdigit', key="categoryMode", subkey='selection') == 2:
            layer = -1 # -> no category
            cat   = -1
        else:
            layer = UserSettings.Get(group='vdigit', key="layer", subkey='value')
            cat   = self.SetCategory()
        
        if line:
            type = wxvdigit.GV_LINE
        else:
            type = wxvdigit.GV_BOUNDARY
        
        listCoords = []
        for c in coords:
            for x in c:
                listCoords.append(x)
        
        snap, thresh = self.__getSnapThreshold()
        
        ret = self.digit.AddLine(type, listCoords, layer, cat,
                                 str(UserSettings.Get(group='vdigit', key="backgroundMap", subkey='value')), snap, thresh)
        
        if ret == -1:
            raise gcmd.DigitError, _("Adding new feature to vector map <%s> failed.") % map
        
        self.toolbar.EnableUndo()
        
    def DeleteSelectedLines(self):
        """Delete selected features

        @return number of deleted lines
        """
        nlines = self.digit.DeleteLines(UserSettings.Get(group='vdigit', key='delRecord', subkey='enabled'))
        
        if nlines > 0:
            self.toolbar.EnableUndo()
            
        return nlines

    def MoveSelectedLines(self, move):
        """Move selected features

        @param move direction (x, y)
        """
        snap, thresh = self.__getSnapThreshold()
        
        nlines = self.digit.MoveLines(move[0], move[1], 0.0, # TODO 3D
                                      str(UserSettings.Get(group='vdigit', key="backgroundMap", subkey='value')), snap, thresh)
        
        if nlines > 0:
            self.toolbar.EnableUndo()
        
        return nlines

    def MoveSelectedVertex(self, coords, move):
        """Move selected vertex of the line

        @param coords click coordinates
        @param move   X,Y direction

        @return 1 vertex moved
        @return 0 vertex not moved (not found, line is not selected)
        """
        snap, thresh = self.__getSnapThreshold()

        moved = self.digit.MoveVertex(coords[0], coords[1], 0.0, # TODO 3D
                                      move[0], move[1], 0.0,
                                      str(UserSettings.Get(group='vdigit', key="backgroundMap", subkey='value')), snap,
                                      self.driver.GetThreshold(type='selectThresh'), thresh)

        if moved:
            self.toolbar.EnableUndo()

        return moved

    def AddVertex(self, coords):
        """Add new vertex to the selected line/boundary on position 'coords'

        @param coords coordinates to add vertex

        @return 1 vertex added
        @return 0 nothing changed
        @return -1 on failure
        """
        added = self.digit.ModifyLineVertex(1, coords[0], coords[1], 0.0, # TODO 3D
                                            self.driver.GetThreshold(type='selectThresh'))

        if added > 0:
            self.toolbar.EnableUndo()

        return added

    def RemoveVertex(self, coords):
        """Remove vertex from the selected line/boundary on position 'coords'

        @param coords coordinates to remove vertex

        @return 1 vertex removed
        @return 0 nothing changed
        @return -1 on failure
        """
        deleted = self.digit.ModifyLineVertex(0, coords[0], coords[1], 0.0, # TODO 3D
                                              self.driver.GetThreshold(type='selectThresh'))

        if deleted > 0:
            self.toolbar.EnableUndo()

        return deleted


    def SplitLine(self, coords):
        """Split selected line/boundary on position 'coords'

        @param coords coordinates to split line

        @return 1 line modified
        @return 0 nothing changed
        @return -1 error
        """
        ret = self.digit.SplitLine(coords[0], coords[1], 0.0, # TODO 3D
                                   self.driver.GetThreshold('selectThresh'))

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def EditLine(self, line, coords):
        """Edit existing line/boundary

        @param line id of line to be modified
        @param coords list of coordinates of modified line

        @return feature id of new line
        @return -1 on error
        """
        try:
            lineid = line[0]
        except:
            lineid = -1

        if len(coords) < 2:
            self.DeleteSelectedLines()
            return 0
            
        listCoords = []
        for c in coords:
            for x in c:
                listCoords.append(x)

        snap, thresh = self.__getSnapThreshold()
        
        ret = self.digit.RewriteLine(lineid, listCoords,
                                     str(UserSettings.Get(group='vdigit', key="backgroundMap", subkey='value')), snap, thresh)

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def FlipLine(self):
        """Flip selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        ret = self.digit.FlipLines()

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def MergeLine(self):
        """Merge selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        ret = self.digit.MergeLines()

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def BreakLine(self):
        """Break selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        ret = self.digit.BreakLines()

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def SnapLine(self):
        """Snap selected lines/boundaries

        @return on success
        @return -1 on error
        """
        snap, thresh = self.__getSnapThreshold()
        ret = self.digit.SnapLines(thresh)
        
        if ret == 0:
            self.toolbar.EnableUndo()

        return ret

    def ConnectLine(self):
        """Connect selected lines/boundaries

        @return 1 lines connected
        @return 0 lines not connected
        @return -1 on error
        """
        snap, thresh = self.__getSnapThreshold()
        ret = self.digit.ConnectLines(thresh)

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret
        
    def CopyLine(self, ids=[]):
        """Copy features from (background) vector map

        @param ids list of line ids to be copied

        @return number of copied features
        @return -1 on error
        """
        bgmap = str(UserSettings.Get(group='vdigit', key='backgroundMap', subkey='value'))
        if len(bgmap) > 0:
            ret = self.digit.CopyLines(ids, bgmap)
        else:
            ret = self.digit.CopyLines(ids, None)

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def CopyCats(self, cats, ids):
        """Copy given categories to objects with id listed in ids

        @param cats list of cats to be copied
        @param ids  ids of lines to be modified

        @return number of modified features
        @return -1 on error
        """
        if len(cats) == 0 or len(ids) == 0:
            return 0

        ret = self.digit.CopyCats(cats, ids)

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def SelectLinesByQuery(self, pos1, pos2):
        """Select features by query

        @param pos1, pos2 bounding box definition
        """
        thresh = self.SelectLinesByQueryThresh()
        
        w, n = pos1
        e, s = pos2

        query = wxvdigit.QUERY_UNKNOWN
        if UserSettings.Get(group='vdigit', key='query', subkey='selection') == 0:
            query = wxvdigit.QUERY_LENGTH
        else:
            query = wxvdigit.QUERY_DANGLE

        type = wxvdigit.GV_POINTS | wxvdigit.GV_LINES # TODO: 3D
        
        ids = self.digit.SelectLinesByQuery(w, n, 0.0, e, s, 1000.0,
                                            UserSettings.Get(group='vdigit', key='query', subkey='box'),
                                            query, type, thresh)

        Debug.msg(4, "VDigit.SelectLinesByQuery(): %s" % \
                      ",".join(["%d" % v for v in ids]))
        
        return ids

    def GetLineCats(self, line=-1):
        """Get layer/category pairs from given (selected) line
        
        @param line feature id (-1 for first selected line)
        """
        return self.digit.GetLineCats(line)

    def SetLineCats(self, line, layer, cats, add=True):
        """Set categories for given line and layer

        @param line feature id
        @param layer layer number (-1 for first selected line)
        @param cats list of categories
        @param add if True to add, otherwise do delete categories

        @return new feature id (feature need to be rewritten)
        @return -1 on error
        """
        ret = self.digit.SetLineCats(line, layer, cats, add)

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def GetLayers(self):
        """Get list of layers"""
        return self.digit.GetLayers()

    def TypeConvForSelectedLines(self):
        """Feature type conversion for selected objects.

        Supported conversions:
         - point <-> centroid
         - line <-> boundary

        @return number of modified features
        @return -1 on error
        """
        ret = self.digit.TypeConvLines()

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def Undo(self, level=-1):
        """Undo action

        @param level levels to undo (0 to revert all)

        @return id of current changeset
        """
        try:
            ret = self.digit.Undo(level)
        except SystemExit:
            ret = -2

        if ret == -2:
            raise gcmd.DigitError, _("Undo failed, data corrupted.")

        self.mapWindow.UpdateMap(render=False)
        
        if ret < 0: # disable undo tool
            self.toolbar.EnableUndo(False)

    def GetUndoLevel(self):
        """Get undo level (number of active changesets)"""
        return self.digit.GetUndoLevel()

    def UpdateSettings(self):
        """Update digit settigs"""
        self.digit.UpdateSettings(UserSettings.Get(group='vdigit', key='breakLines',
                                                   subkey='enabled'))
        
    def __getSnapThreshold(self):
        """Get snap mode and threshold value

        @return (snap, thresh)
        """
        thresh = self.driver.GetThreshold()

        if thresh > 0.0:
            if UserSettings.Get(group='vdigit', key='snapToVertex', subkey='enabled') is True:
                snap = wxvdigit.SNAPVERTEX
            else:
                snap = wxvdigit.SNAP
        else:
            snap = wxvdigit.NO_SNAP

        return (snap, thresh)

if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') == 'vedit':
    class Digit(VEdit):
        """Default digit class"""
        def __init__(self, mapwindow):
            VEdit.__init__(self, mapwindow)
            self.type = 'vedit'
else:
    class Digit(VDigit):
        """Default digit class"""
        def __init__(self, mapwindow):
            VDigit.__init__(self, mapwindow)
            self.type = 'vdigit'
            
        def __del__(self):
            VDigit.__del__(self)
            
class AbstractDisplayDriver:
    """Abstract classs for display driver"""
    def __init__(self, parent, mapwindow):
        """Initialization

        @param parent
        @param mapwindow reference to mapwindow (MFrame)
        """
        self.parent      = parent
        self.mapwindow   = mapwindow
        
        self.ids         = {}   # dict[g6id] = [pdcId]
        self.selected    = []   # list of selected objects (grassId!)

    def GetThreshold(self, type='snapping', value=None, units=None):
        """Return threshold in map units

        @param value threshold to be set up
        @param units units (map, screen)
        """
        if value is None:
            value = UserSettings.Get(group='vdigit', key=type, subkey='value')

        if units is None:
            units = UserSettings.Get(group='vdigit', key=type, subkey='units')

        if units == "screen pixels":
            # pixel -> cell
            reg = self.mapwindow.Map.region
            if reg['nsres'] > reg['ewres']:
                res = reg['nsres']
            else:
                res = reg['ewres']

            threshold = value * res
        else:
            threshold = value

        Debug.msg(4, "AbstractDisplayDriver.GetThreshold(): type=%s, thresh=%f" % (type, threshold))
        
        return threshold

class CDisplayDriver(AbstractDisplayDriver):
    """
    Display driver using grass6_wxdriver module
    """
    def __init__(self, parent, mapwindow):
        """Initialization

        @param parent
        @param mapwindow reference to mapwindow (MFrame)
        """
        AbstractDisplayDriver.__init__(self, parent, mapwindow)

        self.mapWindow = mapwindow

        # initialize wx display driver
        try:
            self.__display = wxvdigit.DisplayDriver(mapwindow.pdcVector)
        except:
            self.__display = None
            
        self.UpdateSettings()

    def GetDevice(self):
        """Get device"""
        return self.__display
    
    def SetDevice(self, pdc):
        """Set device for driver

        @param pdc wx.PseudoDC instance
        """
        self.__display.SetDevice(pdc)
            
    def Reset(self, map):
        """Reset map

        Open or close the vector map by driver.

        @param map map name or None to close the map

        @return 0 on success (close map)
        @return topo level on success (open map)
        @return non-zero (close map)
        @return -1 on error (open map)
        """
        if map:
            name, mapset = map.split('@')
            try:
                if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') == 'vedit':
                    ret = self.__display.OpenMap(str(name), str(mapset), False)
                else:
                    ret = self.__display.OpenMap(str(name), str(mapset), True)
            except SystemExit:
                ret = -1
        else:
            ret = self.__display.CloseMap()

        return ret
    
    def ReloadMap(self):
        """Reload map (close and re-open).

        Needed for v.edit, TODO: get rid of that..."""
        
        Debug.msg(4, "CDisplayDriver.ReloadMap():")
        self.__display.ReloadMap()

    def DrawMap(self):
        """Draw vector map layer content

        @return wx.Image instance
        """
        nlines = self.__display.DrawMap(True) # force
        Debug.msg(3, "CDisplayDriver.DrawMap(): nlines=%d" % nlines)

        return nlines

    def SelectLinesByBox(self, begin, end, type=0):
        """Select vector features by given bounding box.

        If type is given, only vector features of given type are selected.

        @param begin,end bounding box definition
        @param type      select only objects of given type
        """
        x1, y1 = begin
        x2, y2 = end

        nselected = self.__display.SelectLinesByBox(x1, y1, -1.0 * wxvdigit.PORT_DOUBLE_MAX,
                                                    x2, y2, wxvdigit.PORT_DOUBLE_MAX,
                                                    type)
        
        Debug.msg(4, "CDisplayDriver.SelectLinesByBox(): selected=%d" % \
                      nselected)
        
        return nselected

    def SelectLineByPoint(self, point, type=0):
        """Select vector feature by coordinates of click point (in given threshold).

        If type is given, only vector features of given type are selected.

        @param point click coordinates (bounding box given by threshold)
        @param type  select only objects of given type
        """
        pointOnLine = self.__display.SelectLineByPoint(point[0], point[1], 0.0,
                                                       self.GetThreshold(type='selectThresh'),
                                                       type, 0); # without_z

        if len(pointOnLine) > 0:
            Debug.msg(4, "CDisplayDriver.SelectLineByPoint(): pointOnLine=%f,%f" % \
                          (pointOnLine[0], pointOnLine[1]))
            return pointOnLine
        else:
            Debug.msg(4, "CDisplayDriver.SelectLineByPoint(): no line found")
            return None
        
    def GetSelected(self, grassId=True):
        """Return ids of selected vector features
        
        @param grassId if grassId is True returns GRASS ids, otherwise
        internal ids of objects drawn in PseudoDC"""
        if grassId:
            selected = self.__display.GetSelected(True)
        else:
            selected = self.__display.GetSelected(False)
            
        Debug.msg(4, "CDisplayDriver.GetSelected(): grassId=%d, ids=%s" % \
                      (grassId, (",".join(["%d" % v for v in selected]))))
            
        return selected

    def GetDuplicates(self):
        """Return ids of (selected) duplicated vector features
        """
        # -> id : (list of ids)
        dupl = dict(self.__display.GetDuplicates())

        vdigitComp = UserSettings.Get(group='advanced', key='digitInterface', subkey='type')

        # -> id : ((id, cat), ...)
        dupl_full = {}
        for key in dupl.keys():
            dupl_full[key] = []
            for id in dupl[key]:
                catStr = ''

                # categories not supported for v.edit !
                if vdigitComp == 'vdigit':
                    cats = self.parent.GetLineCats(line=id)

                    for layer in cats.keys():
                        if len(cats[layer]) > 0:
                            catStr = "%d: (" % layer
                            for cat in cats[layer]:
                                catStr += "%d," % cat
                            catStr = catStr.rstrip(',')
                            catStr += ')'

                dupl_full[key].append([id, catStr])

        return dupl_full

    def GetSelectedVertex(self, coords):
        """Get PseudoDC id(s) of vertex (of selected line)
        on position 'coords'

        @param coords click position
        """
        x, y = coords

        id = self.__display.GetSelectedVertex(x, y, self.GetThreshold(type='selectThresh'))

        Debug.msg(4, "CDisplayDriver.GetSelectedVertex(): id=%s" % \
                      (",".join(["%d" % v for v in id])))

        return id 

    def SetSelected(self, id):
        """Set selected vector features

        @param id line id to be selected
        """
        Debug.msg(4, "CDisplayDriver.SetSelected(): id=%s" % \
                  ",".join(["%d" % v for v in id]))

        self.__display.SetSelected(id)

    def UnSelect(self, id):
        """Unselect vector features

        @param id list of feature id(s)
        """

        Debug.msg(4, "CDisplayDriver.UnSelect(): id=%s" % \
                      ",".join(["%d" % v for v in id]))
        
        self.__display.UnSelect(id)

    def UpdateRegion(self):
        """Set geographical region
        
        Needed for 'cell2pixel' conversion"""
        
        map = self.mapwindow.Map
        reg = map.region
        
        self.__display.SetRegion(reg['n'],
                                 reg['s'],
                                 reg['e'],
                                 reg['w'],
                                 reg['nsres'],
                                 reg['ewres'],
                                 reg['center_easting'],
                                 reg['center_northing'],
                                 map.width, map.height)

    def GetMapBoundingBox(self):
        """Return bounding box of given vector map layer

        @return (w,s,b,e,n,t)
        """

        return self.__display.GetMapBoundingBox()

    def DrawSelected(self, draw=True):
        """Show/hide selected features"""
        self.__display.DrawSelected(draw)
        
    def UpdateSettings(self):
        """Update display driver settings"""
        # TODO map units

        if not self.__display:
            return
        
        self.__display.UpdateSettings (wx.Color(UserSettings.Get(group='vdigit', key='symbolHighlight', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolHighlight', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolHighlight', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='checkForDupl', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolHighlightDupl', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolHighlightDupl', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolHighlightDupl', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolPoint', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolPoint', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolPoint', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolPoint', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolLine', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolLine', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolLine', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolLine', subkey='color')[2],
                                           255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolBoundaryNo', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolBoundaryNo', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolBoundaryNo', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolBoundaryNo', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolBoundaryOne', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolBoundaryOne', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolBoundaryOne', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolBoundaryOne', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolBoundaryTwo', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolBoundaryTwo', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolBoundaryTwo', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolBoundaryTwo', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolCentroidIn', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolCentroidIn', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolCentroidIn', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolCentroidIn', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolCentroidOut', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolCentroidOut', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolCentroidOut', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolCentroidOut', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolCentroidDup', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolCentroidDup', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolCentroidDup', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolCentroidDup', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolNodeOne', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolNodeOne', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolNodeOne', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolNodeOne', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolNodeTwo', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolNodeTwo', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolNodeTwo', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolNodeTwo', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolVertex', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolVertex', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolVertex', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolVertex', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolArea', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolArea', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolArea', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolArea', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='symbolDirection', subkey='enabled'),
                                       wx.Color(UserSettings.Get(group='vdigit', key='symbolDirection', subkey='color')[0],
                                                UserSettings.Get(group='vdigit', key='symbolDirection', subkey='color')[1],
                                                UserSettings.Get(group='vdigit', key='symbolDirection', subkey='color')[2],
                                                255).GetRGB(),
                                       UserSettings.Get(group='vdigit', key='lineWidth', subkey='value'))

class VDigitSettingsDialog(wx.Dialog):
    """
    Standard settings dialog for digitization purposes
    """
    def __init__(self, parent, title, style=wx.DEFAULT_DIALOG_STYLE):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=title, style=style)

        self.parent = parent # mapdisplay.BufferedWindow class instance

        # notebook
        notebook = wx.Notebook(parent=self, id=wx.ID_ANY, style=wx.BK_DEFAULT)
        self.__CreateSymbologyPage(notebook)
        parent.digit.SetCategory() # update category number (next to use)
        self.__CreateGeneralPage(notebook)
        self.__CreateAttributesPage(notebook)
        self.__CreateQueryPage(notebook)

        # buttons
        btnApply = wx.Button(self, wx.ID_APPLY)
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnSave = wx.Button(self, wx.ID_SAVE)
        btnSave.SetDefault()

        # bindigs
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnApply.SetToolTipString(_("Apply changes for this session"))
        btnApply.SetDefault()
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnSave.SetToolTipString(_("Close dialog and save changes to user settings file"))
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        btnCancel.SetToolTipString(_("Close dialog and ignore changes"))
        
        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnApply)
        btnSizer.AddButton(btnSave)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item=notebook, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(item=btnSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)
        
        self.Bind(wx.EVT_CLOSE, self.OnCancel)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def __CreateSymbologyPage(self, notebook):
        """Create notebook page concerning with symbology settings"""

        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Symbology"))

        sizer = wx.BoxSizer(wx.VERTICAL)
        
        flexSizer = wx.FlexGridSizer (cols=3, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(0)

        self.symbology = {}
        for label, key in self.__SymbologyData():
            textLabel = wx.StaticText(panel, wx.ID_ANY, label)
            color = csel.ColourSelect(panel, id=wx.ID_ANY,
                                      colour=UserSettings.Get(group='vdigit', key=key, subkey='color'), size=(25, 25))
            isEnabled = UserSettings.Get(group='vdigit', key=key, subkey='enabled')
            if isEnabled is not None:
                enabled = wx.CheckBox(panel, id=wx.ID_ANY, label="")
                enabled.SetValue(isEnabled)
                self.symbology[key] = (enabled, color)
            else:
                enabled = (1, 1)
                self.symbology[key] = (None, color)
            
            flexSizer.Add(textLabel, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
            flexSizer.Add(enabled, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
            flexSizer.Add(color, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)
            color.SetName("GetColour")
        
        sizer.Add(item=flexSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=10)
        
        panel.SetSizer(sizer)
        
        return panel

    def __CreateGeneralPage(self, notebook):
        """Create notebook page concerning with symbology settings"""

        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("General"))

        border = wx.BoxSizer(wx.VERTICAL)
        
        #
        # display section
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Display"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols=3, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(0)
        # line width
        text = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Line width"))
        self.lineWidthValue = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(75, -1),
                                          initial=UserSettings.Get(group='vdigit', key="lineWidth", subkey='value'),
                                          min=1, max=1e6)
        units = wx.StaticText(parent=panel, id=wx.ID_ANY, size=(115, -1),
                              label=UserSettings.Get(group='vdigit', key="lineWidth", subkey='units'),
                              style=wx.ALIGN_LEFT)
        flexSizer.Add(text, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.lineWidthValue, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(units, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
                      border=10)

        sizer.Add(item=flexSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=1)
        border.Add(item=sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=5)

        #
        # snapping section
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Snapping"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer1 = wx.FlexGridSizer (cols=3, hgap=5, vgap=5)
        flexSizer1.AddGrowableCol(0)
        flexSizer2 = wx.FlexGridSizer (cols=2, hgap=5, vgap=5)
        flexSizer2.AddGrowableCol(0)
        # snapping
        text = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Snapping threshold"))
        self.snappingValue = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(75, -1),
                                         initial=UserSettings.Get(group='vdigit', key="snapping", subkey='value'),
                                         min=0, max=1e6)
        self.snappingValue.Bind(wx.EVT_SPINCTRL, self.OnChangeSnappingValue)
        self.snappingUnit = wx.Choice(parent=panel, id=wx.ID_ANY, size=(125, -1),
                                      choices=["screen pixels", "map units"])
        self.snappingUnit.SetStringSelection(UserSettings.Get(group='vdigit', key="snapping", subkey='units'))
        self.snappingUnit.Bind(wx.EVT_CHOICE, self.OnChangeSnappingUnits)
        flexSizer1.Add(text, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer1.Add(self.snappingValue, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer1.Add(self.snappingUnit, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)
        # background map
        text = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Background vector map"))
        self.backgroundMap = gselect.Select(parent=panel, id=wx.ID_ANY, size=(200,-1),
                                           type="vector", exceptOf=[self.parent.digit.map])
        self.backgroundMap.SetValue(UserSettings.Get(group='vdigit', key="backgroundMap", subkey='value'))
        self.backgroundMap.Bind(wx.EVT_TEXT, self.OnChangeBackgroundMap)
        flexSizer2.Add(text, proportion=1, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer2.Add(self.backgroundMap, proportion=1, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        #flexSizer.Add(self.snappingUnit, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        vertexSizer = wx.BoxSizer(wx.VERTICAL)
        self.snapVertex = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                      label=_("Snap also to vertex"))
        self.snapVertex.SetValue(UserSettings.Get(group='vdigit', key="snapToVertex", subkey='enabled'))
        vertexSizer.Add(item=self.snapVertex, proportion=0, flag=wx.EXPAND)
        self.mapUnits = self.parent.MapWindow.Map.ProjInfo()['units']
        self.snappingInfo = wx.StaticText(parent=panel, id=wx.ID_ANY,
                                          label=_("Snapping threshold is %(value).1f %(units)s") % \
                                              {'value' : self.parent.digit.driver.GetThreshold(),
                                               'units' : self.mapUnits})
        vertexSizer.Add(item=self.snappingInfo, proportion=0,
                        flag=wx.ALL | wx.EXPAND, border=1)

        sizer.Add(item=flexSizer1, proportion=1, flag=wx.TOP | wx.LEFT | wx.EXPAND, border=1)
        sizer.Add(item=flexSizer2, proportion=1, flag=wx.TOP | wx.LEFT | wx.EXPAND, border=1)
        sizer.Add(item=vertexSizer, proportion=1, flag=wx.BOTTOM | wx.LEFT | wx.EXPAND, border=1)
        border.Add(item=sizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=5)

        #
        # select box
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Select vector features"))
        # feature type
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        inSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.selectFeature = {}
        for feature in ('Point', 'Line',
                        'Centroid', 'Boundary'):
            chkbox = wx.CheckBox(parent=panel, label=feature)
            self.selectFeature[feature] = chkbox.GetId()
            chkbox.SetValue(UserSettings.Get(group='vdigit', key='selectFeature'+feature, subkey='enabled'))
            inSizer.Add(item=chkbox, proportion=0,
                        flag=wx.EXPAND | wx.ALL, border=5)
        sizer.Add(item=inSizer, proportion=0, flag=wx.EXPAND)
        # threshold
        flexSizer = wx.FlexGridSizer (cols=3, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(0)
        text = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Select threshold"))
        self.selectThreshValue = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(75, -1),
                                             initial=UserSettings.Get(group='vdigit', key="selectThresh", subkey='value'),
                                             min=1, max=1e6)
        units = wx.StaticText(parent=panel, id=wx.ID_ANY, size=(115, -1),
                              label=UserSettings.Get(group='vdigit', key="lineWidth", subkey='units'),
                              style=wx.ALIGN_LEFT)
        flexSizer.Add(text, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.selectThreshValue, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(units, proportion=0, flag=wx.ALIGN_RIGHT | wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
                      border=10)

        self.checkForDupl = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                        label=_("Check for duplicates"))
        self.checkForDupl.SetValue(UserSettings.Get(group='vdigit', key="checkForDupl", subkey='enabled'))
        flexSizer.Add(item=self.checkForDupl, proportion=0, flag=wx.EXPAND)

        sizer.Add(item=flexSizer, proportion=0, flag=wx.EXPAND)
        border.Add(item=sizer, proportion=0, flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        #
        # digitize lines box
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Digitize line features"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        self.intersect = wx.CheckBox(parent=panel, label=_("Break lines on intersection"))
        self.intersect.SetValue(UserSettings.Get(group='vdigit', key='breakLines', subkey='enabled'))
        if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') == 'vedit':
            self.intersect.Enable(False)
        
        sizer.Add(item=self.intersect, proportion=0, flag=wx.ALL | wx.EXPAND, border=1)

        border.Add(item=sizer, proportion=0, flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        #
        # save-on-exit box
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Save changes"))
        # save changes on exit?
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        self.save = wx.CheckBox(parent=panel, label=_("Save changes on exit"))
        self.save.SetValue(UserSettings.Get(group='vdigit', key='saveOnExit', subkey='enabled'))
        sizer.Add(item=self.save, proportion=0, flag=wx.ALL | wx.EXPAND, border=1)
        border.Add(item=sizer, proportion=0, flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        panel.SetSizer(border)
        
        return panel

    def __CreateQueryPage(self, notebook):
        """Create notebook page for query tool"""

        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Query tool"))

        border = wx.BoxSizer(wx.VERTICAL)

        #
        # query tool box
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Choose query tool"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        LocUnits = self.parent.MapWindow.Map.ProjInfo()['units']

        self.queryBox = wx.CheckBox(parent=panel, id=wx.ID_ANY, label=_("Select by box"))
        self.queryBox.SetValue(UserSettings.Get(group='vdigit', key="query", subkey='box'))

        sizer.Add(item=self.queryBox, proportion=0, flag=wx.ALL | wx.EXPAND, border=1)
        sizer.Add((0, 5))

        #
        # length
        #
        self.queryLength = wx.RadioButton(parent=panel, id=wx.ID_ANY, label=_("length"))
        self.queryLength.Bind(wx.EVT_RADIOBUTTON, self.OnChangeQuery)
        sizer.Add(item=self.queryLength, proportion=0, flag=wx.ALL | wx.EXPAND, border=1)
        flexSizer = wx.FlexGridSizer (cols=4, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(0)
        txt = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Select lines"))
        self.queryLengthSL = wx.Choice (parent=panel, id=wx.ID_ANY, 
                                        choices = [_("shorter than"), _("longer than")])
        self.queryLengthSL.SetSelection(UserSettings.Get(group='vdigit', key="queryLength", subkey='than-selection'))
        self.queryLengthValue = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(100, -1),
                                            initial=1,
                                            min=0, max=1e6)
        self.queryLengthValue.SetValue(UserSettings.Get(group='vdigit', key="queryLength", subkey='thresh'))
        units = wx.StaticText(parent=panel, id=wx.ID_ANY, label="%s" % LocUnits)
        flexSizer.Add(txt, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.queryLengthSL, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(self.queryLengthValue, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(units, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(item=flexSizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=1)

        #
        # dangle
        #
        self.queryDangle = wx.RadioButton(parent=panel, id=wx.ID_ANY, label=_("dangle"))
        self.queryDangle.Bind(wx.EVT_RADIOBUTTON, self.OnChangeQuery)
        sizer.Add(item=self.queryDangle, proportion=0, flag=wx.ALL | wx.EXPAND, border=1)
        flexSizer = wx.FlexGridSizer (cols=4, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(0)
        txt = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Select dangles"))
        self.queryDangleSL = wx.Choice (parent=panel, id=wx.ID_ANY, 
                                        choices = [_("shorter than"), _("longer than")])
        self.queryDangleSL.SetSelection(UserSettings.Get(group='vdigit', key="queryDangle", subkey='than-selection'))
        self.queryDangleValue = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(100, -1),
                                       initial=1,
                                       min=0, max=1e6)
        self.queryDangleValue.SetValue(UserSettings.Get(group='vdigit', key="queryDangle", subkey='thresh'))
        units = wx.StaticText(parent=panel, id=wx.ID_ANY, label="%s" % LocUnits)
        flexSizer.Add(txt, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.queryDangleSL, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(self.queryDangleValue, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(units, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(item=flexSizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=1)

        if UserSettings.Get(group='vdigit', key="query", subkey='selection') == 0:
            self.queryLength.SetValue(True)
        else:
            self.queryDangle.SetValue(True)

        # enable & disable items
        self.OnChangeQuery(None)

        border.Add(item=sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=5)

        panel.SetSizer(border)
        
        return panel

    def __CreateAttributesPage(self, notebook):
        """Create notebook page for query tool"""

        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Attributes"))

        border = wx.BoxSizer(wx.VERTICAL)

        #
        # add new record
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Digitize new feature"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        # checkbox
        self.addRecord = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                     label=_("Add new record into table"))
        self.addRecord.SetValue(UserSettings.Get(group='vdigit', key="addRecord", subkey='enabled'))
        sizer.Add(item=self.addRecord, proportion=0, flag=wx.ALL | wx.EXPAND, border=1)
        # settings
        flexSizer = wx.FlexGridSizer(cols=2, hgap=3, vgap=3)
        flexSizer.AddGrowableCol(0)
        settings = ((_("Layer"), 1), (_("Category"), 1), (_("Mode"), _("Next to use")))
        # layer
        text = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Layer"))
        if self.parent.digit.map:
            layers = map(str, self.parent.digit.GetLayers())
            if len(layers) == 0:
                layers = [str(UserSettings.Get(group='vdigit', key="layer", subkey='value')), ]
        else:
            layers = [str(UserSettings.Get(group='vdigit', key="layer", subkey='value')), ]
        
        self.layer = wx.Choice(parent=panel, id=wx.ID_ANY, size=(125, -1),
                               choices=layers)
        self.layer.SetStringSelection(str(UserSettings.Get(group='vdigit', key="layer", subkey='value')))
        flexSizer.Add(item=text, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item=self.layer, proportion=0,
                      flag=wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        # category number
        text = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Category number"))
        self.category = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(125, -1),
                                    initial=UserSettings.Get(group='vdigit', key="category", subkey='value'),
                                    min=-1e9, max=1e9) 
        if UserSettings.Get(group='vdigit', key="categoryMode", subkey='selection') != 1:
            self.category.Enable(False)
        flexSizer.Add(item=text, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item=self.category, proportion=0,
                      flag=wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        # category mode
        text = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Category mode"))
        self.categoryMode = wx.Choice(parent=panel, id=wx.ID_ANY, size=(125, -1),
                                      choices=[_("Next to use"), _("Manual entry"), _("No category")])
        self.categoryMode.SetSelection(UserSettings.Get(group='vdigit', key="categoryMode", subkey='selection'))
        flexSizer.Add(item=text, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item=self.categoryMode, proportion=0,
                      flag=wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)

        sizer.Add(item=flexSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=1)
        border.Add(item=sizer, proportion=0,
                   flag=wx.ALL | wx.EXPAND, border=5)

        #
        # delete existing record
        #
        box   = wx.StaticBox (parent=panel, id=wx.ID_ANY, label=" %s " % _("Delete existing feature(s)"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        # checkbox
        self.deleteRecord = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                        label=_("Delete record from table"))
        self.deleteRecord.SetValue(UserSettings.Get(group='vdigit', key="delRecord", subkey='enabled'))
        sizer.Add(item=self.deleteRecord, proportion=0, flag=wx.ALL | wx.EXPAND, border=1)
        border.Add(item=sizer, proportion=0,
                   flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=5)

        # bindings
        self.Bind(wx.EVT_CHECKBOX, self.OnChangeAddRecord, self.addRecord)
        self.Bind(wx.EVT_CHOICE, self.OnChangeCategoryMode, self.categoryMode)
        self.Bind(wx.EVT_CHOICE, self.OnChangeLayer, self.layer)

        panel.SetSizer(border)
        
        return panel

    def __SymbologyData(self):
        """
        Data for __CreateSymbologyPage()

        label | checkbox | color
        """

        return (
            #            ("Background", "symbolBackground"),
            (_("Highlight"), "symbolHighlight"),
            (_("Highlight (duplicates)"), "symbolHighlightDupl"),
            (_("Point"), "symbolPoint"),
            (_("Line"), "symbolLine"),
            (_("Boundary (no area)"), "symbolBoundaryNo"),
            (_("Boundary (one area)"), "symbolBoundaryOne"),
            (_("Boundary (two areas)"), "symbolBoundaryTwo"),
            (_("Centroid (in area)"), "symbolCentroidIn"),
            (_("Centroid (outside area)"), "symbolCentroidOut"),
            (_("Centroid (duplicate in area)"), "symbolCentroidDup"),
            (_("Node (one line)"), "symbolNodeOne"),
            (_("Node (two lines)"), "symbolNodeTwo"),
            (_("Vertex"), "symbolVertex"),
            (_("Area (closed boundary + centroid)"), "symbolArea"),
            (_("Direction"), "symbolDirection"),)

    def OnChangeCategoryMode(self, event):
        """Change category mode"""

        mode = event.GetSelection()
        UserSettings.Set(group='vdigit', key="categoryMode", subkey='selection', value=mode)
        if mode == 1: # manual entry
            self.category.Enable(True)
        elif self.category.IsEnabled(): # disable
            self.category.Enable(False)

        if mode == 2 and self.addRecord.IsChecked(): # no category
            self.addRecord.SetValue(False)

        self.parent.digit.SetCategory()
        self.category.SetValue(UserSettings.Get(group='vdigit', key='category', subkey='value'))

    def OnChangeLayer(self, event):
        """Layer changed"""
        layer = int(event.GetString())
        if layer > 0:
            UserSettings.Set(group='vdigit', key='layer', subkey='value', value=layer)
            self.parent.digit.SetCategory()
            self.category.SetValue(UserSettings.Get(group='vdigit', key='category', subkey='value'))
            
        event.Skip()

    def OnChangeAddRecord(self, event):
        """Checkbox 'Add new record' status changed"""
        self.category.SetValue(self.parent.digit.SetCategory())
            
    def OnChangeSnappingValue(self, event):
        """Change snapping value - update static text"""
        value = self.snappingValue.GetValue()
        
        if self.snappingUnit.GetStringSelection() == "map units":
            threshold = value
        else:
            threshold = self.parent.digit.driver.GetThreshold(value=value)

        self.snappingInfo.SetLabel(_("Snapping threshold is %(value).1f %(units)s") % 
                                   {'value' : threshold,
                                    'units' : self.mapUnits})

        event.Skip()

    def OnChangeSnappingUnits(self, event):
        """Snapping units change -> update static text"""
        value = self.snappingValue.GetValue()
        units = self.snappingUnit.GetStringSelection()
        threshold = self.parent.digit.driver.GetThreshold(value=value, units=units)

        if units == "map units":
            self.snappingInfo.SetLabel(_("Snapping threshold is %(value).1f %(units)s") % 
                                       {'value' : value,
                                        'units' : self.mapUnits})
        else:
            self.snappingInfo.SetLabel(_("Snapping threshold is %(value).1f %(units)s") % 
                                       {'value' : threshold,
                                        'units' : self.mapUnits})
            
        event.Skip()

    def OnChangeBackgroundMap(self, event):
        """Change background map"""
        map = self.backgroundMap.GetValue()
        
        UserSettings.Set(group='vdigit', key='backgroundMap', subkey='value', value=map)
        
    def OnChangeQuery(self, event):
        """Change query"""
        if self.queryLength.GetValue():
            # length
            self.queryLengthSL.Enable(True)
            self.queryLengthValue.Enable(True)
            self.queryDangleSL.Enable(False)
            self.queryDangleValue.Enable(False)
        else:
            # dangle
            self.queryLengthSL.Enable(False)
            self.queryLengthValue.Enable(False)
            self.queryDangleSL.Enable(True)
            self.queryDangleValue.Enable(True)

    def OnSave(self, event):
        """Button 'Save' clicked"""
        self.UpdateSettings()
        self.parent.toolbars['vdigit'].settingsDialog = None

        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings['vdigit'] = UserSettings.Get(group='vdigit')
        file = UserSettings.SaveToFile(fileSettings)
        self.parent.gismanager.goutput.WriteLog(_('Vector digitizer settings saved to file <%s>.') % file)

        self.Close()

    def OnApply(self, event):
        """Button 'Apply' clicked"""
        self.UpdateSettings()

    def OnCancel(self, event):
        """Button 'Cancel' clicked"""
        self.parent.toolbars['vdigit'].settingsDialog = None
        self.Close()

        if event:
            event.Skip()
        
    def UpdateSettings(self):
        """Update UserSettings"""

        # symbology
        for key, (enabled, color) in self.symbology.iteritems():
            if enabled:
                UserSettings.Set(group='vdigit', key=key, subkey='enabled',
                                 value=enabled.IsChecked())
                UserSettings.Set(group='vdigit', key=key, subkey='color',
                                 value=tuple(color.GetColour()))
            else:
                UserSettings.Set(group='vdigit', key=key, subkey='color',
                                 value=tuple(color.GetColour()))
        # display
        UserSettings.Set(group='vdigit', key="lineWidth", subkey='value',
                         value=int(self.lineWidthValue.GetValue()))

        # snapping
        UserSettings.Set(group='vdigit', key="snapping", subkey='value',
                         value=int(self.snappingValue.GetValue()))
        UserSettings.Set(group='vdigit', key="snapping", subkey='units',
                         value=self.snappingUnit.GetStringSelection())
        UserSettings.Set(group='vdigit', key="snapToVertex", subkey='enabled',
                         value=self.snapVertex.IsChecked())
        
        # digitize new feature
        UserSettings.Set(group='vdigit', key="addRecord", subkey='enabled',
                         value=self.addRecord.IsChecked())
        UserSettings.Set(group='vdigit', key="layer", subkey='value',
                         value=int(self.layer.GetStringSelection()))
        UserSettings.Set(group='vdigit', key="category", subkey='value',
                         value=int(self.category.GetValue()))
        UserSettings.Set(group='vdigit', key="categoryMode", subkey='selection',
                         value=self.categoryMode.GetSelection())

        # delete existing feature
        UserSettings.Set(group='vdigit', key="delRecord", subkey='enabled',
                         value=self.deleteRecord.IsChecked())

        # snapping threshold
        self.parent.digit.threshold = self.parent.digit.driver.GetThreshold()

        # query tool
        if self.queryLength.GetValue():
            UserSettings.Set(group='vdigit', key="query", subkey='selection',
                             value=0)
        else:
            UserSettings.Set(group='vdigit', key="query", subkey='type',
                             value=1)
        UserSettings.Set(group='vdigit', key="query", subkey='box',
                         value=self.queryBox.IsChecked())
        UserSettings.Set(group='vdigit', key="queryLength", subkey='than-selection',
                         value=self.queryLengthSL.GetSelection())
        UserSettings.Set(group='vdigit', key="queryLength", subkey='thresh',
                         value=int(self.queryLengthValue.GetValue()))
        UserSettings.Set(group='vdigit', key="queryDangle", subkey='than-selection',
                         value=self.queryDangleSL.GetSelection())
        UserSettings.Set(group='vdigit', key="queryDangle", subkey='thresh',
                         value=int(self.queryDangleValue.GetValue()))

        # select features
        for feature in ('Point', 'Line',
                        'Centroid', 'Boundary'):
            UserSettings.Set(group='vdigit', key='selectFeature'+feature, subkey='enabled',
                                           value=self.FindWindowById(self.selectFeature[feature]).IsChecked())
        UserSettings.Set(group='vdigit', key="selectThresh", subkey='value',
                         value=int(self.selectThreshValue.GetValue()))
        UserSettings.Set(group='vdigit', key="checkForDupl", subkey='enabled',
                         value=self.checkForDupl.IsChecked())

        # on-exit
        UserSettings.Set(group='vdigit', key="saveOnExit", subkey='enabled',
                         value=self.save.IsChecked())

        # break lines
        UserSettings.Set(group='vdigit', key="breakLines", subkey='enabled',
                         value=self.intersect.IsChecked())
        
        # update driver settings
        self.parent.digit.driver.UpdateSettings()

        # update digit settings
        self.parent.digit.UpdateSettings()
        
        # redraw map if auto-rendering is enabled
        if self.parent.autoRender.GetValue(): 
            self.parent.OnRender(None)

class VDigitCategoryDialog(wx.Dialog, listmix.ColumnSorterMixin):
    """
    Dialog used to display/modify categories of vector objects
    
    @param parent
    @param title dialog title
    @param query {coordinates, qdist}    - v.edit/v.what
    @param cats  directory of categories - vdigit
    @param line  line id                 - vdigit
    @param pos
    @param style
    """
    def __init__(self, parent, title,
                 map, query=None, cats=None, line=None,
                 pos=wx.DefaultPosition,
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):

        # parent
        self.parent = parent # mapdisplay.BufferedWindow class instance

        # map name
        self.map = map

        # line id (if not found remains 'None')
        self.line = None

        # {layer: [categories]}
        self.cats = {}

        # do not display dialog if no line is found (-> self.cats)
        if cats is None:
            if self.__GetCategories(query[0], query[1]) == 0 or not self.line:
                Debug.msg(3, "VDigitCategoryDialog(): nothing found!")
                return
        else:
            # self.cats = dict(cats)
            for layer in cats.keys():
                self.cats[layer] = list(cats[layer]) # TODO: tuple to list
            self.line = line

        # make copy of cats (used for 'reload')
        self.cats_orig = copy.deepcopy(self.cats)

        Debug.msg(3, "VDigitCategoryDialog(): line=%d, cats=%s" % \
                      (self.line, self.cats))

        wx.Dialog.__init__(self, parent=self.parent, id=wx.ID_ANY, title=title,
                           style=style, pos=pos)

        # list of categories
        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("List of categories"))
        listSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        self.list = CategoryListCtrl(parent=self, id=wx.ID_ANY,
                                     style=wx.LC_REPORT |
                                     wx.BORDER_NONE |
                                     wx.LC_SORT_ASCENDING |
                                     wx.LC_HRULES |
                                     wx.LC_VRULES)
        # sorter
        self.itemDataMap = self.list.Populate()
        listmix.ColumnSorterMixin.__init__(self, 2)

        listSizer.Add(item=self.list, proportion=1, flag=wx.EXPAND)

        # add new category
        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("Add new category"))
        addSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols=5, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(3)

        layerNewTxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                                 label="%s:" % _("Layer"))
        self.layerNew = wx.SpinCtrl(parent=self, id=wx.ID_ANY, size=(50, -1),
                                    initial=1, min=1, max=1e9)
        catNewTxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                               label="%s:" % _("Category"))
        try:
            newCat = max(self.cats[1]) + 1
        except:
            newCat = 1
        self.catNew = wx.SpinCtrl(parent=self, id=wx.ID_ANY, size=(75, -1),
                                  initial=newCat, min=-1e9, max=1e9)
        btnAddCat = wx.Button(self, wx.ID_ADD)
        flexSizer.Add(item=layerNewTxt, proportion=0,
                      flag=wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item=self.layerNew, proportion=0,
                      flag=wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item=catNewTxt, proportion=0,
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT,
                      border=10)
        flexSizer.Add(item=self.catNew, proportion=0,
                      flag=wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item=btnAddCat, proportion=0,
                      flag=wx.EXPAND | wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)
        addSizer.Add(item=flexSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)

        # buttons
        btnApply = wx.Button(self, wx.ID_APPLY)
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        #btnReload = wx.Button(self, wx.ID_UNDO, _("&Reload"))
        btnOk = wx.Button(self, wx.ID_OK)
        btnOk.SetDefault()

        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        #btnSizer.AddButton(btnReload)
        #btnSizer.SetNegativeButton(btnReload)
        btnSizer.AddButton(btnApply)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item=listSizer, proportion=1,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)
        mainSizer.Add(item=addSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALIGN_CENTER |
                      wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)
        mainSizer.Add(item=btnSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        self.SetAutoLayout(True)

        # set min size for dialog
        self.SetMinSize(self.GetBestSize())

        # bindings
        # buttons
        #btnReload.Bind(wx.EVT_BUTTON, self.OnReload)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnOk.Bind(wx.EVT_BUTTON, self.OnOK)
        btnAddCat.Bind(wx.EVT_BUTTON, self.OnAddCat)
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)

        # list
        # self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected, self.list)
        # self.list.Bind(wx.EVT_RIGHT_DOWN, self.OnRightDown)
        self.list.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnRightUp) #wxMSW
        self.list.Bind(wx.EVT_RIGHT_UP, self.OnRightUp) #wxGTK
        self.Bind(wx.EVT_LIST_BEGIN_LABEL_EDIT, self.OnBeginEdit, self.list)
        self.Bind(wx.EVT_LIST_END_LABEL_EDIT, self.OnEndEdit, self.list)
        self.Bind(wx.EVT_LIST_COL_CLICK, self.OnColClick, self.list)

    def GetListCtrl(self):
        """Used by ColumnSorterMixin"""
        return self.list

    def OnColClick(self, event):
        """Click on column header (order by)"""
        event.Skip()
        
    def OnBeginEdit(self, event):
        """Editing of item started"""
        event.Allow()

    def OnEndEdit(self, event):
        """Finish editing of item"""
        itemIndex = event.GetIndex()
        layerOld = int (self.list.GetItem(itemIndex, 0).GetText())
        catOld = int (self.list.GetItem(itemIndex, 1).GetText())

        if event.GetColumn() == 0:
            layerNew = int(event.GetLabel())
            catNew = catOld
        else:
            layerNew = layerOld
            catNew = int(event.GetLabel())

        try:
            if layerNew not in self.cats.keys():
                self.cats[layerNew] = []
            self.cats[layerNew].append(catNew)
            self.cats[layerOld].remove(catOld)
        except:
            event.Veto()
            self.list.SetStringItem(itemIndex, 0, str(layerNew))
            self.list.SetStringItem(itemIndex, 1, str(catNew))
            dlg = wx.MessageDialog(self, _("Unable to add new layer/category <%(layer)s/%(category)s>.\n"
                                           "Layer and category number must be integer.\n"
                                           "Layer number must be greater then zero.") %
                                   { 'layer': str(self.layerNew.GetValue()),
                                     'category' : str(self.catNew.GetValue()) },
                                   _("Error"), wx.OK | wx.ICON_ERROR)
            dlg.ShowModal()
            dlg.Destroy()
            return False

    def OnRightDown(self, event):
        """Mouse right button down"""
        x = event.GetX()
        y = event.GetY()
        item, flags = self.list.HitTest((x, y))

        if item !=  wx.NOT_FOUND and \
                flags & wx.LIST_HITTEST_ONITEM:
            self.list.Select(item)

        event.Skip()

    def OnRightUp(self, event):
        """Mouse right button up"""
        if not hasattr(self, "popupID1"):
            self.popupID1 = wx.NewId()
            self.popupID2 = wx.NewId()
            self.popupID3 = wx.NewId()
            self.Bind(wx.EVT_MENU, self.OnItemDelete,    id=self.popupID1)
            self.Bind(wx.EVT_MENU, self.OnItemDeleteAll, id=self.popupID2)
            self.Bind(wx.EVT_MENU, self.OnReload, id=self.popupID3)

        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupID1, _("Delete selected"))
        if self.list.GetFirstSelected() == -1:
            menu.Enable(self.popupID1, False)

        menu.Append(self.popupID2, _("Delete all"))
        menu.AppendSeparator()
        menu.Append(self.popupID3, _("Reload"))

        self.PopupMenu(menu)
        menu.Destroy()

    def OnItemSelected(self, event):
        """Item selected"""
        event.Skip()

    def OnItemDelete(self, event):
        """Delete selected item(s) from the list (layer/category pair)"""
        item = self.list.GetFirstSelected()
        while item != -1:
            layer = int (self.list.GetItem(item, 0).GetText())
            cat = int (self.list.GetItem(item, 1).GetText())
            self.list.DeleteItem(item)
            self.cats[layer].remove(cat)

            item = self.list.GetFirstSelected()
            
        event.Skip()
        
    def OnItemDeleteAll(self, event):
        """Delete all items from the list"""
        self.list.DeleteAllItems()
        self.cats = {}

        event.Skip()

    def __GetCategories(self, coords, qdist):
        """Get layer/category pairs for all available
        layers

        Return True line found or False if not found"""
        
        cmdWhat = gcmd.Command(cmd=['v.what',
                                   '--q',
                                   'map=%s' % self.map,
                                   'east_north=%f,%f' % \
                                       (float(coords[0]), float(coords[1])),
                                   'distance=%f' % qdist])

        if cmdWhat.returncode != 0:
            return False

        for item in cmdWhat.ReadStdOutput():
            litem = item.lower()
            if "line:" in litem: # get line id
                self.line = int(item.split(':')[1].strip())
            elif "layer:" in litem: # add layer
                layer = int(item.split(':')[1].strip())
                if layer not in self.cats.keys():
                    self.cats[layer] = []
            elif "category:" in litem: # add category
                self.cats[layer].append(int(item.split(':')[1].strip()))

        return True

    def OnReload(self, event):
        """Reload button pressed"""
        # restore original list
        self.cats = copy.deepcopy(self.cats_orig)

        # polulate list
        self.itemDataMap = self.list.Populate(update=True)

        event.Skip()

    def OnCancel(self, event):
        """Cancel button pressed"""
        self.parent.parent.dialogs['category'] = None
        if self.parent.parent.digit:
            self.parent.parent.digit.driver.SetSelected([])
            self.parent.UpdateMap(render=False)
        else:
            self.parent.parent.OnRender(None)
            
        self.Close()

    def OnApply(self, event):
        """Apply button pressed"""

        # action : (catsFrom, catsTo)
        check = {'catadd': (self.cats,      self.cats_orig),
                 'catdel': (self.cats_orig, self.cats)}

        # add/delete new category
        for action, cats in check.iteritems():
            for layer in cats[0].keys():
                catList = []
                for cat in cats[0][layer]:
                    if layer not in cats[1].keys() or \
                            cat not in cats[1][layer]:
                        catList.append(cat)
                if catList != []:
                    if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') == 'vedit':
                        vEditCmd = ['v.edit', '--q',
                                    'map=%s' % self.map,
                                    'layer=%d' % layer,
                                    'tool=%s' % action,
                                    'cats=%s' % ",".join(["%d" % v for v in catList]),
                                    'id=%d' % self.line]
            
                        gcmd.Command(vEditCmd)
                    else:
                        if action == 'catadd':
                            add = True
                        else:
                            add = False
                        self.line = self.parent.parent.digit.SetLineCats(-1, layer,
                                                                          catList, add)
                        if self.line < 0:
                            wx.MessageBox(parent=self, message=_("Unable to update vector map."),
                                          caption=_("Error"), style=wx.OK | wx.ICON_ERROR)
        if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') == 'vedit':           
            # reload map (needed for v.edit)
            self.parent.parent.digit.driver.ReloadMap()

        self.cats_orig = copy.deepcopy(self.cats)

        event.Skip()

    def OnOK(self, event):
        """OK button pressed"""
        self.OnApply(event)
        self.OnCancel(event)

    def OnAddCat(self, event):
        """Button 'Add' new category pressed"""
        try:
            layer = int(self.layerNew.GetValue())
            cat   = int(self.catNew.GetValue())
            if layer <= 0:
                raise ValueError
        except ValueError:
            dlg = wx.MessageDialog(self, _("Unable to add new layer/category <%(layer)s/%(category)s>.\n"
                                           "Layer and category number must be integer.\n"
                                           "Layer number must be greater then zero.") %
                                   {'layer' : str(self.layerNew.GetValue()),
                                    'category' : str(self.catNew.GetValue())},
                                   _("Error"), wx.OK | wx.ICON_ERROR)
            dlg.ShowModal()
            dlg.Destroy()
            return False

        if layer not in self.cats.keys():
            self.cats[layer] = []

        self.cats[layer].append(cat)

        # reload list
        self.itemDataMap = self.list.Populate(update=True)

        # update category number for add
        self.catNew.SetValue(cat + 1)

        event.Skip()

        return True

    def GetLine(self):
        """Get id of selected line of 'None' if no line is selected"""
        return self.line

    def UpdateDialog(self, query=None, cats=None, line=None):
        """Update dialog
        
        @param query {coordinates, distance} - v.edit/v.what
        @param cats  directory layer/cats    - vdigit
        Return True if updated otherwise False
        """

        # line id (if not found remains 'None')
        self.line = None

        # {layer: [categories]}
        self.cats = {}

        # do not display dialog if no line is found (-> self.cats)
        if cats is None:
            ret = self.__GetCategories(query[0], query[1])
        else:
            # self.cats = dict(cats)
            for layer in cats.keys():
                self.cats[layer] = list(cats[layer]) # TODO: tuple to list
            self.line = line
            ret = 1
        if ret == 0 or not self.line:
            Debug.msg(3, "VDigitCategoryDialog(): nothing found!")
            return False
        
        # make copy of cats (used for 'reload')
        self.cats_orig = copy.deepcopy(self.cats)

        # polulate list
        self.itemDataMap = self.list.Populate(update=True)

        try:
            newCat = max(self.cats[1]) + 1
        except:
            newCat = 1
        self.catNew.SetValue(newCat)

        return True

class CategoryListCtrl(wx.ListCtrl,
                       listmix.ListCtrlAutoWidthMixin,
                       listmix.TextEditMixin):
    """List of layers/categories"""

    def __init__(self, parent, id, pos=wx.DefaultPosition,
                 size=wx.DefaultSize, style=0):
        
        self.parent = parent
        
        wx.ListCtrl.__init__(self, parent, id, pos, size, style)

        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.TextEditMixin.__init__(self)

    def Populate(self, update=False):
        """Populate the list"""

        itemData = {} # requested by sorter

        if not update:
            self.InsertColumn(0, _("Layer"))
            self.InsertColumn(1, _("Category"))
        else:
            self.DeleteAllItems()

        i = 1
        for layer in self.parent.cats.keys():
            catsList = self.parent.cats[layer]
            for cat in catsList:
                index = self.InsertStringItem(sys.maxint, str(catsList[0]))
                self.SetStringItem(index, 0, str(layer))
                self.SetStringItem(index, 1, str(cat))
                self.SetItemData(index, i)
                itemData[i] = (str(layer), str(cat))
                i = i + 1

        if not update:
            self.SetColumnWidth(0, 100)
            self.SetColumnWidth(1, wx.LIST_AUTOSIZE)

        self.currentItem = 0

        return itemData

class VDigitZBulkDialog(wx.Dialog):
    """
    Dialog used for Z bulk-labeling tool
    """
    def __init__(self, parent, title, nselected, style=wx.DEFAULT_DIALOG_STYLE):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=title, style=style)

        self.parent = parent # mapdisplay.BufferedWindow class instance

        # panel  = wx.Panel(parent=self, id=wx.ID_ANY)

        border = wx.BoxSizer(wx.VERTICAL)
        
        txt = wx.StaticText(parent=self,
                            label=_("%d lines selected for z bulk-labeling") % nselected);
        border.Add(item=txt, proportion=0, flag=wx.ALL | wx.EXPAND, border=5)

        box   = wx.StaticBox (parent=self, id=wx.ID_ANY, label=" %s " % _("Set value"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols=2, hgap=5, vgap=5)
        flexSizer.AddGrowableCol(0)

        # starting value
        txt = wx.StaticText(parent=self,
                            label=_("Starting value"));
        self.value = wx.SpinCtrl(parent=self, id=wx.ID_ANY, size=(150, -1),
                                 initial=0,
                                 min=-1e6, max=1e6)
        flexSizer.Add(txt, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.value, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)

        # step
        txt = wx.StaticText(parent=self,
                            label=_("Step"))
        self.step  = wx.SpinCtrl(parent=self, id=wx.ID_ANY, size=(150, -1),
                                 initial=0,
                                 min=0, max=1e6)
        flexSizer.Add(txt, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.step, proportion=0, flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE)

        sizer.Add(item=flexSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=1)
        border.Add(item=sizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)

        # buttons
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnOk = wx.Button(self, wx.ID_OK)
        btnOk.SetDefault()

        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item=border, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(item=btnSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

class VDigitDuplicatesDialog(wx.Dialog):
    """
    Show duplicated feature ids
    """
    def __init__(self, parent, data, title=_("List of duplicates"),
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
                 pos=wx.DefaultPosition):

        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=title, style=style,
                           pos=pos)
        
        self.parent = parent # BufferedWindow
        self.data = data
        self.winList = []

        # panel  = wx.Panel(parent=self, id=wx.ID_ANY)

        # notebook
        self.notebook = wx.Notebook(parent=self, id=wx.ID_ANY, style=wx.BK_DEFAULT)

        id = 1
        for key in self.data.keys():
            panel = wx.Panel(parent=self.notebook, id=wx.ID_ANY)
            self.notebook.AddPage(page=panel, text=" %d " % (id))
            
            # notebook body
            border = wx.BoxSizer(wx.VERTICAL)

            win = CheckListFeature(parent=panel, data=list(self.data[key]))
            self.winList.append(win.GetId())

            border.Add(item=win, proportion=1,
                       flag=wx.ALL | wx.EXPAND, border=5)

            panel.SetSizer(border)

            id += 1

        # buttons
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnOk = wx.Button(self, wx.ID_OK)
        btnOk.SetDefault()

        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item=self.notebook, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(item=btnSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        self.SetAutoLayout(True)

        # set min size for dialog
        self.SetMinSize((250, 180))

    def GetUnSelected(self):
        """Get unselected items (feature id)

        @return list of ids
        """
        ids = []
        for id in self.winList:
            wlist = self.FindWindowById(id)

            for item in range(wlist.GetItemCount()):
                if not wlist.IsChecked(item):
                    ids.append(int(wlist.GetItem(item, 0).GetText()))
                    
        return ids

class CheckListFeature(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin, listmix.CheckListCtrlMixin):
    """List of mapset/owner/group"""
    def __init__(self, parent, data,
                 pos=wx.DefaultPosition, log=None):
        self.parent = parent
        self.data = data

        wx.ListCtrl.__init__(self, parent, wx.ID_ANY,
                             style=wx.LC_REPORT)

        listmix.CheckListCtrlMixin.__init__(self)

        self.log = log

        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)

        self.LoadData(self.data)

    def LoadData(self, data):
        """Load data into list"""
        self.InsertColumn(0, _('Feature id'))
        self.InsertColumn(1, _('Layer (Categories)'))

        for item in data:
            index = self.InsertStringItem(sys.maxint, str(item[0]))
            self.SetStringItem(index, 1, str(item[1]))

        # enable all items by default
        for item in range(self.GetItemCount()):
            self.CheckItem(item, True)

        self.SetColumnWidth(col=0, width=wx.LIST_AUTOSIZE_USEHEADER)
        self.SetColumnWidth(col=1, width=wx.LIST_AUTOSIZE_USEHEADER)
                
    def OnCheckItem(self, index, flag):
        """Mapset checked/unchecked"""
        pass
