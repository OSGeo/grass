---
description: wxGUI Toolboxes
index: wxGUI
keywords: [general, GUI]
---

# wxGUI Toolboxes

## DESCRIPTION

The **Toolboxes** is a way to customize items in *[wxGUI](wxGUI.md)*
menu. Toolboxes enable to:

- hide unused menu items in menu (e.g. Imagery, Database) or submenu
  (e.g. Wildfire modeling)
- change order of menu items and subitems
- add new menu items (e.g. Temporal)
- add addons modules
- add your own modules

Toolboxes are configured through two XML files (`main_menu.xml` and
`toolboxes.xml`) located in your user home GRASS directory, subdirectory
`toolboxes` (`$HOME/.grass8/toolboxes/` on UNIX). Currently, there is no
GUI front-end for toolboxes, however only simple editing of text files
is needed.

### Brief description of file `main_menu.xml`

This file represents the main menu (File, Settings, Raster, ...). By
modifying this file you show and hide menu items which are represented
by `subtoolbox` tag.

Tag `user-toolboxes-list` is interpreted as a menu containing a list of
all user-defined toolboxes. If not needed it can be removed.

Following lines can be copied to `.grass8/toolboxes/main_menu.xml` and
by removing, adding or reordering lines users can change the main menu
items. See further examples.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<toolbox name="MyMainMenu">
  <label>Default GRASS GIS main menu bar</label>
  <items>
    <subtoolbox name="File"/>
    <subtoolbox name="Settings"/>
    <subtoolbox name="Raster"/>
    <subtoolbox name="Vector"/>
    <subtoolbox name="Imagery"/>
    <subtoolbox name="Volumes"/>
    <subtoolbox name="Database"/>
    <user-toolboxes-list />
    <subtoolbox name="Help"/>
  </items>
</toolbox>
```

### Brief description of file `toolboxes.xml`

This file contains structure and description of individual toolboxes.
Note that both *Raster* and e.g. *Query raster maps* are individual
toolboxes although one contains the other. Tag `toolbox` contains
`subtoolbox` tags which are defined later in the file. These nested
toolboxes are linked through `name` attribute.

Apart from `subtoolbox` tag, tag `toolbox` can contain individual items
(modules) and separators (for visual separation in the menu tree).

```xml
<?xml version="1.0" encoding="UTF-8"?>
<toolboxes>
  <toolbox name="Raster">
    <label>&amp;Raster</label>
    <items>
      <subtoolbox name="DevelopRasterMap"/>
      <subtoolbox name="ManageRasterColors"/>
      <subtoolbox name="QueryRasterMaps"/>
      <subtoolbox name="RasterMapTypeConversions"/>
      <separator/>
      <module-item name="r.buffer">
        <label>Buffer rasters</label>
      </module-item>
      ...
      ...
  <toolbox name="QueryRasterMaps">
    <label>Query raster maps</label>
    <items>
      <module-item name="r.what">
        <label>Query values by coordinates</label>
      </module-item>
      <module-item name="r.what.color">
        <label>Query colors by value</label>
      </module-item>
    </items>
  </toolbox>
```

To redefine a toolbox (or use it as a template), copy specific part of
file `grass7/gui/wxpython/xml/toolboxes.xml` from GRASS installation to
a new file in user home (`.grass8/toolboxes/toolboxes.xml`) and edit it.
Rename this new toolbox.

## EXAMPLES

### Hiding menu items

If we are for example working only with raster data, we can hide menu
items *Vector* and *Database*. The file `main_menu.xml` then contains
the following lines where we omitted the two toolboxes:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<toolbox name="CustomizedMainMenu">
  <label>Default GRASS GIS main menu bar</label>
  <items>
    <subtoolbox name="File"/>
    <subtoolbox name="Settings"/>
    <subtoolbox name="Raster"/>
    <subtoolbox name="Imagery"/>
    <subtoolbox name="Volumes"/>
    <user-toolboxes-list />
    <subtoolbox name="Help"/>
  </items>
</toolbox>
```

### Creating custom toolbox

In this example we create a new toolbox *Favorites* containing existing
GRASS module and toolbox, custom module created by the user and addon
module. The `toolboxes.xml` file contains following lines:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<toolboxes>
  <toolbox name="MyFavorites">
    <label>&amp;Favorites</label>
    <items>
      <module-item name="g.region">
        <label>Set region</label>
      </module-item>
      <module-item name="r.mask">
        <label>Mask</label>
      </module-item>
      <separator/>
      <module-item name="m.myown">
        <label>Do my own stuff</label>
      </module-item>
      <module-item name="i.histo.match">
        <label>Calculate histogram matching</label>
      </module-item>
      <subtoolbox name="RasterReportsAndStatistics"/>
    </items>
  </toolbox>
</toolboxes>
```

Optionally, we can add this toolbox to the main menu items. The
`main_menu.xml` file contains following lines:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<toolbox name="CustomizedMainMenu">
  <label>Default GRASS GIS main menu bar</label>
  <items>
    <subtoolbox name="File"/>
    <subtoolbox name="Settings"/>
    <subtoolbox name="Raster"/>
    <subtoolbox name="Vector"/>
    <subtoolbox name="Imagery"/>
    <subtoolbox name="Volumes"/>
    <subtoolbox name="Database"/>
    <user-toolboxes-list />
    <subtoolbox name="Favorites"/>
    <subtoolbox name="Help"/>
  </items>
</toolbox>
```

If we have `user-toolboxes-list` tag in the `main_menu.xml` file, our
custom toolbox will be listed in the automatically added *Toolboxes*
main menu item. The screenshot shows the resulting menu:

![Toolboxes - menu customization](wxGUI_toolboxes.jpg)

## NOTES

After the first start of wxGUI with custom toolboxes, `.grass/toolboxes`
directory will contain file `menudata.xml` which is auto-generated and
should not be edited.

## SEE ALSO

*[wxGUI](wxGUI.md), [wxGUI components](wxGUI.components.md)*

## AUTHORS

Anna Petrasova, OSGeoREL, Faculty of Civil Engineering, Czech Technical
University in Prague  
Vaclav Petras, OSGeoREL, Faculty of Civil Engineering, Czech Technical
University in Prague
