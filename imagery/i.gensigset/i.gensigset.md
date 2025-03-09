## DESCRIPTION

*i.gensigset* is a non-interactive method for generating input into
*[i.smap](i.smap.md).* It is used as the first pass in the a two-pass
classification process. It reads a raster map layer, called the training
map, which has some of the pixels or regions already classified.
*i.gensigset* will then extract spectral signatures from an image based
on the classification of the pixels in the training map and make these
signatures available to *[i.smap](i.smap.md).*

The user would then execute the GRASS program *[i.smap](i.smap.md)* to
create the final classified map.

This module generates signature files of type "sigset". Use module
[i.signatures](i.signatures.md) to manage generated signature files.

For all raster maps used to generate signature file it is recommended to
have semantic label set. Use *[r.support](r.support.md)* to set semantic
labels of each member of the imagery group. Signatures generated for one
scene are suitable for classification of other scenes as long as they
consist of same raster bands (semantic labels match). If semantic labels
are not set, it will be possible to use obtained signature file to
classify only the same imagery group used for generating signatures.

An usage example can be found in [i.smap](i.smap.md) documentation.

## OPTIONS

### Parameters

The **trainingmap** raster layer, supplied as input by the user, has some of
its pixels already classified, and the rest (probably most) of the pixels
unclassified. Classified means that the pixel has a non-zero value and
unclassified means that the pixel has a zero value.

This map must be prepared by the user in advance by using a combination
of *[wxGUI vector digitizer](wxGUI.vdigit.md)* and
*[v.to.rast](v.to.rast.md)*, or some other import/development process
(e.g.,
*[v.transects](https://grass.osgeo.org/grass-stable/manuals/addons/v.transects.html))*
to define the areas representative of the classes in the image.

At present, there is no fully-interactive tool specifically designed for
producing this layer.

Option **group** is the name of the group that contains the band files which
comprise the image to be analyzed. The *[i.group](i.group.md)* command
is used to construct groups of raster layers which comprise an image.

Option **subgroup** names the subgroup within the group that selects a subset
of the bands to be analyzed. The *[i.group](i.group.md)* command is also used
to prepare this subgroup. The subgroup mechanism allows the user to
select a subset of all the band files that form an image.

Option **signaturefile** is the resultant signature file (containing the means and
covariance matrices) for each class in the training map that is
associated with the band files in the subgroup selected.

Option **maxsig** is the maximum number of sub-signatures in any class
(default: 5).

The spectral signatures which are produced by this program are "mixed"
signatures (see [NOTES](#notes)). Each signature contains one or more
subsignatures (representing subclasses). The algorithm in this program
starts with a maximum number of subclasses and reduces this number to a
minimal number of subclasses which are spectrally distinct. The user has
the option to set this starting value with this option.

## NOTES

The algorithm in *i.gensigset* determines the parameters of a spectral
class model known as a Gaussian mixture distribution. The parameters are
estimated using multispectral image data and a training map which labels
the class of a subset of the image pixels. The mixture class parameters
are stored as a class signature which can be used for subsequent
segmentation (i.e., classification) of the multispectral image.

The Gaussian mixture class is a useful model because it can be used to
describe the behavior of an information class which contains pixels with
a variety of distinct spectral characteristics. For example, forest,
grasslands or urban areas are examples of information classes that a
user may wish to separate in an image. However, each of these
information classes may contain subclasses each with its own distinctive
spectral characteristic. For example, a forest may contain a variety of
different tree species each with its own spectral behavior.

The objective of mixture classes is to improve segmentation performance
by modeling each information class as a probabilistic mixture with a
variety of subclasses. The mixture class model also removes the need to
perform an initial unsupervised segmentation for the purposes of
identifying these subclasses. However, if misclassified samples are used
in the training process, these erroneous samples may be grouped as a
separate undesired subclass. Therefore, care should be taken to provided
accurate training data.

This clustering algorithm estimates both the number of distinct
subclasses in each class, and the spectral mean and covariance for each
subclass. The number of subclasses is estimated using Rissanen's minimum
description length (MDL) criteria (Rissanen, 1983). This criteria
attempts to determine the number of subclasses which "best" describe the
data. The approximate maximum likelihood estimates of the mean and
covariance of the subclasses are computed using the expectation
maximization (EM) algorithm (Dempster, 1977 and Redner, 1984).

### WARNINGS

If warnings like this occur, reducing the remaining classes to 0:

```sh
...
WARNING: Removed a singular subsignature number 1 (4 remain)
WARNING: Removed a singular subsignature number 1 (3 remain)
WARNING: Removed a singular subsignature number 1 (2 remain)
WARNING: Removed a singular subsignature number 1 (1 remain)
WARNING: Unreliable clustering. Try a smaller initial number of clusters
WARNING: Removed a singular subsignature number 1 (-1 remain)
WARNING: Unreliable clustering. Try a smaller initial number of clusters
Number of subclasses is 0
```

then the user should check for:

- the range of the input data should be between 0 and 100 or 255 but not
  between 0.0 and 1.0 (*r.info* and *r.univar* show the range)
- the training areas need to contain a sufficient amount of pixels

## REFERENCES

1. J. Rissanen, "A Universal Prior for
  Integers and Estimation by Minimum Description Length," *Annals of
  Statistics,* vol. 11, no. 2, pp. 417-431, 1983.
2. A. Dempster, N. Laird and D. Rubin,
  "Maximum Likelihood from Incomplete Data via the EM Algorithm," *J.
  Roy. Statist. Soc. B,* vol. 39, no. 1, pp. 1-38, 1977.
3. E. Redner and H. Walker, "Mixture
  Densities, Maximum Likelihood and the EM Algorithm," *SIAM Review,*
  vol. 26, no. 2, April 1984.

## SEE ALSO

*[r.support](r.support.md), [i.group](i.group.md), [i.smap](i.smap.md),
[r.info](r.info.md), [r.univar](r.univar.md), [wxGUI vector
digitizer](wxGUI.vdigit.md)*

## AUTHORS

Charles Bouman, School of Electrical Engineering, Purdue University  
Michael Shapiro, U.S.Army Construction Engineering Research Laboratory  
Semantic label support: Maris Nartiss, University of Latvia
