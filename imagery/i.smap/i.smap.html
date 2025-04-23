<h2>DESCRIPTION</h2>

The <em>i.smap</em> program is used to segment
multispectral images using a spectral class model known as
a Gaussian mixture distribution.  Since Gaussian mixture
distributions include conventional multivariate Gaussian
distributions, this program may also be used to segment
multispectral images based on simple spectral mean and
covariance parameters.

<p>
<em>i.smap</em> has two modes of operation. The first mode
is the sequential maximum a posteriori (SMAP) mode (see
Bouman and Shapiro, 1992; Bouman and Shapiro, 1994). The SMAP
segmentation algorithm attempts to improve segmentation
accuracy by segmenting the image into regions rather than
segmenting each pixel separately (see NOTES below).

<p>
The second mode is the more conventional maximum likelihood (ML)
classification which classifies each pixel separately,
but requires somewhat less computation. This mode is selected with
the <b>-m</b> flag (see below).

<h2>OPTIONS</h2>

<h3>Flags:</h3>

<dl>
<dt><b>-m</b>
<dd>Use maximum likelihood estimation (instead of smap).
Normal operation is to use SMAP estimation (see NOTES below).
</dl>

<h3>Parameters:</h3>

<dl>
<dt><b>group=</b><em>name</em>

<dd>imagery group<br>
The imagery group that defines the image to be classified.

<dt><b>subgroup=</b><em>name</em>

<dd>imagery subgroup<br>
The subgroup within the group specified that specifies the
subset of the band files that are to be used as image data
to be classified.

<dt><b>signaturefile=</b><em>name</em>

<dd>imagery signaturefile<br>
The signature file that contains the spectral signatures (i.e., the
statistics) for the classes to be identified in the image.
This signature file is produced by the program
<em><a href="i.gensigset.html">i.gensigset</a></em>
(see NOTES below).

<dt><b>blocksize=</b><em>value</em>

<dd>size of submatrix to process at one time<br>
default: 1024<br>
This option specifies the size of the "window" to be used when
reading the image data.

<p>
This program was written to be nice about memory usage
without influencing the resultant classification. This
option allows the user to control how much memory is used.
More memory may mean faster (or slower) operation depending
on how much real memory your machine has and how much
virtual memory the program uses.

<p>
The size of the submatrix used in segmenting the image has
a principle function of controlling memory usage; however,
it also can have a subtle effect on the quality of the
segmentation in the smap mode.  The smoothing parameters
for the smap segmentation are estimated separately for each
submatrix.  Therefore, if the image has regions with
qualitatively different behavior, (e.g., natural woodlands
and man-made agricultural fields) it may be useful to use a
submatrix small enough so that different smoothing
parameters may be used for each distinctive region of the
image.

<p>
The submatrix size has no effect on the performance of the
ML segmentation method.

<dt><b>output=</b><em>name</em>

<dd>output raster map.<br>
The name of a raster map that will contain the
classification results.  This new raster map layer will
contain categories that can be related to landcover
categories on the ground.

</dl>


<h2>NOTES</h2>

The SMAP algorithm exploits the fact that nearby pixels in
an image are likely to have the same class.  It works by
segmenting the image at various scales or resolutions and
using the coarse scale segmentations to guide the finer
scale segmentations.  In addition to reducing the number of
misclassifications, the SMAP algorithm generally produces
segmentations with larger connected regions of a fixed
class which may be useful in some applications.

<p>
The amount of smoothing that is performed in the
segmentation is dependent of the behaviour of the data in
the image.  If the data suggests that the nearby pixels
often change class, then the algorithm will adaptively
reduce the amount of smoothing.  This ensures that
excessively large regions are not formed.

<p>
The degree of misclassifications can be investigated with the goodness
of fit output map. Lower values indicate a better fit. The largest 5 to
15% of the goodness values may need some closer inspection.

<p>
The module <em>i.smap</em> does not support NULL cells (in the
raster image or from raster mask). Therefore, if the input image has NULL
cells, it might be necessary to create a masked classification results
as part of post-processing using e.g. <em>r.mapcalc</em>:

<div class="code"><pre>
r.mapcalc "masked_results = if(isnull(input_image), null(), classification_results)"
</pre></div>

<p>
Similarly, if the raster mask is active,
it might be necessary to post-process the classification results
using <em>r.mapcalc</em> which will automatically mask the classification
results:

<div class="code"><pre>
r.mapcalc "masked_results = classification_results"
</pre></div>

<h2>EXAMPLE</h2>

Supervised classification of LANDSAT scene (complete NC dataset)

<div class="code"><pre>
# Align computation region to the scene
g.region raster=lsat7_2002_10 -p

# store VIZ, NIR, MIR into group/subgroup
i.group group=lsat7_2002 subgroup=res_30m \
  input=lsat7_2002_10,lsat7_2002_20,lsat7_2002_30,lsat7_2002_40,lsat7_2002_50,lsat7_2002_70

# Now digitize training areas "training" with the digitizer
# and convert to raster model with v.to.rast
v.to.rast input=training output=training use=cat label_column=label
# If you are just playing around and do not care about the accuracy of outcome,
# just use one of existing maps instead e.g.
# g.copy rast=landuse96_28m,training

# Create a signature file with statistics for each class
i.gensigset trainingmap=training group=lsat7_2002 subgroup=res_30m \
            signaturefile=lsat7_2002_30m maxsig=5

# Predict classes based on whole LANDSAT scene
i.smap group=lsat7_2002 subgroup=res_30m signaturefile=lsat7_2002_30m \
       output=lsat7_2002_smap_classes

# Visually check result
d.mon wx0
d.rast.leg lsat7_2002_smap_classes

# Statistically check result
r.kappa -w classification=lsat7_2002_smap_classes reference=training
</pre></div>

<p>
The signature file obtained in the example above will allow
to classify the current imagery group only (lsat7_2002).
If the user would like to re-use the signature file for the
classification of different imagery group(s), they can set
semantic labels for each group member beforehand, i.e.,
before generating the signature files.
Semantic labels are set by means of <em>r.support</em>
as shown below:

<div class="code"><pre>
# Define semantic labels for all LANDSAT bands
r.support map=lsat7_2002_10 semantic_label=TM7_1
r.support map=lsat7_2002_20 semantic_label=TM7_2
r.support map=lsat7_2002_30 semantic_label=TM7_3
r.support map=lsat7_2002_40 semantic_label=TM7_4
r.support map=lsat7_2002_50 semantic_label=TM7_5
r.support map=lsat7_2002_61 semantic_label=TM7_61
r.support map=lsat7_2002_62 semantic_label=TM7_62
r.support map=lsat7_2002_70 semantic_label=TM7_7
r.support map=lsat7_2002_80 semantic_label=TM7_8
</pre></div>

<h2>REFERENCES</h2>

<ul>
<li>C. Bouman and M. Shapiro,
"Multispectral Image Segmentation using a Multiscale Image Model",
<em>Proc. of IEEE Int'l Conf. on Acoust., Speech and Sig. Proc.,</em>
pp. III-565 - III-568, San Francisco, California, March 23-26, 1992.</li>

<li>C. Bouman and M. Shapiro 1994,
"A Multiscale Random Field Model for Bayesian Image Segmentation",
<em>IEEE Trans. on Image Processing., 3(2), 162-177"
(<a href="http://dynamo.ecn.purdue.edu/~bouman/publications/pdf/ip2.pdf">PDF</a>)</em></li>

<li>McCauley, J.D. and B.A. Engel 1995,
"Comparison of Scene Segmentations: SMAP, ECHO and Maximum Likelihood",
<em>IEEE Trans. on Geoscience and Remote Sensing, 33(6): 1313-1316.</em></li>
</ul>

<h2>SEE ALSO</h2>

<em>
<a href="r.support.html">r.support</a></em> for setting semantic labels,
<br>
<em>
<a href="i.group.html">i.group</a></em> for creating groups and subgroups,
<br>
<em><a href="r.mapcalc.html">r.mapcalc</a></em>
to copy classification result in order to cut out masked subareas,
<br>
<em><a href="i.gensigset.html">i.gensigset</a></em>
to generate the signature file required by this program
<p>
<em>
<a href="g.gui.iclass.html">g.gui.iclass</a>,
<a href="i.maxlik.html">i.maxlik</a>,
<a href="r.kappa.html">r.kappa</a>
</em>

<h2>AUTHORS</h2>

<a href="https://engineering.purdue.edu/~bouman/software/segmentation/">Charles Bouman,
School of Electrical Engineering, Purdue University</a>
<br>
Michael Shapiro,
U.S.Army Construction Engineering
Research Laboratory
<br>
Semantic label support: Maris Nartiss,
University of Latvia
