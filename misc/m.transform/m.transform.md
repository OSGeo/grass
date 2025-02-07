## DESCRIPTION

*m.transform* is an utility to compute transformation based upon GCPs
and output error measurements.

If coordinates are given with the **input** file option or fed from
`stdin`, both the input and the output format is "x y" with one
coordinate pair per line. Reverse transform is performed with the **-r**
flag.

The **format** option determines how control points are printed out. A
summary on the control points can be printed with the **-s** flag. The
summary includes maximum deviation observed when transforming GCPs and
overall RMS. The **format** option is ignored when coordinates are given
with the **input** file option.

## NOTES

The transformations are:

order=1:

```sh
    e = [E0 E1][1]·[1]
        [E2  0][e] [n]

    n = [N0 N1][1]·[1]
        [N2  0][e] [n]
```

order=2:

```sh
    e = [E0 E1 E3][1 ] [1 ]
        [E2 E4  0][e ]·[n ]
        [E5  0  0][e²] [n²]

    n = [N0 N1 N3][1 ] [1 ]
        [N2 N4  0][e ]·[n ]
        [N5  0  0][e²] [n²]
```

order=3:

```sh
    e = [E0 E1 E3 E6][1 ] [1 ]
        [E2 E4 E7  0][e ]·[n ]
        [E5 E8  0  0][e²] [n²]
        [E9  0  0  0][e³] [n³]

    n = [N0 N1 N3 N6][1 ] [1 ]
        [N2 N4 N7  0][e ]·[n ]
        [N5 N8  0  0][e²] [n²]
        [N9  0  0  0][e³] [n³]
```

\["·" = dot-product, (AE)·N = N'EA\]

In other words, *order=1* and *order=2* are equivalent to *order=3* with
the higher coefficients equal to zero.

## SEE ALSO

*[i.ortho.transform](i.ortho.transform.md), [i.rectify](i.rectify.md),
[v.rectify](v.rectify.md), [v.transform](v.transform.md)*

## AUTHORS

Brian J. Buckley  
Glynn Clements  
Hamish Bowman
