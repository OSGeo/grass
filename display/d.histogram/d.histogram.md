## DESCRIPTION

*d.histogram* displays the category-value distribution for a
user-specified raster map layer, in the form of a bar chart or a pie
chart. The display will be displayed in the active display frame on the
graphics monitor, using the colors in the raster map layer's color
table. The program determines the raster map's category value
distribution by counting cells.

## NOTES

*d.histogram* respects the current geographic region settings and the
current raster mask (if mask is active).

*d.histogram* uses the colors in the map's color look-up table (i.e.,
the map's *colr* or *colr2* file).

## EXAMPLES

Running the command below will generate the bar graph shown in the
figure:

```shell
g.region raster=elevation -p
d.mon wx0
d.histogram map=elevation
```

<div align="center" style="margin: 10px">

<img src="d_histogram_bar.png" data-border="0"
alt="d.histogram bar graph example" />  
*Figure: Bar graph histogram for elevation map*

</div>

Running the command below will generate the pie graph shown in the
figure:

```shell
g.region raster=landuse96_28m -p
d.histogram map=landuse96_28m style=pie
```

<div align="center" style="margin: 10px">

<img src="d_histogram_pie.png" data-border="0"
alt="d.histogram pie graph example" />  
*Figure: Pie graph histogram for landuse map*

</div>

## SEE ALSO

*[d.colortable](d.colortable.md), [d.frame](d.frame.md),
[d.graph](d.graph.md), [d.linegraph](d.linegraph.md), [d.mon](d.mon.md),
[d.polar](d.polar.md), [g.region](g.region.md), [r.stats](r.stats.md)*

## AUTHOR

Dave Johnson  
DBA Systems, Inc.  
10560 Arrowhead Drive  
Fairfax, Virginia 22030
