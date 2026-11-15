## DESCRIPTION

*r.quant* produces the quantization file for a floating-point map.

The *map* parameter defines the map for which the rules are to be
created. If more than one map is specified, then this implies that the
floating-point range is the minimum and maximum of all the maps
together, unless either *basemap=map* or *fprange=min,max* is specified.

### Quant rules

The quant rules have to be entered interactively.

If rules is specified, the input has the form:

```sh
value1:value2:cat1:[cat2]
```

where *value1* and *value2* are floating point values and *cat1* and
*cat2* are integers. If *cat2* is missing, it is taken to be equal to
*cat1*. All values can be *"\*"* which means infinity.

## NOTE

It is an error to specify both *basemap* and *fprange*.

## SEE ALSO

*[r.support](r.support.md), [r.null](r.null.md)*

## AUTHORS

Michael Shapiro, Olga Waupotitsch, U.S.Army Construction Engineering
Research Laboratory
