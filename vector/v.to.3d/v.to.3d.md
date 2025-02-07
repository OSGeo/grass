## DESCRIPTION

The *v.to.3d* module is used to transform 2D vector features to 3D.
Height (z-coordinate) of 3D vector features can be specified by
**height** parameter as fixed value or by **column** parameter.

The flag **-r** enables to perform reverse transformation, i.e.,
transform 3D vector to 2D by omitting z-coordinate. The height of input
3D features can be optionally stored in **column**.

## NOTES

When transforming 2D vector features to 3D based on attribute, all NULL
values are silently converted to height 0.0.

The reverse transformation, 2D to 3D, is possible for points and lines.
In the case of lines, the reverse transformation should be used only
when all vertices of a line have the same z-coordinate (for example
contours).

## EXAMPLES

### Transform 2D vector features to 3D

```sh
# convert z-values from string to double
v.db.addcolumn map=geodetic_pts columns="Z_VALUE_D double precision"
v.db.update map=geodetic_pts column=Z_VALUE_D qcolumn=Z_VALUE
v.db.select map=geodetic_pts columns=cat,Z_VALUE,Z_VALUE_D

# convert 2D vector point map to 3D based on attribute
v.to.3d input=geodetic_pts out=geodetic_pts_3d column=Z_VALUE_D
```

### Transform 3D vector features to 2D

```sh
v.to.3d -rt input=elev_lid792_bepts output=elev_lid_2d
```

## SEE ALSO

*[v.transform](v.transform.md), [v.extrude](v.extrude.md),
[v.drape](v.drape.md)*

## AUTHOR

Martin Landa, Czech Technical University in Prague, Czech Republic
