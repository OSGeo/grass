<h2>DESCRIPTION</h2>

<em>r.mask</em> facilitates the creation of a raster "MASK" map to
control raster operations.

<p>
The MASK is applied when <em>reading</em> an existing GRASS raster map,
for example when used as an input map in a module. The MASK will block out
certain areas of a raster map from analysis and/or display, by "hiding" them
from sight of other GRASS modules. Data falling within the boundaries of the
MASK can be modified and operated upon by other GRASS raster modules; data
falling outside the MASK is treated as if it were NULL.
<p>
By default, <em>r.mask</em> converts any non-NULL value in the input map,
including zero, to 1. All these areas will be part of the MASK (see the notes
for more details). To only convert specific values (or range of values) to 1
and the rest to NULL, use the <em>maskcats</em> parameter.
<p>
Because the MASK created with <em>r.mask</em> is actually only a reclass map
named "MASK", it can be copied, renamed, removed, and used in analyses, just
like other GRASS raster map layers.
<p>
The user should be aware that a MASK remains in place until a user renames it
to something other than "MASK", or removes it. To remove a mask and restore
raster operations to normal (i.e., all cells of the current region), remove the
MASK by setting the <b>-r</b> remove MASK flag (<code>r.mask -r</code>).
Alternatively, a mask can be removed using <em>g.remove</em> or by renaming it
to any other name with <em>g.rename</em>.

<h2>NOTES</h2>

The above method for specifying a "mask" may seem counterintuitive. Areas
inside the MASK are not hidden; areas outside the MASK will be ignored until
the MASK file is removed.
<p>
<em>r.mask</em> uses <em>r.reclass</em> to create a reclassification of an
existing raster map and name it <code>MASK</code>. A reclass map takes up less
space, but is affected by any changes to the underlying map from which it was
created. The user can select category values from the input raster to use in the
MASK with the <em>maskcats</em> parameter; if <em>r.mask</em> is run from the
command line, the category values listed in <em>maskcats</em> must be quoted
(see example below). Note that the <em>maskcats</em> can only be used if the
input map is an integer map.

<h3>Different ways to create a MASK</h3>

The <em>r.mask</em> function creates a MASK with values 1 and NULL. But note
that a MASK can also be created using other functions that have a raster as
output, by naming the output raster 'MASK'. Such layers could have other
values than 1 and NULL. The user should therefore be aware that grid cells
in the MASK map containing <code>NULL</code> or <code>0</code> will replace data with
NULL, while cells containing other values will allow data to pass through
unaltered. This means that:
<p>
If a binary map with [0,1] values is used as input in <em>r.mask</em>, all
raster cells with 0 and 1 will be part of the MASK. This is because
<em>r.mask</em> converts all non-NULL cells to 1.

<div class="code"><pre>
r.mapcalc -s "map1 = round(rand(0,1))"
r.mask raster=map1
</pre></div>

On the other hand, if a binary map is used as an input in <em>g.copy</em> to create a MASK,
only the raster cells with value 1 will be part of the MASK.

<div class="code"><pre>
r.mapcalc -s "map2 = round(rand(0,1))"
g.copy raster=map2,MASK
</pre></div>

<h3>Handling of floating-point maps</h3>

<em>r.mask</em> treats floating-point maps the same as integer maps (except that
floating maps are not allowed in combination with the <em>maskcats</em>
parameter); all non-NULL values of the input raster map are converted to 1 and
are thus part of the MASK. In the example below, all raster cells are part of
the MASK, i.e., nothing is blocked out from analysis and/or display.

<div class="code"><pre>
r.mapcalc -s "map3 = rand(0.0,1.0)"
r.mask raster=map3
</pre></div>

However, when using another method than <em>r.mask</em> to create a mask,
the user should be aware that the MASK is read as an integer map. If MASK is
a floating-point map, the values will be converted to integers using
the map's quantisation rules (this defaults to round-to-nearest, but can be
changed with r.quant).

<div class="code"><pre>
r.mapcalc -s "map4 = rand(0.0,1.0)"
g.copy raster=map4,MASK
</pre></div>

In the example above, raster cells with a rounded value of 1 are part of
the MASK, while raster cells with a rounded value of 0 are converted to NULL
and consequently blocked out from analysis and/or display.

<h2>EXAMPLES</h2>

The examples are based on the North Carolina sample dataset.
<p>
Create a raster mask, for constraining the calculation of
univariate statistics of the elevation values for "lakes":
<div class="code"><pre>
# set computation region to lakes raster map
g.region raster=lakes -p
# use lakes as MASK
r.mask raster=lakes
# get statistics for elevation pixels of lakes:
r.univar elevation
</pre></div>

Remove the raster mask ("MASK" map) with the -r flag:
<div class="code"><pre>
r.mask -r
</pre></div>

Creating a mask from selected categories in the North Carolina
'geology_30m' raster map:
<div class="code"><pre>
g.region raster=geology_30m -p
r.category geology_30m
d.mon wx0
d.rast geology_30m
r.mask raster=geology_30m maskcats="217 thru 720"
d.mon wx0
d.rast geology_30m
</pre></div>

<h2>SEE ALSO</h2>

<em>
<a href="g.region.html">g.region</a>,
<a href="r.mapcalc.html">r.mapcalc</a>,
<a href="r.reclass.html">r.reclass</a>,
<a href="g.remove.html">g.remove</a>,
<a href="g.rename.html">g.rename</a>,
<a href="r.quant.html">r.quant</a>
</em>

<h2>AUTHOR</h2>

Michael Barton, Arizona State University
