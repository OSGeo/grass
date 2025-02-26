## DESCRIPTION

*r.kappa* tabulates the error matrix of classification result by
crossing classified map layer with respect to reference map layer. Both
overall *kappa* (accompanied by its *variance*) and conditional *kappa*
values are calculated. This analysis program respects the current
geographic region and mask settings.

*r.kappa* calculates the error matrix of the two map layers and prepares
the table from which the report is to be created. *kappa* values for
overall and each classes are computed along with their variances. Also
percent of commission and omission error, total correct classified
result by pixel counts, total area in pixel counts and percentage of
overall correctly classified pixels are tabulated.

The report will be written to an output file which is in plain text
format and named by user at prompt of running the program. To obtain
machine readable version, specify a *json* output format.

The body of the report is arranged in panels. The classified result map
layer categories is arranged along the vertical axis of the table, while
the reference map layer categories along the horizontal axis. Each panel
has a maximum of 5 categories (9 if wide format) across the top. In
addition, the last column of the last panel reflects a cross total of
each column for each row. All of the categories of the map layer
arranged along the vertical axis, i.e., the reference map layer, are
included in each panel. There is a total at the bottom of each column
representing the sum of all the rows in that column.

## OUTPUT VARIABLES

All output variables (except kappa variance) have been validated to
produce correct values in accordance to formulas given by Rossiter,
D.G., 2004. "Technical Note: Statistical methods for accuracy assessment
of classified thematic maps".

Observations  
Overall count of observed cells (sum of both correct and incorrect
ones).

Correct  
Overall count of correct cells (cells with equal value in reference and
classification maps).

Overall accuracy  
Number of correct cells divided by overall cell count (expressed in
percent).

User's accuracy  
Share of correctly classified cells out of all cells classified as
belonging to specified class (expressed in percent). Inverse of
commission error.

Commission  
Commission error = 100 - user's accuracy.

Producer's accuracy  
Share of correctly classified cells out of all cells known to belong to
specified class (expressed in percent). Inverse of omission error.

Omission  
Omission error = 100 - producer's accuracy.

Kappa  
Choen's kappa index value.

Kappa variance  
Variance of kappa index. Correctness needs to be validated.

Conditional kappa  
Conditional user's kappa for specified class.

MCC  
Matthews (Mattheus) Correlation Coefficient is implemented according to
Grandini, M., Bagli, E., Visani, G. 2020. "Metrics for multi-class
classification: An overview."

## NOTES

It is recommended to reclassify categories of classified result map
layer into a more manageable number before running *r.kappa* on the
classified raster map layer. Because *r.kappa* calculates and then
reports information for each and every category.

*NA*'s in output mean it was not possible to calculate the value (e.g.
calculation would involve division by zero). In JSON output *NA*'s are
represented with value *null*. If there is no overlap between both maps,
a warning is printed and output values are set to 0 or *null*
respectively.

The **Estimated kappa value** in *r.kappa* is the value only for one
class, i.e. the observed agreement between the classifications for those
observations that have been classified by classifier 1 into the class i.
In other words, here the choice of reference is important.

It is calculated as:

kpp\[i\] = (pii\[i\] - pi\[i\] \* pj\[i\]) / (pi\[i\] - pi\[i\] \*
pj\[i\]);

where=

- pii\[i\] is the probability of agreement (i.e. number of pixels for
  which there is agreement divided by total number of assessed pixels)
- Pi\[i\] is the probability of classification i having classified the
  point as i
- Pj\[i\] is the probability of classification j having classified the
  point as i.

Some of reported values (overall accuracy, Choen's kappa, MCC) can be
misleading if cell count among classes is not balanced. See e.g. Powers,
D.M.W., 2012. "The Problem with Kappa"; Zhu, Q., 2020. "On the
performance of Matthews correlation coefficient (MCC) for imbalanced
dataset".

## EXAMPLE

Example for North Carolina sample dataset:

```sh
g.region raster=landclass96 -p
r.kappa -w classification=landuse96_28m reference=landclass96

# export Kappa matrix as CSV file "kappa.csv"
r.kappa classification=landuse96_28m reference=landclass96 output=kappa.csv -m -h
```

Verification of classified LANDSAT scene against training areas:

```sh
r.kappa -w classification=lsat7_2002_classes reference=training
```

## SEE ALSO

*[g.region](g.region.md), [r.category](r.category.md),
[r.mask](r.mask.md), [r.reclass](r.reclass.md), [r.report](r.report.md),
[r.stats](r.stats.md)*

## AUTHORS

Tao Wen, University of Illinois at Urbana-Champaign, Illinois  
Maris Nartiss, University of Latvia (JSON output, MCC)
