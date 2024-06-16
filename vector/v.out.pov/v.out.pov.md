## DESCRIPTION

*v.out.pov* converts GRASS vector data to POV-Ray format
(www.povray.com)

## EXAMPLE

::: code
    v.out.pov input=vector3d output=vector3d.pov objmod="pigment { color red 0 green 1 blue 0 }"
:::

The generated file can be included in an existing .pov file with the
following statement:

::: code
    #include "vector3d.pov"
:::

## REFERENCES

[POV-Ray](http://www.povray.com)

## SEE ALSO

*[r.out.pov](r.out.pov.html)*

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy
