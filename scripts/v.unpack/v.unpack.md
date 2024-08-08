## DESCRIPTION

*v.unpack* allows unpacking vector maps packed by
*[v.pack](v.pack.html)*.

## NOTES

Name of the vector map is determined by default from pack file
internals. Optionally the name can be given by **output** parameter.

## EXAMPLE

Pack up vector map *random_point* into *random_point.pack* file.

```
v.pack input=random_point
```

the vector map can be afterwards unpacked by

```
v.unpack input=random_point.pack
```

## SEE ALSO

*[v.pack](v.pack.html), [v.in.ogr](v.in.ogr.html),
[r.pack](r.pack.html)*

## AUTHOR

Luca Delucchi, Fondazione E. Mach (Italy), based on the *r.unpack* code
