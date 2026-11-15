---
description: Support for landscape index calculations on raster
---

# Support for landscape index calculations on raster

## DESCRIPTION

This documentation is focused on scientists and developers who wants to
implement a new landscape index computation. Refer to the
[r.li](r.li.md) modules overview and [g.gui.rlisetup](g.gui.rlisetup.md)
module for user-focused documentation.

*r.li.daemon* provides support for landscape index calculations on
raster maps. It hides the management of areas, defined using
[g.gui.rlisetup](g.gui.rlisetup.md) command. It is not used like a
standalone program, but its functions are a library used by all
*r.li.\[index\]* commands.  
This description is a tutorial for new index definition.  
  
The developer has only to focus on a unique area, like in mathematical
definitions, and has to write a C implementation of it.  
The areas are defined using a *struct* called **area_des** and it
members are explained in the source code (doxygen) documentation.

To write a new index only two steps are needed:

1. Define a function and insert its declaration on file **index.h** in
    *r.li.daemon* folder, which contains all index declarations. This
    function must be of this kind:

    ```sh
            int index(int fd, char ** par, area_des ad, double * result)
        
    ```

    where:

    - *fd* is the raster map descriptor
    - *par* is a matrix for special parameter (like argv in main)
    - *ad* is the area descriptor
    - *result* is where to put the index calculation result

    This function has to return 1 on success and 0 otherwise. This
    function type is defined using typedef named `rli_func`.

2. Create a main for command line arguments parsing, and call the
    function

    ```sh
            int calculateIndex(char *file, rli_func *f,
                               char **parameters, char *raster, char *output);
        
    ```

    from the *r.li* library, for starting raster analysis.  
    It follows the meaning of parameters:

    - *file* name of configuration file created using
      [g.gui.rlisetup](g.gui.rlisetup.md)
    - *f* pointer to index function defined above
    - *parameters* pointer to index special parameters
    - *raster* name of raster to use
    - *output* output file name

Compile it using a changed Makefile based on the file for
*r.li.patchdensity*.

Refer to the *r.li* library documentation in the source code and
implementation of *r.li* modules for details and examples.

## NOTES

Using GRASS library function to access raster rows can slow down moving
windows execution. It is recommended to use  

```sh
RLI_get_cell_row(int, int, area_des)
RLI_get_fcell_row(int, int, area_des)
RLI_get_dcell_row(int, int, area_des)
```

to use an ad hoc build memory management developed to speed up the
system. The documentation is in doxygen files.

## SEE ALSO

*[old r.le
manual](https://grass.osgeo.org/gdp/landscape/r_le_manual5.pdf)*  
*[r.li](r.li.md)* - package overview  
*[g.gui.rlisetup](g.gui.rlisetup.md)*

## REFERENCES

McGarigal, K., and B. J. Marks. 1995. FRAGSTATS: spatial pattern
analysis program for quantifying landscape structure. USDA For. Serv.
Gen. Tech. Rep. PNW-351. ([PDF](https://doi.org/10.2737/PNW-GTR-351))

## AUTHORS

Claudio Porta and Lucio Davide Spano, students of Computer Science
University of Pisa (Italy).  
Commission from Faunalia Pontedera (PI)  
Rewritten from "r.le.setup" by William L. Baker  
Various bug fixes by Markus Metz
