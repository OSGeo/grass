## Description

*v.ppa* allows users to perform point pattern analysis with the G, F, K, L, and
bivariate Ripley's K functions.

## Notes

## Examples

### G Function

Determine the distribution of distances from each point to its nearest neighbor
to learn if the points are clustered, dispersed, or randomly distributed across
the computational region.

```bash
v.ppa input=crash output=crash_g.csv method=g
```

### F Function

Determine the distribution of the distances from random points in the
computational region to the points in the input vector map. This function is to
determine if the points are clustered, dispersed, or randomly distributed across
the computational region. The output file will contain the distances from the
random points to the nearest crash location.

```bash
v.ppa input=crash output=crash_f.csv method=f
```

### K and L Functions

Determine if the points are clustered, dispersed, or randomly distributed across
patial scales in the computational region.

```bash
v.ppa input=crash output=crash_k.csv method=k
```

The *L-Function* is a transformation of the K-Function that allows for easier
interpretation of the results.

- If L(r) is less than the expected value, the points are dispersed.
- If L(r) is greater than the expected value, the points are clustered.
- If L(r) is equal to the expected value, the points are randomly distributed
(Poisson Process).

```bash
v.ppa input=crash output=crash_l.csv method=l
```

## Authors

Corey T. White, OpenPlains Inc.
