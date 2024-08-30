#ifndef GRASS_VECTOR_H
#define GRASS_VECTOR_H
#include <grass/gis.h>
#include <grass/vect/digit.h>

#ifdef HAVE_GEOS
#include <geos_c.h>
#if GEOS_VERSION_MAJOR < 3
typedef struct GEOSGeom_t GEOSGeometry;
typedef struct GEOSCoordSeq_t GEOSCoordSequence;
#endif
#endif

#include <grass/defs/vector.h>

#endif /* GRASS_VECTOR_H */
