<h2>DESCRIPTION</h2>

The <em>r.mask.status</em> reports information about the 2D raster mask and its
status. The tool reports whether the mask is present or not. For both active
and inactive mask, the tool reports a full name of the raster (name including
the mapset) which represents or would represent the mask.
It can also report full name of the underlying raster if the mask is
reclassified from another raster.

The tool can be used to check if the mask is currently set
(<code>present</code> boolean in JSON), what is raster name used to represent
the mask (<code>name</code> string in JSON), and whether the raster is
reclassifed from another (<code>is_reclass_of</code> string or null in JSON).
YAML and shell script style outputs are following the JSON output if possible.
The plain text format outputs multi-line human-readable information in natural
language.

<p>
With the <b>-t</b> flag, no output is printed, instead a return code is used to
indicate presence or absence. The convention is the same same the POSIX
<em>test</em> utility, so <em>r.mask.status</em> returns 0 when the mask is
present and 1 otherwise.

<h2>EXAMPLES</h2>

<h3>Generate JSON output</h3>

To generate JSON output in Bash, use the <b>format</b> option:

<div class="code"><pre>
r.mask.status format=json
</pre></div>

In Python, use:

<div class="code"><pre>
import grass.script as gs
gs.parse_command("r.mask.status", format="json")
</pre></div>

This returns a dictionary with keys <code>present</code>,
<code>full_name</code>, and <code>is_reclass_of</code>.

<h3>Use as the test utility</h3>

The POSIX <em>test</em> utility uses return code 0 to indicate presence
and 1 to indicate absence of a file, so testing existence of a file with
<code>test -f</code> gives return code 0 when the file exists.
<em>r.mask.status</em> can be used in the same with the the <b>-t</b> flag:

<div class="code"><pre>
r.mask.status -t
</pre></div>

In a Bash script:

<div class="code"><pre>
# Bash
if r.mask.status -t; then
    echo "Masking is active"
else
    echo "Masking is not active"
fi
</pre></div>

<h2>SEE ALSO</h2>

<em>
<a href="r.mask.html">r.mask</a>,
<a href="g.region.html">g.region</a>
</em>

<h2>AUTHORS</h2>

Vaclav Petras, NC State University, Center for Geospatial Analytics
