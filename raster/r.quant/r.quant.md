## DESCRIPTION

*r.quant* produces the quantization file for a floating-point map.

The *map* parameter defines the map for which the rules are to be
created. If more than one map is specified, then this implies that the
floating-point range is the minimum and maximum of all the maps
together, unless either basemap=map or fprange=min,max is specified.

### Quant rules

The quant rules have to be entered interactively.

If rules is specified, the input has the form:

value1:value2:cat1:\[cat2\]

where value1 and value2 are floating point values and cat1 cand cat2 are
integers. If cat2 is missing, it is taken to be equal to cat1. All
values can be \"\*\" which means infinity.

## NOTE

It is an error for both basemap and fprange to be specified.

## SEE ALSO

*[r.support](r.support.html)*, *[r.null](r.null.html)*

## AUTHOR

Michael Shapiro, Olga Waupotitsch, U.S.Army Construction Engineering
Research Laboratory
