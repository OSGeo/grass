## DESCRIPTION

*r.colors.out* allows the user to export the color table for a raster
map to a file which is suitable as input to *[r.colors](r.colors.md)*.

## EXAMPLES

```sh
r.colors.out map=el_D782_6m rules=rules.txt
r.colors map=el_D783_6m rules=rules.txt
r.colors.out map=el_D782_6m rules=rules.json format=json
```

## SEE ALSO

*[r.colors](r.colors.md)*

## AUTHOR

Glynn Clements
