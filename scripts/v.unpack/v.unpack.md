## DESCRIPTION

*v.unpack* allows unpacking vector maps packed by *[v.pack](v.pack.md)*.

## NOTES

Name of the vector map is determined by default from pack file
internals. Optionally the name can be given by **output** parameter.

## EXAMPLE

Pack up vector map *random_point* into *random_point.pack* file.

```sh
v.pack input=random_point
```

the vector map can be afterwards unpacked by

```sh
v.unpack input=random_point.pack
```

## SEE ALSO

*[v.pack](v.pack.md), [v.in.ogr](v.in.ogr.md), [r.pack](r.pack.md)*

## AUTHOR

Luca Delucchi, Fondazione E. Mach (Italy), based on the *r.unpack* code
