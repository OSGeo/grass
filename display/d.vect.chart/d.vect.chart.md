## DESCRIPTION

*d.vect.chart* displays charts for GRASS vector data in the active frame
on the graphics monitor.

## NOTES

The charts are positioned as follows:

- vector points: on point position,
- vector lines: on line centers,
- vector areas: on area centroids.

Bar charts are placed with their lower edge starting from the
y-coordinate of the feature being symbolized, and centered with respect
to the x-coordinate. The **-c** flag can be used to center the bar chart
in both x and y directions.

The 'sizecol' parameter is proportionate to the radius.

The optional **max_ref** parameter accepts a list of values that
represent the maximum value for each column listed in the values for the
parameter **columns**. These values are used to create a framed bar plot
if **chart_type** is *bar* (See Example 2).

## EXAMPLES

### Example 1

Pie-charts of monthly winter precipitation (North Carolina sample
dataset):

```sh
g.region vector=nc_state -p
d.vect nc_state
d.vect.chart precip_30ynormals chart_type=pie columns=nov,dec,jan,feb -l

# show pie chart as 3D
d.erase
d.vect nc_state
d.vect.chart precip_30ynormals chart_type=pie columns=nov,dec,jan,feb -l -3
```

![d.vect.chart 2D pie chart](d_vect_chart_pie_2d.png)  
2D pie-chart of monthly winter precipitation in North Carolina

![d.vect.chart 3D pie chart](d_vect_chart_pie_3d.png)  
3D pie-chart of monthly winter precipitation in North Carolina

### Example 2

Bar-chart of different census map values:

```sh
d.vect.chart map=vectmap columns=cens51,cens61,cens71,cens81 chart_type=bar
```

### Example 3

Creation of framed bar charts of an erodibiliy index from the Spearfish
sample dataset:

```sh
g.region raster=erode.index -p
r.to.vect -s input=erode.index output=erode_index type=area
v.extract input=erode_index output=erode_index_ctrds type=centroid
d.rast aspect
d.vect.chart map=erode_index_ctrds chart_type=bar columns=cat \
             size=10 max_ref=12 scale=0.05 colors=yellow
d.vect erode_index_ctrds icon=basic/circle fcol=black col=black size=5
```

![d.vect.chart example](d.vect.chart_example.jpg)  
Bar charts of an erodibiliy index (spatial subset)

## SEE ALSO

*[d.erase](d.erase.md), [d.vect](d.vect.md),
[d.vect.thematic](d.vect.thematic.md), [d.what.vect](d.what.vect.md),
[d.rast](d.rast.md)*

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy
