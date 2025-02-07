## DESCRIPTION

*v.support* is used to set/update vector map metadata. While GRASS GIS
typically generates these metadata entries automatically, *v.support*
allows users to manually edit them when necessary.

## EXAMPLE

```sh
# update scale to 1:24000
v.support myvectmap scale=24000

# update organization
v.support myvectmap organization="OSGeo labs"
v.info myvectmap
```

## SEE ALSO

*[v.build](v.build.md), [v.info](v.info.md)*

## AUTHOR

Markus Neteler, Trento
