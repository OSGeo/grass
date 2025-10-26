<h2>DESCRIPTION</h2>

<em>v.import</em> imports vector data from files and database connections
supported by the <a href="https://gdal.org/">OGR</a> library into the
current project (previously called location) and mapset. If the coordinate
reference system (CRS) of the input
does not match the CRS of the project, the input is reprojected
into the current project. In case that the CRS of the input map
does match the CRS of the project, the input is imported directly.

<h3>Supported Vector Formats</h3>

<em>v.import</em> uses the OGR library which supports various vector data
formats including <a href="https://gdal.org/en/stable/drivers/vector/shapefile.html">ESRI
Shapefile</a>, <a href="https://gdal.org/en/stable/drivers/vector/mitab.html">Mapinfo
File</a>, UK .NTF, SDTS, TIGER, IHO S-57 (ENC), DGN, GML, GPX, AVCBin, REC,
Memory, OGDI, and PostgreSQL, depending on the local OGR installation.
For details see the <a href="https://gdal.org/en/stable/drivers/vector/">OGR web
site</a>. The OGR (Simple Features Library) is part of the
<a href="https://gdal.org">GDAL</a> library, hence GDAL needs to be
installed to use <em>v.import</em>.

<p>
The list of actually supported formats can be printed by <b>-f</b> flag.

<h2>NOTES</h2>

<em>v.import</em> checks the CRS metadata of the dataset to be
imported against that of the current project. If not identical a
related error message is shown.
<br>
To override this CRS check (i.e. to use current project's CRS)
by assuming that the dataset has the same CRS as the current project
the <b>-o</b> flag can be used. This is also useful when geodata to be
imported do not contain any CRS metadata at all. The user must be
sure that the CRS is identical in order to avoid introducing data
errors.

<h3>Topology cleaning</h3>

When importing polygons, non-topological polygons are converted to
topological areas. If the input polygons contain errors (unexpected
overlapping areas, small gaps between polygons, or warnings about being
unable to calculate centroids), the import might need to be repeated
using a <em>snap</em> value as suggested in the output messages. The
default value of <code>snap=-1</code> means that no snapping will be done.

<!-- TODO: add hints for latlong -->

<p>
The <em>snap</em> threshold defines the maximal distance from one to another
vertex in map units (for latitude-longitude projects in degrees). If there
is no other vertex within <em>snap</em> distance, no snapping will be done.
Note that a too large value can severely damage area topology, beyond repair.
<p>
<i>Post-processing:</i> Snapped boundaries may need to be cleaned with
<em>v.clean</em>, using its tools <em>break,rmdupl,rmsa</em>. For details,
refer to the <em>v.clean</em> manual page.

<h2>EXAMPLE</h2>

<div class="code"><pre>
# import SHAPE file at full extent and reproject to current project CRS
v.import input=research_area.shp output=research_area extent=input
</pre></div>

<h2>SEE ALSO</h2>

<em>
<a href="v.clean.html">v.clean</a>,
<a href="v.in.lines.html">v.in.lines</a>,
<a href="v.in.ogr.html">v.in.ogr</a>,
<a href="v.proj.html">v.proj</a>
</em>

<h2>AUTHORS</h2>

Markus Metz<br>
Improvements: Martin Landa, Anna Petrasova
