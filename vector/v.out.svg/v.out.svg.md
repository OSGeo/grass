## DESCRIPTION

*v.out.svg* converts GRASS vector data to SVG (Scalable Vector Graphics)
code notation. In particular, it

- converts GRASS line, boundary, area, point, centroid objects to SVG
  \<path /\>, \<circle /\> elements,
- reduces coordinate precision in SVG-output to save bandwidth,
- extracts GRASS attributes to gg:name="value" attributes.

The *precision* parameter controls the number of decimals for
coordinates output (*precision=0* corresponds to integer precision in
the output SVG file).

## EXAMPLE

Export **polygons** from GRASS vector map soils (Spearfish sample data)
to SVG format:

```sh
v.out.svg input=soils output=/tmp/output.svg type=poly
```

Export **lines** from GRASS vector map t_hydro (Spearfish sample data)
to SVG format, set coordinate precision to 0:

```sh
v.out.svg input=t_hydro output=/tmp/output.svg type=line precision=0
```

Export **points** from GRASS vector map archsites (Spearfish sample
data) to SVG format, include attribute **str1** (name):

```sh
v.out.svg input=archsites output=/tmp/output.svg type=point precision=0 attrib=str1
```

## REFERENCES

- [Module v.out.svg at svg.cc](http://svg.cc/grass/index.html)
- [SVG (Scalable Vector Graphics) at
  w3c.org](http://www.w3.org/Graphics/SVG/)

## SEE ALSO

*[v.out.ogr](v.out.ogr.md)*

## AUTHOR

Klaus Foerster (klaus svg.cc), Innsbruck, Austria
