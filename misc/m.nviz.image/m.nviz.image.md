## DESCRIPTION

*m.nviz.image* allows users to realistically render multiple *surfaces*
(raster data) in a 3D space, optionally using thematic coloring, draping
2D *vector* data over the surfaces, displaying 3D vector data in the
space, and visualization of *volume* data (3D raster data) from the
command line.

## EXAMPLE

Render elevation map in a 3D space.

```sh
g.region raster=elevation
m.nviz.image elevation_map=elevation output=elev perspective=15
```

## SEE ALSO

*[wxGUI 3D viewer](wxGUI.nviz.md)*

## AUTHORS

[Martin Landa](http://geo.fsv.cvut.cz/gwiki/Landa), [Google Summer of
Code 2008](https://grasswiki.osgeo.org/wiki/WxNviz_GSoC_2008) (mentor:
Michael Barton) and [Google Summer of Code
2010](https://grasswiki.osgeo.org/wiki/WxNviz_GSoC_2010) (mentor: Helena
Mitasova)  
Anna Kratochvilova, [Google Summer of Code
2011](https://grasswiki.osgeo.org/wiki/WxNviz_GSoC_2011) (mentor: Martin
Landa)
