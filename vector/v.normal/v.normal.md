## DESCRIPTION

*v.normal* computes tests of normality on vector points.

## NOTES

The tests that *v.normal* performs are indexed below. The tests that are
performed are specified by giving an index, ranges of indices, or
multiple thereof.

1. Sample skewness and kurtosis
2. Geary's a-statistic and an approximate normal transformation
3. Extreme normal deviates
4. D'Agostino's D-statistic
5. Modified Kuiper V-statistic
6. Modified Watson U^2-statistic
7. Durbin's Exact Test (modified Kolmogorov)
8. Modified Anderson-Darling statistic
9. Modified Cramer-Von Mises W^2-statistic
10. Kolmogorov-Smirnov D-statistic (modified for normality testing)
11. Chi-Square test statistic (equal probability classes) and the number
    of degrees of freedom
12. Shapiro-Wilk W Test
13. Weisberg-Binghams W'' (similar to Shapiro-Francia's W')
14. Royston's extension of W for large samples
15. Kotz Separate-Families Test for Lognormality vs. Normality

## EXAMPLE

Compute the sample skewness and kurtosis, Geary's a-statistic and an
approximate normal transformation, extreme normal deviates, and
Royston's W for the *random* vector points:

```sh
g.region raster=elevation -p
v.random random n=200
v.db.addtable random column="elev double precision"
v.what.rast random rast=elevation column=elev
v.normal random tests=1-3,14 column=elev
```

## SEE ALSO

*[v.univar](v.univar.md)*

## AUTHORS

[James Darrell McCauley](http://mccauley-usa.com/)
[\<darrell@mccauley-usa.com\>](mailto:darrell@mccauley-usa.com),  
when he was at: [Agricultural
Engineering](https://engineering.purdue.edu/ABE/) [Purdue
University](http://www.purdue.edu/)
