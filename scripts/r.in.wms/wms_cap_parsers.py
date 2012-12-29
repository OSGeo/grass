"""!
@brief Parsers for WMS/WMTS/NASA OnEarth capabilities files.

List of classes:
 - wms_cap_parsers::BaseCapabilitiesTree
 - wms_cap_parsers::WMSXMLNsHandler
 - wms_cap_parsers::WMSCapabilitiesTree
 - wms_cap_parsers::WMTSXMLNsHandler
 - wms_cap_parsers::WMTSCapabilitiesTree
 - wms_cap_parsers::OnEarthCapabilitiesTree

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (Mentor: Martin Landa)
"""

import xml.etree.ElementTree as etree
import grass.script as grass 

class BaseCapabilitiesTree(etree.ElementTree):
    def __init__(self, cap_file):
        """!Initialize xml.etree.ElementTree
        """
        try:
            etree.ElementTree.__init__(self, file = cap_file)
        except etree.ParseError:
            raise etree.ParseError(_("Unable to parse XML file"))
        except IOError as error:
            raise etree.ParseError(_("Unabble to open XML file '%s'.\n%s\n" % (cap_file, error)))

        if self.getroot() is None:
            raise etree.ParseError(_("Root node was not found.")) 
        
class WMSXMLNsHandler:
    def __init__(self, caps):
        """!Handle XML namespaces according to WMS version of capabilities.
        """
        self.namespace = "{http://www.opengis.net/wms}"
        
        if caps.getroot().find("Service") is not None:
            self.use_ns = False
        elif caps.getroot().find(self.namespace + "Service") is not None:
            self.use_ns = True
        else:
            raise etree.ParseError(_("Unable to parse capabilities file.\n\
                                      Tag <%s> was not found.") % "Service")
    
    def Ns(self, tag_name):
        """!Add namespace to tag_name according to version
        """
        if self.use_ns:
            tag_name = self.namespace + tag_name
        return tag_name

class WMSCapabilitiesTree(BaseCapabilitiesTree):
    def __init__(self, cap_file, force_version = None):
        """!Parses WMS capabilities file.
            If the capabilities file cannot be parsed if it raises xml.etree.ElementTree.ParseError.

        The class manges inheritance in 'Layer' elements. Inherited elements 
        are added to 'Layer' element.
        The class also removes elements which are in invalid form and are needed 
        by wxGUI capabilities dialog.

        @param cap_file - capabilities file        
        @param force_version - force capabilities file version (1.1.1, 1.3.0)
        """
        BaseCapabilitiesTree.__init__(self, cap_file)
        self.xml_ns = WMSXMLNsHandler(self)

        grass.debug('Checking WMS capabilities tree.', 4)
        
        if not "version" in self.getroot().attrib:
            raise etree.ParseError(_("Missing version attribute root node "
                                     "in Capabilities XML file"))
        else:
            wms_version = self.getroot().attrib["version"]
        
        if wms_version == "1.3.0":
            self.proj_tag = "CRS"
        else:
            self.proj_tag = "SRS"

        if force_version is not None:
            if wms_version != force_version:
                raise etree.ParseError(_("WMS server does not support '%s' version.") % wms_version)

        capability = self._find(self.getroot(), "Capability")
        root_layer = self._find(capability, "Layer")

        self._checkFormats(capability)
        self._checkLayerTree(root_layer)

        grass.debug('Check of WMS capabilities tree was finished.', 4)

    def _checkFormats(self, capability):
        """!Check if format element is defined.
        """        
        request = self._find(capability, "Request")
        get_map = self._find(request, "GetMap")
        formats = self._findall(get_map, "Format")
 
    def _checkLayerTree(self, parent_layer, first = True):
        """!Recursively check layer tree and manage inheritance in the tree
        """
        if first:
            self._initLayer(parent_layer, None)
        
        layers = parent_layer.findall((self.xml_ns.Ns("Layer")))
        
        for l in layers:
            self._initLayer(l, parent_layer)
            self._checkLayerTree(l, False)
        
    def _initLayer(self, layer, parent_layer):
        """Inherit elements from parent layer

        @param layer - <Layer> element which inherits
        @param parent_layer - <Layer> element which is inherited from 
        """
        if parent_layer is not None:
            replaced_elements = [ ["EX_GeographicBoundingBox", "replace"],
                                  ["Attribution", "replace"],
                                  ["MinScaleDenominator", "replace"],
                                  ["MaxScaleDenominator", "replace"],
                                  ["AuthorityURL", "add"]]
            
            for element in replaced_elements:
                elems = layer.findall(self.xml_ns.Ns(element[0]))

                if len(elems) != 0 or element[1] == "add":
                    for e in parent_layer.findall(self.xml_ns.Ns(element[0])):
                        layer.append(e)
            
            inh_arguments = ["queryable", "cascaded", "opaque",
                             "noSubsets", "fixedWidth", "fixedHeight"]
            
            for attr in parent_layer.attrib:
                if attr not in layer.attrib and attr in inh_arguments:
                    layer.attrib[attr] = parent_layer.attrib[attr]
            
            self._inhNotSame(self.proj_tag, "element_content", layer, parent_layer)
            self._inhNotSame("BoundingBox", "attribute", layer, parent_layer, self.proj_tag)

            # remove invalid Styles 
            styles = layer.findall(self.xml_ns.Ns('Style'))
            for s in styles:
                s_name = s.find(self.xml_ns.Ns('Name'))
                if s_name is None or not s_name.text:
                    grass.debug('Removed invalid <Style> element.', 4)
                    layer.remove(s)

            self._inhNotSame("Style", "child_element_content", layer, parent_layer, "Name")
            self._inhNotSame("Dimension", "attribute", layer, parent_layer, "name")

    def _inhNotSame(self, element_name, cmp_type, layer, parent_layer, add_arg = None):
        """Inherit elements which have unique values.

        @param element_name - name of inherited element
        @param cmp_type - 'element_content' - compared value is text of <Layer> element
        @param cmp_type - 'child_element_content' - compared value is text of a child of the <Layer> element
        @param cmp_type - 'attribute' - compared value is text of the <Layer> element attribute
        @param layer - <Layer> element which inherits
        @param parent_layer - <Layer> element which is inherited from 
        @param add_arg - name of child element or attribute
        """
        elem = layer.findall(self.xml_ns.Ns(element_name))

        parent_elems = []
        if parent_layer is not None:
            parent_elems = parent_layer.findall(self.xml_ns.Ns(element_name))
        
        for par_elem in parent_elems:
            parent_cmp_text = None
            if cmp_type == "attribute":
                if add_arg in par_elem.attrib:
                    parent_cmp_text = par_elem.attrib[add_arg];

            elif cmp_type == "element_content":
                parent_cmp_text = par_elem.text
                
            elif cmp_type == "child_element_content":
                parent_cmp = par_elem.find(self.xml_ns.Ns(add_arg))
                if parent_cmp is not None:
                    parent_cmp_text = parent_cmp.text
            
            if parent_cmp_text is None:
                continue
            
            is_there = False
            for elem in elem:
                cmp_text = None
                if cmp_type == "attribute":
                    if add_arg in elem.attrib:
                        cmp_text = elem.attrib[add_arg]
                
                elif cmp_type == "element_content":
                    cmp_text = elem.text
                
                elif cmp_type == "child_element_content":
                    cmp = elem.find(self.xml_ns.Ns(add_arg))
                    if cmp is not None:
                        cmp_text = cmp.text
                
                if cmp_text is None or \
                   cmp_text.lower() == parent_cmp_text.lower():
                    is_there = True
                    break
            
            if not is_there:
                layer.append(par_elem)

    def _find(self, etreeElement, tag):
        """!Find child element.
            If the element is not found it raises xml.etree.ElementTree.ParseError.  
        """
        res = etreeElement.find(self.xml_ns.Ns(tag))

        if res is None:
            raise etree.ParseError(_("Unable to parse capabilities file. \n\
                                      Tag <%s> was not found.") % tag)

        return res

    def _findall(self, etreeElement, tag):
        """!Find all children element.
            If no element is found it raises xml.etree.ElementTree.ParseError.  
        """
        res = etreeElement.findall(self.xml_ns.Ns(tag))

        if not res:
            raise etree.ParseError(_("Unable to parse capabilities file. \n\
                                      Tag <%s> was not found.") % tag)

        return res

    def getprojtag(self):
        """!Return projection tag according to version of capabilities (CRS/SRS).
        """
        return self.proj_tag

    def getxmlnshandler(self):
        """!Return WMSXMLNsHandler object.
        """
        return self.xml_ns

class WMTSXMLNsHandler:
    """!Handle XML namespaces which are used in WMTS capabilities file.
    """
    def NsWmts(self, tag):
        """!Add namespace.
        """
        return "{http://www.opengis.net/wmts/1.0}" + tag

    def NsOws(self, tag):
        """!Add namespace.
        """
        return "{http://www.opengis.net/ows/1.1}" + tag

class WMTSCapabilitiesTree(BaseCapabilitiesTree):
    def __init__(self, cap_file):
        """!Parses WMTS capabilities file.
            If the capabilities file cannot be parsed it raises xml.etree.ElementTree.ParseError.

        The class also removes elements which are in invalid form and are needed 
        by wxGUI capabilities dialog or for creation of GetTile request by GRASS WMS library.

        @param cap_file - capabilities file        
        """
        BaseCapabilitiesTree.__init__(self, cap_file)
        self.xml_ns = WMTSXMLNsHandler()
        
        grass.debug('Checking WMTS capabilities tree.', 4)

        contents = self._find(self.getroot(), 'Contents', self.xml_ns.NsWmts)

        tile_mat_sets = self._findall(contents, 'TileMatrixSet', self.xml_ns.NsWmts)

        for mat_set in tile_mat_sets:
            if not self._checkMatSet(mat_set):
                grass.debug('Removed invalid <TileMatrixSet> element.', 4)
                contents.remove(mat_set)

        # are there any <TileMatrixSet> elements after the check
        self._findall(contents, 'TileMatrixSet', self.xml_ns.NsWmts)

        layers = self._findall(contents, 'Layer', self.xml_ns.NsWmts)
        for l in layers:
            if not self._checkLayer(l):
                grass.debug('Removed invalid <Layer> element.', 4)
                contents.remove(l)

        # are there any <Layer> elements after the check
        self._findall(contents, 'Layer', self.xml_ns.NsWmts)

        grass.debug('Check of WMTS capabilities tree was finished.', 4)

    def _checkMatSet(self, mat_set):
        """!Check <TileMatrixSet>.
        """
        mat_set_id = mat_set.find(self.xml_ns.NsOws('Identifier'))
        if mat_set_id is None or not mat_set_id.text:
            return False

        mat_set_srs = mat_set.find(self.xml_ns.NsOws('SupportedCRS'))
        if mat_set_srs is None or \
           not mat_set_srs.text:
            return False

        tile_mats = mat_set.findall(self.xml_ns.NsWmts('TileMatrix'))
        if not tile_mats:
            return False

        for t_mat in tile_mats:
            if not self._checkMat(t_mat):
               grass.debug('Removed invalid <TileMatrix> element.', 4)
               mat_set.remove(t_mat)

        tile_mats = mat_set.findall(self.xml_ns.NsWmts('TileMatrix'))
        if not tile_mats:
            return False

        return True

    def _checkMat(self, t_mat):        
        """!Check <TileMatrix>.
        """
        def _checkElement(t_mat, e, func):
            element = t_mat.find(self.xml_ns.NsWmts(e))
            if element is None or not element.text: 
                return False

            try:
                e = func(element.text)
            except ValueError:
                return False

            if e < 0:
                return False
            return True

        for e, func in [['ScaleDenominator', float], 
                        ['TileWidth', int], 
                        ['TileHeight', int]]:
            if not _checkElement(t_mat, e, func):
                return False
                        
        tile_mat_id = t_mat.find(self.xml_ns.NsOws('Identifier'))
        if tile_mat_id is None or not tile_mat_id.text:
            return False

        tl_str = t_mat.find(self.xml_ns.NsWmts('TopLeftCorner'))
        if tl_str is None or not tl_str.text:
            return False

        tl = tl_str.text.split(' ')
        if len(tl) < 2:
            return False

        for t in tl:              
            try:
                t = float(t)
            except ValueError:
                return False
        return True
    
    def _checkLayer(self, layer):
        """!Check <Layer> element.
        """
        layer_id = layer.find(self.xml_ns.NsOws('Identifier'))
        if layer_id is None or not layer_id.text:
            return False

        mat_set_links = layer.findall(self.xml_ns.NsWmts('TileMatrixSetLink'))
        if not mat_set_links:
            return False

        styles = layer.findall(self.xml_ns.NsWmts('Style'))
        if not styles:
            return False

        for s in styles:
            s_name = s.find(self.xml_ns.NsOws('Identifier'))
            if s_name is None or not s_name.text:
                grass.debug('Removed invalid <Style> element.', 4)
                layer.remove(s_name)

        contents = self.getroot().find(self.xml_ns.NsWmts('Contents'))
        mat_sets = contents.findall(self.xml_ns.NsWmts('TileMatrixSet'))

        for link in  mat_set_links:
            # <TileMatrixSetLink> does not point to existing  <TileMatrixSet>
            if not self._checkMatSetLink(link, mat_sets):
                grass.debug('Removed invalid <TileMatrixSetLink> element.', 4)
                layer.remove(link)

        return True

    def _checkMatSetLink(self, link, mat_sets):
        """!Check <TileMatrixSetLink> element.
        """
        mat_set_link_id = link.find(self.xml_ns.NsWmts('TileMatrixSet')).text
        found = False

        for mat_set in mat_sets:
            mat_set_id = mat_set.find(self.xml_ns.NsOws('Identifier')).text

            if mat_set_id != mat_set_link_id:
                continue

            # the link points to existing <TileMatrixSet>
            found = True

            tile_mat_set_limits = link.find(self.xml_ns.NsWmts('TileMatrixSetLimits'))
            if tile_mat_set_limits is None:
                continue

            tile_mat_limits = tile_mat_set_limits.findall(self.xml_ns.NsWmts('TileMatrixLimits'))
            for limit in tile_mat_limits:
                if not self._checkMatSetLimit(limit):
                    grass.debug('Removed invalid <TileMatrixLimits> element.', 4)
                    tile_mat_limits.remove(limit)

            # are there any <TileMatrixLimits> elements after the check
            tile_mat_limits = tile_mat_set_limits.findall(self.xml_ns.NsWmts('TileMatrixLimits'))
            if not tile_mat_limits:
                grass.debug('Removed invalid <TileMatrixSetLimits> element.', 4)
                link.remove(tile_mat_set_limits)
          
        if not found:
            return False

        return True

    def _checkMatSetLimit(self, limit):
        """!Check <TileMatrixLimits> element.
        """
        limit_tile_mat = limit.find(self.xml_ns.NsWmts('TileMatrix'))
        if limit_tile_mat is None or not limit_tile_mat.text:
            return False

        for i in ['MinTileRow', 'MaxTileRow', 'MinTileCol', 'MaxTileCol']:
            i_tag = limit.find(self.xml_ns.NsWmts(i))
            if i_tag is None:
                return False
            try:
                int(i_tag.text)
            except ValueError:
                return False
        return True

    def _find(self, etreeElement, tag, ns = None):
        """!Find child element.
            If the element is not found it raises xml.etree.ElementTree.ParseError.  
        """
        if not ns:
            res = etreeElement.find(tag)
        else:
            res = etreeElement.find(ns(tag))

        if res is None:
            raise etree.ParseError(_("Unable to parse capabilities file. \n\
                                      Tag '%s' was not found.") % tag)

        return res

    def _findall(self, etreeElement, tag, ns = None):
        """!Find all children element.
            If no element is found it raises xml.etree.ElementTree.ParseError.  
        """
        if not ns:
            res = etreeElement.findall(tag)
        else:
            res = etreeElement.findall(ns(tag))

        if not res:
            raise etree.ParseError(_("Unable to parse capabilities file. \n\
                                      Tag '%s' was not found.") % tag)

        return res

    def getxmlnshandler(self):
        """!Return WMTSXMLNsHandler object.
        """
        return self.xml_ns

class OnEarthCapabilitiesTree(BaseCapabilitiesTree):
    def __init__(self, cap_file):
        """!Parse NASA OnEarth tile service file.
            If the file cannot be parsed it raises xml.etree.ElementTree.ParseError.

        The class also removes elements which are in invalid form and are needed 
        by wxGUI capabilities dialog or for creation of GetMap request by GRASS WMS library.

        @param cap_file - capabilities file        
        """
        BaseCapabilitiesTree.__init__(self, cap_file)

        grass.debug('Checking OnEarth capabilities tree.', 4)

        self._checkLayerTree(self.getroot())

        grass.debug('Check if OnEarth capabilities tree was finished.', 4)

    def _checkLayerTree(self, parent_layer, first = True):
        """!Recursively check layer tree.
        """
        if first:
            tiled_patterns = self._find(parent_layer, 'TiledPatterns')
            layers = tiled_patterns.findall('TiledGroup')
            layers += tiled_patterns.findall('TiledGroups')
            parent_layer = tiled_patterns
        else:
            layers = parent_layer.findall('TiledGroup')
            layers += parent_layer.findall('TiledGroups')

        for l in layers:
            if not self._checkLayer(l):
                grass.debug(('Removed invalid <%s> element.' % l.tag), 4)
                parent_layer.remove(l)
            if l.tag == 'TiledGroups':
               self._checkLayerTree(l, False)

    def _find(self, etreeElement, tag):
        """!Find child element.
            If the element is not found it raises xml.etree.ElementTree.ParseError.  
        """
        res = etreeElement.find(tag)

        if res is None:
            raise  etree.ParseError(_("Unable to parse tile service file. \n\
                                       Tag <%s> was not found.") % tag)

        return res

    def _checkLayer(self, layer):
        """!Check <TiledGroup>/<TiledGroups> elements.
        """
        if layer.tag == 'TiledGroups':
            return True
            
        name = layer.find('Name')
        if name is None or not name.text:
            return False

        t_patts = layer.findall('TilePattern')

        for patt in t_patts:
            urls = self._getUrls(patt)
            for url in urls:
                if not self.gettilepatternurldata(url):
                    urls.remove(url)

            # check if there are any vaild urls
            if not urls:
                grass.debug('<TilePattern>  was removed. It has no valid url.', 4)
                layer.remove(patt)
            patt.text = '\n'.join(urls)

        t_patts = layer.findall('TilePattern')
        if not t_patts:
            return False

        return True

    def _getUrls(self, tile_pattern):
        """!Get all urls from tile pattern.
        """
        urls = []
        if  tile_pattern.text is not None:
            tile_patt_lines = tile_pattern.text.split('\n')

            for line in tile_patt_lines:
                if 'request=GetMap' in line:
                    urls.append(line.strip())
        return urls

    def gettilepatternurldata(self, url):
        """!Parse url string in Tile Pattern.
        """
        par_url = bbox = width = height = None 

        bbox_idxs = self.geturlparamidxs(url, "bbox=")
        if bbox_idxs is None:
            return None

        par_url = [url[:bbox_idxs[0] - 1], url[bbox_idxs[1]:]]

        bbox = url[bbox_idxs[0] + len('bbox=') : bbox_idxs[1]]
        bbox_list = bbox.split(',')
        if len(bbox_list) < 4:
            return None

        try:
            bbox = map(float, bbox.split(','))
        except ValueError:
            return None

        width_idxs = self.geturlparamidxs(url, "width=")
        if width_idxs is None:
            return None

        try:
            width = int(url[width_idxs[0] + len('width=') : width_idxs[1]])
        except  ValueError:
            return None

        height_idxs = self.geturlparamidxs(url, "height=")
        if height_idxs is None:
            return None

        try:
            height = int(url[height_idxs[0] + len('height=') : height_idxs[1]])
        except  ValueError:
            return None

        if height < 0 or width < 0:
            return None

        return par_url, bbox, width, height

    def geturlparamidxs(self, params_str, param_key):
        """!Find start and end index of parameter and it's value in url string
        """
        start_i = params_str.lower().find(param_key)
        if start_i < 0: 
            return None
        end_i = params_str.find("&", start_i)
        if end_i < 0:
            end_i = len(params_str)

        return (start_i, end_i)
