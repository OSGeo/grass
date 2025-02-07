## DESCRIPTION

*i.ortho.transform* is an utility to compute transformation based upon
GCPs and output error measurements.

If coordinates are given with the **input** file option or fed from
`stdin`, both the input and the output format is "x y z" with one
coordinate pair per line. Reverse transform is performed with the **-r**
flag.

The **format** option determines how control points are printed out. A
summary on the control points can be printed with the **-s** flag. The
summary includes maximum deviation observed when transforming GCPs and
overall RMS. The **format** option is ignored when coordinates are given
with the **input** file option.

## NOTES

Ortho-transformation is a 2-step transformation. First, source
coordinates are transformed to sensor coordinates, then sensor
coordinates are transformed to target coordinates.

## SEE ALSO

*[i.rectify](i.rectify.md)*

## TODO

Update this document with x,y,z\<-\>E,N,H information

## AUTHORS

Brian J. Buckley  
Glynn Clements  
Hamish Bowman
