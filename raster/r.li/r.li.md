<!-- meta page name: r.li -->
<!-- meta page name description: Landscape structure analysis package overview -->

<h2>DESCRIPTION</h2>

The <em>r.li</em> suite is a toolset for multiscale analysis of
landscape structure. It aims at implementing metrics as found in
external software for quantitative measures of landscape structure like
FRAGSTATS (McGarigal and Marks 1995).
<p>
The <em>r.li</em> suite offers a set of patch and diversity indices.
It supports analysis of landscapes composed of a mosaic of
patches, but, more generally, the modules work with any two-dimensional
raster map whose cell values are integer (e.g., 1, 2) or floating point
(e.g., 1.1, 3.2) values. The <em>g.gui.rlisetup</em> module has options for
controlling the shape, size, number, and distribution of sampling
areas used to collect information about the landscape structure.
Sampling area shapes can be the entire map or a moving
window of square, rectangular or circular shape. The size of
sampling areas can be changed, so that the landscape can be analyzed
at a variety of spatial scales simultaneously. Sampling areas may be
distributed across the landscape in a random, systematic, or
stratified-random manner, or as a moving window.
<p>
The <em>r.li</em> modules can calculate a number of measures that produce
single values as output (e.g. mean patch size in the sampling area),
as well as measures that produce a distribution of values as output
(e.g. frequency distribution of patch sizes in the sampling area). The
results are stored as raster maps.
<p>
All modules require configuration file which can be created by the
<em>g.gui.rlisetup</em> module which is a GUI tool providing a convenient
way to set all necessary parameters. This file can be used repetitively
saving user from the need to specify all parameters over and over again.

<h2>NOTES</h2>

The general procedure to calculate an index from a raster map is two-fold:

<ol>
<li>run <em>g.gui.rlisetup</em>: create a configuration file selecting
    the parts of raster map to be analyzed. This file allows re-running
    an analysis easily. It is stored on Windows in the directory <code>C:\Users\userxy\AppData\Roaming\GRASS8\r.li\</code>, on GNU/Linux in
    <code>$HOME/.grass8/r.li/</code>.</li> <!-- TODO: and Mac OSX? -->

<li>run one or more of the <em>r.li.<b>[index]</b></em> modules (e.g.,
    <em>r.li.<b>patchdensity</b></em>) to calculate the selected index
    using on the areas selected on configuration file.</li>
</ol>

<!-- mhh ??:
The <em>r.li.daemon</em> source code has a "main" function front-end
which can be run, but it is only a template for development of new
indices.
-->

<h2>EXAMPLES</h2>

Calculate a patch density index on the entire 'geology' raster map
in the Spearfish sample dataset, using a 5x5 moving window:

<!-- TODO: update to new wxGUI: -->

<ol>
<li> CREATE A NEW CONFIGURATION FILE
<ol>
<li> run
<div class="code"><pre>
g.gui.rlisetup
</pre></div>
</li>
<li> The main <em>g.gui.rlisetup</em> window is displayed, click on "New"</li>
<li> The new configuration window is now displayed, enter the<br>
configuration file name (e.g., "my_conf", do not use absolute paths)
<br><br>
Now the new configuration window is displayed.<br>
Enter the configuration file name (e.g., "my_conf", do not use absolute paths)<br>
and the name of raster map (e.g., "geology").<br>
The other fields are not needed for this configuration.</li>
<li> Click on "Setup sampling frame", select "Whole map layer" and click "OK"</li>
<li> Click on "Setup sampling areas", select "Moving window" and click "OK"</li>
<li> Click on "Use keyboard to enter moving window dimension"</li>
<li> Select "Rectangle" and enter 5 in the "height" and "width" fields</li>
<li> Click on "Save settings"</li>
<li> Close the <em>g.gui.rlisetup</em> window</li>
</ol></li>

<li> CALCULATE PATCHDENSITY INDEX
<ol>
<li> set the region settings to the "<code>geology</code>" raster map:
<div class="code"><pre>
g.region raster=geology -p
</pre></div>
</li>
<li> run <em>r.li.patchdensity</em>:
<div class="code"><pre>
r.li.patchdensity input=geology conf=my_conf out=patchdens
</pre></div>
</li>
</ol></li>
</ol>

The resulting patch density is stored in "<code>patchdens</code>" raster map.

You can verify the result for example with contour lines:
<div class="code"><pre>
r.contour in=patchdens out=patchdens step=5
d.rast patchdens
d.vect -c patchdens
</pre></div>

Note that if you want to run another index with the same area
configuration, you don't have to create another configuration file.

You can also use the same area configuration file on another map. The
program rescale it automatically. For instance if you have selected a
5x5 sample area on 100x100 raster map, and you use the same
configuration file on a 200x200 raster map, then the sample area is
10x10.

<h2>SEE ALSO</h2>

<b>GUI tools</b>:
<ul>
  <li> <a href="g.gui.rlisetup.html">g.gui.rlisetup</a>: Configuration editor for the <code>r.li.*</code> module where <code>*</code> is name of the index</li>
</ul>

<b>Patch indices</b>:
<ul>
<li>Indices based on patch number:
<ul>
  <li> <a href="r.li.patchdensity.html">r.li.patchdensity</a>: Calculates patch density index on a raster map, using a 4 neighbour algorithm</li>
  <li> <a href="r.li.patchnum.html">r.li.patchnum</a>: Calculates patch number index on a raster map, using a 4 neighbour algorithm</li>
</ul>

<li>Indices based on patch dimension:
<ul>
  <li> <a href="r.li.mps.html">r.li.mps</a>: Calculates mean patch size index on a raster map, using a 4 neighbour algorithm</li>
  <li> <a href="r.li.padcv.html">r.li.padcv</a>: Calculates coefficient of variation of patch area on a raster map</li>
  <li> <a href="r.li.padrange.html">r.li.padrange</a>: Calculates range of patch area size on a raster map</li>
  <li> <a href="r.li.padsd.html">r.li.padsd</a>: Calculates standard deviation of patch area a raster map</li>
</ul>

<li>Indices based on patch shape:
<ul>
  <li> <a href="r.li.shape.html">r.li.shape</a>: Calculates shape index on a raster map</li>
</ul>

<li>Indices based on patch edge: <!-- border? -->
<ul>
  <li> <a href="r.li.edgedensity.html">r.li.edgedensity</a>: Calculates edge density index on a raster map, using a 4 neighbour algorithm</li>
</ul>

<li>Indices based on patch attributes:
<ul>
  <li> <a href="r.li.cwed.html">r.li.cwed</a>: Calculates contrast Weighted Edge Density index on a raster map</li>
  <li> <a href="r.li.mpa.html">r.li.mpa</a>: Calculates mean pixel attribute index on a raster map</li>
</ul>

</ul>

<b>Diversity indices</b>:
<ul>
  <li> <a href="r.li.dominance.html">r.li.dominance</a>: Calculates dominance diversity index on a raster map</li>
  <li> <a href="r.li.pielou.html">r.li.pielou</a>: Calculates Pielou eveness index on a raster map</li>
  <li> <a href="r.li.renyi.html">r.li.renyi</a>: Calculates Renyi entropy on a raster map</li>
  <li> <a href="r.li.richness.html">r.li.richness</a>: Calculates richness diversity index on a raster map</li>
  <li> <a href="r.li.shannon.html">r.li.shannon</a>: Calculates Shannon diversity index on a raster map</li>
  <li> <a href="r.li.simpson.html">r.li.simpson</a>: Calculates Simpson diversity index on a raster map</li>
</ul>

<b>Core library</b>:
<ul>
  <li> <a href="r.li.daemon.html">r.li.daemon</a>: library with common functionality (not visible to the user)</li>
</ul>

<h2>ADDING NEW INDICES</h2>

New indices can be defined and implemented by any C programmer, without
having to deal with all basic functions (IO etc.). The computing
architecture and the functions are clearly separated, thus allowing an
easy expandability. Every index is defined separately, placed in a
directory along with its Makefile for compiling it and a file
&lt;module_name&gt;.html which describes the index including a simple
example of use. See <a href="r.li.daemon.html">r.li.daemon</a>
for more information about development.

<h2>REFERENCES</h2>

<ul>
<li>
McGarigal, K., and B. J. Marks. 1995. FRAGSTATS: spatial pattern
analysis program for quantifying landscape structure. USDA For. Serv.
Gen. Tech. Rep. PNW-351
 (<a href="https://doi.org/10.2737/PNW-GTR-351">PDF</a>).</li>

<li>
Baker, W.L. and Y. Cai. 1992. The r.le programs for multiscale analysis of
landscape structure using the GRASS geographical information system.
Landscape Ecology 7(4):291-302.</li>
</ul>

<h2>AUTHORS</h2>

Claudio Porta and Lucio Davide Spano, students of Computer Science,
University of Pisa (Italy).<br>
Commission from Faunalia Pontedera (PI)
<p>
Partially rewritten by Markus Metz
