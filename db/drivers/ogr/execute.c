/*!
  \file db/drivers/execute.c
  
  \brief Low level OGR SQL driver
 
  (C) 2011 by the GRASS Development Team
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com> (2011/07)
*/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <ogr_api.h>
#include <cpl_error.h>

#include "globals.h"
#include "proto.h"

static int parse_sql_update(const char *, char **, column_info **, int *, char **);

int db__driver_execute_immediate(dbString * sql)
{
    char *where, *table;
    int res, ncols, i;
    column_info *cols;

    OGRLayerH hLayer;
    OGRFeatureH hFeature;
    OGRFeatureDefnH hFeatureDefn;
    OGRFieldDefnH hFieldDefn;
    
    G_debug(1, "db__driver_execute_immediate():");
    
    G_debug(3, "\tSQL: '%s'", db_get_string(sql));
    
    /* try RDBMS SQL */
    OGR_DS_ExecuteSQL(hDs, db_get_string(sql), NULL, NULL);
    if (CPLGetLastErrorType() == CE_None)
	return DB_OK;
    
    /* parse UPDATE statement */
    res = parse_sql_update(db_get_string(sql), &table, &cols, &ncols, &where);
    G_debug(3, "\tUPDATE: table=%s, where=%s, ncols=%d", table, where ? where : "", ncols);
    if (res != 0)
	return DB_FAILED;
    
    /* get OGR layer */
    hLayer = OGR_DS_GetLayerByName(hDs, table);
    if (!hLayer) {
	db_d_append_error(_("OGR layer <%s> not found"), table);
	db_d_report_error();
	return DB_FAILED;
    }
    
    if (where)
	OGR_L_SetAttributeFilter(hLayer, where);
    
    /* get columns info */
    hFeatureDefn = OGR_L_GetLayerDefn(hLayer);
    for (i = 0; i < ncols; i++) {
	cols[i].index = OGR_FD_GetFieldIndex(hFeatureDefn, cols[i].name);
	if (cols[i].index < 0) {
	    db_d_append_error(_("Column <%s> not found in table <%s>"),
			      cols[i].name, table);
	    db_d_report_error();
	    return DB_FAILED;
	}
	cols[i].qindex = OGR_FD_GetFieldIndex(hFeatureDefn, cols[i].value);
	hFieldDefn = OGR_FD_GetFieldDefn(hFeatureDefn, cols[i].index);
	cols[i].type = OGR_Fld_GetType(hFieldDefn);

	G_debug(3, "\t\tcol=%s, val=%s, idx=%d, type=%d, qidx=%d",
		cols[i].name, cols[i].value, cols[i].index, cols[i].type,
		cols[i].qindex);
    }
    
    /* update features */
    OGR_L_ResetReading(hLayer);
    while(TRUE) {
	char *value;
	
	hFeature = OGR_L_GetNextFeature(hLayer);
	if (!hFeature)
	    break;
	G_debug(5, "\tfid=%ld", OGR_F_GetFID(hFeature));
	
	for (i = 0; i < ncols; i++) {
	    if (cols[i].qindex > -1) {
		value = (char *)OGR_F_GetFieldAsString(hFeature, cols[i].qindex);
	    }
	    else {
		if ((cols[i].type != OFTInteger ||
#if GDAL_VERSION_NUM >= 2000000
		     cols[i].type != OFTInteger64 ||
#endif
		     cols[i].type != OFTReal) && *(cols[i].value) == '\'') {
		    value = G_strchg(cols[i].value, '\'', ' ');
		    G_strip(value);
		}
		else {
		    value = cols[i].value;
		}
	    }
	    OGR_F_SetFieldString(hFeature, cols[i].index, value);
	}
	OGR_L_SetFeature(hLayer, hFeature);
	OGR_F_Destroy(hFeature);
    }
    
    G_free(table);
    G_free(where);
    for (i = 0; i < ncols; i++) {
	G_free(cols[i].name);
	G_free(cols[i].value);
    }
    
    return DB_OK;
}

int parse_sql_update(const char *sql, char **table, column_info **cols, int *ncols, char **where)
{
    int nprefix, n;
    int has_where;
    char *prefix;
    char *p, *w, *c, *t;
    char **token, **itoken;

    prefix = "UPDATE";
    nprefix = strlen(prefix);
    if (G_strncasecmp(sql, prefix, nprefix) != 0)
	return 1;
	
    p = (char *) sql + nprefix; /* skip 'UPDATE' */
    if (*p != ' ')
	return 1;
    p++;
    
    /* table */
    t = strchr(p, ' ');
    n = t - p;
    *table = G_malloc(n + 1);
    strncpy(*table, p, n);
    (*table)[n] = '\0';
    G_strip(*table);
    
    p += n;
    if (*p != ' ')
	return 1;
    p++;

    if (G_strncasecmp(p, "SET", 3) != 0)
	return 1;

    p += 3; /* skip 'SET' */

    if (*p != ' ')
	return 1;
    p++;
    
    w = G_strcasestr(p, "WHERE");
    
    if (!w) {
	has_where = FALSE;
	w = (char *)sql + strlen(sql);
    }
    else {
	has_where = TRUE;
    }
    
    /* process columns & values */
    n = w - p;
    c = G_malloc(n + 1);
    strncpy(c, p, n);
    c[n] = '\0';
    
    token = G_tokenize2(c, ",", "'");
    *ncols = G_number_of_tokens(token);
    *cols = (column_info *)G_malloc(sizeof(column_info) * (*ncols));
    
    for (n = 0; n < (*ncols); n++) {
	G_chop(token[n]);
	itoken = G_tokenize(token[n], "=");
	if (G_number_of_tokens(itoken) != 2)
	    return FALSE;
	G_strip(itoken[0]);
	G_strip(itoken[1]);
	(*cols)[n].name  = G_store(itoken[0]);
	(*cols)[n].value = G_store(itoken[1]);
	G_free_tokens(itoken);
    }
    
    G_free_tokens(token);
    G_free(c);

    if (!has_where) {
	*where = NULL;
	return 0;
    }
    
    /* where */
    w += strlen("WHERE");
    if (*w != ' ')
	return 1;
    w++;

    G_strip(w);
    *where = G_store(w);
    
    return 0;
}
