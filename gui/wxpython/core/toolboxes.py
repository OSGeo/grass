"""
@package core.toolboxes

@brief Functions for modifying menu from default/user toolboxes specified in XML files

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
"""

from __future__ import print_function

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

import grass.script.task as gtask
import grass.script.core as gcore
from grass.script.utils import try_remove, decode
from grass.exceptions import ScriptError, CalledModuleError


# duplicating code from core/globalvar.py
# if this will become part of grass Python library or module, this should be
# parametrized, so we will get rid of the definition here
# (GUI will use its definition and build also its own)
WXGUIDIR = os.path.join(os.getenv("GISBASE"), "gui", "wxpython")


# this could be placed to functions
mainMenuFile = os.path.join(WXGUIDIR, 'xml', 'main_menu.xml')
toolboxesFile = os.path.join(WXGUIDIR, 'xml', 'toolboxes.xml')
wxguiItemsFile = os.path.join(WXGUIDIR, 'xml', 'wxgui_items.xml')
moduleItemsFile = os.path.join(WXGUIDIR, 'xml', 'module_items.xml')


def GetSettingsPath():
    # this is for cases during compilation and it is not possible to import wx
    # TODO: if the function would be in the grass Python library there would
    # no need to do this
    try:
        from core.utils import GetSettingsPath as actualGetSettingsPath
        return actualGetSettingsPath()
    except ImportError:
        # expecting that there will be no such path
        # (these files are always check for existence here)
        return ""


def _getUserToolboxesFile():
    userToolboxesFile = os.path.join(
        GetSettingsPath(),
        'toolboxes', 'toolboxes.xml')
    if not os.path.exists(userToolboxesFile):
        userToolboxesFile = None
    return userToolboxesFile


def _getUserMainMenuFile():
    userMainMenuFile = os.path.join(
        GetSettingsPath(),
        'toolboxes', 'main_menu.xml')
    if not os.path.exists(userMainMenuFile):
        userMainMenuFile = None
    return userMainMenuFile


# TODO: this should be part of some reader object
_MESSAGES = []


def _warning(message):
    """Show warning"""
    _MESSAGES.append("WARNING: %s" % message)


def getMessages():
    return _MESSAGES


def clearMessages():
    del _MESSAGES[:]


def _debug(level, message):
    """Show debug message"""
    # this has interface as originally used GUI Debug but uses grass.script
    gcore.debug(message, level)


def toolboxesOutdated():
    """Removes auto-generated menudata.xml
    to let gui regenerate it next time it starts."""
    path = os.path.join(GetSettingsPath(), 'toolboxes', 'menudata.xml')
    if os.path.exists(path):
        try_remove(path)


def getMenudataFile(userRootFile, newFile, fallback):
    """Returns path to XML file for building menu or another tree.

    Creates toolbox directory where user defined toolboxes should be
    located. Checks whether it is needed to create new XML file (user
    changed toolboxes) or the already generated file could be used.
    If something goes wrong during building or user doesn't modify menu,
    default file (from distribution) is returned.
    """
    _debug(
        1,
        "toolboxes.getMenudataFile: {userRootFile}, {newFile}, {fallback}".format(
            **locals()))

    distributionRootFile = os.path.join(WXGUIDIR, 'xml', userRootFile)
    userRootFile = os.path.join(GetSettingsPath(), 'toolboxes', userRootFile)
    if not os.path.exists(userRootFile):
        userRootFile = None

    # always create toolboxes directory if does not exist yet
    tbDir = _setupToolboxes()

    if tbDir:
        menudataFile = os.path.join(tbDir, newFile)
        generateNew = False
        # when any of main_menu.xml or toolboxes.xml are changed,
        # generate new menudata.xml

        if os.path.exists(menudataFile):
            # remove menu file when there is no main_menu and toolboxes
            if not _getUserToolboxesFile() and not userRootFile:
                os.remove(menudataFile)
                _debug(
                    2, "toolboxes.getMenudataFile: no user defined files, menudata deleted")
                return fallback

            if bool(_getUserToolboxesFile()) != bool(userRootFile):
                # always generate new because we don't know if there has been
                # any change
                generateNew = True
                _debug(
                    2, "toolboxes.getMenudataFile: only one of the user defined files")
            else:
                # if newer files -> generate new
                menudataTime = os.path.getmtime(menudataFile)
                if _getUserToolboxesFile():
                    if os.path.getmtime(
                            _getUserToolboxesFile()) > menudataTime:
                        _debug(
                            2, "toolboxes.getMenudataFile: user toolboxes is newer than menudata")
                        generateNew = True
                if userRootFile:
                    if os.path.getmtime(userRootFile) > menudataTime:
                        _debug(
                            2, "toolboxes.getMenudataFile: user root file is newer than menudata")
                        generateNew = True
        elif _getUserToolboxesFile() or userRootFile:
            _debug(2, "toolboxes.getMenudataFile: no menudata")
            generateNew = True
        else:
            _debug(2, "toolboxes.getMenudataFile: no user defined files")
            return fallback

        if generateNew:
            try:
                # The case when user does not have custom root
                # file but has toolboxes requieres regeneration.
                # Unfortunately, this is the case can be often: defined
                # toolboxes but undefined module tree file.
                _debug(2, "toolboxes.getMenudataFile: creating a tree")
                tree = createTree(
                    distributionRootFile=distributionRootFile,
                    userRootFile=userRootFile)
            except ETREE_EXCEPTIONS:
                _warning(_("Unable to parse user toolboxes XML files. "
                           "Default files will be loaded."))
                return fallback

            try:
                xml = _getXMLString(tree.getroot())
                fh = open(menudataFile, 'w')
                fh.write(xml)
                fh.close()
                return menudataFile
            except:
                _debug(
                    2,
                    "toolboxes.getMenudataFile: writing menudata failed, returning fallback file")
                return fallback
        else:
            return menudataFile
    else:
        _debug(2, "toolboxes.getMenudataFile: returning menudata fallback file")
        return fallback


def _setupToolboxes():
    """Create 'toolboxes' directory if doesn't exist."""
    basePath = GetSettingsPath()
    path = os.path.join(basePath, 'toolboxes')
    if not os.path.exists(basePath):
        return None

    if _createPath(path):
        return path
    return None


def _createPath(path):
    """Creates path (for toolboxes) if it doesn't exist'"""
    if not os.path.exists(path):
        try:
            os.mkdir(path)
        except OSError as e:
            # we cannot use GError or similar because the gui doesn't start at
            # all
            gcore.warning(
                '%(reason)s\n%(detail)s' % ({
                    'reason': _('Unable to create toolboxes directory.'),
                    'detail': str(e)}))
            return False
    return True


def createTree(distributionRootFile, userRootFile, userDefined=True):
    """Creates XML file with data for menu.

    Parses toolboxes files from distribution and from users,
    puts them together, adds metadata to modules and convert
    tree to previous format used for loading menu.

    :param userDefined: use toolboxes defined by user or not (during compilation)

    :return: ElementTree instance
    """
    if userDefined and userRootFile:
        mainMenu = etree.parse(userRootFile)
    else:
        mainMenu = etree.parse(distributionRootFile)

    toolboxes = etree.parse(toolboxesFile)

    if userDefined and _getUserToolboxesFile():
        userToolboxes = etree.parse(_getUserToolboxesFile())
    else:
        userToolboxes = None

    wxguiItems = etree.parse(wxguiItemsFile)
    moduleItems = etree.parse(moduleItemsFile)

    return toolboxes2menudata(mainMenu=mainMenu,
                              toolboxes=toolboxes,
                              userToolboxes=userToolboxes,
                              wxguiItems=wxguiItems,
                              moduleItems=moduleItems)


def toolboxes2menudata(mainMenu, toolboxes, userToolboxes,
                       wxguiItems, moduleItems):
    """Creates XML file with data for menu.

    Parses toolboxes files from distribution and from users,
    puts them together, adds metadata to modules and convert
    tree to previous format used for loading menu.

    :param userDefined: use toolboxes defined by user or not (during compilation)

    :return: ElementTree instance
    """
    root = mainMenu.getroot()

    userHasToolboxes = False

    # in case user has empty toolboxes file (to avoid genereation)
    if userToolboxes and userToolboxes.findall('.//toolbox'):
        _expandUserToolboxesItem(root, userToolboxes)
        _expandToolboxes(root, userToolboxes)
        userHasToolboxes = True

    if not userHasToolboxes:
        _removeUserToolboxesItem(root)

    _expandToolboxes(root, toolboxes)

    # we do not expand addons here since they need to be expanded in runtime

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
    """Helper function to fix indentation of XML files."""
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


def expandAddons(tree):
    """Expands addons element.
    """
    root = tree.getroot()
    _expandAddonsItem(root)
    # expanding and converting is done twice, so there is some overhead
    # we don't load metadata by calling modules on Windows because when
    # installed addons are not compatible, Windows show ugly crash dialog
    # for every incompatible addon
    if sys.platform == "win32":
        _expandRuntimeModules(root, loadMetadata=False)
    else:
        _expandRuntimeModules(root, loadMetadata=True)
    _addHandlers(root)
    _convertTree(root)


def _expandToolboxes(node, toolboxes):
    """Expands tree with toolboxes.

    Function is called recursively.

    :param node: tree node where to look for subtoolboxes to be expanded
    :param toolboxes: tree of toolboxes to be used for expansion

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
    >>> print(etree.tostring(menu))
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
            idx = list(items).index(subtoolbox)

            if has_xpath:
                toolbox = toolboxes.find(
                    './/toolbox[@name="%s"]' %
                    subtoolbox.get('name'))
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
    """Expand tag 'user-toolboxes-list'.

    Include all user toolboxes.

    >>> tree = etree.fromstring('<toolbox><items><user-toolboxes-list/></items></toolbox>')
    >>> toolboxes = etree.fromstring('<toolboxes><toolbox name="UserToolbox"><items><module-item name="g.region"/></items></toolbox></toolboxes>')
    >>> _expandUserToolboxesItem(tree, toolboxes)
    >>> etree.tostring(tree)
    b'<toolbox><items><toolbox name="GeneratedUserToolboxesList"><label>Custom toolboxes</label><items><toolbox name="UserToolbox"><items><module-item name="g.region" /></items></toolbox></items></toolbox></items></toolbox>'
    """
    tboxes = toolboxes.findall('.//toolbox')

    for n in node.findall('./items/user-toolboxes-list'):
        items = node.find('./items')
        idx = list(items).index(n)
        el = etree.Element(
            'toolbox', attrib={
                'name': 'GeneratedUserToolboxesList'})
        items.insert(idx, el)
        label = etree.SubElement(el, 'label')
        label.text = _("Custom toolboxes")
        it = etree.SubElement(el, 'items')
        for toolbox in tboxes:
            it.append(copy.deepcopy(toolbox))
        items.remove(n)


def _removeUserToolboxesItem(root):
    """Removes tag 'user-toolboxes-list' if there are no user toolboxes.

    >>> tree = etree.fromstring('<toolbox><items><user-toolboxes-list/></items></toolbox>')
    >>> _removeUserToolboxesItem(tree)
    >>> etree.tostring(tree)
    b'<toolbox><items /></toolbox>'
    """
    for n in root.findall('./items/user-toolboxes-list'):
        items = root.find('./items')
        items.remove(n)


def _getAddons():
    try:
        output = gcore.read_command('g.extension', quiet=True, flags='ag')
    except CalledModuleError as error:
        _warning(_("List of addons cannot be obtained"
                   " because g.extension failed."
                   " Details: %s") % error)
        return []

    flist = []
    for line in output.splitlines():
        if not line.startswith('name'):
            continue
        for fexe in line.split('=', 1)[1].split(','):
            flist.append(fexe)

    return sorted(flist)


def _removeAddonsItem(node, addonsNodes):
    # TODO: change impl to be similar with the remove toolboxes
    for n in addonsNodes:
        items = node.find('./items')
        if items is not None:
            items.remove(n)
        # because of inconsistent menudata file
        items = node.find('./menubar')
        if items is not None:
            items.remove(n)


def _expandAddonsItem(node):
    """Expands addons element with currently installed addons.

    .. note::
        there is no mechanism yet to tell the gui to rebuild the
        menudata.xml file when new addons are added/removed.
    """
    # no addonsTag -> do nothing
    addonsTags = node.findall('.//addons')
    if not addonsTags:
        return
    # fetch addons
    addons = _getAddons()

    # no addons -> remove addons tag
    if not addons:
        _removeAddonsItem(node, addonsTags)
        return

    # create addons toolbox
    # keywords and desc are handled later automatically
    for n in addonsTags:
        # find parent is not possible with implementation of etree (in 2.7)
        items = node.find('./menubar')
        idx = list(items).index(n)
        # do not set name since it is already in menudata file
        # attib={'name': 'AddonsList'}
        el = etree.Element('menu')
        items.insert(idx, el)
        label = etree.SubElement(el, 'label')
        label.text = _("Addons")
        it = etree.SubElement(el, 'items')
        for addon in addons:
            addonItem = etree.SubElement(it, 'module-item')
            addonItem.attrib = {'name': addon}
            addonLabel = etree.SubElement(addonItem, 'label')
            addonLabel.text = addon
        items.remove(n)


def _expandItems(node, items, itemTag):
    """Expand items from file

    >>> tree = etree.fromstring('<items><module-item name="g.region"></module-item></items>')
    >>> items = etree.fromstring('<module-items><module-item name="g.region"><module>g.region</module><description>GRASS region management</description></module-item></module-items>')
    >>> _expandItems(tree, items, 'module-item')
    >>> etree.tostring(tree)
    b'<items><module-item name="g.region"><module>g.region</module><description>GRASS region management</description></module-item></items>'
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
        tagList = [n.tag for n in moduleItem]
        for node in moduleNode:
            if node.tag not in tagList:
                moduleItem.append(node)


def _expandRuntimeModules(node, loadMetadata=True):
    """Add information to modules (desc, keywords)
    by running them with --interface-description.
    If loadMetadata is False, modules are not called,
    useful for incompatible addons.

    >>> tree = etree.fromstring('<items>'
    ...                         '<module-item name="g.region"></module-item>'
    ...                         '</items>')
    >>> _expandRuntimeModules(tree)
    >>> etree.tostring(tree)
    b'<items><module-item name="g.region"><module>g.region</module><description>Manages the boundary definitions for the geographic region.</description><keywords>general,settings,computational region,extent,resolution,level1</keywords></module-item></items>'
    >>> tree = etree.fromstring('<items>'
    ...                         '<module-item name="m.proj"></module-item>'
    ...                         '</items>')
    >>> _expandRuntimeModules(tree)
    >>> etree.tostring(tree)
    b'<items><module-item name="m.proj"><module>m.proj</module><description>Converts coordinates from one projection to another (cs2cs frontend).</description><keywords>miscellaneous,projection,transformation</keywords></module-item></items>'
    """
    hasErrors = False
    modules = node.findall('.//module-item')
    for module in modules:
        name = module.get('name')
        if module.find('module') is None:
            n = etree.SubElement(module, 'module')
            n.text = name

        if module.find('description') is None:
            if loadMetadata:
                desc, keywords = _loadMetadata(name)
            else:
                desc, keywords = '', ''
            n = etree.SubElement(module, 'description')
            n.text = _escapeXML(desc)
            n = etree.SubElement(module, 'keywords')
            n.text = _escapeXML(','.join(keywords))
            if loadMetadata and not desc:
                hasErrors = True

    if hasErrors:
        # not translatable until toolboxes compilation on Mac is fixed
        # translating causes importing globalvar, where sys.exit is called
        sys.stderr.write(
            "WARNING: Some addons failed when loading. "
            "Please consider to update your addons by running 'g.extension.all -f'.\n")


def _escapeXML(text):
    """Helper function for correct escaping characters for XML.

    Duplicate function in core/toolboxes and probably also in man compilation
    and some existing Python package.

    >>> _escapeXML('<>&')
    '&amp;lt;&gt;&amp;'
    """
    return text.replace('<', '&lt;').replace("&", '&amp;').replace(">", '&gt;')


def _loadMetadata(module):
    """Load metadata to modules.

    :param module: module name
    :return: (description, keywords as a list)
    """
    try:
        task = gtask.parse_interface(module)
    except ScriptError as e:
        sys.stderr.write("%s: %s\n" % (module, e))
        return '', ''

    return task.get_description(full=True), \
        task.get_keywords()


def _addHandlers(node):
    """Add missing handlers to modules"""
    for n in node.findall('.//module-item'):
        if n.find('handler') is None:
            handlerNode = etree.SubElement(n, 'handler')
            handlerNode.text = 'OnMenuCmd'

    # e.g. g.region -p
    for n in node.findall('.//wxgui-item'):
        if n.find('command') is not None:
            handlerNode = etree.SubElement(n, 'handler')
            handlerNode.text = 'RunMenuCmd'


def _convertTag(node, old, new):
    """Converts tag name.

    >>> tree = etree.fromstring('<toolboxes><toolbox><items><module-item/></items></toolbox></toolboxes>')
    >>> _convertTag(tree, 'toolbox', 'menu')
    >>> _convertTag(tree, 'module-item', 'menuitem')
    >>> etree.tostring(tree)
    b'<toolboxes><menu><items><menuitem /></items></menu></toolboxes>'
    """
    for n in node.findall('.//%s' % old):
        n.tag = new


def _convertTagAndRemoveAttrib(node, old, new):
    """Converts tag name and removes attributes.

    >>> tree = etree.fromstring('<toolboxes><toolbox name="Raster"><items><module-item name="g.region"/></items></toolbox></toolboxes>')
    >>> _convertTagAndRemoveAttrib(tree, 'toolbox', 'menu')
    >>> _convertTagAndRemoveAttrib(tree, 'module-item', 'menuitem')
    >>> etree.tostring(tree)
    b'<toolboxes><menu><items><menuitem /></items></menu></toolboxes>'
    """
    for n in node.findall('.//%s' % old):
        n.tag = new
        n.attrib = {}


def _convertTree(root):
    """Converts tree to be the form readable by core/menutree.py.

    >>> tree = etree.fromstring('<toolbox name="MainMenu"><label>Main menu</label><items><toolbox><label>Raster</label><items><module-item name="g.region"><module>g.region</module></module-item></items></toolbox></items></toolbox>')
    >>> _convertTree(tree)
    >>> etree.tostring(tree)
    b'<menudata><menubar><menu><label>Raster</label><items><menuitem><command>g.region</command></menuitem></items></menu></menubar></menudata>'
    """
    root.attrib = {}
    label = root.find('label')
    # must check because of inconsistent XML menudata file
    if label is not None:
        root.remove(label)
    _convertTag(root, 'description', 'help')
    _convertTag(root, 'wx-id', 'id')
    _convertTag(root, 'module', 'command')
    _convertTag(root, 'related-module', 'command')
    _convertTagAndRemoveAttrib(root, 'wxgui-item', 'menuitem')
    _convertTagAndRemoveAttrib(root, 'module-item', 'menuitem')

    root.tag = 'menudata'
    i1 = root.find('./items')
    # must check because of inconsistent XML menudata file
    if i1 is not None:
        i1.tag = 'menubar'
    _convertTagAndRemoveAttrib(root, 'toolbox', 'menu')


def _getXMLString(root):
    """Converts XML tree to string

    Since it is usually requier, this function adds a comment (about
    autogenerated file) to XML file.

    :return: XML as string
    """
    xml = etree.tostring(root, encoding='UTF-8')
    return xml.replace(b"<?xml version='1.0' encoding='UTF-8'?>\n",
                       b"<?xml version='1.0' encoding='UTF-8'?>\n"
                       b"<!--This is an auto-generated file-->\n")


def do_doctest_gettext_workaround():
    """Setups environment for doing a doctest with gettext usage.

    When using gettext with dynamically defined underscore function
    (`_("For translation")`), doctest does not work properly.

    One option is to use `import as` instead of dynamically defined
    underscore function but this requires change all modules which are
    used by tested module.

    The second option is to define dummy underscore function and one
    other function which creates the right environment to satisfy all.
    This is done by this function. Moreover, `sys.displayhook` and also
    `sys.__displayhook__` needs to be redefined too (the later one probably
    should not be newer redefined but some cases just requires that).

    GRASS specific note is that wxGUI switched to use imported
    underscore function for translation. However, GRASS Python libraries
    still uses the dynamically defined underscore function, so this
    workaround function is still needed when you import something from
    GRASS Python libraries.
    """
    def new_displayhook(string):
        """A replacement for default `sys.displayhook`"""
        if string is not None:
            sys.stdout.write("%r\n" % (string,))

    def new_translator(string):
        """A fake gettext underscore function."""
        return string

    sys.displayhook = new_displayhook
    sys.__displayhook__ = new_displayhook

    import __builtin__
    __builtin__._ = new_translator


def doc_test():
    """Tests the module using doctest

    :return: a number of failed tests
    """
    import doctest
    do_doctest_gettext_workaround()
    return doctest.testmod().failed


def module_test():
    """Tests the module using test files included in the current
    directory and in files from distribution.
    """
    toolboxesFile = os.path.join(WXGUIDIR, 'xml', 'toolboxes.xml')
    userToolboxesFile = 'data/test_toolboxes_user_toolboxes.xml'
    menuFile = 'data/test_toolboxes_menu.xml'
    wxguiItemsFile = os.path.join(WXGUIDIR, 'xml', 'wxgui_items.xml')
    moduleItemsFile = os.path.join(WXGUIDIR, 'xml', 'module_items.xml')

    toolboxes = etree.parse(toolboxesFile)
    userToolboxes = etree.parse(userToolboxesFile)
    menu = etree.parse(menuFile)

    wxguiItems = etree.parse(wxguiItemsFile)
    moduleItems = etree.parse(moduleItemsFile)

    tree = toolboxes2menudata(mainMenu=menu,
                              toolboxes=toolboxes,
                              userToolboxes=userToolboxes,
                              wxguiItems=wxguiItems,
                              moduleItems=moduleItems)
    root = tree.getroot()
    tested = _getXMLString(root)

    # for generating correct test file supposing that the implementation
    # is now correct and working
    # run the normal test and check the difference before overwriting
    # the old correct test file
    if len(sys.argv) > 2 and sys.argv[2] == "generate-correct-file":
        sys.stdout.write(_getXMLString(root))
        return 0

    menudataFile = 'data/test_toolboxes_menudata_ref.xml'
    with open(menudataFile) as correctMenudata:
        correct = str(correctMenudata.read())

    import difflib
    differ = difflib.Differ()
    result = list(differ.compare(correct.splitlines(True),
                                 tested.splitlines(True)))

    someDiff = False
    for line in result:
        if line.startswith('+') or line.startswith('-'):
            sys.stdout.write(line)
            someDiff = True
    if someDiff:
        print("Difference between files.")
        return 1
    else:
        print("OK")
        return 0


def validate_file(filename):
    try:
        etree.parse(filename)
    except ETREE_EXCEPTIONS as error:
        print("XML file <{name}> is not well formed: {error}".format(
            name=filename, error=error))
        return 1
    return 0


def main():
    """Converts the toolboxes files on standard paths to the menudata file

    File is written to the standard output.
    """
    # TODO: fix parameter handling
    if len(sys.argv) > 1:
        mainFile = os.path.join(WXGUIDIR, 'xml', 'module_tree.xml')
    else:
        mainFile = os.path.join(WXGUIDIR, 'xml', 'main_menu.xml')
    tree = createTree(distributionRootFile=mainFile, userRootFile=None,
                      userDefined=False)
    root = tree.getroot()
    sys.stdout.write(decode(_getXMLString(root), encoding='UTF-8'))

    return 0


if __name__ == '__main__':
    # TODO: fix parameter handling
    if len(sys.argv) > 1:
        if sys.argv[1] == 'doctest':
            sys.exit(doc_test())
        elif sys.argv[1] == 'test':
            sys.exit(module_test())
        elif sys.argv[1] == 'validate':
            sys.exit(validate_file(sys.argv[2]))
    sys.exit(main())
