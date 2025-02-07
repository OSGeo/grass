<h2>DESCRIPTION</h2>

<b>r.support</b> allows the user to create and/or edit raster map support
information. Editing of raster map color tables, category labels, header,
history, semantic label elements and title is supported.
Category labels can also be copied from another raster map.

<h3>Raster semantic labels and band management</h3>

Raster semantic label concept is similar to dimension name in other GIS and
remote sensing applications. Most common usage will be assigning a remote
sensing platform sensor band identifier to the raster map metadata, although
any identifier is supported (see <a href="i.band.library.html">i.band.library</a>).
Raster semantic label is suggested to work with imagery classification tools.

<h2>EXAMPLES</h2>

These examples are based on the North Carolina dataset, more specifically
the <em>landuse</em> raster map.

Copy the landuse map to the current mapset

<div class="code"><pre>
g.copy raster=landuse,my_landuse
</pre></div>

<h3>Update statistics</h3>

<div class="code"><pre>
r.support -s map=my_landuse
</pre></div>

<h3>Update Title</h3>

<div class="code"><pre>
r.support map=my_landuse title="Landuse copied"
</pre></div>

<h3>Append to History Metadata</h3>

<div class="code"><pre>
r.support map=my_landuse history="Copied from PERMANENT mapset"
</pre></div>

<h3>Update Units Display</h3>

<div class="code"><pre>
r.support map=my_landuse units=meter
</pre></div>

<h3>Set semantic label</h3>

Note: landuse map doesn't confirm to CORINE specification. This is an example only.

<div class="code"><pre>
r.support map=my_landuse semantic_label=CORINE_LULC
</pre></div>

<h2>NOTES</h2>

If metadata options such as <b>title</b> or <b>history</b> are given the
module will run  non-interactively. If only the map name is given
<em>r.support</em> will run interactively within a terminal shell and the
user with be prompted for input.
<p>Freeform metadata information is stored in a "<code>hist</code>" file which may be
appended to by using the <b>history</b> option. Currently this is limited to
50 lines of text with a maximum line length of 78 characters. Any input
larger than this will be wrapped to the next line.
All other metadata strings available as standard options are limited to
79 characters.

<h2>SEE ALSO</h2>

<em>
<a href="i.band.library.html">i.band.library</a>,
<a href="r.category.html">r.category</a>,
<a href="r.describe.html">r.describe</a>,
<a href="r.info.html">r.info</a>,
<a href="r.null.html">r.null</a>,
<a href="r.region.html">r.region</a>,
<a href="r.report.html">r.report</a>,
<a href="r.semantic.label.html">r.semantic.label</a>,
<a href="r.timestamp.html">r.timestamp</a>
</em>

<h2>AUTHORS</h2>

Micharl Shapiro, CERL: Original author<br>
<a href="MAILTO:rez@touchofmadness.com">Brad Douglas</a>: GRASS 6 Port<br>
M. Hamish Bowman: command line enhancements<br>
Markus Neteler: category copy from other map<br>
Maris Nartiss: semantic label management
