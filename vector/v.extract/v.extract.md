## DESCRIPTION

*v.extract* allows a user to select vector objects from an existing
vector map and creates a new map containing only the selected objects.
Database tables can be queried with SQL statements, if a connection is
established. Dissolving (optional) is based on the output categories. If
2 adjacent areas have the same output category, the boundary is removed.

If **cats**, **file**, **random** or **where** options are not
specified, all features of given type and layer are extracted.
Categories are not changed in that case.

## NOTES

Only features with a category number in the selected layer will be
extracted. So if you want to extract boundaries (which are usually
without category, as that information is normally held in the area's
centroid) you must first use *[v.category](v.category.md)* to add them,
or use **layer=-1**.

## EXAMPLES

The examples are intended for the North Carolina sample dataset:

### Extract areas by category number with dissolving \#1

```sh
v.extract -d cats=1,2,3,4 input=soils_wake output=soil_groupa type=area new=0
```

produces a new vector **soil_groupa**, containing those areas from
vector **soils** which have category numbers **1 thru 4**; any common
boundaries are dissolved, and all areas in the new map will be assigned
category number 0.

### Extract areas by category number with dissolving \#2

```sh
v.extract -d cats=1-4 input=soils_wake output=soil_groupa type=area new=-1
```

produces a new vector map **soil_groupa** containing the areas from
vector **soils** which have categories **1 thru 4**. Any common
boundaries are dissolved, all areas in the new map will retain their
original category numbers 1 thru 4, since **new** was set to -1.

### Extract all areas and assign the same category to all

```sh
v.extract input=soils_wake output=soil_groupa type=area new=1
```

produces a new vector map **soil_groupa** containing all areas from
**soils**. No common boundaries are dissolved, all areas of the new map
will be assigned category number 1.

### Extract vectors with SQL

```sh
v.extract input=markveggy.shp output=markveggy.1 new=13 \
  where="(VEGTYPE = 'Wi') or (VEGTYPE = 'PS') or (PRIME_TYPE='Wi')"
```

produces a new vector map with category number 13 if the SQL statement
is fulfilled.

### Extract vector features which have the given field empty

```sh
v.extract input=lakes output=lakes_gaps where="FTYPE is NULL"
```

### Extract vector features which have the given field not empty

```sh
v.extract input=lakes output=lakes_ftype where="FTYPE not NULL"
```

### Reverse extracting (behaves like selective vector objects deleting)

Remove meteorological stations from map which are located above 1000m:

```sh
# check what to delete:
v.db.select precip_30ynormals where="elev > 1000"

# perform reverse selection
v.extract -r input=precip_30ynormals output=precip_30ynormals_lowland \
  where="elev > 1000"

# verify
v.db.select precip_30ynormals_lowland
```

### Dissolving based on column attributes

```sh
# check column names:
v.info -c zipcodes_wake

# reclass based on desired column:
v.reclass input=zipcodes_wake output=zipcodes_wake_recl_nam column=ZIPNAME

# verify:
v.info -c zipcodes_wake_recl_nam
v.db.select zipcodes_wake_recl_nam

# dissolve:
v.extract -d input=zipcodes_wake_recl_nam output=zipcodes_wake_regions
```

This produces a new vector map with common boundaries dissolved where
the reclassed attributes of adjacent (left/right) areas are identical.

### Extract 3 random areas from geology map

```sh
v.extract input=geology output=random_geology type=area random=3
```

This creates a new map with three random categories matching areas. Note
that there may be more than one feature with the same category.

## SEE ALSO

*[v.category](v.category.md), [v.dissolve](v.dissolve.md),
[v.reclass](v.reclass.md), [GRASS SQL interface](sql.md)*

## AUTHORS

R.L. Glenn, USDA, SCS, NHQ-CGIS  
GRASS 6 port by Radim Blazek
