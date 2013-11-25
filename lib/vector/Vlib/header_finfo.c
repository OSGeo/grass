/*!
   \file lib/vector/Vlib/header_finfo.c

   \brief Vector library - header manipulation (relevant for external
   formats)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author Update to GRASS 7 (OGR/PostGIS support) by Martin Landa <landa.martin gmail.com>
*/

#include <string.h>

#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Get datasource name (relevant only for non-native formats)

   Returns:
    - datasource name for OGR format (GV_FORMAT_OGR and GV_FORMAT_OGR_DIRECT)
    - database name for PostGIS format (GV_FORMAT_POSTGIS)
   
   \param Map pointer to Map_info structure

   \return string containing OGR/PostGIS datasource name
   \return NULL on error (map format is native)
 */
const char *Vect_get_finfo_dsn_name(const struct Map_info *Map)
{
    if (Map->format == GV_FORMAT_OGR ||
        Map->format == GV_FORMAT_OGR_DIRECT) {
#ifndef HAVE_OGR
        G_warning(_("GRASS is not compiled with OGR support"));
#endif
        return Map->fInfo.ogr.dsn;
    }
    else if (Map->format == GV_FORMAT_POSTGIS) {
#ifndef HAVE_POSTGRES
        G_warning(_("GRASS is not compiled with PostgreSQL support"));
#endif
        return Map->fInfo.pg.db_name;
    }
    
    G_debug(1, "Native vector format detected for <%s>",
            Vect_get_full_name(Map));
    
    return NULL;
}

/*!
   \brief Get layer name (relevant only for non-native formats)

   Returns:
    - layer name for OGR format (GV_FORMAT_OGR and GV_FORMAT_OGR_DIRECT)
    - table name for PostGIS format (GV_FORMAT_POSTGIS) including schema (\<schema\>.\<table\>)

   Note: allocated string should be freed by G_free()

   \param Map pointer to Map_info structure

   \return string containing layer name
   \return NULL on error (map format is native)
 */
char *Vect_get_finfo_layer_name(const struct Map_info *Map)
{
    char *name;
    
    name = NULL;
    if (Map->format == GV_FORMAT_OGR ||
        Map->format == GV_FORMAT_OGR_DIRECT) {
#ifndef HAVE_OGR
        G_warning(_("GRASS is not compiled with OGR support"));
#endif
        name = G_store(Map->fInfo.ogr.layer_name);
    }
    else if (Map->format == GV_FORMAT_POSTGIS) {
#ifndef HAVE_POSTGRES
        G_warning(_("GRASS is not compiled with PostgreSQL support"));
#endif
        G_asprintf(&name, "%s.%s", Map->fInfo.pg.schema_name,
                   Map->fInfo.pg.table_name);
    }
    else {
        G_debug(1, "Native vector format detected for <%s>",
                  Vect_get_full_name(Map));
    }
    
    return name;
}

/*!
  \brief Get format info as string (relevant only for non-native formats)

  \param Map pointer to Map_info structure
  
  \return string containing name of OGR format
  \return "PostgreSQL" for PostGIS format (GV_FORMAT_POSTGIS)
  \return NULL on error (or on missing OGR/PostgreSQL support)
*/
const char *Vect_get_finfo_format_info(const struct Map_info *Map)
{
    if (Map->format == GV_FORMAT_OGR ||
        Map->format == GV_FORMAT_OGR_DIRECT) {
#ifndef HAVE_OGR
        G_warning(_("GRASS is not compiled with OGR support"));
#else
        if (!Map->fInfo.ogr.ds)
            return NULL;

        return OGR_Dr_GetName(OGR_DS_GetDriver(Map->fInfo.ogr.ds));
#endif
    }
    else if (Map->format == GV_FORMAT_POSTGIS) {
#ifndef HAVE_OGR
        G_warning(_("GRASS is not compiled with PostgreSQL support"));
#else
        return "PostgreSQL";
#endif
    }
    
    return NULL;
}

/*!
  \brief Get geometry type as string (relevant only for non-native formats)

  Note: All inner spaces are removed, function returns feature type in
  lowercase.

  \param Map pointer to Map_info structure

  \return allocated string containing geometry type info
  (point, linestring, polygon, ...)
  \return NULL on error (map format is native)
*/
const char *Vect_get_finfo_geometry_type(const struct Map_info *Map)
{
    int dim;
    char *ftype, *ftype_tmp;
    
    ftype_tmp = ftype = NULL;
    if (Map->format == GV_FORMAT_OGR ||
        Map->format == GV_FORMAT_OGR_DIRECT) {
#ifndef HAVE_OGR
    G_warning(_("GRASS is not compiled with OGR support"));
#else
    OGRwkbGeometryType Ogr_geom_type;
    OGRFeatureDefnH    Ogr_feature_defn;
    
    if (!Map->fInfo.ogr.layer)
        return NULL;

    dim = -1;
    
    Ogr_feature_defn = OGR_L_GetLayerDefn(Map->fInfo.ogr.layer);
    Ogr_geom_type = wkbFlatten(OGR_FD_GetGeomType(Ogr_feature_defn));
    
    ftype_tmp = G_store(OGRGeometryTypeToName(Ogr_geom_type));
#endif
    }
    else if (Map->format == GV_FORMAT_POSTGIS) {
#ifndef HAVE_POSTGRES
        G_warning(_("GRASS is not compiled with PostgreSQL support"));
#else
        char stmt[DB_SQL_MAX];
        
        const struct Format_info_pg *pg_info;
        
        PGresult *res;
                
        pg_info = &(Map->fInfo.pg);
        sprintf(stmt, "SELECT type,coord_dimension FROM geometry_columns "
                "WHERE f_table_schema = '%s' AND f_table_name = '%s'",
                pg_info->schema_name, pg_info->table_name);
        G_debug(2, "SQL: %s", stmt);
        
        res = PQexec(pg_info->conn, stmt);
        if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
            PQntuples(res) != 1) {
            G_debug(1, "Unable to get feature type: %s",
                    PQresultErrorMessage(res));
            return NULL;
        }
        ftype_tmp = G_store(PQgetvalue(res, 0, 0));
        dim = atoi(PQgetvalue(res, 0, 1));

        PQclear(res);
#endif
    }
    
    if (!ftype_tmp)
        return NULL;

    ftype = G_str_replace(ftype_tmp, " ", "");
    G_free(ftype_tmp);
    ftype_tmp = NULL;
    G_str_to_lower(ftype);

    if (dim == 3) {
        ftype_tmp = (char *) G_malloc(3 + strlen(ftype) + 1);
        sprintf(ftype_tmp, "3D %s", ftype);
        G_free(ftype);
        ftype = ftype_tmp;
    }

    return ftype;
}

/*!
  \brief Get header info for non-native formats

  \param Map pointer to Map_info structure
  
  \return pointer to Format_info structure
  \return NULL for native format
*/
const struct Format_info* Vect_get_finfo(const struct Map_info *Map)
{
    /* do not check Map-format which is native (see
     * GRASS_VECTOR_EXTERNAL_IMMEDIATE) */
    
    if (Map->fInfo.ogr.driver_name || Map->fInfo.pg.conninfo)
        return &(Map->fInfo);

    return NULL;
}

/*!
  \brief Get topology type (relevant only for non-native formats)

  \param Map pointer to Map_info structure
  \param[out] toposchema Topology schema name or NULL
  \param[out] topogeom   TopoGeometry column name or NULL
  \param[out] topo_geo_only TRUE for Topo-Geo data model or NULL

  \return GV_TOPO_NATIVE for native format
  \return GV_TOPO_PSEUDO for pseudo-topology
  \return GV_TOPO_POSTGIS for PostGIS Topology
*/
int Vect_get_finfo_topology_info(const struct Map_info *Map,
                                 char **toposchema, char **topogeom, int* topo_geo_only)
{
    if (Map->format == GV_FORMAT_OGR ||
        Map->format == GV_FORMAT_OGR_DIRECT) {
#ifndef HAVE_OGR
        G_warning(_("GRASS is not compiled with OGR support"));
#else
        return GV_TOPO_PSEUDO;
#endif
    }
    
    if (Map->format == GV_FORMAT_POSTGIS) {
        const struct Format_info_pg *pg_info;

        pg_info = &(Map->fInfo.pg);
        if (pg_info->toposchema_name) {
            if (toposchema)
                *toposchema = G_store(pg_info->toposchema_name);
            if (topogeom)
                *topogeom = G_store(pg_info->topogeom_column);
            if (topo_geo_only)
                *topo_geo_only = pg_info->topo_geo_only;
            
            return GV_TOPO_POSTGIS;
        }
        else {
            return GV_TOPO_PSEUDO;
        }
    }

    return GV_TOPO_NATIVE;
}
