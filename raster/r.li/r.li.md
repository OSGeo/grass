---
name: r.li
description: Landscape structure analysis package overview
---

# Landscape structure analysis package overview

## DESCRIPTION

The *r.li* suite is a toolset for multiscale analysis of landscape
structure. It aims at implementing metrics as found in external software
for quantitative measures of landscape structure like FRAGSTATS
(McGarigal and Marks 1995).

The *r.li* suite offers a set of patch and diversity indices. It
supports analysis of landscapes composed of a mosaic of patches, but,
more generally, the modules work with any two-dimensional raster map
whose cell values are integer (e.g., 1, 2) or floating point (e.g., 1.1,
3.2) values. The *g.gui.rlisetup* module has options for controlling the
shape, size, number, and distribution of sampling areas used to collect
information about the landscape structure. Sampling area shapes can be
the entire map or a moving window of square, rectangular or circular
shape. The size of sampling areas can be changed, so that the landscape
can be analyzed at a variety of spatial scales simultaneously. Sampling
areas may be distributed across the landscape in a random, systematic,
or stratified-random manner, or as a moving window.

The *r.li* modules can calculate a number of measures that produce
single values as output (e.g. mean patch size in the sampling area), as
well as measures that produce a distribution of values as output (e.g.
frequency distribution of patch sizes in the sampling area). The results
are stored as raster maps.

All modules require configuration file which can be created by the
*g.gui.rlisetup* module which is a GUI tool providing a convenient way
to set all necessary parameters. This file can be used repetitively
saving user from the need to specify all parameters over and over again.

## NOTES

The general procedure to calculate an index from a raster map is
two-fold:

1. run *g.gui.rlisetup*: create a configuration file selecting the
    parts of raster map to be analyzed. This file allows re-running an
    analysis easily. It is stored on Windows in the directory
    `C:\Users\userxy\AppData\Roaming\GRASS8\r.li\`, on GNU/Linux in
    `$HOME/.grass8/r.li/`.
2. run one or more of the *r.li.**\[index\]*** modules (e.g.,
    *r.li.**patchdensity***) to calculate the selected index using on
    the areas selected on configuration file.

## EXAMPLES

Calculate a patch density index on the entire 'geology' raster map in
the Spearfish sample dataset, using a 5x5 moving window:

1. CREATE A NEW CONFIGURATION FILE
    1. run

        ```sh
        g.gui.rlisetup
        ```

    2. The main *g.gui.rlisetup* window is displayed, click on "New"

    3. The new configuration window is now displayed, enter the  
        configuration file name (e.g., "my_conf", do not use absolute
        paths)  

        Now the new configuration window is displayed.  
        Enter the configuration file name (e.g., "my_conf", do not use
        absolute paths)  
        and the name of raster map (e.g., "geology").  
        The other fields are not needed for this configuration.

    4. Click on "Setup sampling frame", select "Whole map layer" and
        click "OK"

    5. Click on "Setup sampling areas", select "Moving window" and
        click "OK"

    6. Click on "Use keyboard to enter moving window dimension"

    7. Select "Rectangle" and enter 5 in the "height" and "width"
        fields

    8. Click on "Save settings"

    9. Close the *g.gui.rlisetup* window
2. CALCULATE PATCHDENSITY INDEX
    1. set the region settings to the "`geology`" raster map:

        ```sh
        g.region raster=geology -p
        ```

    2. run *r.li.patchdensity*:

        ```sh
        r.li.patchdensity input=geology conf=my_conf out=patchdens
        ```

The resulting patch density is stored in "`patchdens`" raster map. You
can verify the result for example with contour lines:

```sh
r.contour in=patchdens out=patchdens step=5
d.rast patchdens
d.vect -c patchdens
```

Note that if you want to run another index with the same area
configuration, you don't have to create another configuration file. You
can also use the same area configuration file on another map. The
program rescale it automatically. For instance if you have selected a
5x5 sample area on 100x100 raster map, and you use the same
configuration file on a 200x200 raster map, then the sample area is
10x10.

## SEE ALSO

**GUI tools**:

- [g.gui.rlisetup](g.gui.rlisetup.md): Configuration editor for the
  `r.li.*` module where `*` is name of the index

**Patch indices**:

- Indices based on patch number:
  - [r.li.patchdensity](r.li.patchdensity.md): Calculates patch density
    index on a raster map, using a 4 neighbour algorithm
  - [r.li.patchnum](r.li.patchnum.md): Calculates patch number index on
    a raster map, using a 4 neighbour algorithm
- Indices based on patch dimension:
  - [r.li.mps](r.li.mps.md): Calculates mean patch size index on a
    raster map, using a 4 neighbour algorithm
  - [r.li.padcv](r.li.padcv.md): Calculates coefficient of variation of
    patch area on a raster map
  - [r.li.padrange](r.li.padrange.md): Calculates range of patch area
    size on a raster map
  - [r.li.padsd](r.li.padsd.md): Calculates standard deviation of patch
    area a raster map
- Indices based on patch shape:
  - [r.li.shape](r.li.shape.md): Calculates shape index on a raster map
- Indices based on patch edge:
  - [r.li.edgedensity](r.li.edgedensity.md): Calculates edge density
    index on a raster map, using a 4 neighbour algorithm
- Indices based on patch attributes:
  - [r.li.cwed](r.li.cwed.md): Calculates contrast Weighted Edge Density
    index on a raster map
  - [r.li.mpa](r.li.mpa.md): Calculates mean pixel attribute index on a
    raster map

**Diversity indices**:

- [r.li.dominance](r.li.dominance.md): Calculates dominance diversity
  index on a raster map
- [r.li.pielou](r.li.pielou.md): Calculates Pielou eveness index on a
  raster map
- [r.li.renyi](r.li.renyi.md): Calculates Renyi entropy on a raster map
- [r.li.richness](r.li.richness.md): Calculates richness diversity index
  on a raster map
- [r.li.shannon](r.li.shannon.md): Calculates Shannon diversity index on
  a raster map
- [r.li.simpson](r.li.simpson.md): Calculates Simpson diversity index on
  a raster map

**Core library**:

- [r.li.daemon](r.li.daemon.md): library with common functionality (not
  visible to the user)

## ADDING NEW INDICES

New indices can be defined and implemented by any C programmer, without
having to deal with all basic functions (IO etc.). The computing
architecture and the functions are clearly separated, thus allowing an
easy expandability. Every index is defined separately, placed in a
directory along with its Makefile for compiling it and a file
\<module_name\>.html which describes the index including a simple
example of use. See [r.li.daemon](r.li.daemon.md) for more information
about development.

## REFERENCES

- McGarigal, K., and B. J. Marks. 1995. FRAGSTATS: spatial pattern
  analysis program for quantifying landscape structure. USDA For. Serv.
  Gen. Tech. Rep. PNW-351 ([PDF](https://doi.org/10.2737/PNW-GTR-351)).
- Baker, W.L. and Y. Cai. 1992. The r.le programs for multiscale
  analysis of landscape structure using the GRASS geographical
  information system. Landscape Ecology 7(4):291-302.

## AUTHORS

Claudio Porta and Lucio Davide Spano, students of Computer Science,
University of Pisa (Italy).  
Commission from Faunalia Pontedera (PI)

Partially rewritten by Markus Metz
