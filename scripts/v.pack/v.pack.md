## DESCRIPTION

*v.pack* collects (packs) vector map elements and support files in GRASS
Database and creates an compressed file using *gzip* algorithm. This
file can be used to copy the vector map to another machine. The packed
file can be afterwards unpacked by *[v.unpack](v.unpack.md)*.

## NOTES

Name of the pack file is determined by default from **input** parameter.
Optionally the name can be given by **output** parameter.

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

*[v.unpack](v.unpack.md), [v.in.ogr](v.in.ogr.md), [g.copy](g.copy.md),
[v.proj](v.proj.md), [r.unpack](r.unpack.md)*

## AUTHOR

Luca Delucchi, Fondazione E. Mach (Italy), based on the *r.pack* code
