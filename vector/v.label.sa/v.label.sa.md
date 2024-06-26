## DESCRIPTION

*v.label.sa* makes a label-file from a GRASS vector map with labels
created from attributes in the attached table. The labels are placed in
as optimal place as possible. The label file has the same syntax as the
one created by [v.label](v.label.html)

## EXAMPLE

North Carolina example:

::: code
    # get font names:
    d.font -L

    v.label.sa roadsmajor labels=roads_labels column=ROAD_NAME color=red \
               background=white size=250 font=Vera

    # set region:
    g.region raster=lsat7_2002_10 -p

    # display:
    d.rgb b=lsat7_2002_10 g=lsat7_2002_20 r=lsat7_2002_30
    d.vect roadsmajor col=yellow
    d.labels roads_labels
:::

![Road labeling with v.label.sa](v_label_sa.jpg)\
*Road labeling with v.label.sa (Raleigh, North Carolina, USA, area)*

## REFERENCES

Edmondson, Christensen, Marks and Shieber: A General Cartographic
Labeling Algorithm, Cartographica, Vol. 33, No. 4, Winter 1996, pp.
13-23 The algorithm works by the principle of Simulated Annealing.

## SEE ALSO

*[d.label](v.labels.html)\
[d.labels](d.labels.html)\
[ps.map](ps.map.html) [Wikipedia article on simulated
annealing](http://en.wikipedia.org/wiki/Simulated_Annealing)*\

## AUTHOR

Wolf Bergenheim
