## DESCRIPTION

*v.kcv* randomly divides a points lists into *k* sets of test/train data
(for **npartitions**-fold **c**ross **v**alidation). Test partitions are
mutually exclusive. That is, a point will appear in only one test
partition and *k-1* training partitions. The module generates a random
point using the selected random number generator and then finds the
closest point to it. This site is removed from the candidate list
(meaning that it will not be selected for any other test set) and saved
in the first test partition file. This is repeated until enough points
have been selected for the test partition. The number of points chosen
for test partitions depends upon the number of sites available and the
number of partitions chosen (this number is made as consistent as
possible while ensuring that all sites will be chosen for testing). This
process of filling up a test partition is done *k* times.

## NOTES

An ideal random sites generator will follow a Poisson distribution and
will only be as random as the original sites. This module simply divides
vector points up in a random manner.

Be warned that random number generation occurs over the intervals
defined by the current region of the map.

This program may not work properly with Lat-long data.

## EXAMPLES

All examples are based on the North Carolina sample dataset.

```sh
g.copy vect=geonames_wake,my_geonames_wake
v.kcv map=my_geonames_wake column=part npartitions=10
```

```sh
g.copy vect=geodetic_pts,my_geodetic_pts
v.kcv map=my_geodetic_pts column=part npartitions=10
```

## SEE ALSO

*[v.random](v.random.md), [g.region](g.region.md)*

## AUTHORS

James Darrell McCauley,  
when he was at: [Agricultural
Engineering](http://ABE.www.ecn.purdue.edu/ABE/) [Purdue
University](http://www.purdue.edu/)

27 Jan 1994: fixed RAND_MAX for Solaris 2.3  
13 Sep 2000: released under GPL  
Updated to 5.7 Radim Blazek 10 / 2004  
OGR support by Martin Landa (2009)  
Speed-up by Jan Vandrol and Jan Ruzicka (2013)
