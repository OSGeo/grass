## DESCRIPTION

*v.what* outputs the category number value(s) associated with
user-specified location(s) in user-specified vector map layer(s). This
module was derived from the *d.what.vect* module by removing all
interactive code and modification of the output for easy parsing. Using
the *-g* flag generates script-style output which is easily parsable.

## EXAMPLE

North Carolina sample dataset example:

Query polygon at given position:

```
v.what zipcodes_wake coordinates=637502.25,221744.25
```

Find closest hospital to given position within given distance (search
radius):

```
v.what hospitals coordinates=542690.4,204802.7 distance=2000000
```

## SEE ALSO

*[d.what.rast](d.what.rast.html), [d.what.vect](d.what.vect.html),
[v.rast.stats](v.rast.stats.html), [v.vect.stats](v.vect.stats.html),
[v.what.rast](v.what.rast.html), [v.what.rast3](v.what.rast3.html),
[v.what.vect](v.what.vect.html)*

## AUTHOR

Trevor Wiens\
Edmonton, Alberta, Canada
