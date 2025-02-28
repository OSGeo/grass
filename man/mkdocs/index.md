---
title: GRASS GIS 8.5.0dev Reference Manual
author: GRASS Development Team
---


## **What is GRASS?**

[GRASS](https://grass.osgeo.org/) is a geosptial processing engine for
advance analysis and visualization of geospatial data. It is a powerful tool for
processing and analyzing geospatial data sets. GRASS is a free and open source
software, released under an open source [GNU GPLed](https://www.gnu.org/licenses/gpl.html).

## **User and Development Community**

The [GRASS community](https://grass.osgeo.org/support/community/) of users and
developers, and is used in academic, government, and commercial settings around
the world. GRASS can be used for a wide range of applications, including
environmental modeling, land use planning, and spatial data analysis.

## :book: Basic Concepts

```mermaid
flowchart TB
    A@{ shape: docs, label: "grassdata"} --> P1[Project1]

    P1 --> P1P[PERMANENT]
   
    P1 -- sub-project --> P1M1[mapset1]
    P1M1 --> P1M1D@{ shape: docs, label: "Mutable Sub-project Data"}
    P1PD --> P1M1D
    P1M1D --> P1M1R(Raster)
    P1M1D --> P1M1V(Vector)
    P1M1D --> P1M1T(Temporal)

    P1P --> P1PD@{ shape: docs, label: "Immutable Shared Project Data"}
    P1PD --> P1PDR(Raster)
    P1PD --> P1PDV(Vector)

    P1 -- sub-project --> P1M2[mapset2]
    P1M2 --> P1M2D@{ shape: docs, label: "Mutable Sub-project Data"}
    P1PD --> P1M2D
    P1M2D --> P1M2R(Raster)
    P1M2D --> P1M2V(Vector)
    P1M2D --> P1M2T(Temporal)
```

- [Basic Concepts](helptext.md)
- [Project Managment](grass_database.md)
- [Projections and Spatial Transformations](projectionintro.md)

## :books: Advanced Concepts

- [GRASS ASCII Vector Format](vectorascii.md)
- [GRASS Environment Variables](variables.md)
