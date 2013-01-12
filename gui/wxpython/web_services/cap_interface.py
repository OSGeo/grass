"""!
@package web_services.cap_interface

@brief Provides common interface for GUI web_services.widgets to capabilities data of web services.

List of classes:
 - cap_interface::CapabilitiesBase
 - cap_interface::LayerBase
 - cap_interface::WMSCapabilities
 - cap_interface::WMSLayer
 - cap_interface::WMTSCapabilities
 - cap_interface::WMTSLayer
 - cap_interface::OnEarthCapabilities
 - cap_interface::OnEarthLayer

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (Mentor: Martin Landa)
"""

import os
import sys

WMSLibPath = os.path.join(os.getenv("GISBASE"), "etc", "r.in.wms")
sys.path.append(WMSLibPath)

from wms_cap_parsers import WMSCapabilitiesTree, \
                            WMTSCapabilitiesTree, \
                            OnEarthCapabilitiesTree

class CapabilitiesBase:
    def GetLayerByName(self, name):
        """!Find layer by name
        """
        for l in self.layers_by_id:
            if name == l.GetLayerData('name'):
                return l
        return None

    def GetRootLayer(self):
        """!Get children layers
        """
        if self.layers_by_id:
            return self.layers_by_id[0]
        else:
            return None

class LayerBase:
    def GetId(self):
        """!Get layer id
        """
        return self.id

    def GetChildren(self):
        """!Get children layers
        """
        return self.child_layers

    def GetLayerNode(self):
        """!Get layer node
        """
        return self.layer_node

    def AddChildLayer(self, layer):
        """!Add child layer
        """
        self.child_layers.append(layer)

class WMSCapabilities(CapabilitiesBase, WMSCapabilitiesTree):
    def __init__(self, cap_file, force_version = None):
        """!Create common interface for web_services.widgets to WMS capabilities data
        """
        # checks all elements needed for creation of GetMap requests
        # by r.in.wms/d.wms modules, invalid elements are removed
        WMSCapabilitiesTree.__init__(self, cap_file, force_version)

        self.cap_node = self.getroot().find(self.xml_ns.Ns("Capability"))
        self.root_layer = self.cap_node.find(self.xml_ns.Ns("Layer"))

        self.layers_by_id = {}
        self._initializeLayerTree(self.root_layer)

    def _initializeLayerTree(self, parent_layer, id = 0):
        """!Build tree, which represents layers
        """
        if id == 0:
            parent_layer = WMSLayer(parent_layer, id, self)
            self.layers_by_id[id] = parent_layer
            id += 1
        
        layer_nodes = parent_layer.GetLayerNode().findall((self.xml_ns.Ns("Layer")))
        
        for l in layer_nodes:
            layer = WMSLayer(l, id, self)
            parent_layer.AddChildLayer(layer)
            self.layers_by_id[id] = layer
            id += 1
            id = self._initializeLayerTree(layer, id)
        
        return id

    def GetFormats(self):
        """!Get supported formats
        """       
        request_node = self.cap_node.find(self.xml_ns.Ns("Request"))
        get_map_node = request_node.find(self.xml_ns.Ns("GetMap"))
        format_nodes = get_map_node.findall(self.xml_ns.Ns("Format"))
 
        formats = []
        for node in format_nodes:
            formats.append(node.text)

        return formats

class WMSLayer(LayerBase):
    def __init__(self, layer_node, id, cap):
        """!Common interface for web_services.widgets to WMS capabilities <Layer> element
        """
        self.id = id
        self.cap = cap
        self.child_layers = []
        self.layer_node = layer_node
        self.xml_ns = self.cap.getxmlnshandler()

    def GetLayerData(self, param):
        """!Get layer data"""
        title = self.xml_ns.Ns("Title")
        name = self.xml_ns.Ns("Name")

        if param == 'title':
            title_node = self.layer_node.find(title)
            if title_node is not None:
                return title_node.text 
            else:
                return None

        if param == 'name':
            name_node = self.layer_node.find(name)
            if name_node is not None:
                return name_node.text 
            else:
                return None

        if param == 'format':
            return self.cap.GetFormats()

        if param == 'styles':
            styles = []
            style = self.xml_ns.Ns("Style")
            for style_node in self.layer_node.findall(style):
                style_name = '' 
                style_title = ''

                if style_node.find(title) is not None:
                    style_title = style_node.find(title).text
                if style_node.find(name) is not None:
                    style_name = style_node.find(name).text

                styles.append({'title' : style_title, 
                               'name' : style_name,
                               'isDefault' : False})
            return styles

        if param == 'srs':
            projs_nodes = self.layer_node.findall(self.xml_ns.Ns(self.cap.getprojtag()))

            projs = []
            if projs_nodes is None:
                return projs
            for p in projs_nodes:
                projs.append(p.text.strip())
            return projs

    def IsRequestable(self):
        """!Is it possible to use the layer for WMS GetMap request?
        """
        name = self.xml_ns.Ns("Name")
        name_node = self.layer_node.find(name)

        if name_node is not None:
            return True
        else:
            return False

class WMTSCapabilities(CapabilitiesBase, WMTSCapabilitiesTree):
    def __init__(self, cap_file):
        """!Create common interface for web_services.widgets to WMTS capabilities data
        """
        # checks all elements needed for creation of GetTile requests
        # by r.in.wms/d.wms modules, invalid elements are removed
        WMTSCapabilitiesTree.__init__(self, cap_file)

        contents = self._find(self.getroot(), 'Contents', self.xml_ns.NsWmts)
        layers = self._findall(contents, 'Layer', self.xml_ns.NsWmts)

        self.layers_by_id = {}
        
        id = 0
        root_layer = WMTSLayer(None, id, self)
        self.layers_by_id[id] = root_layer

        for layer_node in layers:
            id += 1
            self.layers_by_id[id] = WMTSLayer(layer_node, id, self)
            root_layer.child_layers.append(self.layers_by_id[id])
    
class WMTSLayer(LayerBase):
    def __init__(self, layer_node, id, cap):
        """!Common interface for web_services.widgets to WMTS capabilities <Layer> element
        """
        self.id = id
        self.cap = cap
        self.child_layers = []
        self.layer_node = layer_node
        self.xml_ns = self.cap.getxmlnshandler()
        self.projs = self._getProjs()

    def GetLayerData(self, param):
        """!Get layer data
        """ 
        title = self.xml_ns.NsOws("Title")
        name = self.xml_ns.NsOws("Identifier")

        if self.layer_node is None and param in ['title', 'name']:
            return None
        elif self.layer_node is None:
            return []

        if param == 'title':
            title_node = self.layer_node.find(title)
            if title_node is not None:
                return title_node.text 
            else:
                return None

        if param == 'name':
            name_node = self.layer_node.find(name)
            if name_node is not None:
                return name_node.text 
            else:
                return None

        if param == 'styles':
            styles = []
            for style_node in self.layer_node.findall(self.xml_ns.NsWmts("Style")):

                style_name = '' 
                style_title = ''

                if style_node.find(title) is not None:
                    style_title = style_node.find(title).text
                if style_node.find(name) is not None:
                    style_name = style_node.find(name).text

                is_def = False
                if 'isDefault' in style_node.attrib and\
                    style_node.attrib['isDefault'] == 'true':
                    is_def = True

                styles.append({'title' : style_title, 
                               'name' : style_name,
                               'isDefault' : is_def})
            
            return styles

        if param == 'format':
            formats = []
            for frmt in self.layer_node.findall(self.xml_ns.NsWmts('Format')):
                formats.append(frmt.text.strip())
            return formats

        if param == 'srs':
            return self.projs

    def _getProjs(self):
        """!Get layer projections
        """ 
        layer_projs = []
        if self.layer_node is None:
            return layer_projs

        mat_set_links = self.layer_node.findall(self.xml_ns.NsWmts('TileMatrixSetLink'))

        contents = self.cap.getroot().find(self.xml_ns.NsWmts('Contents'))
        tileMatrixSets = contents.findall(self.xml_ns.NsWmts('TileMatrixSet'))

        for link in  mat_set_links:
            mat_set_link_id = link.find(self.xml_ns.NsWmts('TileMatrixSet')).text
            if not mat_set_link_id:
                continue

            for mat_set in tileMatrixSets:
                mat_set_id = mat_set.find(self.xml_ns.NsOws('Identifier')).text 
                if mat_set_id and mat_set_id != mat_set_link_id:
                    continue
                mat_set_srs = mat_set.find(self.xml_ns.NsOws('SupportedCRS')).text.strip()
                layer_projs.append(mat_set_srs)
        return layer_projs

    def IsRequestable(self):
        """!Is it possible to use the layer for WMTS request?
        """
        if self.layer_node is None:
           return False
        else:
            return True

class OnEarthCapabilities(CapabilitiesBase, OnEarthCapabilitiesTree):
    def __init__(self, cap_file):
        """!Create Common interface for web_services.widgets to NASA OnEarth 
            tile service data (equivalent to  WMS, WMTS capabilities data)
        """
        # checks all elements needed for creation of GetMap requests
        # by r.in.wms/d.wms modules, invalid elements are removed
        OnEarthCapabilitiesTree.__init__(self, cap_file)

        self.layers_by_id = {}
        self._initializeLayerTree(self.getroot())
        
    def _initializeLayerTree(self, parent_layer, id = 0):
        """!Build tree, which represents layers
        """
        if id == 0:
            tiled_patterns = parent_layer.find('TiledPatterns')
            layer_nodes = tiled_patterns.findall('TiledGroup')
            layer_nodes += tiled_patterns.findall('TiledGroups')
            parent_layer = OnEarthLayer(None, None, id, self)
            self.layers_by_id[id] = parent_layer
            id += 1
        else:
            layer_nodes = parent_layer.layer_node.findall('TiledGroup')
            layer_nodes += parent_layer.layer_node.findall('TiledGroups')

        for layer_node in layer_nodes:
            layer = OnEarthLayer(layer_node, parent_layer, id, self)
            self.layers_by_id[id] = layer
            id += 1
            parent_layer.child_layers.append(layer)
            if layer_node.tag == 'TiledGroups':
               id = self._initializeLayerTree(layer, id)

        return id

class OnEarthLayer(LayerBase):
    def __init__(self, layer_node, parent_layer, id, cap):
        """!Common interface for web_services.widgets to NASA Earth
            capabilities <TiledGroup>\<TiledGroups> element 
            (equivalent to  WMS, WMTS <Layer> element)
        """
        self.id = id
        self.cap = cap
        self.layer_node = layer_node
        self.child_layers = []
        self.parent_layer = parent_layer

    def IsRequestable(self):
        """!Is it possible to use the layer for NASA OnEarth GetMap request?
        """
        if self.layer_node is None or \
           self.layer_node.tag == 'TiledGroups':
           return False
        else:
            return True

    def GetLayerData(self, param):
        """!Get layer data
        """
        if self.layer_node is None and param in ['title', 'name']:
            return None
        elif self.layer_node is None:
            return []

        if param == 'title':
            title_node = self.layer_node.find("Title")
            if title_node is not None:
                return title_node.text 
            else:
                return None

        if param == 'name':
            name_node = self.layer_node.find("Name")
            if name_node is not None:
                return name_node.text 
            else:
                return None

        if param == 'styles':
            return []

        if param == 'format':
            return []

        if param == 'srs':
            return []
