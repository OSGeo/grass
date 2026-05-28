## DESCRIPTION

*i.svm.predict* predicts values with a Support Vector Machine (SVM) and
stores them in a raster file. Predictions are based on a signature file
generated with [i.svm.train](i.svm.train.md).

Internally the module performs input value rescaling of each of imagery
group rasters by minimum and maximum range determined during training.

## NOTES

*i.svm.train* internally is using the LIBSVM. For introduction into
value prediction or estimation with LIBSVM, see [a Practical Guide to
Support Vector
Classification](https://www.csie.ntu.edu.tw/~cjlin/papers/guide/guide.pdf)
by Chih-Wei Hsu, Chih-Chung Chang, and Chih-Jen Lin.

It is strongly suggested to have semantic labels set for each raster map
in the training data (feature value) and in value prediction imagery
groups. Use [r.support](r.support.md) to set semantic labels.

## PERFORMANCE

Value prediction is done cell by cell and thus memory consumption should
be constant.

The *cache* parameter determines the maximum memory allocated for kernel
caching to enhance computational speed. It's important to note that the
actual module's memory consumption may vary from this setting, as it
solely impacts LIBSVM's internal caching. The cache is utilized on an
as-needed basis, so it's unlikely to reach the specified value.

## EXAMPLE

This is the second part of classification process. See
[i.svm.train](i.svm.train.md) for the first part.

Predict land use classes form a LANDSAT scene from October of 2002 with
a SVM trained on a 1996 land use map *landuse96_28m*.

```sh
i.svm.predict group=lsat7_2002 subgroup=res_30m \
    signaturefile=landuse96_rnd_points output=pred_landuse_2002
```

## SEE ALSO

*Train SVM: [i.svm.train](i.svm.train.md)  
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
