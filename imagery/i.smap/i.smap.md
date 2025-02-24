## DESCRIPTION

The *i.smap* program is used to segment multispectral images using a
spectral class model known as a Gaussian mixture distribution. Since
Gaussian mixture distributions include conventional multivariate
Gaussian distributions, this program may also be used to segment
multispectral images based on simple spectral mean and covariance
parameters.

*i.smap* has two modes of operation. The first mode is the sequential
maximum a posteriori (SMAP) mode (see Bouman and Shapiro, 1992; Bouman
and Shapiro, 1994). The SMAP segmentation algorithm attempts to improve
segmentation accuracy by segmenting the image into regions rather than
segmenting each pixel separately (see NOTES below).

The second mode is the more conventional maximum likelihood (ML)
classification which classifies each pixel separately, but requires
somewhat less computation. This mode is selected with the **-m** flag
(see below).

## OPTIONS

### Flags

**-m**  
Use maximum likelihood estimation (instead of smap). Normal operation is
to use SMAP estimation (see NOTES below).

### Parameters

**group**=*name*  
imagery group  
The imagery group that defines the image to be classified.

**subgroup**=*name*  
imagery subgroup  
The subgroup within the group specified that specifies the subset of the
band files that are to be used as image data to be classified.

**signaturefile**=*name*  
imagery signaturefile  
The signature file that contains the spectral signatures (i.e., the
statistics) for the classes to be identified in the image. This
signature file is produced by the program
*[i.gensigset](i.gensigset.md)* (see NOTES below).

**blocksize**=*value*  
size of submatrix to process at one time  
default: 1024  
This option specifies the size of the "window" to be used when reading
the image data.

This program was written to be nice about memory usage without
influencing the resultant classification. This option allows the user to
control how much memory is used. More memory may mean faster (or slower)
operation depending on how much real memory your machine has and how
much virtual memory the program uses.

The size of the submatrix used in segmenting the image has a principle
function of controlling memory usage; however, it also can have a subtle
effect on the quality of the segmentation in the smap mode. The
smoothing parameters for the smap segmentation are estimated separately
for each submatrix. Therefore, if the image has regions with
qualitatively different behavior, (e.g., natural woodlands and man-made
agricultural fields) it may be useful to use a submatrix small enough so
that different smoothing parameters may be used for each distinctive
region of the image.

The submatrix size has no effect on the performance of the ML
segmentation method.

**output**=*name*  
output raster map.  
The name of a raster map that will contain the classification results.
This new raster map layer will contain categories that can be related to
landcover categories on the ground.

## NOTES

The SMAP algorithm exploits the fact that nearby pixels in an image are
likely to have the same class. It works by segmenting the image at
various scales or resolutions and using the coarse scale segmentations
to guide the finer scale segmentations. In addition to reducing the
number of misclassifications, the SMAP algorithm generally produces
segmentations with larger connected regions of a fixed class which may
be useful in some applications.

The amount of smoothing that is performed in the segmentation is
dependent of the behaviour of the data in the image. If the data
suggests that the nearby pixels often change class, then the algorithm
will adaptively reduce the amount of smoothing. This ensures that
excessively large regions are not formed.

The degree of misclassifications can be investigated with the goodness
of fit output map. Lower values indicate a better fit. The largest 5 to
15% of the goodness values may need some closer inspection.

The module *i.smap* does not support NULL cells (in the raster image or
from raster mask). Therefore, if the input image has NULL cells, it
might be necessary to create a masked classification results as part of
post-processing using e.g. *r.mapcalc*:

```sh
r.mapcalc "masked_results = if(isnull(input_image), null(), classification_results)"
```

Similarly, if the raster mask is active, it might be necessary to
post-process the classification results using *r.mapcalc* which will
automatically mask the classification results:

```sh
r.mapcalc "masked_results = classification_results"
```

## EXAMPLE

Supervised classification of LANDSAT scene (complete NC dataset)

```sh
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
```

The signature file obtained in the example above will allow to classify
the current imagery group only (lsat7_2002). If the user would like to
re-use the signature file for the classification of different imagery
group(s), they can set semantic labels for each group member beforehand,
i.e., before generating the signature files. Semantic labels are set by
means of *r.support* as shown below:

```sh
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
```

## REFERENCES

- C. Bouman and M. Shapiro, "Multispectral Image Segmentation using a
  Multiscale Image Model", *Proc. of IEEE Int'l Conf. on Acoust., Speech
  and Sig. Proc.,* pp. III-565 - III-568, San Francisco, California,
  March 23-26, 1992.
- C. Bouman and M. Shapiro 1994, "A Multiscale Random Field Model for
  Bayesian Image Segmentation", *IEEE Trans. on Image Processing., 3(2),
  162-177"
  ([PDF](http://dynamo.ecn.purdue.edu/~bouman/publications/pdf/ip2.pdf))*
- McCauley, J.D. and B.A. Engel 1995, "Comparison of Scene
  Segmentations: SMAP, ECHO and Maximum Likelihood", *IEEE Trans. on
  Geoscience and Remote Sensing, 33(6): 1313-1316.*

## SEE ALSO

*[r.support](r.support.md)* for setting semantic labels,  
*[i.group](i.group.md)* for creating groups and subgroups,  
*[r.mapcalc](r.mapcalc.md)* to copy classification result in order to
cut out masked subareas,  
*[i.gensigset](i.gensigset.md)* to generate the signature file required
by this program

*[g.gui.iclass](g.gui.iclass.md), [i.maxlik](i.maxlik.md),
[r.kappa](r.kappa.md)*

## AUTHORS

[Charles Bouman, School of Electrical Engineering, Purdue
University](https://engineering.purdue.edu/~bouman/software/segmentation/)  
Michael Shapiro, U.S.Army Construction Engineering Research Laboratory  
Semantic label support: Maris Nartiss, University of Latvia
