<h2>DESCRIPTION</h2>

<em>r.kappa</em> tabulates the error matrix of classification result by
crossing classified map layer with respect to reference map layer.  Both
overall <em>kappa</em> (accompanied by its <em>variance</em>) and
conditional <em>kappa</em> values are calculated.  This analysis program
respects the current geographic region and mask settings.
<p>
<em>r.kappa</em> calculates the error matrix of the
two map layers and prepares the table from which the report
is to be created.  <em>kappa</em> values for overall and
each classes are computed along with their variances. Also
percent of commission and omission error, total correct
classified result by pixel counts, total area in pixel
counts and percentage of overall correctly classified
pixels are tabulated.

<p>
The report will be written to an output file which is in
plain text format and named by user at prompt of running
the program. To obtain machine readable version, specify a
<em>json</em> output format.

<p>
The body of the report is arranged in panels.  The
classified result map layer categories is arranged along
the vertical axis of the table, while the reference map
layer categories along the horizontal axis.  Each panel has
a maximum of 5 categories (9 if wide format) across the
top.  In addition, the last column of the last panel
reflects a cross total of each column for each row.  All of
the categories of the map layer arranged along the vertical
axis, i.e., the reference map layer,  are included in each
panel.  There is a total at the bottom of each column
representing the sum of all the rows in that column.

<h2>OUTPUT VARIABLES</h2>
<p>
All output variables (except kappa variance) have been
validated to produce correct values in accordance
to formulas given by Rossiter, D.G., 2004. "Technical Note:
Statistical methods for accuracy assessment of classified
thematic maps".
<dl>
    <dt>Observations</dt>
    <dd>Overall count of observed cells (sum of both correct
    and incorrect ones).</dd>
    <dt>Correct</dt>
    <dd>Overall count of correct cells (cells with equal value
    in reference and classification maps).</dd>
    <dt>Overall accuracy</dt>
    <dd>Number of correct cells divided by overall cell count
    (expressed in percent).</dd>
    <dt>User's accuracy</dt>
    <dd>Share of correctly classified cells out of all cells
    classified as belonging to specified class (expressed in percent).
    Inverse of commission error.</dd>
    <dt>Commission</dt>
    <dd>Commission error = 100 - user's accuracy.</dd>
    <dt>Producer's accuracy</dt>
    <dd>Share of correctly classified cells out of all cells
    known to belong to specified class (expressed in percent).
    Inverse of omission error.</dd>
    <dt>Omission</dt>
    <dd>Omission error = 100 - producer's accuracy.</dd>
    <dt>Kappa</dt>
    <dd>Choen's kappa index value.</dd>
    <dt>Kappa variance</dt>
    <dd>Variance of kappa index. Correctness needs to be validated.</dd>
    <dt>Conditional kappa</dt>
    <dd>Conditional user's kappa for specified class.</dd>
    <dt>MCC</dt>
    <dd>Matthews (Mattheus) Correlation Coefficient is implemented
    according to Grandini, M., Bagli, E., Visani, G. 2020.
    "Metrics for multi-class classification: An overview."</dd>
</dl>

<h2>NOTES</h2>

<p>
It is recommended to reclassify categories of classified
result map layer into a more manageable number before
running <em>r.kappa</em> on the classified raster map
layer. Because <em>r.kappa</em> calculates and then reports
information for each and every category.

<p>
<em>NA</em>'s in output mean it was not possible to calculate the value
(e.g. calculation would involve division by zero).
In JSON output <em>NA</em>'s are represented with value <em>null</em>.
If there is no overlap between both maps, a warning is printed and
output values are set to 0 or <em>null</em> respectively.

<p>
The <b>Estimated kappa value</b> in <em>r.kappa</em> is the value
only for one class, i.e. the observed agreement between the
classifications for those observations that have been classified by
classifier 1 into the class i. In other words, here the choice of
reference is important.
<p>
It is calculated as:
<p>
kpp[i] = (pii[i] - pi[i] * pj[i]) / (pi[i] - pi[i] * pj[i]);
<p>
where=
<ul>
<li>pii[i] is the probability of agreement (i.e. number of pixels for which there is agreement divided by total number of assessed pixels)</li>
<li>Pi[i] is the probability of classification i having classified the point as i</li>
<li>Pj[i] is the probability of classification j having classified the point as i.</li>
</ul>

<p>
Some of reported values (overall accuracy, Choen's kappa, MCC) can be
misleading if cell count among classes is not balanced. See e.g.
Powers, D.M.W., 2012. "The Problem with Kappa"; Zhu, Q., 2020.
"On the performance of Matthews correlation coefficient (MCC) for
imbalanced dataset".

<h2>EXAMPLE</h2>

Example for North Carolina sample dataset:

<div class="code"><pre>
g.region raster=landclass96 -p
r.kappa -w classification=landuse96_28m reference=landclass96

# export Kappa matrix as CSV file "kappa.csv"
r.kappa classification=landuse96_28m reference=landclass96 output=kappa.csv -m -h
</pre></div>
<p>

Verification of classified LANDSAT scene against training areas:

<div class="code"><pre>
r.kappa -w classification=lsat7_2002_classes reference=training
</pre></div>

<h2>SEE ALSO</h2>

<em>
<a href="g.region.html">g.region</a>,
<a href="r.category.html">r.category</a>,
<a href="r.mask.html">r.mask</a>,
<a href="r.reclass.html">r.reclass</a>,
<a href="r.report.html">r.report</a>,
<a href="r.stats.html">r.stats</a>
</em>

<h2>AUTHORS</h2>

Tao Wen, University of Illinois at Urbana-Champaign, Illinois<br>
Maris Nartiss, University of Latvia (JSON output, MCC)
