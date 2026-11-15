## DESCRIPTION

*v.what.vect* transfers attributes from the **query_map**'s attribute
table into the **map**'s attribute table. The module can be used to
transfer attributes from the table of a polygon map into the attribute
table of a point vector map, as well as the other way around, i.e., from
a point map into the attribute table of a polygon map (See examples).
The script is based on *v.distance*.

## NOTES

The upload **column** into which the query results are stored must be
present in **map**. Use *v.db.addcolumn* to add one if needed.

Use the **dmax** parameter to control the query distance tolerance,
i.e., how far **map**'s points/centroids can be from the **query_map**
features. For further options, use *v.distance*.

In case of a multipoint input **map** with several points having the
same category number, it can happen that the query result is NULL if the
same category number falls into different **query_map** polygons.

When transferring attributes from a point map into a polygon map,
**dmax** has to be larger than zero, i.e., it will be determined by the
distance between query points and polygon centroids. Importantly,
distance is in meters for latitude-longitude projects.

In case that one or both input vector maps are 3D, features need to
touch also in the 3rd dimension (z coordinate) in order to transfer
attributes.

## EXAMPLES

In this example, the 'hospitals' point map in the North Carolina dataset
is copied to the current mapset, a new attribute column is added and the
urban names from the 'urbanarea' polygon map are transferred to hospital
points locations in 'myhospitals' map:

```sh
g.copy vect=hospitals,myhospitals
v.db.addcolumn myhospitals column="urb_name varchar(25)"
v.what.vect myhospitals query_map=urbanarea column=urb_name query_column=NAME
# verification:
v.db.select myhospitals
```

In this example, city names, population data and others from
[Geonames.org country files](https://download.geonames.org/export/dump/)
are transferred to selected EU CORINE landuse/landcover classes
("Continuous urban fabric", 111, and "Discontinuous urban fabric", 112).
Note: The example is in UTM projection to which the input maps have been
projected beforehand.

```sh
# extract populated places from geonames
v.extract geonames_IT where="featurecla='P'" output=geonames_IT_cities
# add new column
v.db.addcol corine_code111_112_cities column="gnameid double precision"
# transfer geonameid (3000m maximal distance between points and centroids)
v.what.vect corine_code111_112_cities query_map=geonames_IT_cities column=gnameid \
            query_column=geonameid dmax=3000
# now gnameid can be used for v.db.join to join further
# attributes from geonames.org
```

## SEE ALSO

*[v.db.addcolumn](v.db.addcolumn.md), [v.db.select](v.db.select.md),
[v.distance](v.distance.md), [v.rast.stats](v.rast.stats.md),
[v.what.rast](v.what.rast.md), [v.what.rast3](v.what.rast3.md),
[v.vect.stats](v.vect.stats.md)*

## AUTHOR

Markus Neteler
