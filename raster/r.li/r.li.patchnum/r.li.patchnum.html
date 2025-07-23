<h2>DESCRIPTION</h2>

<em>r.li.patchnum</em> calculates the "patch number index" as:<br>
<i> f(sample_area)= Patch_Number </i><br>

<p>
This index is calculated using a 4 neighbour algorithm, diagonal cells
are ignored when tracing a patch.

<h2>NOTES</h2>

Do not use absolute path names for the <b>config</b> and <b>output</b>
file/map parameters.

If the "moving window" method was selected in <b>g.gui.rlisetup</b>, then the
output will be a raster map, otherwise an ASCII file will be generated in
the folder <code>C:\Users\userxy\AppData\Roaming\GRASS8\r.li\output\</code>
(MS-Windows) or <code>$HOME/.grass8/r.li/output/</code> (GNU/Linux).
<p>
If the sample area contains only NULL values then it is considered to
have zero patches. <br>

<h2>EXAMPLES</h2>

To calculate patch number index on map <em>my_map</em>, using
<em>my_conf</em> configuration file (previously defined with
<em>g.gui.rlisetup</em>) and saving results in <em>my_out</em>, run:
<div class="code"><pre>
r.li.patchnum input=my_map conf=my_conf out=my_out
</pre></div>
<p>

Forest map (Spearfish sample dataset) example:
<div class="code"><pre>
g.region raster=landcover.30m -p
r.mapcalc "forests = if(landcover.30m &gt;= 41 &amp;&amp; landcover.30m &lt;= 43,1,null())"
r.li.patchnum input=forests conf=movwindow7 out=forests_patchnum_mov7
r.univar forests_patchnum_mov7
</pre></div>
<p>

Forest map (North Carolina sample dataset) example:
<div class="code"><pre>
g.region raster=landclass96 -p
r.mapcalc "forests = if(landclass96 == 5, 1, null() )"
r.li.patchnum input=forests conf=movwindow7 out=forests_patchnum_mov7

# verify
r.univar forests_patchnum_mov7
r.to.vect input=forests output=forests type=area
d.mon wx0
d.rast forests_patchnum_mov7
d.vect forests type=boundary
</pre></div>

<h2>SEE ALSO</h2>

<em>
<a href="r.li.html">r.li</a> (package overview),
<a href="g.gui.rlisetup.html">g.gui.rlisetup</a>
</em>

<h2>REFERENCES</h2>

McGarigal, K., and B. J. Marks. 1995. FRAGSTATS: spatial pattern
analysis program for quantifying landscape structure. USDA For. Serv.
Gen. Tech. Rep. PNW-351. (<a href="https://doi.org/10.2737/PNW-GTR-351">PDF</a>)

<h2>AUTHORS</h2>

Michael Shapiro - CERL (patch identification)<br>
Markus Metz (statistics)
