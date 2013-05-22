"""!
@package core.toolboxes

@brief Functions for modifying menu from default/user toolboxes specified in XML files

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
"""

import os
import sys
import copy
import xml.etree.ElementTree as etree
from xml.parsers import expat

# Get the XML parsing exceptions to catch. The behavior chnaged with Python 2.7
# and ElementTree 1.3.
if hasattr(etree, 'ParseError'):
    ETREE_EXCEPTIONS = (etree.ParseError, expat.ExpatError)
else:
    ETREE_EXCEPTIONS = (expat.ExpatError)

if sys.version_info[0:2] > (2, 6):
    has_xpath = True
else:
    has_xpath = False


if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.globalvar import ETCWXDIR
from core.utils import GetSettingsPath
from core.gcmd import GError

import grass.script.task as gtask
import grass.script.core as gcore
from grass.script.core import ScriptError


# this could be placed to functions
mainMenuFile = os.path.join(ETCWXDIR, 'xml', 'main_menu.xml')
toolboxesFile = os.path.join(ETCWXDIR, 'xml', 'toolboxes.xml')
wxguiItemsFile = os.path.join(ETCWXDIR, 'xml', 'wxgui_items.xml')
moduleItemsFile = os.path.join(ETCWXDIR, 'xml', 'module_items.xml')

userToolboxesFile = os.path.join(GetSettingsPath(), 'toolboxes', 'toolboxes.xml')
userMainMenuFile = os.path.join(GetSettingsPath(), 'toolboxes', 'main_menu.xml')
if not os.path.exists(userToolboxesFile):
    userToolboxesFile = None
if not os.path.exists(userMainMenuFile):
    userMainMenuFile = None


def getMenuFile():
    """!Returns path to XML file for building menu.

    Creates toolbox directory where user defined toolboxes should be located.
    Checks whether it is needed to create new XML file (user changed toolboxes)
    or the already generated file could be used.
    If something goes wrong during building or user doesn't modify menu,
    default file (from distribution) is returned.
    """
    fallback = os.path.join(ETCWXDIR, 'xml', 'menudata.xml')
    # always create toolboxes directory if does not exist yet
    tbDir = _setupToolboxes()

    if tbDir:
        menudataFile = os.path.join(tbDir, 'menudata.xml')
        generateNew = False
        # when any of main_menu.xml or toolboxes.xml are changed,
        # generate new menudata.xml

        if os.path.exists(menudataFile):
            # remove menu file when there is no main_menu and toolboxes
            if not userToolboxesFile and not userMainMenuFile:
                os.remove(menudataFile)
                return fallback

            if bool(userToolboxesFile) != bool(userMainMenuFile):
                # always generate new because we don't know if there has been any change
                generateNew = True
            else:
                # if newer files -> generate new
                menudataTime = os.path.getmtime(menudataFile)
                if userToolboxesFile:
                    if os.path.getmtime(userToolboxesFile) > menudataTime:
                        generateNew = True
                if userMainMenuFile:
                    if os.path.getmtime(userMainMenuFile) > menudataTime:
                        generateNew = True
        elif userToolboxesFile or userMainMenuFile:
            generateNew = True
        else:
            return fallback

        if generateNew:
            try:
                tree = toolboxes2menudata()
            except ETREE_EXCEPTIONS:
                GError(_("Unable to parse user toolboxes XML files. "
                         "Default toolboxes will be loaded."))
                return fallback

            try:
                xml = _getXMLString(tree.getroot())
                fh = open(os.path.join(tbDir, 'menudata.xml'), 'w')
                fh.write(xml)
                fh.close()
                return menudataFile
            except:
                return fallback
        else:
            return menudataFile
    else:
        return fallback


def _setupToolboxes():
    """!Create 'toolboxes' directory if doesn't exist."""
    basePath = GetSettingsPath()
    path = os.path.join(basePath, 'toolboxes')
    if not os.path.exists(basePath):
        return None

    if _createPath(path):
           return path
    return None


def _createPath(path):
    """!Creates path (for toolboxes) if it doesn't exist'"""
    if not os.path.exists(path):
        try:
            os.mkdir(path)
        except OSError, e:
            # we cannot use GError or similar because the gui doesn''t start at all
            gcore.warning('%(reason)s\n%(detail)s' % 
                    ({'reason':_('Unable to create toolboxes directory.'),
                      'detail': str(e)}))
            return False
    return True


def toolboxes2menudata(userDefined=True):
    """!Creates XML file with data for menu.

    Parses toolboxes files from distribution and from users,
    puts them together, adds metadata to modules and convert
    tree to previous format used for loading menu.

    @param userDefined use toolboxes defined by user or not (during compilation)

    @return ElementTree instance
    """
    wxguiItems = etree.parse(wxguiItemsFile)
    moduleItems = etree.parse(moduleItemsFile)

    if userDefined and userMainMenuFile:
        mainMenu = etree.parse(userMainMenuFile)
    else:
        mainMenu = etree.parse(mainMenuFile)
    root = mainMenu.getroot()

    if userDefined and userToolboxesFile:
        userToolboxes = etree.parse(userToolboxesFile)
        _expandUserToolboxesItem(root, userToolboxes)
        _expandToolboxes(root, userToolboxes)

    if not userToolboxesFile:
        _removeUserToolboxesItem(root)

    toolboxes = etree.parse(toolboxesFile)
    _expandToolboxes(root, toolboxes)

    _expandItems(root, moduleItems, 'module-item')
    _expandItems(root, wxguiItems, 'wxgui-item')

    # in case of compilation there are no additional runtime modules
    # but we need to create empty elements
    _expandRuntimeModules(root)

    _addHandlers(root)
    _convertTree(root)
    _indent(root)

    return mainMenu


def _indent(elem, level=0):
    """!Helper function to fix indentation of XML files."""
    i = "\n" + level * "  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            _indent(elem, level + 1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i


def _expandToolboxes(node, toolboxes):
    """!Expands tree with toolboxes.

    Function is called recursively.

    @param node tree node where to look for subtoolboxes to be expanded
    @param toolboxes tree of toolboxes to be used for expansion
    
    >>> menu = etree.fromstring('''
    ... <toolbox name="Raster">
    ...   <label>&amp;Raster</label>
    ...   <items>
    ...     <module-item name="r.mask"/>
    ...     <wxgui-item name="RasterMapCalculator"/>
    ...     <subtoolbox name="NeighborhoodAnalysis"/>
    ...     <subtoolbox name="OverlayRasters"/>
    ...   </items>
    ... </toolbox>''')
    >>> toolboxes = etree.fromstring('''
    ... <toolboxes>
    ...   <toolbox name="NeighborhoodAnalysis">
    ...     <label>Neighborhood analysis</label>
    ...     <items>
    ...       <module-item name="r.neighbors"/>
    ...       <module-item name="v.neighbors"/>
    ...     </items>
    ...   </toolbox>
    ...   <toolbox name="OverlayRasters">
    ...     <label>Overlay rasters</label>
    ...     <items>
    ...       <module-item name="r.cross"/>
    ...     </items>
    ...   </toolbox>
    ... </toolboxes>''')
    >>> _expandToolboxes(menu, toolboxes)
    >>> print etree.tostring(menu)
    <toolbox name="Raster">
      <label>&amp;Raster</label>
      <items>
        <module-item name="r.mask" />
        <wxgui-item name="RasterMapCalculator" />
        <toolbox name="NeighborhoodAnalysis">
        <label>Neighborhood analysis</label>
        <items>
          <module-item name="r.neighbors" />
          <module-item name="v.neighbors" />
        </items>
      </toolbox>
      <toolbox name="OverlayRasters">
        <label>Overlay rasters</label>
        <items>
          <module-item name="r.cross" />
        </items>
      </toolbox>
    </items>
    </toolbox>
    """
    nodes = node.findall('.//toolbox')
    if node.tag == 'toolbox':  # root
        nodes.append(node)
    for n in nodes:
        if n.find('items') is None:
            continue
        for subtoolbox in n.findall('./items/subtoolbox'):
            items = n.find('./items')
            idx = items.getchildren().index(subtoolbox)

            if has_xpath:
                toolbox = toolboxes.find('.//toolbox[@name="%s"]' % subtoolbox.get('name'))
            else:
                toolbox = None
                potentialToolboxes = toolboxes.findall('.//toolbox')
                sName = subtoolbox.get('name')
                for pToolbox in potentialToolboxes:
                    if pToolbox.get('name') == sName:
                        toolbox = pToolbox
                        break

            if toolbox is None:  # not in file
                continue
            _expandToolboxes(toolbox, toolboxes)
            items.insert(idx, toolbox)
            items.remove(subtoolbox)


def _expandUserToolboxesItem(node, toolboxes):
    """!Expand tag 'user-toolboxes-list'.

    Include all user toolboxes.

    >>> tree = etree.fromstring('<toolbox><items><user-toolboxes-list/></items></toolbox>')
    >>> toolboxes = etree.fromstring('<toolboxes><toolbox name="UserToolbox"><items><module-item name="g.region"/></items></toolbox></toolboxes>')
    >>> _expandUserToolboxesItem(tree, toolboxes)
    >>> etree.tostring(tree)
    '<toolbox><items><toolbox name="GeneratedUserToolboxesList"><label>Toolboxes</label><items><toolbox name="UserToolbox"><items><module-item name="g.region" /></items></toolbox></items></toolbox></items></toolbox>'
    """
    tboxes = toolboxes.findall('.//toolbox')

    for n in node.findall('./items/user-toolboxes-list'):
        items = node.find('./items')
        idx = items.getchildren().index(n)
        el = etree.Element('toolbox', attrib={'name': 'GeneratedUserToolboxesList'})
        items.insert(idx, el)
        label = etree.SubElement(el, tag='label')
        label.text = _("Toolboxes")
        it = etree.SubElement(el, tag='items')
        for toolbox in tboxes:
            it.append(copy.deepcopy(toolbox))
        items.remove(n)


def _removeUserToolboxesItem(root):
    """!Removes tag 'user-toolboxes-list' if there are no user toolboxes.

    >>> tree = etree.fromstring('<toolbox><items><user-toolboxes-list/></items></toolbox>')
    >>> _removeUserToolboxesItem(tree)
    >>> etree.tostring(tree)
    '<toolbox><items /></toolbox>'
    """
    for n in root.findall('./items/user-toolboxes-list'):
        items = root.find('./items')
        items.remove(n)


def _expandItems(node, items, itemTag):
    """!Expand items from file

    >>> tree = etree.fromstring('<items><module-item name="g.region"></module-item></items>')
    >>> items = etree.fromstring('<module-items><module-item name="g.region"><module>g.region</module><description>GRASS region management</description></module-item></module-items>')
    >>> _expandItems(tree, items, 'module-item')
    >>> etree.tostring(tree)
    '<items><module-item name="g.region"><module>g.region</module><description>GRASS region management</description></module-item></items>'
    """
    for moduleItem in node.findall('.//' + itemTag):
        itemName = moduleItem.get('name')
        if has_xpath:
            moduleNode = items.find('.//%s[@name="%s"]' % (itemTag, itemName))
        else:
            moduleNode = None
            potentialModuleNodes = items.findall('.//%s' % itemTag)
            for mNode in potentialModuleNodes:
                if mNode.get('name') == itemName:
                    moduleNode = mNode
                    break

        if moduleNode is None:  # module not available in dist
            continue
        mItemChildren = moduleItem.getchildren()
        tagList = [n.tag for n in mItemChildren]
        for node in moduleNode.getchildren():
            if node.tag not in tagList:
                moduleItem.append(node)


def _expandRuntimeModules(node):
    """!Add information to modules (desc, keywords)
    by running them with --interface-description.

    >>> tree = etree.fromstring('<items>'
    ...                         '<module-item name="g.region"></module-item>'
    ...                         '</items>')
    >>> _expandRuntimeModules(tree)
    >>> etree.tostring(tree)
    '<items><module-item name="g.region"><module>g.region</module><description>Manages the boundary definitions for the geographic region.</description><keywords>general,settings</keywords></module-item></items>'
    """
    modules = node.findall('.//module-item')
    for module in modules:
        name = module.get('name')
        if module.find('module') is None:
            n = etree.SubElement(parent=module, tag='module')
            n.text = name

        if module.find('description') is None:
            desc, keywords = _loadMetadata(name)
            n = etree.SubElement(parent=module, tag='description')
            n.text = _escapeXML(desc)
            n = etree.SubElement(parent=module, tag='keywords')
            n.text = _escapeXML(','.join(keywords))


def _escapeXML(text):
    """!Helper function for correct escaping characters for XML.

    Duplicate function in core/toolboxes and probably also in man compilation
    and some existing Python package.

    >>> _escapeXML('<>&')
    '&amp;lt;&gt;&amp;'
    """
    return text.replace('<', '&lt;').replace("&", '&amp;').replace(">", '&gt;')


def _loadMetadata(module):
    """!Load metadata to modules.

    @param module module name
    @return (description, keywords as a list)
    """
    try:
        task = gtask.parse_interface(module)
    except ScriptError:
        return '', ''

    return task.get_description(full=True), \
        task.get_keywords()


def _addHandlers(node):
    """!Add missing handlers to modules"""
    for n in node.findall('.//module-item'):
        if n.find('handler') is None:
            handlerNode = etree.SubElement(parent=n, tag='handler')
            handlerNode.text = 'OnMenuCmd'

    # e.g. g.region -p
    for n in node.findall('.//wxgui-item'):
        if n.find('command') is not None:
            handlerNode = etree.SubElement(parent=n, tag='handler')
            handlerNode.text = 'RunMenuCmd'


def _convertTag(node, old, new):
    """!Converts tag name.
    
    >>> tree = etree.fromstring('<toolboxes><toolbox><items><module-item/></items></toolbox></toolboxes>')
    >>> _convertTag(tree, 'toolbox', 'menu')
    >>> _convertTag(tree, 'module-item', 'menuitem')
    >>> etree.tostring(tree)
    '<toolboxes><menu><items><menuitem /></items></menu></toolboxes>'
    """
    for n in node.findall('.//%s' % old):
        n.tag = new


def _convertTagAndRemoveAttrib(node, old, new):
    """Converts tag name and removes attributes.

    >>> tree = etree.fromstring('<toolboxes><toolbox name="Raster"><items><module-item name="g.region"/></items></toolbox></toolboxes>')
    >>> _convertTagAndRemoveAttrib(tree, 'toolbox', 'menu')
    >>> _convertTagAndRemoveAttrib(tree, 'module-item', 'menuitem')
    >>> etree.tostring(tree)
    '<toolboxes><menu><items><menuitem /></items></menu></toolboxes>'
    """
    for n in node.findall('.//%s' % old):
        n.tag = new
        n.attrib = {}


def _convertTree(root):
    """!Converts tree to be the form readable by core/menutree.py.

    >>> tree = etree.fromstring('<toolbox name="MainMenu"><label>Main menu</label><items><toolbox><label>Raster</label><items><module-item name="g.region"><module>g.region</module></module-item></items></toolbox></items></toolbox>')
    >>> _convertTree(tree)
    >>> etree.tostring(tree)
    '<menudata><menubar><menu><label>Raster</label><items><menuitem><command>g.region</command></menuitem></items></menu></menubar></menudata>'
    """
    root.attrib = {}
    label = root.find('label')
    root.remove(label)
    _convertTag(root, 'description', 'help')
    _convertTag(root, 'wx-id', 'id')
    _convertTag(root, 'module', 'command')
    _convertTag(root, 'related-module', 'command')
    _convertTagAndRemoveAttrib(root, 'wxgui-item', 'menuitem')
    _convertTagAndRemoveAttrib(root, 'module-item', 'menuitem')

    root.tag = 'menudata'
    i1 = root.find('./items')
    i1.tag = 'menubar'
    _convertTagAndRemoveAttrib(root, 'toolbox', 'menu')


def _getXMLString(root):
    """!Converts XML tree to string

    Since it is usually requier, this function adds a comment (about
    autogenerated file) to XML file.

    @return XML as string
    """
    xml = etree.tostring(root, encoding='UTF-8')
    return xml.replace("<?xml version='1.0' encoding='UTF-8'?>\n",
                       "<?xml version='1.0' encoding='UTF-8'?>\n"
                       "<!--This is an auto-generated file-->\n")


def do_doctest_gettext_workaround():
    """Setups environment for doing a doctest with gettext usage.

    When using gettext with dynamically defined underscore function
    (`_("For translation")`), doctest does not work properly. One option is to
    use `import as` instead of dynamically defined underscore function but this
    would require change all modules which are used by tested module. This
    should be considered for the future. The second option is to define dummy
    underscore function and one other function which creates the right
    environment to satisfy all. This is done by this function.
    """
    def new_displayhook(string):
        """A replacement for default `sys.displayhook`"""
        if string is not None:
            sys.stdout.write("%r\n" % (string,))

    def new_translator(string):
        """A fake gettext underscore function."""
        return string

    sys.displayhook = new_displayhook

    import __builtin__
    __builtin__._ = new_translator


def test():
    """Tests the module using doctest

    @return a number of failed tests
    """
    import doctest

    do_doctest_gettext_workaround()

    return doctest.testmod().failed


def main():
    """Converts the toolboxes files on standard paths to the menudata file

    File is written to the standard output.
    """
    tree = toolboxes2menudata(userDefined=False)
    root = tree.getroot()
    sys.stdout.write(_getXMLString(root))

    return 0


if __name__ == '__main__':
    if len(sys.argv) > 1:
        if sys.argv[1] == 'doctest':
            sys.exit(test())
    sys.exit(main())
