## DESCRIPTION

*v.pack* collects (packs) vector map elements and support files in GRASS
Database and creates an compressed file using *gzip* algorithm. This
file can be used to copy the vector map to another machine. The packed
file can be afterwards unpacked by *[v.unpack](v.unpack.html)*.

## NOTES

Name of the pack file is determined by default from **input** parameter.
Optionally the name can be given by **output** parameter.

## EXAMPLE

Pack up vector map *random_point* into *random_point.pack* file.

::: code
    v.pack input=random_point
:::

the vector map can be afterwards unpacked by

::: code
    v.unpack input=random_point.pack
:::

## SEE ALSO

*[v.unpack](v.unpack.html), [v.in.ogr](v.in.ogr.html),
[g.copy](g.copy.html), [v.proj](v.proj.html), [r.unpack](r.unpack.html)*

## AUTHOR

Luca Delucchi, Fondazione E. Mach (Italy), based on the *r.pack* code
