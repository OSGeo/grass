## DESCRIPTION

*m.measure* provides the user with a way to measure the lengths and
areas of lines and polygons. Areas can be stated in acres, hectares,
square miles, square feet, square meters and square kilometers.

## EXAMPLES

Distance example in a latitude-longitude coordinate reference system (on
great circle, i.e. an orthodrome):

```sh
Bonn_DE="7.09549,50.73438"
Philadelphia_US="-75.16379,39.95233"

m.measure coordinates="$Bonn_DE,$Philadelphia_US" units=kilometers
Length:  6217.916452 kilometers
```

![Visualization (with d.geodesic) of m.measure distance example](m_measure_distance.png)  
*Visualization (with d.geodesic) of m.measure
distance example*

As an example for the North Carolina sample dataset, here four points
describing a square of 1000m side length:

```sh
m.measure units=meters \
  coordinates=922000,2106000,923000,2106000,923000,2107000,922000,2107000
Length:  3000.000000 meters
Area:    1000000.000000 square meters

# script style output:
m.measure -g units=hectares \
  coordinates=922000,2106000,923000,2106000,923000,2107000,922000,2107000
units=meters,square meters
length=3000.000000
area=1000000.000000
```

## SEE ALSO

*[d.geodesic](d.geodesic.md)*

## AUTHORS

Glynn Clements  
Some updates by Martin Landa, CTU in Prague, Czech Republic  
  
Derived from d.measure by James Westervelt, Michael Shapiro, U.S. Army
Construction Engineering Research Laboratory
