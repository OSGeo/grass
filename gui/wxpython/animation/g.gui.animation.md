---
description: wxGUI Animation tool
index: topic_GUI|GUI
---

# wxGUI Animation tool

## DESCRIPTION

The **Animation Tool** is a *[wxGUI](wxGUI.md)* component for animating
series of GRASS raster or vector maps or space time datasets (created by
t.\* modules).

Animation Tool allows you to:

- display up to 4 synchronized animations
- each animation can consist of base map layer(s) and (multiple) series
  in arbitrary order (for example, raising water level with elevation)
- control the animation speed
- interactively change active frame using a slider
- visualize space time datasets with unequally spaced intervals
- animate 3d view (partially implemented, not supported on Windows)
- export animation as a series of images, animated GIF, AVI or SWF
- choose format of time labels in case of animating maps with absolute
  time
- choose background color
- set starting and ending region in order to change region during
  animation (alternatively you can set N-S/E-W values instead of the
  ending region; these are used for making the region smaller or larger
  for each step)

3D view animation enables to animate raster (as an elevation map or a
color map) or vector map (points, lines). Internally, module
*[m.nviz.image](m.nviz.image.md)* is used. To display 3D view animation
follow these steps:

- open GRASS GUI, load maps and start 3D view
- set view, light and other parameters as you like
- save workspace file
- add new animation in Animation Tool, choose 3D view mode
- choose data (series of maps or space time dataset) used for animation
- set workspace file
- choose parameter (parameter of *[m.nviz.image](m.nviz.image.md)*) to
  animate (e.g. color_map)

![Animation Tool screenshot](wxGUI_animation_tool.jpg)
  
## NOTE

The Animation Tool follows the computational region settings, so please
be sure your computational region is set to the geographic extent of
maps you are animating. You can change the computational region (using
*[g.region](g.region.md)*) and then reload the maps to update the
animation.

## EXAMPLES

```sh
g.gui.animation raster=rmap1,rmap2,rmap3

g.gui.animation vector=vmap1,vmap2,vmap3

g.gui.animation strds=precipitation_2000_2010
```

The loading of a series of maps into the Animation Tool can be
simplified with *[g.list](g.list.md)* (back ticks syntax works for Linux
and Mac only):

```sh
g.gui.animation raster=`g.list type=raster mapset=. separator=comma pattern="precip*"`
```

Using extended regular expressions, the list of a series of raster maps
can be subset by e.g., numeric range (here: precipitation for the years
1997-2012):

```sh
g.gui.animation raster=`g.list -e type=raster mapset=. separator=comma pattern="precip_total.(199[7-9]|200[0-9]|201[0-2]).sum"`
```

## SEE ALSO

*[wxGUI](wxGUI.md), [wxGUI components](wxGUI.components.md)*

*[g.gui.timeline](g.gui.timeline.md), [g.list](g.list.md),
[m.nviz.image](m.nviz.image.md)*

See also related [wiki
page](https://grasswiki.osgeo.org/wiki/WxGUI_Animation_Tool).

## AUTHOR

Anna Kratochvilova, [Czech Technical University in
Prague](https://www.cvut.cz), Czech Republic
