WxGUI Toolboxes
===============

Introduction
------------

wxGUI toolboxes enables GUI customization and organization of menu (and other
views) according to user needs.

XML files are used for code generation and in case of toolboxes enables
wxGUI (menu) customization.

Currently, all the files described here are related to the menus which are
generated from these files. The most of the files described here are
the part of implementation of toolboxes in wxGUI.

Each XML file has a DTD file which can by used for validation. XSD files are
not provided (but this can change in the future). Some file types can occur
multiple times, some occur only once.

Note that in XML context, the term *tag* partially overlaps with the term
*element*. Element emphasizes the XML structure, XML object model and
content of these objects. Tag emphasizes the markup and the name of the element.


Files overview
--------------

Files related to toolboxes
^^^^^^^^^^^^^^^^^^^^^^^^^^

GRASS source code contains these XML files:

* ``gui/wxpython/xml/main_menu.xml``
* ``gui/wxpython/xml/toolboxes.xml``
* ``gui/wxpython/xml/wxgui_items.xml``


GRASS distribution contains these XML files:

* ``etc/gui/wxpython/xml/main_menu.xml``
* ``etc/gui/wxpython/xml/toolboxes.xml``
* ``etc/gui/wxpython/xml/wxgui_items.xml``
* ``etc/gui/wxpython/xml/module_items.xml`` (generated during compilation)
* ``etc/gui/wxpython/xml/menudata.xml`` (generated during compilation)


GRASS directory (``.grass7``) in user home directory
(i.e., ``$HOME`` on Unix) contains these XML files:

* ``toolboxes/main_menu.xml`` (created by user)
* ``toolboxes/toolboxes.xml`` (created by user)
* ``toolboxes/menudata.xml`` (generated on wxGUI startup)


.. graphviz::

    digraph toolboxes {
        graph [rankdir="LR"];
        node [shape="record", style="rounded"];

        menudata [label="menudata.xml in distribution", shape="note", URL="#menudata-file"];
        umenudata [label="menudata.xml in user home", shape="note", URL="#menudata-file"];
        toolboxes [label="toolboxes.xml in distribution", shape="note", URL="#toolboxes-file"];
        utoolboxes [label="toolboxes.xml in user home", shape="note", URL="#toolboxes-file"];
        main_menu [label="main_menu.xml in distribution", shape="note", URL="#main-menu-file"];
        umain_menu [label="main_menu.xml in user home", shape="note", URL="#main-menu-file"];

        wxgui_items [label="wxgui_items.xml in distribution", shape="note", URL="#wxgui-items-file"];
        module_items [label="module_items.xml in distribution", shape="note", URL="#module-items-file"];

        menustrings [label="menustrings.py\n(used for translations)", shape="note"];

        module_items -> menudata;
        wxgui_items -> menudata;
        main_menu -> menudata;
        toolboxes -> menudata;

        module_items -> umenudata;
        wxgui_items -> umenudata;
        toolboxes -> umenudata;
        main_menu -> umenudata;
        umain_menu -> umenudata;
        utoolboxes -> umenudata;

        menudata -> menustrings;

        // guimenu [label="Main menu in wxGUI", shape="box3d"];
        // menustrings -> guimenu;
        // menudata -> guimenu;
        // umenudata -> guimenu;
    }


Other files
^^^^^^^^^^^

GRASS source code contains these XML files:

* ``gui/wxpython/xml/menudata_gmodeler.xml``
* ``gui/wxpython/xml/menudata_psmap.xml``

In GRASS distribution these XML files are in the ``etc/gui/wxpython/xml``
directory.


Generation of files and menu
----------------------------

As noted in the section \ref toolboxesFilesOverview, there are files in the
GRASS distribution and in the user home directory (particularly in
``.grass7/tooboxes`` subdirectory).

When user doesn't have any ``toolboxes.xml`` or ``main_menu.xml`` files in the
home directory, file ``menudata.xml`` included in the distribution is used to
build a menu.

When ``toolboxes.xml`` or ``main_menu.xml`` file (in user home directory) is newer
than ``menudata.xml`` in user home directory or ``menudata.xml`` does not exists
in user home directory, the ``menudata.xml`` is generated when GUI starts.

When ``menudata.xml`` in user home directory is fresh enough,
it is used to create a menu.

When ``toolboxes.xml`` or ``main_menu.xml`` file is not in user home directory
but ``menudata.xml`` is, the file is re-generated (each time the GUI starts).
So, if you just have your own ``main_menu.xml``, it is better to create also
a ``toolboxes.xml`` file with no toolboxes (note that you still need to create
a valid XML toolbox file).
Similarly, if you have only the ``toolboxes.xml`` file it
is better to copy the ``main_menu.xml`` file from distribution into your home
directory.

When reading the main_menu file, user toolboxes are expanded first and then
toolboxes from distribution are expanded.


Toolboxes file
--------------

This file contains definition of toolboxes. A toolbox contains references
(links) to other items, namely ``<module-items>``, ``<wxgui-items>``
and other toolboxes using tag ``<subtoolbox>``. Tag ``<separator>`` is
used when the view supports some kind of visual separators to group parts
of the toolbox (or menu).

Items are referenced using ``name`` attribute. In case of ``<module-items>``,
``<wxgui-items>`` also subelements can be added to create new items or to
replace subelements values from item definition.

.. graphviz::

    graph toolboxes {
        graph [rankdir="LR"];
        node [shape="record", style="rounded"];

        // ∞ causes Doxygen warning but it's harmless for dot and html output

        toolboxes -- toolbox [label="1..∞"];

        toolbox -- label;
        toolbox -- items [label="1..∞"];

        items -- "module-item" [label="0..1"];
        items -- "wxgui-item" [label="0..1"];
        items -- subtoolbox [label="0..1"];
        items -- separator [label="0..1"];

        milabel [label="label"];

        "module-item" -- milabel;
        "module-item" -- module [label="0..1"];
        "module-item" -- description [label="0..1"];
        "module-item" -- keywords [label="0..1"];

        wilabel [label="label"];
        widescription [label="description"];
        wikeywords [label="keywords"];

        "wxgui-item" -- wilabel [label="0..1"];
        "wxgui-item" -- handler [label="0..1"];
        "wxgui-item" -- "related-module" [label="0..1"];
        "wxgui-item" -- command [label="0..1"];
        "wxgui-item" -- widescription [label="0..1"];
        "wxgui-item" -- wikeywords [label="0..1"];
        "wxgui-item" -- shortcut [label="0..1"];
        "wxgui-item" -- "wx-id" [label="0..1"];
    }



Main menu file
--------------

File has a layout similar to the \ref toolboxesFile but contains only one
toolbox (``<toolbox>``) which can contain only subtoolboxes
(``<subtoolbox>`` elements) and one special
element ``<user-toolboxes-list>`` which will be replaced by a menu with the list
of toolboxes in the user toolbox file.


Modules items file
------------------

The file contains information obtained from modules' interface descriptions.
The structure of one ``module-item`` is the same as in the \ref toolboxesFile
but some subelements are mandatory.

File contained in distribution is generated during compilation from available
modules using the script ``gui/wxpython/tools/build_modules_xml.py``.

Element ``<module>`` is the name of the executable, e.g. ``r.info``.

Element ``<label>`` is currently not present. It represents the short label in
menu and it is added in toolboxes.

Element ``<description>`` is created from module's description (or if
it exists, label concatenated with description).

Element ``<keywords>`` is created from module's keywords.


wxGUI items file
----------------

The file contains definitions of wxGUI actions which can be accessed for
example, from menu.
The structure of one ``wxgui-item`` is the same as in \ref toolboxesFile
but some subelements are mandatory.


Menudata file
-------------

Historically, menudata.xml file was in the source codes and was partially
maintained by the script ``gui/wxpython/tools/update_menudata.py``
which updated the description and keywords (based on module's
label or description, and keywords).
Other items (menu structure, menu item labels and non-module only items) were
edited in the menudata.xml file directly.

Currently, the file is generated during compilation or at startup. It serves
as an intermediate layer between all toolboxes XMLs and GUI menu tree
generation.


How to write a custom toolbox
-----------------------------

To create a new toolbox use ``<toolbox>`` tag:

.. code-block:: xml

    <toolbox name="MyRaster">
      <label>My &amp;raster</label>
      <items>
        <!-- ... -->
      </items>
    </toolbox>

To create a new item which represents a module use ``<module-item>`` tag:

.. code-block:: xml

    <module-item name="r.buffer">
      <label>Buffer rasters</label>
    </module-item>

This works for modules contained in distribution. For modules from addons or
some your modules which are on path use ``<module-item>`` tag together with
``<module>`` tag:

.. code-block:: xml

    <module-item name="r.agent">
      <label>Buffer rasters</label>
      <module>r.agent</module>
    </module-item>

The name of a module is duplicated in the XML but anyway, try to keep ``name``
attribute and ``module`` element in sync.

To create a new item which triggers some wxGUI action defined in distribution
use ``<wxgui-item>`` tag:

.. code-block:: xml

    <wxgui-item name="RasterMapCalculator"/>

Note that now it is not possible to create custom wxGUI items.

To include an existing toolbox use ``<subtoolbox>`` tag:

.. code-block:: xml

    <subtoolbox name="NeighborhoodAnalysis"/>

To create a submenu in your new menu (toolbox), you need to create a new toolbox
and include this toolbox.

To create your custom main menu create a file main_menu.xml in your user home
directory, in ``.grass7/toolboxes`` subdirectory. Directory ``.grass7`` may be
hidden directory on your system. The XML file should contain the definition of
only one toolbox. The name attribute and label element are not used but should
be filled for documentation purposes and future compatibility.

If you want to base your toolbox or main menu on existing toolbox or main menu
copy the part of existing XML file from GRASS GIS distribution (installation)
directory or GRASS GIS source code. If you want to include some existing
toolboxes or wxGUI items defined in GRASS GIS you need to look to these files
too and find the proper ``name`` attributes.


Example
^^^^^^^

Files should be placed in user home directory in ``.grass7/toolboxes``
subdirectory, e.g. ``/home/john/.grass7/toolboxes``.

toolboxes.xml
"""""""""""""

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE toolboxes SYSTEM "toolboxes.dtd">
    <toolboxes>
      <toolbox name="MyRaster">
        <label>My &amp;raster</label>
        <items>
          <module-item name="r.buffer">
            <label>Buffer rasters</label>
          </module-item>
          <module-item name="r.mask">
            <label>Mask</label>
          </module-item>
          <separator/>
          <wxgui-item name="RasterMapCalculator"/>
          <subtoolbox name="NeighborhoodAnalysis"/>
          <subtoolbox name="ReportAndStatistics"/>
        </items>
      </toolbox>
    </toolboxes>


main_menu.xml
"""""""""""""

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE toolbox SYSTEM "main_menu.dtd">
    <toolbox name="MyCustomMainMenu">
      <label>MyCustomMainMenu</label>
      <items>
        <subtoolbox name="File"/>
        <subtoolbox name="Settings"/>
        <subtoolbox name="MyRaster"/>
        <subtoolbox name="Imagery"/>
        <subtoolbox name="Help"/>
      </items>
    </toolbox>


Validation
^^^^^^^^^^

You should validate your XML before running wxGUI, e.g. using ``xmllint``
(no output means that document is valid)::

    xmllint --noout --dtdvalid toolboxes.dtd toolboxes.xml

You can find ``toolboxes.dtd`` and ``main_menu.dtd`` in your GRASS GIS directory,
in ``etc/gui/wxpython/xml`` subdirectory. Depending on the validator,
you might need to add DTD file to the drectory with the XML file.

If you will provide an invalid, not well formed or empty file loading of
toolboxes will obviously fail.


Labels
^^^^^^

The label shortly describes the toolbox or the action which will happen after
running an item. The label can be a command such as *"Create table"*
or the general name such as *"Table management"*.
You should add label to each toolbox you create and to each item you create.
However, if you are just using (and thus referencing) existing items
(or toolboxes), you don't need to include labels (so you can use just empty
tags only with the name attribute).

Important items in menu usually have a automatically assigned shortcut which
depends on their label. This shortcut is assigned to ``Alt+Letter``
(on most platforms) where letter is a letter after an ampersand (``&``) in the
item label and  in the user interface the letter is underlined.
Note that in XML you cannot write ``&`` but you have to write ``&amp;``.
This concept is not related to the standard shortcut assigned to the item
according to the shortcut in the XML file.

Don't be confused with the label which is set for the module in the source code.
These labels play different role, they must be short and usually
with the wording of a command.
