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
from grass.script.core import ScriptError


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
    path = os.path.join(GetSettingsPath(), 'toolboxes')
    if not os.path.exists(path):
        try:
            os.mkdir(path)
        except:
            GError(_('Unable to create toolboxes directory.'))
            return None
    return path


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
    """
    tboxes = toolboxes.findall('.//toolbox')

    for n in node.findall('./items/user-toolboxes-list'):
        items = node.find('./items')
        idx = items.getchildren().index(n)
        el = etree.Element('toolbox', attrib={'name': 'dummy'})
        items.insert(idx, el)
        label = etree.SubElement(el, tag='label')
        label.text = _("Toolboxes")
        it = etree.SubElement(el, tag='items')
        for toolbox in tboxes:
            it.append(copy.deepcopy(toolbox))


def _removeUserToolboxesItem(root):
    """!Removes tag 'user-toolboxes-list' if there are no user toolboxes."""
    for n in root.findall('./items/user-toolboxes-list'):
        items = root.find('./items')
        items.remove(n)


def _expandItems(node, items, itemTag):
    """!Expand items from file"""
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
    by running them with --interface-description."""
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

    Duplicate function in core/toolboxes.
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
    """!Converts tag name."""
    for n in node.findall('.//%s' % old):
        n.tag = new


def _convertTagAndRemoveAttrib(node, old, new):
    "Converts tag name and removes attributes."
    for n in node.findall('.//%s' % old):
        n.tag = new
        n.attrib = {}


def _convertTree(root):
    """!Converts tree to be the form readable by core/menutree.py."""
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
    """!Adds comment (about aotogenerated file) to XML.

    @return XML as string
    """
    xml = etree.tostring(root, encoding='UTF-8')
    return xml.replace("<?xml version='1.0' encoding='UTF-8'?>\n",
                       "<?xml version='1.0' encoding='UTF-8'?>\n"
                       "<!--This is an auto-generated file-->\n")


def main():
    tree = toolboxes2menudata(userDefined=False)
    root = tree.getroot()
    sys.stdout.write(_getXMLString(root))

    return 0


if __name__ == '__main__':
    sys.exit(main())
