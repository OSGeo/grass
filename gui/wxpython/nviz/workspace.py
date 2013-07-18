"""!
@package nviz.workspace

@brief wxNviz workspace settings

Classes:
 - workspace::NvizSettings

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com> (wxNviz / Google SoC 2011)
"""

import copy

from core.settings import UserSettings
from core.utils import _

try:
    from nviz      import wxnviz
except ImportError:
    wxnviz = None

class NvizSettings(object):
    def __init__(self):
        pass
        
    def SetConstantDefaultProp(self):
        """Set default constant data properties"""
        data = dict()
        for key, value in UserSettings.Get(group='nviz', key='constant').iteritems():
            data[key] = value
        color = str(data['color'][0]) + ':' + str(data['color'][1]) + ':' + str(data['color'][2])
        data['color'] = color

        return data
    
    def SetSurfaceDefaultProp(self, data = None):
        """Set default surface data properties"""
        if not data:
            data = dict()
        for sec in ('attribute', 'draw', 'mask', 'position'):
            data[sec] = {}
        
        #
        # attributes
        #
        for attrb in ('shine', ):
            data['attribute'][attrb] = {}
            for key, value in UserSettings.Get(group='nviz', key='surface',
                                               subkey=attrb).iteritems():
                data['attribute'][attrb][key] = value
            data['attribute'][attrb]['update'] = None
        
        #
        # draw
        #
        data['draw']['all'] = False # apply only for current surface
        for control, value in UserSettings.Get(group='nviz', key='surface', subkey='draw').iteritems():
            if control[:3] == 'res':
                if 'resolution' not in data['draw']:
                    data['draw']['resolution'] = {}
                if 'update' not in data['draw']['resolution']:
                    data['draw']['resolution']['update'] = None
                data['draw']['resolution'][control[4:]] = value
                continue
            
            if control == 'wire-color':
                value = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
            elif control in ('mode', 'style', 'shading'):
                if 'mode' not in data['draw']:
                    data['draw']['mode'] = {}
                continue

            data['draw'][control] = { 'value' : value }
            data['draw'][control]['update'] = None
        
        value, desc = self.GetDrawMode(UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'mode']),
                                       UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'style']),
                                       UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'shading']))
    
        data['draw']['mode'] = { 'value' : value,
                                 'desc' : desc, 
                                 'update': None }
        # position
        for coord in ('x', 'y', 'z'):
            data['position'][coord] = UserSettings.Get(group='nviz', key='surface', subkey=['position', coord])
        data['position']['update'] = None
            
        return data
    
    def SetVolumeDefaultProp(self):
        """Set default volume data properties"""
        data = dict()
        for sec in ('attribute', 'draw', 'position'):
            data[sec] = dict()
            for sec in ('isosurface', 'slice'):
                    data[sec] = list()
        
        #
        # draw
        #
        for control, value in UserSettings.Get(group='nviz', key='volume', subkey='draw').iteritems():
            if control == 'shading':
                sel = UserSettings.Get(group='nviz', key='volume', subkey=['draw', 'shading'])
                value, desc = self.GetDrawMode(shade=sel, string=False)
                
                data['draw']['shading'] = {}
                data['draw']['shading']['isosurface'] = { 'value' : value,
                                                          'desc' : desc['shading'] }
                data['draw']['shading']['slice'] = { 'value' : value,
                                                     'desc' : desc['shading'] }
            elif control == 'mode':
                sel = UserSettings.Get(group='nviz', key='volume', subkey=['draw', 'mode'])
                if sel == 0:
                    desc = 'isosurface'
                else:
                    desc = 'slice'
                data['draw']['mode'] = { 'value' : sel,
                                         'desc' : desc, }
            elif control == 'box':
                box = UserSettings.Get(group = 'nviz', key = 'volume', subkey = ['draw', 'box'])
                data['draw']['box'] = {'enabled': box}

            else:
                data['draw'][control] = {}
                data['draw'][control]['isosurface'] = { 'value' : value }
                data['draw'][control]['slice'] = { 'value' : value }

            if 'update' not in data['draw'][control]:
                data['draw'][control]['update'] = None
        
        #
        # isosurface attributes
        #
        for attrb in ('shine', ):
            data['attribute'][attrb] = {}
            for key, value in UserSettings.Get(group='nviz', key='volume',
                                               subkey=attrb).iteritems():
                data['attribute'][attrb][key] = value
        
        return data
    
    def SetIsosurfaceDefaultProp(self):
        """!Set default isosurface properties"""
        data = dict()
        for attr in ('shine', 'topo', 'transp', 'color'):
            data[attr] = {}
            for key, value in UserSettings.Get(group = 'nviz', key = 'volume',
                                               subkey = attr).iteritems():
                data[attr][key] = value
            data[attr]['update'] = None
        return data
    
    def SetSliceDefaultProp(self):
        """!Set default slice properties"""
        data = dict()
        data['position'] = copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'volume',
                                               subkey = 'slice_position'))
        data['position']['update'] = None
        
        data['transp'] = copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'volume',
                                               subkey = 'transp'))
        return data
    
    def SetVectorDefaultProp(self, data = None):
        """Set default vector data properties"""
        if not data:
            data = dict()
        for sec in ('lines', 'points'):
            data[sec] = {}
        
        self.SetVectorLinesDefaultProp(data['lines'])
        self.SetVectorPointsDefaultProp(data['points'])

        return data
    
    def SetVectorLinesDefaultProp(self, data):
        """Set default vector properties -- lines"""
        # width
        data['width'] = {'value' : UserSettings.Get(group='nviz', key='vector',
                                                    subkey=['lines', 'width']) }
        
        # color
        value = UserSettings.Get(group='nviz', key='vector',
                                 subkey=['lines', 'color'])
        color = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
        data['color'] = { 'value' : color }

        # mode
        if UserSettings.Get(group='nviz', key='vector',
                            subkey=['lines', 'flat']):
            type = 'flat'
            
        else:
            type = 'surface'
            
        data['mode'] = {}
        data['mode']['type'] = type
        data['mode']['update'] = None
    
        # height
        data['height'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['lines', 'height']) }
        # thematic
        data['thematic'] = {'rgbcolumn' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['lines', 'rgbcolumn']),
                            'sizecolumn' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['lines', 'sizecolumn']),
                            'layer': 1,
                            'usecolor' : False,
                            'usewidth' : False}
        if 'object' in data:
            for attrb in ('color', 'width', 'mode', 'height', 'thematic'):
                data[attrb]['update'] = None
        
    def SetVectorPointsDefaultProp(self, data):
        """Set default vector properties -- points"""
        # size
        data['size'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                    subkey=['points', 'size']) }

        # width
        data['width'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                     subkey=['points', 'width']) }

        # marker
        data['marker'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['points', 'marker']) }

        # color
        value = UserSettings.Get(group='nviz', key='vector',
                                 subkey=['points', 'color'])
        color = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
        data['color'] = { 'value' : color }

        # mode
        data['mode'] = { 'type' : 'surface'}
##                         'surface' : '', }
        
        # height
        data['height'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['points', 'height']) }
        
        data['thematic'] = {'rgbcolumn' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['points', 'rgbcolumn']),
                            'sizecolumn' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['points', 'sizecolumn']),
                            'layer': 1,
                            'usecolor' : False,
                            'usesize' : False}
        if 'object' in data:
            for attrb in ('size', 'width', 'marker',
                          'color', 'height', 'thematic'):
                data[attrb]['update'] = None
        
    def GetDrawMode(self, mode=None, style=None, shade=None, string=False):
        """Get surface draw mode (value) from description/selection

        @param mode,style,shade modes
        @param string if True input parameters are strings otherwise
        selections
        """
        if not wxnviz:
            return None
        
        value = 0
        desc = {}

        if string:
            if mode is not None:
                if mode == 'coarse':
                    value |= wxnviz.DM_WIRE
                elif mode == 'fine':
                    value |= wxnviz.DM_POLY
                else: # both
                    value |= wxnviz.DM_WIRE_POLY

            if style is not None:
                if style == 'wire':
                    value |= wxnviz.DM_GRID_WIRE
                else: # surface
                    value |= wxnviz.DM_GRID_SURF
                    
            if shade is not None:
                if shade == 'flat':
                    value |= wxnviz.DM_FLAT
                else: # surface
                    value |= wxnviz.DM_GOURAUD

            return value

        # -> string is False
        if mode is not None:
            if mode == 0: # coarse
                value |= wxnviz.DM_WIRE
                desc['mode'] = 'coarse'
            elif mode == 1: # fine
                value |= wxnviz.DM_POLY
                desc['mode'] = 'fine'
            else: # both
                value |= wxnviz.DM_WIRE_POLY
                desc['mode'] = 'both'

        if style is not None:
            if style == 0: # wire
                value |= wxnviz.DM_GRID_WIRE
                desc['style'] = 'wire'
            else: # surface
                value |= wxnviz.DM_GRID_SURF
                desc['style'] = 'surface'

        if shade is not None:
            if shade == 0:
                value |= wxnviz.DM_FLAT
                desc['shading'] = 'flat'
            else: # surface
                value |= wxnviz.DM_GOURAUD
                desc['shading'] = 'gouraud'
        
        return (value, desc)
    
    def SetDecorDefaultProp(self, type):
        """!Set default arrow properties
        """
        data = {}
        
        # arrow
        if type == 'arrow':
            data['arrow'] = copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'arrow'))
            data['arrow']['color'] = "%d:%d:%d" % (
                UserSettings.Get(group = 'nviz', key = 'arrow', subkey = 'color')[:3])
            data['arrow'].update(copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'arrow', internal = True)))
            data['arrow']['show'] = False
        
        # arrow
        if type == 'scalebar':
            data['scalebar'] = copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'scalebar'))
            data['scalebar']['color'] = "%d:%d:%d" % (
                UserSettings.Get(group = 'nviz', key = 'scalebar', subkey = 'color')[:3])
            data['scalebar'].update(copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'scalebar', internal = True)))
            data['scalebar']['id'] = 0
        return data
