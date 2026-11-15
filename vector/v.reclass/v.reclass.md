## DESCRIPTION

*v.reclass* allows user to create a new vector map based on the
reclassification of an existing vector map. It also allows the user to
change the *key column* away from the default of "**cat**" with the
**column** option.

Rules file may contain on each row either pair:

```sh
keyword value
```

(separated by space) or comment beginning with '#' (hash). Definition of
new category begins with keyword *cat* followed by the new category
value. Keyword *where* specifies SQL where condition.

## NOTES

No table is created for the reclassed map if the **column** option is
used and the column type is integer (as the result could contain
ambiguities). If the **column** option is used and the column type is
string, a new table is created containing the newly generated cat
numbers and a single column containing the unique string column values,
sorted in alphabetical order.

For dissolving common boundaries, see *[v.dissolve](v.dissolve.md)*.

Either the **rules** or **column** option must be specified.

## EXAMPLES

### Example 1: Reclass by rules

```sh
v.reclass input=land output=land_u type=boundary rules=land.rcl
```

The rules file contains:

```sh
# land reclass file
cat 1
where use = 'E13' and owner = 'Jara Cimrman'
cat 2
where use = 'E14'
```

Produces a new vector area map *land_u* containing boundaries from
*land* with area category values selected from database by SQL select
statement:  
`select id from tland where use = 'E13' and owner = 'Jara Cimrman'`
changed to category 1;  
values selected from database by SQL select statement:  
`select id from tland where use = 'E14'` changed to category 2.

### Example 2: Reclass by attribute column

(North Carolina sample dataset)  

```sh
v.reclass in=streams out=streams_by_type column=I_vs_P

v.db.select streams_by_type
cat|I_vs_P
1|intermittent
2|perennial
```

## KNOWN ISSUES

No table is created for reclassed layer if the **rules** option is used.

## SEE ALSO

*[v.dissolve](v.dissolve.md), [v.extract](v.extract.md)*

*[GRASS SQL interface](sql.md)*

## AUTHORS

R.L. Glenn, USDA, SCS, NHQ-CGIS  
from v.reclass to v.db.reclass and later to v.reclass in 5.7 rewritten
by Radim Blazek
