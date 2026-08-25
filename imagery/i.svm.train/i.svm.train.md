## DESCRIPTION

*i.svm.train* finds parameters for a Support Vector Machine and stores
them in a signature file for later usage by
[i.svm.predict](i.svm.predict.md).

Internally the module performs input value rescaling of each of imagery
group rasters by mean normalisation based on minimum and maximum value
present in the raster metadata. Rescaling parameters are written into
the signature file for use during prediction.

## NOTES

*i.svm.train* internally is using the LIBSVM. For introduction into
value prediction or estimation with LIBSVM, see [a Practical Guide to
Support Vector
Classification](https://www.csie.ntu.edu.tw/~cjlin/papers/guide/guide.pdf)
by Chih-Wei Hsu, Chih-Chung Chang, and Chih-Jen Lin.

It is strongly suggested to have semantic labels set for each raster map
in the training data (feature value) imagery group. Use
[r.support](r.support.md) to set semantic labels.

## PERFORMANCE

SVM training is done by loading all training data into memory. In a case
of large input raster files, use sparse label rasters (e.g. raster
points or small patches instead of uninterrupted cover).

During the training process there is no progress output printed.
Training with large number of data points can take significant time -
just be patient.

By default the shrinking heuristics option of LIBSVM is enabled. It
should not impact the outcome, just the training time. On some input
parameter and data combinations training with the shrinking heuristics
disabled might be faster.

The *cache* parameter determines the maximum memory allocated for kernel
caching to enhance computational speed. It's important to note that the
actual module's memory consumption may vary from this setting, as it
solely impacts LIBSVM's internal caching. The cache is utilized on an
as-needed basis, so it's unlikely to reach the specified value.

## EXAMPLE

This is the first part of classification process. See
[i.svm.predict](i.svm.predict.md) for the second part.

Train a SVM to identify land use classes according to the 1996 land use
map *landuse96_28m* and then classify a LANDSAT scene from October of
2002. Example requires the nc_spm_08 dataset.

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
# r.random input=landuse96_28m npoints=10000 raster=training -s

# Train the SVM
i.svm.train group=lsat7_2002 subgroup=res_30m \
    trainingmap=training signaturefile=landuse96_rnd_points

# Go to i.svm.predict for the next step.
```

## SEE ALSO

*Predict values: [i.svm.predict](i.svm.predict.md)  
Set semantic labels: [r.support](r.support.md)  
Other classification modules: [i.maxlik](i.maxlik.md),
[i.smap](i.smap.md)*  
LIBSVM home page: [LIBSVM - A Library for Support Vector
Machines](https://www.csie.ntu.edu.tw/~cjlin/libsvm/)

## REFERENCES

Please cite both - LIBSVM and i.svm.

- For i.svm.\* modules:  
  Nartiss, M., & Melniks, R. (2023). Improving pixel-­based
  classification of GRASS GIS with support vector machine. Transactions
  in GIS, 00, 1–16. <https://doi.org/10.1111/tgis.13102>
- For LIBSVM:  
  Chang, C.-C., & Lin, C.-J. (2011). LIBSVM : a library for support
  vector machines. ACM Transactions on Intelligent Systems and
  Technology, 2:27:1--27:27.

## AUTHOR

Maris Nartiss, University of Latvia.
