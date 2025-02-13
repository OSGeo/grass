## DESCRIPTION

*v.out.ascii* converts a GRASS vector map in binary format to a GRASS
vector map in [ASCII format](vectorascii.md). Using flag **-o**
*v.out.ascii* output will be in old (version 4) ASCII format.

If the **output** parameter is not given then the data is sent to
standard output.

## NOTES

The *[v.in.ascii](v.in.ascii.md)* module performs the function of
*v.out.ascii* in reverse; i.e. it converts vector maps in ASCII format
to their binary format. These two companion modules are useful both for
importing and exporting vector maps between GRASS and other software,
and for transferring data between machines.

If old version is requested, the **output** files from *v.out.ascii* is
placed in the `$LOCATION/$MAPSET/dig_ascii/` and
`$LOCATION/$MAPSET/dig_att` directory.

If **layer \> 0** then only features with a category number will be
exported. Use *[v.category](v.category.md)* to add them if needed or
define **layer=-1** to export also features without category.

*v.out.ascii* in the old version mode (**-o**) does not copy the
`dig_cats` file associated with the binary vector **input** map to the
new **output** file name. The user must copy the `dig_cats` file to the
new **output** name if this is desired (e.g. using the UNIX *cp*
command).

It is possible to output the coordinates of vertices in a non-points
vector feature by first converting the vector feature to a points map
with *[v.to.points](v.to.points.md)* and then exporting with
*v.out.ascii* in **points** mode.

## EXAMPLES

### Standard mode

See [ASCII format](vectorascii.md) specification.

```sh
v.out.ascii input=quads format=standard

ORGANIZATION: US Army Const. Eng. Rsch. Lab
DIGIT DATE:   May 1987
DIGIT NAME:   grass
MAP NAME:     Quads
MAP DATE:     May 1987
MAP SCALE:    24000
OTHER INFO:
ZONE:         13
MAP THRESH:   18.288000
VERTI:
B  4
 599587.1820962 4914067.53414294
 589639.15126831 4913922.5687301
 589440.96838162 4927803.62500018
 599375.87959179 4927959.83330436
B  2
 599375.87959179 4927959.83330436
 599587.1820962 4914067.53414294
B  4
 599587.1820962 4914067.53414294
 609541.5508239 4914236.0597482
 609316.10665227 4928116.8490555
 599375.87959179 4927959.83330436
C  1 1
 594125.63    4921115.58
 1     1
C  1 1
 604433.84    4921087.1
 1     2
```

### Point mode

```sh
v.out.ascii input=quads format=point

594125.63|4921115.58|1
604433.84|4921087.1|2
```

Print also selected attributes:

```sh
v.out.ascii input=geodetic_pts format=point where="cat > 5 and cat <= 8" columns=GEOD_NAME

573638.06289275|271623.25042595|6|27 WC 6
574416.81289275|274116.65542595|7|27 WC 7
575301.31189275|275303.81342595|8|27 WC 8
```

To print all attributes type **columns=\***:

```sh
v.out.ascii input=geodetic_pts format=point where="cat > 5 and cat <= 8" columns=*
573638.06289275|271623.25042595|6|6|0.00000000|0.00000000|6|6|27 WC 6|573638.09200000|271623.24100000|0.00|0|1.00000000|1.00000000
574416.81289275|274116.65542595|7|7|0.00000000|0.00000000|7|7|27 WC 7|574416.84100000|274116.64900000|0.00|0|1.00000000|1.00000000
575301.31189275|275303.81342595|8|8|0.00000000|0.00000000|8|8|27 WC 8|575301.30600000|275303.82600000|0.00|0|1.00000000|1.00000000
```

### WKT mode

WKT is abbreviation for [Well-known
text](https://en.wikipedia.org/wiki/Well-known_text).

```sh
v.out.ascii input=quads format=wkt

POLYGON((599587.18209620 4914067.53414294, 589639.15126831 4913922.56873010,
         589440.96838162 4927803.62500018, 599375.87959179 4927959.83330436,
         599587.18209620 4914067.53414294))
POLYGON((599587.18209620 4914067.53414294, 599375.87959179 4927959.83330436,
         609316.10665227 4928116.84905550, 609541.55082390 4914236.05974820,
         599587.18209620 4914067.53414294))
```

## SEE ALSO

*[v.category](v.category.md), [v.in.ascii](v.in.ascii.md),
[v.to.points](v.to.points.md)*

[GRASS ASCII vector format](vectorascii.md) specification  
[GRASS SQL interface](sql.md)

## AUTHORS

Michael Higgins, U.S. Army Construction Engineering Research
Laboratory  
James Westervelt, U.S. Army Construction Engineering Research
Laboratory  
Radim Blazek, ITC-Irst, Trento, Italy  
Attribute selection added by Martin Landa, Czech Technical University in
Prague, Czech Republic (2008/12)
