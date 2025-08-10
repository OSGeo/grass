<h2>DESCRIPTION</h2>

<p>
<em>r.object.geometry</em> calculates form statistics of raster objects
in the <b>input</b> map and writes it to the <b>output</b> text file
 (or standard output if no output filename or '-' is given),
with fields separated by the chosen <b>separator</b>.  Objects are defined
 as clumps of adjacent cells with the same category value (e.g. output of
<em><a href="r.clump.html">r.clump</a></em> or
<em><a href="i.segment.html">i.segment</a></em>).

<p>
By default, values are in pixels. If values in meters is desired, the user
can set the <b>-m</b> flag. If the current working region is in lat-long or
has non-square pixels, using meters is recommended.

<p>
Statistics currently calculated are exactly the same as in
<em><a href="v.to.db.html">v.to.db</a></em> (except for compact_square and
mean coordinates):

<ul>
<li>area</li>
<li>perimeter</li>
<li>compact_square (compactness compared to a square:
  <code>compact_square = 4 * sqrt(area) / perimeter</code>)</li>
<li>compact_circle (compactness compared to a circle:
  <code>compact_circle = perimeter / ( 2 * sqrt(PI * area) )</code>)</li>
<li>fractal dimension ( <code>fd = 2 * ( log(perimeter) / log(area + 0.001) )</code> )</li>
<li>mean x coordinate of object (in map units)</li>
<li>mean y coordinate of object (in map units)</li>
</ul>

<h2>EXAMPLE</h2>

<div class="code"><pre>
g.region raster=soilsID
r.object.geometry input=soilsID output=soils_geom.txt
</pre></div>

The <b>format=json</b> option can be used to change the output format to JSON:

<div class="code"><pre>
r.object.geometry input=zipcodes format=json
</pre></div>

<div class="code"><pre>
[
    {
        "category": 1,
        "area": 106,
        "perimeter": 62,
        "compact_circle": 1.6987670351864215,
        "compact_square": 0.66423420264432265,
        "fd": 1.7699924681225903,
        "mean_x": 631382.07547169807,
        "mean_y": 222764.15094339623
    },
    {
        "category": 2,
        "area": 57,
        "perimeter": 36,
        "compact_circle": 1.3451172460704992,
        "compact_square": 0.83887049280786108,
        "fd": 1.772672742164326,
        "mean_x": 643460.52631578944,
        "mean_y": 217232.45614035087
    },
    {
        "category": 3,
        "area": 10,
        "perimeter": 16,
        "compact_circle": 1.4272992929222168,
        "compact_square": 0.79056941504209488,
        "fd": 2.4081353865496951,
        "mean_x": 631300,
        "mean_y": 215450
    },
    {
        "category": 4,
        "area": 63,
        "perimeter": 60,
        "compact_circle": 2.1324361862292305,
        "compact_square": 0.52915026221291817,
        "fd": 1.9764401337147652,
        "mean_x": 642345.23809523811,
        "mean_y": 226599.20634920636
    },
    {
        "category": 5,
        "area": 491,
        "perimeter": 156,
        "compact_circle": 1.9859985189304281,
        "compact_square": 0.56816717451693177,
        "fd": 1.6299200778082998,
        "mean_x": 637912.93279022397,
        "mean_y": 220636.96537678209
    },
    {
        "category": 6,
        "area": 83,
        "perimeter": 60,
        "compact_circle": 1.8578355639603314,
        "compact_square": 0.60736223860961991,
        "fd": 1.8531256328449071,
        "mean_x": 635846.38554216863,
        "mean_y": 227219.8795180723
    }
]
</pre></div>

<h2>SEE ALSO</h2>

<em>
<a href="i.segment.html">i.segment</a>,
<a href="r.clump.html">r.clump</a>,
<a href="v.to.db.html">v.to.db</a>
</em>

<h2>AUTHORS</h2>

Moritz Lennert<br>
Markus Metz (diagonal clump tracing)
