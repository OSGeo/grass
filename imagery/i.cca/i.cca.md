## DESCRIPTION

**i.cca** is an image processing program that takes any number of
(raster) band files and a signature file, and outputs the same number of
raster band files transformed to provide maximum separability of the
categories indicated by the signatures. This implementation of the
canonical components transformation is based on the algorithm contained
in the [LAS image processing
system](http://dbwww.essc.psu.edu/lasdoc/user/canal.html). CCA is also
known as "Canonical components transformation".

Typically the user will use the *[g.gui.iclass](g.gui.iclass.md)*
program to collect a set of signatures and then pass those signatures
along with the raster band files to *i.cca*. The raster band file names
are specified on the command line by giving the group and subgroup that
were used to collect the signatures.

The output raster map names are built by appending a ".1", ".2", etc. to
the output raster map name specified on the command line.

### Parameters

**group**=*name*  
Name of the [imagery](i.group.md) group to which the raster band files
used belong.

**subgroup**=*name*  
Name of the [imagery](i.group.md) subgroup to which the raster band
files used belong.

**signature**=*name*  
Name of an ASCII file containing spectral signatures.

**output**=*name*  
Output raster map prefix name. The output raster map layer names are
built by appending a ".1", ".2", etc. onto the *output* name specified
by the user.

## NOTES

*i.cca* respects the current geographic region definition and the
current mask setting while performing the transformation.

## REFERENCES

Schowengerdt, Robert A. **Techniques for Image Processing and
Classification in Remote Sensing**, Academic Press, 1983.

## SEE ALSO

*[g.gui.iclass](g.gui.iclass.md), [i.gensig](i.gensig.md),
[i.cluster](i.cluster.md), [i.pca](i.pca.md), [r.covar](r.covar.md),
[r.mapcalc](r.mapcalc.md)*

## AUTHORS

David Satnik, GIS Laboratory, Central Washington University  
Ali R. Vali, University of Texas  
Semantic label support: Maris Nartiss, University of Latvia
