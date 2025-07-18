## DESCRIPTION

*r.regression.multi* calculates a multiple linear regression from raster
maps, according to the formula

```sh
Y = b0 + sum(bi*Xi) + E
```

where

```sh
X = {X1, X2, ..., Xm}
m = number of explaining variables
Y = {y1, y2, ..., yn}
Xi = {xi1, xi2, ..., xin}
E = {e1, e2, ..., en}
n = number of observations (cases)
```

In R notation:

```sh
Y ~ sum(bi*Xi)
b0 is the intercept, X0 is set to 1
```

*r.regression.multi* is designed for large datasets that can not be
processed in R. A p value is therefore not provided, because even very
small, meaningless effects will become significant with a large number
of cells. Instead it is recommended to judge by the estimator b, the
amount of variance explained (R squared for a given variable) and the
gain in AIC (AIC without a given variable minus AIC global must be
positive) whether the inclusion of a given explaining variable in the
model is justified.

### The global model

The *b* coefficients (b0 is offset), R squared or coefficient of
determination (Rsq) and F are identical to the ones obtained from
R-stats's lm() function and R-stats's anova() function. The AIC value is
identical to the one obtained from R-stats's stepAIC() function (in case
of backwards stepping, identical to the Start value). The AIC value
corrected for the number of explaining variables and the BIC (Bayesian
Information Criterion) value follow the logic of AIC.

### The explaining variables

R squared for each explaining variable represents the additional amount
of explained variance when including this variable compared to when
excluding this variable, that is, this amount of variance is explained
by the current explaining variable after taking into consideration all
the other explaining variables.

The F score for each explaining variable allows testing if the inclusion
of this variable significantly increases the explaining power of the
model, relative to the global model excluding this explaining variable.
That means that the F value for a given explaining variable is only
identical to the F value of the R-function *summary.aov* if the given
explaining variable is the last variable in the R-formula. While R
successively includes one variable after another in the order specified
by the formula and at each step calculates the F value expressing the
gain by including the current variable in addition to the previous
variables, *r.regression.multi* calculates the F-value expressing the
gain by including the current variable in addition to all other
variables, not only the previous variables.

The AIC value is identical to the one obtained from the R-function
stepAIC() when excluding this variable from the full model. The AIC
value corrected for the number of explaining variables and the BIC value
(Bayesian Information Criterion) value follow the logic of AIC. BIC is
identical to the R-function stepAIC with k = log(n). AICc is not
available through the R-function stepAIC.

## EXAMPLE

Multiple regression with soil K-factor and elevation, aspect, and slope
(North Carolina dataset). Output maps are the residuals and estimates:

```sh
g.region raster=soils_Kfactor -p
r.regression.multi mapx=elevation,aspect,slope mapy=soils_Kfactor \
  residuals=soils_Kfactor.resid estimates=soils_Kfactor.estim
```

Using the JSON format option and Python to parse the output:

```python
import grass.script as gs

data = gs.parse_command(
    "r.regression.multi",
    mapx=["elevation", "aspect", "slope"],
    mapy="soils_Kfactor",
    format="json",
)
print(data)
```

Possible JSON Output:

```json
{
 "n": 525000,
 "Rsq": 0.115424,
 "Rsqadj": 0.115419,
 "RMSE": 0.026785,
 "MAE": 0.020049,
 "F": 22834.749577,
 "b0": 0.050201,
 "AIC": -3800909.191798,
 "AICc": -3800909.191798,
 "BIC": -3800864.507184,
 "predictors": [
  {
   "name": "elevation",
   "b": 0.001523,
   "F": 64152.655957,
   "AIC": -3740385.046757,
   "AICc": -3740385.046757,
   "BIC": -3740364.70445
  },
  {
   "name": "aspect",
   "b": 2.2e-05,
   "F": 4922.480908,
   "AIC": -3796011.607461,
   "AICc": -3796011.607461,
   "BIC": -3795991.265154
  },
  {
   "name": "slope",
   "b": 0.002853,
   "F": 11927.479106,
   "AIC": -3789117.096287,
   "AICc": -3789117.096287,
   "BIC": -3789096.75398
  }
 ]
}
```

## SEE ALSO

*[d.correlate](d.correlate.md),
[r.regression.line](r.regression.line.md), [r.stats](r.stats.md)*

## AUTHOR

Markus Metz
