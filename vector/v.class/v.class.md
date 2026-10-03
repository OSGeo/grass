## DESCRIPTION

*v.class* classifies vector attribute data into classes, for example for
thematic mapping. Classification can be on a column or on an expression
including several columns, all in the table linked to the vector map.
The user indicates the number of classes desired and the algorithm to
use for classification. Several algorithms are implemented for
classification: equal interval, standard deviation, quantiles, equal
probabilities, and a discontinuities algorithm developed by Jean-Pierre
Grimmeau at the Free University of Brussels (ULB). It can be used to
pipe class breaks into thematic mapping modules such as
*d.vect.thematic* (see example below);

## NOTES

The *equal interval* algorithm simply divides the range max-min by the
number of breaks to determine the interval between class breaks.

The *quantiles* algorithm creates classes which all contain
approximately the same number of observations.

The *standard deviations* algorithm creates class breaks which are a
combination of the mean +/- the standard deviation. It calculates a
scale factor (\<1) by which to multiply the standard deviation in order
for all of the class breaks to fall into the range min-max of the data
values.

The *equiprobabilites* algorithm creates classes that would be
equiprobable if the distribution was normal. If some of the class breaks
fall outside the range min-max of the data values, the algorithm prints
a warning and reduces the number of breaks, but the probabilities used
are those of the number of breaks asked for.

The *discont* algorithm systematically searches discontinuities in the
slope of the cumulative frequencies curve, by approximating this curve
through straight line segments whose vertices define the class breaks.
The first approximation is a straight line which links the two end nodes
of the curve. This line is then replaced by a two-segmented polyline
whose central node is the point on the curve which is farthest from the
preceding straight line. The point on the curve furthest from this new
polyline is then chosen as a new node to create break up one of the two
preceding segments, and so forth.

The problem of the difference in terms of units between the two axes is
solved by rescaling both amplitudes to an interval between 0 and 1. In
the original algorithm, the process is stopped when the difference
between the slopes of the two new segments is no longer significant
(alpha = 0.05). As the slope is the ratio between the frequency and the
amplitude of the corresponding interval, i.e. its density, this
effectively tests whether the frequencies of the two newly proposed
classes are different from those obtained by simply distributing the sum
of their frequencies amongst them in proportion to the class amplitudes.
In the GRASS implementation, the algorithm continues, but a warning is
printed.

The **-g** flag has been renamed to the **-b** flag. Please use the **-b**
flag to print class breaks. Support for using the **-g** flag for class
breaks is deprecated and will be removed in a future release.

## EXAMPLE

Classify column pop of map communes into 5 classes using quantiles:

```sh
v.class map=communes column=pop algo=qua nbclasses=5
```

This example uses population and area to calculate a population density
and to determine the density classes:

```sh
v.class map=communes column=pop/area algo=std nbclasses=5
```

The following example uses the output of v.class and feeds it directly
into *d.vect.thematic*:

```sh
d.vect.thematic -l map=communes2 column=pop/area \
    breaks=`v.class -b map=communes2 column=pop/area algo=std nbcla=5` \
    colors=0:0:255,50:100:255,255:100:50,255:0:0,156:0:0
```

This example classifies attribute in map bridges by YEAR_BUILT using the
standard deviations and outputs data in CSV format:

```sh
v.class map=bridges column=YEAR_BUILT algorithm=std nbclasses=5 format=csv
```

Possible output:

```text
from,to,frequency
1891.00000,1938.82969,750
1938.82969,1954.72284,1841
1954.72284,1970.61598,5556
1970.61598,1986.50913,1788
1986.50913,1997.00000,1003
```

This example classifies attribute in map bridges by YEAR_BUILT using the standard
deviations and uses pandas to output data:

```python
import pandas as pd
import grass.script as gs

data = gs.parse_command(
    "v.class",
    map="bridges",
    column="YEAR_BUILT",
    algorithm="std",
    nbclasses=5,
    format="json",
)

df = pd.DataFrame(data["intervals"])
print(df)
```

Possible output:

```text
          from           to  frequency
0  1891.000000  1938.829689        750
1  1938.829689  1954.722836       1841
2  1954.722836  1970.615983       5556
3  1970.615983  1986.509130       1788
4  1986.509130  1997.000000       1003
```

The JSON output looks like:

```json
{
    "classes": 5,
    "mean": 1962.6694093984274,
    "standard_deviation": 15.893146881275285,
    "breaks": [
        1938.8296890765146,
        1954.7228359577898,
        1970.6159828390651,
        1986.5091297203403
    ],
    "intervals": [
        {
            "from": 1891,
            "to": 1938.8296890765146,
            "frequency": 750
        },
        {
            "from": 1938.8296890765146,
            "to": 1954.7228359577898,
            "frequency": 1841
        },
        {
            "from": 1954.7228359577898,
            "to": 1970.6159828390651,
            "frequency": 5556
        },
        {
            "from": 1970.6159828390651,
            "to": 1986.5091297203403,
            "frequency": 1788
        },
        {
            "from": 1986.5091297203403,
            "to": 1997,
            "frequency": 1003
        }
    ]
}
```

## SEE ALSO

*[v.univar](v.univar.md), [d.vect.thematic](d.vect.thematic.md)*

## AUTHOR

Moritz Lennert
