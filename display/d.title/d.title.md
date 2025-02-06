<h2>DESCRIPTION</h2>

<em>d.title</em> generates to standard output a string which can be used by
<em><a href="d.text.html">d.text</a></em> to draw a TITLE for the raster map
layer <em>name</em> in the active display frame on the graphics monitor.
Output created by <em>d.title</em> can be redirected into a file, or piped
directly into <em><a href="d.text.html">d.text</a></em> to display the map
TITLE created by <em>d.title</em>.  The map TITLE created will include the
map layer's name, TITLE, MAPSET, LOCATION_NAME, geographic region boundary
coordinates, and cell resolution.
If the <b>-d</b> draw flag is used, then <em>d.title</em> will call
<em>d.text</em> for you and the title will be automatically rendered
to the display.

<h2>NOTES</h2>

The text created with
<em><a href="d.text.html">d.text</a></em>
will not necessarily fit within the
active display frame on the graphics monitor;
the user should choose a text size appropriate to this frame.

<h2>EXAMPLES</h2>

For example, a user wishing to create a suitable TITLE for the
Spearfish, SD <em>soils</em> map layer and to display this TITLE in the
active display frame on the graphics monitor might type the following:

<div class="code"><pre>
d.title map=soils color=red size=5 &gt; TITLE.file
d.text &lt; TITLE.file
</pre></div>

Alternately, the user might pipe <em>d.title</em> output directly
into <em><a href="d.text.html">d.text</a>:</em>

<div class="code"><pre>
d.title map=soils color=red size=5 | d.text
</pre></div>

<p>A file created by <em>d.title</em> can be displayed
with <em><a href="d.text.html">d.text</a></em>.
Information contained in this file takes precedence over the
<em>color</em> and <em>size</em> parameters for
<em><a href="d.text.html">d.text</a></em>.

<h2>SEE ALSO</h2>

<em>
<a href="d.font.html">d.font</a>,
<a href="d.text.html">d.text</a>
</em>

<h2>AUTHOR</h2>

James Westervelt,
U.S. Army Construction Engineering
Research Laboratory
