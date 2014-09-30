How to write a custom toolbox
=============================

To create a new toolbox use
``"<toolbox>"``
tag:

<toolbox name="MyRaster">

<label>My &amp;raster</label>

<items>

...

</items>

</toolbox>

To create a new item which represents a module use
``"<module-item>"``
tag:

<module-item name="r.buffer">

<label>Buffer rasters</label>

</module-item>

This works for modules contained in distribution. For modules from addons or some your modules
which are on path use
``"<module-item>"``
tag together with
``"<module>"``
tag:

<module-item name="r.agent">

<label>Buffer rasters</label>

<module>r.agent</module>

</module-item>

The name of a module is duplicated in the XML but anyway, try to keep
``name``
attribute and
``module``
element in sync.

To create a new item which triggers some wxGUI action defined in distribution use
``"<wxgui-item>"``
tag:

<wxgui-item name="RasterMapCalculator"/>

Note that now it is not possible to create custom wxGUI items.

To include an existing toolbox use
``"<subtoolbox>"``
tag:

<subtoolbox name="NeighborhoodAnalysis"/>

To create a submenu in your new menu (toolbox), you need to create a new toolbox and include this toolbox.

To create your custom main menu create a file main_menu.xml in your user home directory, in .grass7/toolboxes subdirectory. Directory .grass7 may be hidden directory on your system. The XML file should contain the definition of only one toolbox. The name attribute and label element are not used but should be filled for documentation purposes and future compatibility.

If you want to base your toolbox or main menu on existing toolbox or main menu copy the part of existing XML file from GRASS GIS distribution (installation) directory or GRASS GIS source code. If you want to include some existing toolboxes or wxGUI items defined in GRASS GIS you need to look to these files too and find the proper
``name``
attributes.

Example
-------

Files should be placed in user home directory in .grass7/toolboxes subdirectory, e.g.
``/home/john/``
.grass7/toolboxes.

toolboxes.xml

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
----------

You should validate your XML before running wxGUI, e.g. using
``xmllint``
(no output means that document is valid):

xmllint --noout --dtdvalid toolboxes.dtd toolboxes.xml

You can find
``toolboxes.dtd``
and
``main_menu.dtd``
in your GRASS GIS directory, in
``etc/gui/wxpython/xml``
subdirectory.

If you will provide an invalid, not well formed or empty file loading of toolboxes will obviously fail.

Labels
------

The label shortly describes the toolbox or the action which will happen after running an item. The label can be a command such as
*"Create table"*
or the general name such as
*"Table management"*
. You should add label to each toolbox you create and to each item you create. However, if you are just using (and thus referencing) existing items (or toolboxes), you don't need to include labels (so you can use just empty tags only with the name attribute).

Important items in menu usually have a automatically assigned shortcut which depends on their label. This shortcut is assigned to
``Alt+Letter``
(On most platforms) where letter is a letter after an ampersand (
``&``
) in the item label and in the user interface the letter is underlined. Note that in XML you cannot write
``"&"``
but you have to write
``"&amp;"``
. This concept is not related to the standard shortcut assigned to the item according to the shortcut in the XML file.

Don't be confused with the label which is set for the module in the source code.
