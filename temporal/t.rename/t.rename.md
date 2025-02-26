## DESCRIPTION

The *t.rename* module renames space time datasets of different types
(STRDS, STVDS, STR3DS) and updates the space time dataset register
entries of the registered maps.

## NOTES

Renaming of space time datasets works only for SQLite based temporal
databases.

## EXAMPLE

A new vector space time dataset will be created, renamed and in the end
removed

```sh
# Create new and empty STVDS
t.create type=stvds output=toberenamed semantictype=mean \
  title="Example to rename" \
  description="This is an example just to show how rename"

t.rename input=toberenamed output=newname type=stvds

t.remove input=newname type=stvds
```

## SEE ALSO

*[t.create](t.create.md), [t.support](t.support.md),
[t.register](t.register.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
