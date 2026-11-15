## DESCRIPTION

*g.remove* removes data files matching a pattern given by wildcards or
POSIX Extended Regular Expressions. If the **-f** force flag is not
given then nothing is removed, instead the list of selected file names
is printed to `stdout` as a preview of the files to be deleted.

## EXAMPLES

Delete `map1` and `map2` raster maps in the current mapset:

```sh
g.remove -f type=raster name=tmp1,tmp2
```

Delete all raster and vector maps starting with "`tmp_`" in the current
mapset:

```sh
# show matching raster and vector maps but do not delete yet (as verification)
g.remove type=raster,vector pattern="tmp_*"

# actually delete the matching raster and vector maps
g.remove -f type=raster,vector pattern="tmp_*"
```

Delete all vector maps starting with "`stream_`" in the current mapset,
but exclude those ending with "`_final`":

```sh
g.remove -f type=vector pattern="stream_*" exclude="*_final"
```

## AUTHOR

Huidae Cho  
<grass4u@gmail.com>
