"""!
@package psmap.instructions

@brief Map feature objects

Classes:
 - dialogs::Instruction
 - dialogs::InstructionObject
 - dialogs::InitMap
 - dialogs::MapFrame
 - dialogs::PageSetup
 - dialogs::Mapinfo
 - dialogs::Text
 - dialogs::Image
 - dialogs::NorthArrow
 - dialogs::Point
 - dialogs::Line
 - dialogs::Rectangle
 - dialogs::Scalebar
 - dialogs::RasterLegend
 - dialogs::VectorLegend
 - dialogs::Raster
 - dialogs::Vector
 - dialogs::VProperties

(C) 2011-2012 by Anna Kratochvilova, and the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com> (bachelor's project)
@author Martin Landa <landa.martin gmail.com> (mentor)
"""

import os
import string
from math import ceil
from time import strftime, localtime

import wx
import grass.script as grass

from core.gcmd          import RunCommand, GError, GMessage, GWarning
from core.utils         import CmdToTuple, GetCmdString, _
from dbmgr.vinfo        import VectorDBInfo
from psmap.utils        import *

class Instruction:
    """!Class which represents instruction file"""
    def __init__(self, parent, objectsToDraw):
        
        self.parent = parent
        self.objectsToDraw = objectsToDraw
        #here are kept objects like mapinfo, rasterlegend, etc.
        self.instruction = list()
        
    def __str__(self):
        """!Returns text for instruction file"""
        comment = "# timestamp: " + strftime("%Y-%m-%d %H:%M", localtime()) + '\n'
        env = grass.gisenv()
        comment += "# location: %s\n" % env['LOCATION_NAME']
        comment += "# mapset: %s\n" % env['MAPSET']
        comment += "# page orientation: %s\n" % self.FindInstructionByType('page')['Orientation']
        border = ''
        if not self.FindInstructionByType('map'):
            border = 'border n\n'
        text = [str(each) for each in self.instruction]
        return comment + border + '\n'.join(text) + '\nend'
    
    def __getitem__(self, id):
        for each in self.instruction:
            if each.id == id:
                return each
        return None

    def __contains__(self, id):
        """!Test if instruction is included"""
        for each in self.instruction:
            if each.id == id:
                return True
        return False
    
    def __delitem__(self, id):
        """!Delete instruction"""
        for each in self.instruction:
            if each.id == id:
                if each.type == 'map':
                    #must remove raster, vector layers too
                    vektor = self.FindInstructionByType('vector', list = True)
                    vProperties = self.FindInstructionByType('vProperties', list = True)
                    raster = self.FindInstructionByType('raster', list = True)
                    for item in vektor + vProperties + raster:
                        if item in self.instruction:
                            self.instruction.remove(item)
                            
                self.instruction.remove(each)
                if id in self.objectsToDraw:
                    self.objectsToDraw.remove(id)
                return
            
    def AddInstruction(self, instruction):
        """!Add instruction"""
        # add to instructions
        if instruction.type == 'map':
            self.instruction.insert(0, instruction)
        else:
            self.instruction.append(instruction)
        # add to drawable objects
        if instruction.type not in ('page', 'raster', 'vector', 'vProperties', 'initMap'):
            if instruction.type == 'map':
                self.objectsToDraw.insert(0, instruction.id) 
            else:
                self.objectsToDraw.append(instruction.id) 
                
            
    def FindInstructionByType(self, type, list = False):
        """!Find instruction(s) with the given type"""
        inst = []
        for each in self.instruction:
            if each.type == type:
                inst.append(each)
        if len(inst) == 1 and not list:
            return inst[0]
        return inst
    
    def Read(self, filename):
        """!Reads instruction file and creates instruction objects"""
        self.filename = filename
        # open file
        try:
            file = open(filename, 'r')
        except IOError:
            GError(message = _("Unable to open file\n%s") % filename)
            return
        # first read file to get information about region and scaletype
        isRegionComment = False
        orientation = 'Portrait'
        for line in file:
            if '# g.region' in line:
                self.SetRegion(regionInstruction = line)
                isRegionComment = True
                break
            if '# page orientation' in line:
                orientation = line.split(':')[-1].strip()
                
        if not isRegionComment:
            self.SetRegion(regionInstruction = None)
        # then run ps.map -b to get information for maploc
        # compute scale and center 
        map = self.FindInstructionByType('map')
        region = grass.region()
        map['center'] = (region['n'] + region['s']) / 2, (region['w'] + region['e']) / 2
        mapRect = GetMapBounds(self.filename, portrait = (orientation == 'Portrait'))
        map['rect'] = mapRect
        proj = projInfo()
        toM = 1.0
        if proj['units']:
            toM = float(proj['meters'])
        units = UnitConversion(self.parent)
        w = units.convert(value = mapRect.Get()[2], fromUnit = 'inch', toUnit = 'meter') / toM
        map['scale'] = w / abs((region['w'] - region['e']))
        
        SetResolution(dpi = 300, width = map['rect'].width, height = map['rect'].height)
        
        # read file again, now with information about map bounds
        isBuffer = False
        buffer = []
        instruction = None
        vectorMapNumber = 1
        file.seek(0)
        for line in file:
            if not line.strip(): 
                continue
            line = line.strip()
            if isBuffer:
                buffer.append(line)
                if 'end' in line:
                    isBuffer = False
                    kwargs = {}
                    if instruction == 'scalebar':
                        kwargs['scale'] = map['scale']
                    elif instruction in ('text', 'eps', 'point', 'line', 'rectangle'):
                        kwargs['mapInstruction'] = map
                    elif instruction in ('vpoints', 'vlines', 'vareas'):
                        kwargs['id'] = wx.NewId()
                        kwargs['vectorMapNumber'] = vectorMapNumber
                        vectorMapNumber += 1
                    elif instruction == 'paper':
                        kwargs['Orientation'] = orientation
                        
                    ok = self.SendToRead(instruction, buffer, **kwargs)
                    if not ok: return False
                    buffer = []
                continue 
            
            elif line.startswith('paper'):
                instruction = 'paper'
                isBuffer = True
                buffer.append(line)
            
            elif line.startswith('border'):
                if line.split()[1].lower() in ('n', 'no', 'none'):
                    ok = self.SendToRead('border', [line])
                    if not ok: return False
                elif line.split()[1].lower() in ('y', 'yes'):
                    instruction = 'border'
                    isBuffer = True
                    buffer.append(line)
            
            elif line.startswith('scale '):
                if isBuffer:
                    continue
                ok = self.SendToRead('scale', line, isRegionComment = isRegionComment)
                if not ok: return False
            
            elif line.startswith('maploc'):
                ok = self.SendToRead(instruction = 'maploc', text = line)
                if not ok: return False
                
            elif line.startswith('raster'):
                ok = self.SendToRead(instruction = 'raster', text = line)
                if not ok: return False
            
            elif line.startswith('mapinfo'):
                instruction = 'mapinfo'
                isBuffer = True
                buffer.append(line)

            elif line.startswith('scalebar'):
                instruction = 'scalebar'
                isBuffer = True
                buffer.append(line) 
            
            elif line.startswith('text'):
                instruction = 'text'
                isBuffer = True
                buffer.append(line)
                
            elif line.startswith('eps'):
                instruction = 'eps'
                isBuffer = True
                buffer.append(line)

            elif line.startswith('point'):
                instruction = 'point'
                isBuffer = True
                buffer.append(line)

            elif line.startswith('line'):
                instruction = 'line'
                isBuffer = True
                buffer.append(line)

            elif line.startswith('rectangle'):
                instruction = 'rectangle'
                isBuffer = True
                buffer.append(line) 
            
            elif line.startswith('colortable'):
                if len(line.split()) == 2 and line.split()[1].lower() in ('n', 'no', 'none'):
                    break
                instruction = 'colortable'
                isBuffer = True
                buffer.append(line) 
        
            elif line.startswith('vlegend'):
                instruction = 'vlegend'
                isBuffer = True
                buffer.append(line) 
                
            elif line.startswith('vpoints'):
                instruction = 'vpoints'
                isBuffer = True
                buffer.append(line) 
                
            elif line.startswith('vlines'):
                instruction = 'vlines'
                isBuffer = True
                buffer.append(line)
                
            elif line.startswith('vareas'):
                instruction = 'vareas'
                isBuffer = True
                buffer.append(line)

            elif line.startswith('labels'):
                instruction = 'labels'
                isBuffer = True
                buffer.append(line)
            


        
        rasterLegend = self.FindInstructionByType('rasterLegend')
        raster = self.FindInstructionByType('raster')
        page = self.FindInstructionByType('page')
        vector = self.FindInstructionByType('vector')
        vectorLegend = self.FindInstructionByType('vectorLegend')
        vectorMaps = self.FindInstructionByType('vProperties', list = True)

        # check (in case of scaletype 0) if map is drawn also
        map['drawMap'] = False
        if map['scaleType'] == 0:
            mapForRegion = map['map']
            if map['mapType'] == 'raster' and raster:
                if mapForRegion == raster['raster']:
                    map['drawMap'] = True
            elif map['mapType'] == 'vector' and vector:
                for vmap in vector['list']:
                    if mapForRegion == vmap[0]:
                        map['drawMap'] = True

        # rasterLegend
        if rasterLegend:
            if rasterLegend['rasterDefault'] and raster:
                rasterLegend['raster'] = raster['raster']
                if not rasterLegend['discrete']:
                    rasterType = getRasterType(map = rasterLegend['raster'])
                    if rasterType == 'CELL':
                        rasterLegend['discrete'] = 'y'
                    else:
                        rasterLegend['discrete'] = 'n'
            
            #estimate size
            height = rasterLegend.EstimateHeight(raster = rasterLegend['raster'], discrete = rasterLegend['discrete'], 
                                                 fontsize = rasterLegend['fontsize'],
                                                 cols = rasterLegend['cols'], 
                                                 height = rasterLegend['height'])
            width = rasterLegend.EstimateWidth(raster = rasterLegend['raster'], discrete = rasterLegend['discrete'], 
                                               fontsize = rasterLegend['fontsize'],
                                               cols = rasterLegend['cols'] , 
                                               width = rasterLegend['width'],
                                               paperInstr = page)
            rasterLegend['rect'] = Rect2D(x = float(rasterLegend['where'][0]), y = float(rasterLegend['where'][1]),
                                          width = width, height = height)
            
        # vectors, vlegend        
        
        if vector:
            for vmap in vectorMaps:
                for i, each in enumerate(vector['list']):
                    if each[2] == vmap.id:
                        
                        vector['list'][i][4] = vmap['label']
                        vector['list'][i][3] = vmap['lpos']
            if vectorLegend:
                size = vectorLegend.EstimateSize(vectorInstr = vector, fontsize = vectorLegend['fontsize'],
                                                 width = vectorLegend['width'], cols = vectorLegend['cols'])                            
                vectorLegend['rect'] = Rect2D(x = float(vectorLegend['where'][0]), y = float(vectorLegend['where'][1]),
                                              width = size[0], height = size[1])
        
        
        page = self.FindInstructionByType('page')
        if not page:
            page = PageSetup(wx.NewId())
            self.AddInstruction(page)
        else:
            page['Orientation'] = orientation


        #
        return True
    
    def SendToRead(self, instruction, text, **kwargs):
        psmapInstrDict = dict(paper = ['page'],
                              maploc = ['map'],
                              scale = ['map'],
                              border = ['map'],
                              raster = ['raster'],
                              mapinfo = ['mapinfo'],
                              scalebar = ['scalebar'],
                              text = ['text'],
                              eps = ['image', 'northArrow'],
                              point = ['point'],
                              line = ['line'],
                              rectangle = ['rectangle'],
                              vpoints = ['vector', 'vProperties'],
                              vlines = ['vector', 'vProperties'],
                              vareas = ['vector', 'vProperties'],
                              colortable = ['rasterLegend'],
                              vlegend = ['vectorLegend'],
                              labels = ['labels']
                              )
        
        myInstrDict = dict(page = PageSetup,
                           map = MapFrame,
                           raster = Raster,
                           mapinfo = Mapinfo,
                           scalebar = Scalebar,
                           text = Text,
                           image = Image,
                           northArrow = NorthArrow,
                           point = Point,
                           line = Line,
                           rectangle = Rectangle,
                           rasterLegend = RasterLegend,
                           vectorLegend = VectorLegend,
                           vector = Vector,
                           vProperties = VProperties,
                           labels = Labels
                           )
        
        myInstruction = psmapInstrDict[instruction]
        
        for i in myInstruction:
            instr = self.FindInstructionByType(i)
            if i in ('text', 'vProperties', 'image', 'northArrow', 'point', 'line', 'rectangle') or not instr:
                
                id = wx.NewId() #!vProperties expect subtype
                if i == 'vProperties':
                    id = kwargs['id']
                    newInstr = myInstrDict[i](id, subType = instruction[1:])
                elif i in ('image', 'northArrow'):
                    commentFound = False
                    for line in text:
                        if line.find("# north arrow") >= 0:
                            commentFound = True
                    if i == 'image' and commentFound or \
                       i == 'northArrow' and not commentFound:
                        continue
                    newInstr = myInstrDict[i](id, settings = self)
                else:
                    newInstr = myInstrDict[i](id)
                ok = newInstr.Read(instruction, text, **kwargs)
                if ok:
                    self.AddInstruction(newInstr)
                else:
                    return False

            else:
                ok = instr.Read(instruction, text, **kwargs)

                if not ok:
                    return False
        return True
    
    def SetRegion(self, regionInstruction):
        """!Sets region from file comment or sets current region in case of no comment"""
        map = MapFrame(wx.NewId())
        self.AddInstruction(map)
        if regionInstruction:
            cmd = CmdToTuple(regionInstruction.strip('# ').split())
            
            # define scaleType
            if len(cmd[1]) <= 3:
                if 'rast' in cmd[1]:
                    map['scaleType'] = 0
                    map['mapType'] = 'raster'   
                    map['map'] = cmd[1]['rast']  
                elif 'vect' in cmd[1]:
                    map['scaleType'] = 0
                    map['mapType'] = 'vector' 
                    map['map'] = cmd[1]['vect']  
                elif 'region' in cmd[1]:
                    map['scaleType'] = 1  
                    map['region'] = cmd[1]['region']
                    
            else:
                map['scaleType'] = 2  
        else:
            map['scaleType'] = 2
            grass.del_temp_region()
            region = grass.region()
            grass.use_temp_region()    
            cmd = ['g.region', region]
        cmdString = GetCmdString(cmd).replace('g.region', '')
        GMessage(_("Instruction file will be loaded with following region: %s\n") % cmdString)
        try:
            RunCommand(cmd[0], **cmd[1])
            
        except grass.ScriptError, e:
            GError(_("Region cannot be set\n%s") % e)
            return False
        

class InstructionObject:
    """!Abtract class representing single instruction"""
    def __init__(self, id): 
        self.id = id
        
        # default values
        self.defaultInstruction = dict()
        # current values
        self.instruction = self.defaultInstruction   
        # converting units
        self.unitConv = UnitConversion() 
    
    def __str__(self):
        """!Returns particular part of text instruction"""
        return ''
    
    def __getitem__(self, key):
        for each in self.instruction.keys():
            if each == key:
                return self.instruction[key]
        return None
    
    def __setitem__(self, key, value):
        self.instruction[key] = value
    
    def GetInstruction(self):
        """!Get current values"""
        return self.instruction
    
    def SetInstruction(self, instruction):
        """!Set default values"""
        self.instruction = instruction
        
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save them"""
        pass
        
    def PercentToReal(self, e, n):
        """!Converts text coordinates from percent of region to map coordinates"""
        e, n = float(e.strip('%')), float(n.strip('%'))
        region = grass.region()
        N = region['s'] + (region['n'] - region['s']) / 100 * n
        E = region['w'] + (region['e'] - region['w']) / 100 * e
        return E, N

class InitMap(InstructionObject):
    """!Class representing virtual map"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'initMap'
        
        # default values
        self.defaultInstruction = dict(rect = None, scale =  None)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    
class MapFrame(InstructionObject):
    """!Class representing map (instructions maploc, scale, border)"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'map'
        # default values
        self.defaultInstruction = dict(map = None, mapType = None, drawMap = True, region = None,
                                       rect = Rect2D(), scaleType = 0, scale = None, center = None,
                                       resolution = 300, border = 'y', width = 1, color = '0:0:0') 
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = ''
        comment = ''
        
        #region settings
        region = grass.region()
        if self.instruction['scaleType'] == 0: #match map
            map = self.instruction['map']
            if self.instruction['mapType'] == 'raster':
                comment = "# g.region rast=%s nsres=%s ewres=%s\n" % (map, region['nsres'], region['ewres'])
            else:
                comment = "# g.region vect=%s\n" % (map)
        elif self.instruction['scaleType'] == 1:# saved region
            region = self.instruction['region']
            comment = "# g.region region=%s\n" % region
        elif self.instruction['scaleType'] in (2, 3): #current region, fixed scale
            comment = string.Template("# g.region n=$n s=$s e=$e w=$w rows=$rows cols=$cols \n").substitute(**region)
        
        instr += comment
        instr += '\n'
        # maploc
        maplocInstruction = "maploc %.3f %.3f" % (self.instruction['rect'].x, self.instruction['rect'].y)
        if self.instruction['scaleType'] != 3:
            maplocInstruction += "  %.3f %.3f"% (self.instruction['rect'].width, self.instruction['rect'].height)
        instr += maplocInstruction
        instr += '\n'
        
        # scale
        if self.instruction['scaleType'] == 3: #fixed scale
            scaleInstruction = "scale 1:%.0f" % (1/self.instruction['scale'])
            instr += scaleInstruction
            instr += '\n'
        # border
        borderInstruction = ''
        if self.instruction['border'] == 'n':
            borderInstruction = "border n"
        else:
            borderInstruction = "border y\n"
            borderInstruction += string.Template("    width $width\n    color $color\n").substitute(self.instruction)
            borderInstruction += "    end"
        instr += borderInstruction
        instr += '\n'

        return instr  
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        if 'isRegionComment' in kwargs:
            isRegionComment = kwargs['isRegionComment']
        instr = {}
        
        if instruction == 'border':
            for line in text:
                if line.startswith('end'):
                    break
                try:
                    if line.split()[1].lower() in ('n', 'no', 'none'):
                        instr['border'] = 'n'
                        break
                    elif line.split()[1].lower() in ('y', 'yes'):
                        instr['border'] = 'y'
                    elif line.startswith('width'):
                        instr['width'] = line.split()[1]
                    elif line.startswith('color'):
                        instr['color'] = line.split()[1]
                except IndexError:
                    GError(_("Failed to read instruction %s") % instruction)
                    return False
                
        elif instruction == 'scale':
            try:
                scaleText = text.strip('scale ').split(':')[1]
                # when scale instruction given and region comment also, then scaletype is fixed scale
                if not isRegionComment:
                    instr['scaleType'] = 2 
                else:
                    instr['scaleType'] = 3

                scale = 1/float(scaleText)
                if abs(scale - self.instruction['scale']) > (0.01 * scale):
                    GWarning(_("Scale has changed, old value: %(old)s\nnew value: %(new)s") % \
                                 { 'old' : scale, 'new' : self.instruction['scale'] })
            except (ValueError, IndexError):
                GError(_("Failed to read instruction %s.\nUse 1:25000 notation.") % instruction)
                return False
        
        elif instruction == 'maploc':
            maploc = text.strip('maploc ').split()
            if len(maploc) >= 2:
                if  abs(self.instruction['rect'].Get()[0] - float(maploc[0])) > 0.5 or \
                        abs(self.instruction['rect'].Get()[1] - float(maploc[1])) > 0.5:
                    GWarning(_("Map frame position changed, old value: %(old1)s %(old2)s\nnew value: %(new1)s %(new2)s") % \
                                 { 'old1' : maploc[0], 'old2' : maploc[1],
                                   'new1' : self.instruction['rect'].Get()[0], 'new2' : self.instruction['rect'].Get()[1] })
                    
                #instr['rect'] = wx.Rect2D(float(maploc[0]), float(maploc[1]), self.instruction['rect'][2], self.instruction['rect'][3])
            if len(maploc) == 4:
                if  abs(self.instruction['rect'].Get()[2] - float(maploc[2])) > 0.5 or \
                        abs(self.instruction['rect'].Get()[3] - float(maploc[3])) > 0.5:
                    GWarning(_("Map frame size changed, old value: %(old1)s %(old2)s\nnew value: %(new1)s %(new2)s") % \
                                 { 'old1' : maploc[2], 'old2' : maploc[3],
                                   'new1' : self.instruction['rect'].Get()[2], 'new2' : self.instruction['rect'].Get()[3] })
                #instr['rect'] = wx.Rect2D(*map(float, maploc))
        self.instruction.update(instr)   
        return True 
    
class PageSetup(InstructionObject):
    """!Class representing page instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'page'
        # default values
        self.defaultInstruction = dict(Units = 'inch', Format = 'a4', Orientation = 'Portrait',
                                       Width = 8.268, Height = 11.693, Left = 0.5, Right = 0.5, Top = 1, Bottom = 1)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        if self.instruction['Format'] == 'custom':
            instr = string.Template("paper\n    width $Width\n    height $Height\n").substitute(self.instruction)
        else:
            instr = string.Template("paper $Format\n").substitute(self.instruction)
        instr += string.Template("    left $Left\n    right $Right\n    bottom $Bottom\n    top $Top\n    end").substitute(self.instruction)

        return instr
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        instr = {}
        self.cats = ['Width', 'Height', 'Left', 'Right', 'Top', 'Bottom']
        self.subInstr = dict(zip(['width', 'height', 'left', 'right', 'top', 'bottom'], self.cats))
        
        if instruction == 'paper': # just for sure
            for line in text:
                if line.startswith('paper'): 
                    if len(line.split()) > 1:
                        pformat = line.split()[1]
                        availableFormats = self._toDict(grass.read_command('ps.map', flags = 'p',
                                                                           quiet = True))
                        # e.g. paper a3 
                        try:
                            instr['Format'] = pformat
                            for key, value in availableFormats[pformat].iteritems():
                                instr[key] = float(value)
                            break
                        except KeyError:
                            GError(_("Failed to read instruction %(file)s.\nUnknown format %(for)s") % \
                                       { 'file' : instruction, 'for' : format })
                            return False
                    else:
                        # paper
                        # width ...
                        instr['Format'] = 'custom'
                # read subinstructions
                elif instr['Format'] == 'custom' and not line.startswith('end'):
                    text = line.split()
                    try:
                        instr[self.subInstr[text[0]]] = float(text[1])
                    except  (IndexError, KeyError):
                        GError(_("Failed to read instruction %s.") % instruction)
                        return False
                    
            if 'Orientation' in kwargs and kwargs['Orientation'] == 'Landscape':
                instr['Width'], instr['Height'] = instr['Height'], instr['Width']
                
            self.instruction.update(instr)
        return True  
    
    def _toDict(self, paperStr):    
        sizeDict = dict()
#     cats = self.subInstr[ 'Width', 'Height', 'Left', 'Right', 'Top', 'Bottom']
        for line in paperStr.strip().split('\n'):
            d = dict(zip(self.cats, line.split()[1:]))
            sizeDict[line.split()[0]] = d
            
        return sizeDict    
    
class Mapinfo(InstructionObject):
    """!Class representing mapinfo instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'mapinfo'
        # default values
        self.defaultInstruction = dict(unit = 'inch', where = (0, 0),
                                       font = 'Helvetica', fontsize = 10, color = '0:0:0', background = 'none', 
                                       border = 'none', rect = None)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = "mapinfo\n"
        instr += "    where %.3f %.3f\n" % (self.instruction['where'][0], self.instruction['where'][1])
        instr += string.Template("    font $font\n    fontsize $fontsize\n    color $color\n").substitute(self.instruction)            
        instr += string.Template("    background $background\n    border $border\n").substitute(self.instruction)  
        instr += "    end"
        return instr
    
    def Read(self, instruction, text):
        """!Read instruction and save information"""
        instr = {}
        try:
            for line in text:
                sub = line.split(None,1)
                if sub[0] == 'font':
                    instr['font'] = sub[1]
                elif sub[0] == 'fontsize':
                    instr['fontsize'] = int(sub[1])
                elif sub[0] == 'color':
                    instr['color'] = sub[1]
                elif sub[0] == 'background':
                    instr['background'] = sub[1]
                elif sub[0] == 'border':
                    instr['border'] = sub[1]
                elif sub[0] == 'where':
                    instr['where'] = float(sub[1].split()[0]), float(sub[1].split()[1])
        except (ValueError, IndexError):
            GError(_("Failed to read instruction %s") % instruction)
            return False
        self.instruction.update(instr)
        self.instruction['rect'] = self.EstimateRect(mapinfoDict = self.instruction)
        return True
    
    def EstimateRect(self, mapinfoDict):
        """!Estimate size to draw mapinfo"""
        w = mapinfoDict['fontsize'] * 20 # any better estimation? 
        h = mapinfoDict['fontsize'] * 7
        width = self.unitConv.convert(value = w, fromUnit = 'point', toUnit = 'inch')
        height = self.unitConv.convert(value = h, fromUnit = 'point', toUnit = 'inch')
        return Rect2D(x = float(mapinfoDict['where'][0]), y = float(mapinfoDict['where'][1]),
                      width = width, height = height)
    
class Text(InstructionObject):
    """!Class representing text instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'text'
        # default values
        self.defaultInstruction = dict(text = "", font = "Helvetica", fontsize = 10, color = 'black', background = 'none',
                                       hcolor = 'none', hwidth = 1, border = 'none', width = '1', XY = True,
                                       where = (0,0), unit = 'inch', rotate = None, 
                                       ref = "center center", xoffset = 0, yoffset = 0, east = None, north = None)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        text = self.instruction['text'].replace('\n','\\n')
        instr = u"text %s %s" % (self.instruction['east'], self.instruction['north'])
        instr += " %s\n" % text
        instr += (string.Template("    font $font\n    fontsize $fontsize\n    color $color\n").
                                                                   substitute(self.instruction))
        instr += string.Template("    hcolor $hcolor\n").substitute(self.instruction)
        if self.instruction['hcolor'] != 'none':
            instr += string.Template("    hwidth $hwidth\n").substitute(self.instruction)
        instr += string.Template("    border $border\n").substitute(self.instruction)
        if self.instruction['border'] != 'none':
            instr += string.Template("    width $width\n").substitute(self.instruction)
        instr += string.Template("    background $background\n").substitute(self.instruction)
        if self.instruction["ref"] != '0':
            instr += string.Template("    ref $ref\n").substitute(self.instruction)
        if self.instruction["rotate"]:
            instr += string.Template("    rotate $rotate\n").substitute(self.instruction)
        if float(self.instruction["xoffset"]) or float(self.instruction["yoffset"]):
            instr += (string.Template("    xoffset $xoffset\n    yoffset $yoffset\n").
                                                            substitute(self.instruction))
        instr += "    end"
        try:
            instr = instr.encode('latin1')
        except UnicodeEncodeError, err:
            try:
                pos = str(err).split('position')[1].split(':')[0].strip()
            except IndexError:
                pos = ''
            if pos:
                message = _("Characters on position %s are not supported "
                            "by ISO-8859-1 (Latin 1) encoding "
                            "which is required by module ps.map.") % pos
            else:
                message = _("Not all characters are supported "
                            "by ISO-8859-1 (Latin 1) encoding "
                            "which is required by module ps.map.")
            GMessage(message = message)
            return ''
        
        return instr
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        map = kwargs['mapInstruction']
        instr = {}
        for line in text:
            try:
                sub = line.split(None, 1)[0]
                if sub == 'text':
                    e, n = line.split(None, 3)[1:3]
                    if '%' in e and '%' in n:
                        instr['XY'] = True
                        instr['east'], instr['north'] = self.PercentToReal(e, n)
                    else:
                        instr['XY'] = False
                        instr['east'], instr['north'] = float(e), float(n)
                        
                    instr['text'] = line.split(None, 3)[3].decode('latin_1')
                
                elif sub == 'font':
                    instr['font'] = line.split(None, 1)[1]
                elif sub == 'fontsize':
                    instr['fontsize'] = float(line.split(None, 1)[1])
                elif sub == 'color':
                    instr['color'] = line.split(None, 1)[1]
                elif sub == 'width':
                    instr['width'] = line.split(None, 1)[1]
                elif sub == 'hcolor':
                    instr['hcolor'] = line.split(None, 1)[1]
                elif sub == 'hwidth':
                    instr['hwidth'] = line.split(None, 1)[1]
                elif sub == 'background':
                    instr['background'] = line.split(None, 1)[1]
                elif sub == 'border':
                    instr['border'] = line.split(None, 1)[1]
                elif sub == 'ref':
                    instr['ref'] = line.split(None, 1)[1]
                elif sub == 'rotate':
                    instr['rotate'] = float(line.split(None, 1)[1])
                elif sub == 'xoffset':
                    instr['xoffset'] = int(line.split(None, 1)[1])
                elif sub == 'yoffset':
                    instr['yoffset'] = int(line.split(None, 1)[1])
                elif sub == 'opaque':
                    if line.split(None, 1)[1].lower() in ('n', 'none'):
                        instr['background'] = 'none'
                        
            except(IndexError, ValueError):
                GError(_("Failed to read instruction %s") % instruction)
                return False
        instr['where'] = PaperMapCoordinates(mapInstr = map, x = instr['east'], y = instr['north'], paperToMap = False)       
        self.instruction.update(instr)

        return True 
        
class Image(InstructionObject):
    """!Class representing eps instruction - image"""
    def __init__(self, id, settings):
        InstructionObject.__init__(self, id = id)
        self.settings = settings
        self.type = 'image'
        # default values
        self.defaultInstruction = dict(epsfile = "", XY = True, where = (0,0), unit = 'inch',
                                       east = None, north = None,
                                       rotate = None, scale = 1)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        self.ChangeRefPoint(toCenter = True)
        epsfile = self.instruction['epsfile'].replace(os.getenv('GISBASE'), "$GISBASE")
        
        instr = "eps %s %s\n" % (self.instruction['east'], self.instruction['north'])
        instr += "    epsfile %s\n" % epsfile
        if self.instruction["rotate"]:
            instr += string.Template("    rotate $rotate\n").substitute(self.instruction)
        if self.instruction["scale"]:
            instr += string.Template("    scale $scale\n").substitute(self.instruction)
        instr += "    end"
        return instr
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        mapInstr = kwargs['mapInstruction']
        instr = {}
        for line in text:
            try:
                sub = line.split(None, 1)[0]
                if sub == 'eps':
                    e, n = line.split(None, 3)[1:3]
                    if '%' in e and '%' in n:
                        instr['XY'] = True
                        instr['east'], instr['north'] = self.PercentToReal(e, n)
                    else:
                        instr['XY'] = False
                        instr['east'], instr['north'] = float(e), float(n)
                
                elif sub == 'epsfile':
                    epsfile = line.split(None, 1)[1]
                    instr['epsfile'] = epsfile.replace("$GISBASE", os.getenv("GISBASE"))
                elif sub == 'rotate':
                    instr['rotate'] = float(line.split(None, 1)[1])
                elif sub == 'scale':
                    instr['scale'] = float(line.split(None, 1)[1])
                        
            except(IndexError, ValueError):
                GError(_("Failed to read instruction %s") % instruction)
                return False
        if not os.path.exists(instr['epsfile']):
            GError(_("Failed to read instruction %(inst)s: "
                     "file %(file)s not found.") % { 'inst' : instruction,
                                                     'file' : instr['epsfile'] })
            return False
        
        instr['epsfile'] = os.path.abspath(instr['epsfile'])
        instr['size'] = self.GetImageOrigSize(instr['epsfile'])
        if 'rotate' in instr:
            instr['size'] = BBoxAfterRotation(instr['size'][0], instr['size'][1], instr['rotate'])
        self.instruction.update(instr)
        self.ChangeRefPoint(toCenter = False)
        instr['where'] = PaperMapCoordinates(mapInstr = mapInstr, x = self.instruction['east'],
                                             y = self.instruction['north'], paperToMap = False)       
        w = self.unitConv.convert(value = instr['size'][0], fromUnit = 'point', toUnit = 'inch')
        h = self.unitConv.convert(value = instr['size'][1], fromUnit = 'point', toUnit = 'inch')
        instr['rect'] = Rect2D(x = float(instr['where'][0]), y = float(instr['where'][1]),
                               width = w * self.instruction['scale'], height = h * self.instruction['scale'])
        self.instruction.update(instr)

        return True 
        
    def ChangeRefPoint(self, toCenter):
        """!Change reference point (left top x center)"""
        mapInstr = self.settings.FindInstructionByType('map')
        if not mapInstr:
            mapInstr = self.settings.FindInstructionByType('initMap')
        mapId = mapInstr.id
        if toCenter:
            center = self.instruction['rect'].GetCentre()
            ENCenter = PaperMapCoordinates(mapInstr = self.settings[mapId],
                                           x = center[0], y = center[1], paperToMap = True)
                                           
            self.instruction['east'], self.instruction['north'] = ENCenter
        else:
            x, y = PaperMapCoordinates(mapInstr = self.settings[mapId], x = self.instruction['east'],
                                       y = self.instruction['north'], paperToMap = False)
            w = self.unitConv.convert(value = self.instruction['size'][0], fromUnit = 'point', toUnit = 'inch')
            h = self.unitConv.convert(value = self.instruction['size'][1], fromUnit = 'point', toUnit = 'inch')
            x -= w * self.instruction['scale'] / 2
            y -= h * self.instruction['scale'] / 2
            e, n = PaperMapCoordinates(mapInstr = self.settings[mapId], x = x, y = y, paperToMap = True)
            self.instruction['east'], self.instruction['north'] = e, n

    def GetImageOrigSize(self, imagePath):
        """!Get image size.
        
        If eps, size is read from image header.
        """
        fileName = os.path.split(imagePath)[1]
        # if eps, read info from header
        if os.path.splitext(fileName)[1].lower() == '.eps':
            bbInfo = "%%BoundingBox"
            file = open(imagePath,"r")
            w = h = 0
            while file:
                line = file.readline()
                if line.find(bbInfo) == 0:
                    w, h = line.split()[3:5]
                    break
            file.close()
            return float(w), float(h)
        else: # we can use wx.Image
            img = wx.Image(fileName, type=wx.BITMAP_TYPE_ANY)
            return img.GetWidth(), img.GetHeight()
            
class NorthArrow(Image):
    """!Class representing eps instruction -- North Arrow"""
    def __init__(self, id, settings):
        Image.__init__(self, id = id, settings = settings)
        self.type = 'northArrow'
        
    def __str__(self):
        self.ChangeRefPoint(toCenter = True)
        epsfile = self.instruction['epsfile'].replace(os.getenv('GISBASE'), "$GISBASE")

        instr = "eps %s %s\n" % (self.instruction['east'], self.instruction['north'])
        instr += "# north arrow\n"
        instr += "    epsfile %s\n" % epsfile
        if self.instruction["rotate"]:
            instr += string.Template("    rotate $rotate\n").substitute(self.instruction)
        if self.instruction["scale"]:
            instr += string.Template("    scale $scale\n").substitute(self.instruction)
        instr += "    end"
        return instr
        
class Point(InstructionObject):
    """!Class representing point instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'point'
        # default values
        self.defaultInstruction = dict(symbol = os.path.join('basic', 'x'),
                                       color = '0:0:0', fcolor = '200:200:200',
                                       rotate = 0, size = 10,
                                       XY = True, where = (0,0), unit = 'inch',
                                       east = None, north = None)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = string.Template("point $east $north\n").substitute(self.instruction)
        instr += string.Template("    symbol $symbol\n").substitute(self.instruction)
        instr += string.Template("    color $color\n").substitute(self.instruction)
        instr += string.Template("    fcolor $fcolor\n").substitute(self.instruction)
        instr += string.Template("    rotate $rotate\n").substitute(self.instruction)
        instr += string.Template("    size $size\n").substitute(self.instruction)
        instr += "    end"
        return instr
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        mapInstr = kwargs['mapInstruction']
        instr = {}
        for line in text:
            try:
                sub = line.split(None, 1)[0]
                if sub == 'point':
                    e, n = line.split(None, 3)[1:3]
                    if '%' in e and '%' in n:
                        instr['XY'] = True
                        instr['east'], instr['north'] = self.PercentToReal(e, n)
                    else:
                        instr['XY'] = False
                        instr['east'], instr['north'] = float(e), float(n)
                
                elif sub == 'symbol':
                    instr['symbol'] = line.split(None, 1)[1]
                elif sub == 'rotate':
                    instr['rotate'] = float(line.split(None, 1)[1])
                elif sub == 'size':
                    instr['size'] = float(line.split(None, 1)[1])
                elif sub == 'color':
                    instr['color'] = line.split(None, 1)[1]
                elif sub == 'fcolor':
                    instr['fcolor'] = line.split(None, 1)[1]

                        
            except(IndexError, ValueError):
                GError(_("Failed to read instruction %s") % instruction)
                return False
        
        self.instruction.update(instr)
        instr['where'] = PaperMapCoordinates(mapInstr = mapInstr, x = self.instruction['east'],
                                             y = self.instruction['north'], paperToMap = False)
        w = h = self.unitConv.convert(value = instr['size'], fromUnit = 'point', toUnit = 'inch')
        instr['rect'] = Rect2D(x = float(instr['where'][0]) - w / 2, y = float(instr['where'][1] - h / 2),
                               width = w, height = h)
        self.instruction.update(instr)

        return True

class Line(InstructionObject):
    """!Class representing line instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'line'
        # default values
        self.defaultInstruction = dict(color = '0:0:0', width = 2,
                                       where = [wx.Point2D(), wx.Point2D()],
                                       east1 = None, north1 = None,
                                       east2 = None, north2 = None)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = string.Template("line $east1 $north1 $east2 $north2\n").substitute(self.instruction)
        instr += string.Template("    color $color\n").substitute(self.instruction)
        instr += string.Template("    width $width\n").substitute(self.instruction)
        instr += "    end\n"
        return instr
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        mapInstr = kwargs['mapInstruction']
        instr = {}
        for line in text:
            try:
                sub = line.split(None, 1)[0]
                if sub == 'line':
                    e1, n1, e2, n2 = line.split(None, 5)[1:5]
                    if '%' in e1 and '%' in n1 and '%' in e2 and '%' in n2:
                        instr['east1'], instr['north1'] = self.PercentToReal(e1, n1)
                        instr['east2'], instr['north2'] = self.PercentToReal(e2, n2)
                    else:
                        instr['east1'], instr['north1'] = float(e1), float(n1)
                        instr['east2'], instr['north2'] = float(e2), float(n2)
                
                elif sub == 'width':
                    instr['width'] = float(line.split(None, 1)[1])
                elif sub == 'color':
                    instr['color'] = line.split(None, 1)[1]
                        
            except(IndexError, ValueError):
                GError(_("Failed to read instruction %s") % instruction)
                return False
        
        self.instruction.update(instr)
        e1, n1 = PaperMapCoordinates(mapInstr = mapInstr, x = self.instruction['east1'],
                                     y = self.instruction['north1'], paperToMap = False)
        e2, n2 = PaperMapCoordinates(mapInstr = mapInstr, x = self.instruction['east2'],
                                     y = self.instruction['north2'], paperToMap = False)
        instr['where'] = [wx.Point2D(e1, n1), wx.Point2D(e2, n2)]
        instr['rect'] = Rect2DPP(instr['where'][0], instr['where'][1])
        self.instruction.update(instr)

        return True

class Rectangle(InstructionObject):
    """!Class representing rectangle instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'rectangle'
        # default values
        self.defaultInstruction = dict(color = '0:0:0', fcolor = 'none', width = 2,
                                       east1 = None, north1 = None,
                                       east2 = None, north2 = None)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = string.Template("rectangle $east1 $north1 $east2 $north2\n").substitute(self.instruction)
        instr += string.Template("    color $color\n").substitute(self.instruction)
        instr += string.Template("    fcolor $fcolor\n").substitute(self.instruction)
        instr += string.Template("    width $width\n").substitute(self.instruction)
        instr += "    end\n"
        return instr
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        mapInstr = kwargs['mapInstruction']
        instr = {}
        for line in text:
            try:
                sub = line.split(None, 1)[0]
                if sub == 'rectangle':
                    e1, n1, e2, n2 = line.split(None, 5)[1:5]
                    if '%' in e1 and '%' in n1 and '%' in e2 and '%' in n2:
                        instr['east1'], instr['north1'] = self.PercentToReal(e1, n1)
                        instr['east2'], instr['north2'] = self.PercentToReal(e2, n2)
                    else:
                        instr['east1'], instr['north1'] = float(e1), float(n1)
                        instr['east2'], instr['north2'] = float(e2), float(n2)
                
                elif sub == 'width':
                    instr['width'] = float(line.split(None, 1)[1])
                elif sub == 'color':
                    instr['color'] = line.split(None, 1)[1]
                elif sub == 'fcolor':
                    instr['fcolor'] = line.split(None, 1)[1]

                        
            except(IndexError, ValueError):
                GError(_("Failed to read instruction %s") % instruction)
                return False
        
        self.instruction.update(instr)
        e1, n1 = PaperMapCoordinates(mapInstr = mapInstr, x = self.instruction['east1'],
                                       y = self.instruction['north1'], paperToMap = False)
        e2, n2 = PaperMapCoordinates(mapInstr = mapInstr, x = self.instruction['east2'],
                                       y = self.instruction['north2'], paperToMap = False)
        instr['rect'] = Rect2DPP(wx.Point2D(e1, n1), wx.Point2D(e2, n2))
        self.instruction.update(instr)

        return True
        
class Scalebar(InstructionObject):
    """!Class representing scalebar instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'scalebar'
        # default values
        self.defaultInstruction = dict(unit = 'inch', where = (1,1),
                                       unitsLength = 'auto', unitsHeight = 'inch',
                                       length = None, height = 0.1, rect = None,
                                       fontsize = 10, background = 'y',
                                       scalebar = 'f', segment = 4, numbers = 1)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = string.Template("scalebar $scalebar\n").substitute(self.instruction)
        instr += "    where %.3f %.3f\n" % (self.instruction['where'][0], self.instruction['where'][1])
        instr += string.Template("    length $length\n    units $unitsLength\n").substitute(self.instruction)
        instr += string.Template("    height $height\n").substitute(self.instruction)
        instr += string.Template("    segment $segment\n    numbers $numbers\n").substitute(self.instruction)
        instr += string.Template("    fontsize $fontsize\n    background $background\n").substitute(self.instruction)
        instr += "    end"
        return instr
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        scale = kwargs['scale']
        instr = {}
        for line in text:
            try:
                if line.startswith('scalebar'):
                    if 'scalebar s' in line:
                        instr['scalebar'] = 's'
                    else:
                        instr['scalebar'] = 'f'
                elif line.startswith('where'):
                    instr['where'] = map(float, line.split()[1:3])
                elif line.startswith('length'):
                    instr['length'] = float(line.split()[1])
                elif line.startswith('units'):
                    if line.split()[1] in ['auto', 'meters', 'kilometers', 'feet', 'miles', 'nautmiles']:
                        instr['unitsLength'] = line.split()[1]
                elif line.startswith('height'):
                    instr['height'] = float(line.split()[1])
                elif line.startswith('fontsize'):
                    instr['fontsize'] = float(line.split()[1])
                elif line.startswith('numbers'):
                    instr['numbers'] = int(line.split()[1])
                elif line.startswith('segment'):
                    instr['segment'] = int(line.split()[1])
                elif line.startswith('background'):
                    if line.split()[1].strip().lower() in ('y','yes'):
                        instr['background'] = 'y'
                    elif line.split()[1].strip().lower() in ('n','no', 'none'):
                        instr['background'] = 'n'
            except(IndexError, ValueError):
                GError(_("Failed to read instruction %s") % instruction)
                return False
            
        self.instruction.update(instr)
        w, h = self.EstimateSize(scalebarDict = self.instruction, scale = scale)
        x = self.instruction['where'][0] - w / 2 
        y = self.instruction['where'][1] - h / 2
        self.instruction['rect'] = Rect2D(x, y, w, h)
        return True 
    
    def EstimateSize(self, scalebarDict, scale):
        """!Estimate size to draw scalebar"""
        units = projInfo()['units']
        if not units or units not in self.unitConv.getAllUnits():
            units = 'meters'
        if scalebarDict['unitsLength'] != 'auto':
            length = self.unitConv.convert(value = scalebarDict['length'], fromUnit = scalebarDict['unitsLength'], toUnit = 'inch')
        else:
            length = self.unitConv.convert(value = scalebarDict['length'], fromUnit = units, toUnit = 'inch')
            
        length *= scale
        length *= 1.1 #for numbers on the edge
        height = scalebarDict['height'] + 2 * self.unitConv.convert(value = scalebarDict['fontsize'], fromUnit = 'point', toUnit = 'inch')     
        return (length, height)
    
class RasterLegend(InstructionObject):
    """!Class representing colortable instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'rasterLegend'
        # default values
        self.defaultInstruction = dict(rLegend = False, unit = 'inch', rasterDefault = True, raster = None,
                                       discrete = None, type = None,
                                       where = (0, 0),
                                       width = None, height = None, cols = 1, font = "Helvetica", fontsize = 10,
                                       #color = '0:0:0', tickbar = False, range = False, min = 0, max = 0,
                                       color = 'black', tickbar = 'n', range = False, min = 0, max = 0,
                                       nodata = 'n')
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = "colortable y\n"
        instr += string.Template("    raster $raster\n").substitute(self.instruction)
        instr += "    where %.3f %.3f\n" % (self.instruction['where'][0], self.instruction['where'][1])
        if self.instruction['width']:
            instr += string.Template("    width $width\n").substitute(self.instruction)
        instr += string.Template("    discrete $discrete\n").substitute(self.instruction)
        if self.instruction['discrete'] == 'n':
            if self.instruction['height']:
                instr += string.Template("    height $height\n").substitute(self.instruction)
            instr += string.Template("    tickbar $tickbar\n").substitute(self.instruction)
            if self.instruction['range']:
                instr += string.Template("    range $min $max\n").substitute(self.instruction)
        else:
            instr += string.Template("    cols $cols\n").substitute(self.instruction)
            instr += string.Template("    nodata $nodata\n").substitute(self.instruction)
        instr += string.Template("    font $font\n    fontsize $fontsize\n    color $color\n")\
            .substitute(self.instruction)
        instr += "    end"
        return instr    
    
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        instr = {}
        instr['rLegend'] = True
        for line in text:
            try:
                if line.startswith('where'):
                    instr['where'] = map(float, line.split()[1:3])
                elif line.startswith('font '):
                    instr['font'] = line.split()[1]
                elif line.startswith('fontsize'):
                    instr['fontsize'] = float(line.split()[1])
                elif line.startswith('color '):
                    instr['color'] = line.split()[1]
                elif line.startswith('raster'):
                    instr['raster'] = line.split()[1]
                elif line.startswith('width'):
                    instr['width'] = float(line.split()[1])
                elif line.startswith('height'):
                    instr['height'] = float(line.split()[1])
                elif line.startswith('cols'):
                    instr['cols'] = int(line.split()[1])                    
                elif line.startswith('range'):
                    instr['range'] = True
                    instr['min'] = float(line.split()[1])
                    instr['max'] = float(line.split()[2])
                elif line.startswith('nodata'):
                    if line.split()[1].strip().lower() in ('y','yes'):
                        instr['nodata'] = 'y'
                    elif line.split()[1].strip().lower() in ('n','no', 'none'):
                        instr['nodata'] = 'n'
                elif line.startswith('tickbar'):
                    if line.split()[1].strip().lower() in ('y','yes'):
                        instr['tickbar'] = 'y'
                    elif line.split()[1].strip().lower() in ('n','no', 'none'):
                        instr['tickbar'] = 'n'
                elif line.startswith('discrete'):
                    if line.split()[1].strip().lower() in ('y','yes'):
                        instr['discrete'] = 'y'
                    elif line.split()[1].strip().lower() in ('n','no', 'none'):
                        instr['discrete'] = 'n'            

            except(IndexError, ValueError):
                GError(_("Failed to read instruction %s") % instruction)
                return False
            
        if 'raster' in instr:
            instr['rasterDefault'] = False
            if 'discrete' not in instr:
                rasterType = getRasterType(map = instr['raster'])
                instr['type'] = rasterType
                if rasterType == 'CELL':
                    instr['discrete'] = 'y'
                else:
                    instr['discrete'] = 'n'
            
        else:
            instr['rasterDefault'] = True
        self.instruction.update(instr)
        # add 'rect' in the end
            
        return True 
    
    def EstimateHeight(self, raster, discrete, fontsize, cols = None,  height = None):
        """!Estimate height to draw raster legend"""
        if discrete == 'n':
            if height:
                height = height
            else:
                height = self.unitConv.convert(value = fontsize * 10,
                                                    fromUnit = 'point', toUnit = 'inch')
                                                    
        if discrete == 'y':
            if cols:
                cols = cols 
            else:
                cols = 1 

            rinfo = grass.raster_info(raster)
            if rinfo['datatype'] in ('DCELL', 'FCELL'):
                minim, maxim = rinfo['min'], rinfo['max']
                rows = ceil(maxim / cols )
            else:
                cat = grass.read_command('r.category', map = raster,
                                    sep = ':').strip().split('\n')
                rows = ceil(float(len(cat)) / cols )
                            
                
            height = self.unitConv.convert(value =  1.5 * rows * fontsize, fromUnit = 'point', toUnit = 'inch')
            
        return height
        
    def EstimateWidth(self, raster, discrete, fontsize, cols = None, width = None, paperInstr = None):
        """!Estimate size to draw raster legend"""
        
        if discrete == 'n':
            rinfo = grass.raster_info(raster)
            minim, maxim = rinfo['min'], rinfo['max']
            if width:
                width = width
            else:
                width = self.unitConv.convert(value = fontsize * 2,
                                                    fromUnit = 'point', toUnit = 'inch')
            text = len(max(str(minim), str(maxim), key = len))
            textPart = self.unitConv.convert(value = text * fontsize / 2,
                                                    fromUnit = 'point', toUnit = 'inch')
            width += textPart
                                                    
        elif discrete == 'y':
            if cols:
                cols = cols 
            else:
                cols = 1    

            if width:
                width = width
            else:
                paperWidth = paperInstr['Width'] - paperInstr['Right'] - paperInstr['Left']
                width = (paperWidth / cols) * (cols - 1) + 1
                
        return width    
             
class VectorLegend(InstructionObject):
    """!Class representing colortable instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'vectorLegend'
        # default values
        self.defaultInstruction = dict(vLegend = False, unit = 'inch', where = (0, 0),
                                                defaultSize = True, width = 0.4, cols = 1, span = None,
                                                font = "Helvetica", fontsize = 10,
                                                border = 'none')
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = "vlegend\n"
        instr += "    where %.3f %.3f\n" % (self.instruction['where'][0], self.instruction['where'][1])
        instr += string.Template("    font $font\n    fontsize $fontsize\n").substitute(self.instruction)
        instr += string.Template("    width $width\n    cols $cols\n").substitute(self.instruction)
        if self.instruction['span']:
            instr += string.Template("    span $span\n").substitute(self.instruction)
        instr += string.Template("    border $border\n").substitute(self.instruction)  
        instr += "    end"  
        return instr

    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        instr = {}
        instr['vLegend'] = True
        for line in text:
            try:
                if line.startswith('where'):
                    instr['where'] = map(float, line.split()[1:3])
                elif line.startswith('font '):
                    instr['font'] = line.split()[1]
                elif line.startswith('fontsize'):
                    instr['fontsize'] = float(line.split()[1])
                elif line.startswith('width'):
                    instr['width'] = float(line.split()[1])
                elif line.startswith('cols'):
                    instr['cols'] = int(line.split()[1]) 
                elif line.startswith('span'):
                    instr['span'] = float(line.split()[1])
                elif line.startswith('border'):
                    instr['border'] = line.split()[1]
                    
            except(IndexError, ValueError):
                GError(_("Failed to read instruction %s") % instruction)
                return False
            
        self.instruction.update(instr)
            
        return True 
    
    def EstimateSize(self, vectorInstr, fontsize, width = None, cols = None):
        """!Estimate size to draw vector legend"""
        if width:
            width = width 
        else:
            width = fontsize/24.0

        if cols:
            cols = cols 
        else:
            cols = 1

        vectors = vectorInstr['list']
        labels = [vector[4] for vector in vectors if vector[3] != 0]
        extent = (len(max(labels, key = len)) * fontsize / 2, fontsize)
        wExtent = self.unitConv.convert(value = extent[0], fromUnit = 'point', toUnit = 'inch')
        hExtent = self.unitConv.convert(value = extent[1], fromUnit = 'point', toUnit = 'inch')
        w = (width + wExtent) * cols
        h = len(labels) * hExtent / cols
        h *= 1.1
        return (w, h)
            
   
class Raster(InstructionObject):
    """!Class representing raster instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'raster'
        # default values
        self.defaultInstruction = dict(isRaster = False, raster = None)
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = string.Template("raster $raster").substitute(self.instruction)
        return instr
    
    def Read(self, instruction, text):
        """!Read instruction and save information"""
        instr = {}
        instr['isRaster'] = True
        try:
            map = text.split()[1]
        except IndexError:
            GError(_("Failed to read instruction %s") % instruction)
            return False
        try:
            info = grass.find_file(map, element = 'cell')
        except grass.ScriptError, e:
            GError(message = e.value)
            return False
        instr['raster'] = info['fullname']

        
        self.instruction.update(instr)
        return True
    
class Vector(InstructionObject):
    """!Class keeps vector layers"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'vector'
        # default values
        self.defaultInstruction = dict(list = None)# [vmap, type, id, lpos, label] 
        # current values
        self.instruction = dict(self.defaultInstruction)
    def __str__(self):
        return ''
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        instr = {}
        
        for line in text:
            if line.startswith('vpoints') or line.startswith('vlines') or line.startswith('vareas'):
                # subtype
                if line.startswith('vpoints'):
                    subType = 'points'
                elif line.startswith('vlines'):
                    subType = 'lines'
                elif line.startswith('vareas'):
                    subType = 'areas'
                # name of vector map
                vmap = line.split()[1]
                try:
                    info = grass.find_file(vmap, element = 'vector')
                except grass.ScriptError, e:
                    GError(message = e.value)
                    return False
                vmap = info['fullname']
                # id
                id = kwargs['id']
                # lpos
                lpos = kwargs['vectorMapNumber']
                #label
                label = '('.join(vmap.split('@')) + ')'
                break
        instr = [vmap, subType, id, lpos, label] 
        if not self.instruction['list']:
            self.instruction['list'] = []
        self.instruction['list'].append(instr)
        
        return True    
    
class VProperties(InstructionObject):
    """!Class represents instructions vareas, vlines, vpoints"""
    def __init__(self, id, subType):
        InstructionObject.__init__(self, id = id)
        self.type = 'vProperties'
        self.subType = subType
        # default values
        if self.subType == 'points':
            dd = dict(subType  = 'points', name = None, type = 'point or centroid', connection = False, layer = '1',
                        masked = 'n', color = '0:0:0', width = 1,
                        fcolor = '255:0:0', rgbcolumn = None, symbol = os.path.join('basic', 'x'), eps = None,
                        size = 5, sizecolumn = None, scale = None,
                        rotation = False, rotate = 0, rotatecolumn = None, label = None, lpos = None)
        elif self.subType == 'lines':
            dd = dict(subType = 'lines', name = None, type = 'line or boundary', connection = False, layer = '1',
                        masked = 'n', color = '0:0:0', hwidth = 1,
                        hcolor = 'none', rgbcolumn = None,
                        width = 1, cwidth = None,
                        style = 'solid', linecap = 'butt', label = None, lpos = None)
        else: # areas
            dd = dict(subType = 'areas', name = None, connection = False, layer = '1',    
                        masked = 'n', color = '0:0:0', width = 1,
                        fcolor = 'none', rgbcolumn = None,
                        pat = None, pwidth = 1, scale = 1, label = None, lpos = None)
        self.defaultInstruction = dd
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        dic = self.instruction
        vInstruction = string.Template("v$subType $name\n").substitute(dic)
        #data selection
        if self.subType in ('points', 'lines'):
           vInstruction += string.Template("    type $type\n").substitute(dic) 
        if dic['connection']:
            vInstruction += string.Template("    layer $layer\n").substitute(dic)
            if dic.has_key('cats'):
                vInstruction += string.Template("    cats $cats\n").substitute(dic)
            elif dic.has_key('where'):
                    vInstruction += string.Template("    where $where\n").substitute(dic)
        vInstruction += string.Template("    masked $masked\n").substitute(dic)
        #colors
        vInstruction += string.Template("    color $color\n").substitute(dic)
        if self.subType in ('points', 'areas'):
            if dic['color'] != 'none':
                vInstruction += string.Template("    width $width\n").substitute(dic)
            if dic['rgbcolumn']:
                vInstruction += string.Template("    rgbcolumn $rgbcolumn\n").substitute(dic)
            vInstruction += string.Template("    fcolor $fcolor\n").substitute(dic)
        else:
            if dic['rgbcolumn']:
                vInstruction += string.Template("    rgbcolumn $rgbcolumn\n").substitute(dic)
            elif dic['hcolor'] != 'none':
                vInstruction += string.Template("    hwidth $hwidth\n").substitute(dic)
                vInstruction += string.Template("    hcolor $hcolor\n").substitute(dic)
        
        # size and style
        if self.subType == 'points':
            if not dic['eps']:
                vInstruction += string.Template("    symbol $symbol\n").substitute(dic)
            else: #eps
                vInstruction += string.Template("    eps $eps\n").substitute(dic)
            if dic['size']:
                vInstruction += string.Template("    size $size\n").substitute(dic)            
            else: # sizecolumn
                vInstruction += string.Template("    sizecolumn $sizecolumn\n").substitute(dic)
                vInstruction += string.Template("    scale $scale\n").substitute(dic)
            if dic['rotation']:
                if dic['rotate'] is not None:
                    vInstruction += string.Template("    rotate $rotate\n").substitute(dic)
                else:
                    vInstruction += string.Template("    rotatecolumn $rotatecolumn\n").substitute(dic)
                    
        if self.subType == 'areas':
            if dic['pat'] is not None:
                patternFile = dic['pat'].replace(os.getenv("GISBASE"), "$GISBASE")
                vInstruction += "    pat %s\n" % patternFile
                vInstruction += string.Template("    pwidth $pwidth\n").substitute(dic)
                vInstruction += string.Template("    scale $scale\n").substitute(dic)
                
        if self.subType == 'lines':
            if dic['width'] is not None:
                vInstruction += string.Template("    width $width\n").substitute(dic)
            else:
                vInstruction += string.Template("    cwidth $cwidth\n").substitute(dic)
            vInstruction += string.Template("    style $style\n").substitute(dic)
            vInstruction += string.Template("    linecap $linecap\n").substitute(dic)
        #position and label in vlegend
        vInstruction += string.Template("    label $label\n    lpos $lpos\n").substitute(dic)
        
        vInstruction += "    end"
        try:
            vInstruction = vInstruction.encode('Latin_1')
        except UnicodeEncodeError, err:
            try:
                pos = str(err).split('position')[1].split(':')[0].strip()
            except IndexError:
                pos = ''
            if pos:
                message = _("Characters on position %s are not supported "
                            "by ISO-8859-1 (Latin 1) encoding "
                            "which is required by module ps.map.") % pos
            else:
                message = _("Not all characters are supported "
                            "by ISO-8859-1 (Latin 1) encoding "
                            "which is required by module ps.map.")
            GMessage(message = message)
            return ''
        return vInstruction
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        instr = {}
        try:
            info = grass.find_file(name = text[0].split()[1], element = 'vector')
        except grass.ScriptError, e:
            GError(message = e.value)
            return False
        instr['name'] = info['fullname']
        #connection
        instr['connection'] = True
        self.mapDBInfo = VectorDBInfo(instr['name'])
        self.layers = self.mapDBInfo.layers.keys()
        if not self.layers:
            instr['connection'] = False
            
        # points
        if text[0].startswith('vpoints'):
            for line in text[1:]:
                if line.startswith('type'):
                    tp = []
                    if line.find('point') != -1:
                        tp.append('point')
                    if line.find('centroid') != -1:
                        tp.append('centroid')
                    instr['type'] = ' or '.join(tp)
                elif line.startswith('fcolor'):
                    instr['fcolor'] = line.split()[1]
                elif line.startswith('rgbcolumn'):
                    instr['rgbcolumn'] = line.split()[1]
                elif line.startswith('symbol'):
                    instr['symbol'] = line.split()[1]
                elif line.startswith('eps'):
                    instr['eps'] = line.split()[1]
                elif line.startswith('size '):
                    instr['size'] = line.split()[1]
                elif line.startswith('sizecolumn'):
                    instr['size'] = None
                    instr['sizecolumn'] = line.split()[1]
                elif line.startswith('scale '):
                    instr['scale'] = float(line.split()[1])
                elif line.startswith('rotate '):
                    instr['rotation'] = True
                    instr['rotate'] = line.split()[1]
                elif line.startswith('rotatecolumn'):
                    instr['rotatecolumn'] = line.split()[1]
                    instr['rotation'] = True
                    instr['rotate'] = None
                    
        # lines            
        elif text[0].startswith('vlines'):
            for line in text[1:]:
                if line.startswith('type'):
                    tp = []
                    if line.find('line') != -1:
                        tp.append('line')
                    if line.find('boundary') != -1:
                        tp.append('boundary')
                    instr['type'] = ' or '.join(tp)
                elif line.startswith('hwidth'):
                    instr['hwidth'] = float(line.split()[1])
                elif line.startswith('hcolor'):
                    instr['hcolor'] = line.split()[1]
                elif line.startswith('rgbcolumn'):
                    instr['rgbcolumn'] = line.split()[1]                    
                elif line.startswith('cwidth'):
                    instr['cwidth'] = float(line.split()[1])
                    instr['width'] = None
                elif line.startswith('style'):
                    instr['style'] = line.split()[1]       
                elif line.startswith('linecap'):
                    instr['linecap'] = line.split()[1]
         
        elif text[0].startswith('vareas'):
            for line in text[1:]:
                if line.startswith('fcolor'):
                    instr['fcolor'] = line.split()[1]    
                elif line.startswith('pat'):
                    patternFile = line.split()[1]
                    instr['pat'] = patternFile.replace("$GISBASE", os.getenv("GISBASE"))
                elif line.startswith('pwidth'):
                    instr['pwidth'] = float(line.split()[1])
                elif line.startswith('scale'):
                    instr['scale'] = float(line.split()[1])
            
            
        # same properties for all    
        for line in text[1:]:
            if line.startswith('lpos'):
                instr['lpos'] = int(line.split()[1])
            elif line.startswith('label'):
                instr['label'] = line.split(None, 1)[1].decode('latin_1')
            elif line.startswith('layer'):
                instr['layer'] = line.split()[1]
            elif line.startswith('masked'):
                if line.split()[1].lower() in ('y', 'yes'):
                    instr['masked'] = 'y'
                else:
                    instr['masked'] = 'n'
            elif line.startswith('color'):
                instr['color'] = line.split()[1]
            elif line.startswith('rgbcolumn'):
                instr['rgbcolumn'] = line.split()[1] 
            elif line.startswith('width'):
                instr['width'] = float(line.split()[1])
                
        if 'label' not in instr:
            instr['label'] = '('.join(instr['name'].split('@')) + ')'
        if 'lpos' not in instr:
            instr['lpos'] = kwargs['vectorMapNumber']
        self.instruction.update(instr)
        
        return True

class Labels(InstructionObject):
    """!Class representing labels instruction"""
    def __init__(self, id):
        InstructionObject.__init__(self, id = id)
        self.type = 'labels'
        # default values
        self.defaultInstruction = dict(labels=[])
        # current values
        self.instruction = dict(self.defaultInstruction)
        
    def __str__(self):
        instr = ''
        for label in self.instruction['labels']:
            instr += "labels %s\n" % label
            instr += "end\n"
        return instr
    
    def Read(self, instruction, text, **kwargs):
        """!Read instruction and save information"""
        for line in text:
            try:
                if line.startswith('labels'):
                    labels = line.split(None, 1)[1]
                    self.instruction['labels'].append(labels)
            except(IndexError, ValueError):
                GError(_("Failed to read instruction %s") % instruction)
                return False
        

        return True
