/*!
  \file lib/vector/Vlib/field.c
  
  \brief Vector library - field (layer) related fns.
  
  Higher level functions for reading/writing/manipulating vectors.
  
  \todo see Vect_read_dblinks(); activate auto-FID detection once
  OGR_L_GetFIDColumn() is working or solution found if FID not
  available
  
  (C) 2001-2009, 2011-2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  \author Update to GRASS 5.7 by Radim Blazek and David D. Gray.
  \author Various updates by Martin Landa <landa.martin gmail.com>, 2009-2011
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include <grass/vector.h>

#include "local_proto.h"

#ifdef HAVE_GDAL
#include <gdal_version.h>	/* needed for FID detection */
#endif	/* HAVE_GDAL */

#ifdef HAVE_OGR
#include <ogr_api.h>
#endif

/*!
  \brief Create and init new dblinks structure
  
  \return pointer to new dblinks structure
  \return NULL on failure
 */
struct dblinks *Vect_new_dblinks_struct(void)
{
    struct dblinks *p;

    p = (struct dblinks *)G_malloc(sizeof(struct dblinks));

    if (p) {
	/* initialize members */
	G_zero(p, sizeof(struct dblinks));
    }

    return p;
}

/*!
  \brief Reset dblinks structure (number of fields)

  \param p pointer to existing dblinks structure
*/
void Vect_reset_dblinks(struct dblinks *p)
{
    p->n_fields = 0;
}

/*!
  \brief Add new db connection to Map_info structure

  \param Map pointer to Map_info structure
  \param number layer number
  \param name layer name (if not given use table name)
  \param table table name
  \param key key name
  \param db database name
  \param driver driver name

  \return 0 on success
  \return -1 on failure
*/
int Vect_map_add_dblink(struct Map_info *Map, int number, const char *name,
			const char *table, const char *key, const char *db,
			const char *driver)
{
    int ret;

    if (number < 1) {
	G_warning(_("Layer number must be 1 or greater"));
	return -1;
    }

    if (Map->mode != GV_MODE_WRITE && Map->mode != GV_MODE_RW) {
	G_warning(_("Unable to add attribute link, vector map is "
		    "not opened in WRITE mode"));
	return -1;
    }

    ret = Vect_add_dblink(Map->dblnk, number, name, table, key, db, driver);
    if (ret == -1) {
	G_warning(_("Unable to add attribute link"));
	return -1;
    }
    /* write it immediately otherwise it is lost if module crashes */
    ret = Vect_write_dblinks(Map);
    if (ret == -1) {
	G_warning(_("Unable to write attribute links"));
	return -1;
    }
    return 0;
}

/*!
  \brief Delete db connection from Map_info structure
  
  \param Map pointer to Map_info structure
  \param field layer number
  
  \return 0 deleted
  \return -1 error
 */
int Vect_map_del_dblink(struct Map_info *Map, int field)
{
    int i, j, ret;
    struct dblinks *links;

    G_debug(4, "Vect_map_del_dblink() field = %d", field);
    links = Map->dblnk;

    ret = -1;
    for (i = 0; i < links->n_fields; i++) {
	if (links->field[i].number == field) {	/* field found */
	    for (j = i; j < links->n_fields - 1; j++) {
		links->field[j].number = links->field[j + 1].number;
		links->field[j].name = links->field[j + 1].name;
		links->field[j].table = links->field[j + 1].table;
		links->field[j].key = links->field[j + 1].key;
		links->field[j].database = links->field[j + 1].database;
		links->field[j].driver = links->field[j + 1].driver;
	    }
	    ret = 0;
	    links->n_fields--;
	}
    }

    if (ret == -1)
	return -1;

    /* write it immediately otherwise it is lost if module crashes */
    ret = Vect_write_dblinks(Map);
    if (ret == -1) {
	G_warning(_("Unable to write database links"));
	return -1;
    }

    return 0;
}

/*!
  \brief Copy DB links from input vector map to output vector map

  \param In pointer to Map_info structure (input)
  \param Out pointer to Map_info structure (output)
  \param first_only TRUE to copy only first link otherwise all DB links are copied
*/
void Vect_copy_map_dblinks(const struct Map_info *In, struct Map_info *Out,
			   int first_only)
{
    int i, ndblinks;
    struct field_info *Fi;
    
    ndblinks = Vect_get_num_dblinks(In);
    for (i = 0; i < ndblinks; i++) {
	Fi = Vect_get_dblink(In, 0);
	if (!Fi) {
	    G_warning(_("Database connection not defined. Skipping."));
	    continue;
	}
	Vect_map_add_dblink(Out, Fi->number, Fi->name, Fi->table, Fi->key,
			    Fi->database, Fi->driver);
	
	if (first_only && ndblinks > 1)
	    G_warning(_("More DB links defined for input vector map. "
			"Using only first DB link for output."));
    }
}

/*!
  \brief Check if DB connection exists in dblinks structure

  \param Map pointer to Map_info structure
  \param field layer number
  \param name layer name
  
  \return 1 dblink for field exists
  \return 0 dblink does not exist for field
 */
int Vect_map_check_dblink(const struct Map_info *Map, int field, const char *name)
{
    return Vect_check_dblink(Map->dblnk, field, name);
}

/*!
  \brief Check if DB connection exists in dblinks structure

  \param p pointer to existing dblinks structure
  \param field layer number
  \param name layer name

  \return 1 dblink for field exists
  \return 0 dblink does not exist for field
 */
int Vect_check_dblink(const struct dblinks *p, int field, const char *name)
{
    int i;

    G_debug(3, "Vect_check_dblink: field %d, name %s", field, (name != NULL ? name : "not given"));

    for (i = 0; i < p->n_fields; i++) {
	if (p->field[i].number == field) {
	    return 1;
	}
	if (name != NULL && p->field[i].name != NULL) {
	    if (strcmp(p->field[i].name, name) == 0)
		return 1;
	}
    }
    return 0;
}


/*!
  \brief Add new DB connection to dblinks structure
  
  \param[in,out] p pointer to existing dblinks structure
  \param number layer number (1 for OGR)
  \param name   layer name (layer for OGR) - if not given use table name
  \param table  table name (layer for OGR)
  \param key    key name
  \param db     database name (datasource for OGR)
  \param driver driver name (dbf, postgresql, ogr, ...)
  
  \return 0 on success
  \return -1 error
 */
int Vect_add_dblink(struct dblinks *p, int number, const char *name,
		    const char *table, const char *key, const char *db,
		    const char *driver)
{
    int ret;

    G_debug(3, "Field number <%d>, name <%s>", number, name);
    if (!name) {
	/* if name is not given, use table name */
	name = table;
    }
    ret = Vect_check_dblink(p, number, name);
    if (ret == 1) {
	G_warning(_("Layer number %d or name <%s> already exists"), number,
		  name);
	return -1;
    }

    if (p->n_fields == p->alloc_fields) {
	p->alloc_fields += 10;
	p->field = (struct field_info *)G_realloc((void *)p->field,
						  p->alloc_fields *
						  sizeof(struct field_info));
    }

    p->field[p->n_fields].number = number;

    if (name != NULL) {
	p->field[p->n_fields].name = G_store(name);
	/* replace all spaces with underscore, otherwise dbln can't be read */
	G_strchg(p->field[p->n_fields].name, ' ', '_');
    }
    else
	p->field[p->n_fields].name = NULL;

    if (table != NULL)
	p->field[p->n_fields].table = G_store(table);
    else
	p->field[p->n_fields].table = NULL;

    if (key != NULL)
	p->field[p->n_fields].key = G_store(key);
    else
	p->field[p->n_fields].key = NULL;

    if (db != NULL)
	p->field[p->n_fields].database = G_store(db);
    else
	p->field[p->n_fields].database = NULL;

    if (driver != NULL)
	p->field[p->n_fields].driver = G_store(driver);
    else
	p->field[p->n_fields].driver = NULL;

    p->n_fields++;

    return 0;
}

/*!
  \brief Get default information about link to database for new dblink
  
  \param Map pointer to Map_info structure
  \param field layer number
  \param field_name layer name
  \param type how many tables are linked to map: GV_1TABLE / GV_MTABLE 

  \return pointer to allocated field_info structure
 */
struct field_info *Vect_default_field_info(struct Map_info *Map,
					   int field, const char *field_name, int type)
{
    struct field_info *fi;
    char buf[GNAME_MAX], buf2[GNAME_MAX];
    const char *schema;
    dbConnection connection;

    G_debug(1, "Vect_default_field_info(): map = %s field = %d", Map->name,
	    field);
    
    if (Map->format == GV_FORMAT_OGR_DIRECT) {
	G_zero(&connection, sizeof(dbConnection));
	connection.driverName = G_store("ogr");
	connection.databaseName = G_store(Map->fInfo.ogr.dsn);
    }
    else {
	db_get_connection(&connection);
    }
    
    G_debug(2, "drv = %s db = %s", connection.driverName, connection.databaseName);
    
    if (!connection.driverName && !connection.databaseName) {
	/* Set default values */
	db_set_default_connection();
	db_get_connection(&connection);

	G_important_message(_("Default driver / database set to:\n"
			      "driver: %s\ndatabase: %s"), connection.driverName,
			    connection.databaseName);
    }
    /* they must be a matched pair, so if one is set but not the other
       then give up and let the user figure it out */
    else if (!connection.driverName) {
	G_fatal_error(_("Default driver is not set"));
    }
    else if (!connection.databaseName) {
	G_fatal_error(_("Default database is not set"));
    }

    fi = (struct field_info *)G_malloc(sizeof(struct field_info));

    fi->number = field;

    /* Table name */
    if (type == GV_1TABLE) {
	sprintf(buf, "%s", Map->name);
    }
    else {
	if (field_name != NULL && strlen(field_name) > 0)
	    sprintf(buf, "%s_%s", Map->name, field_name);
	else
	    sprintf(buf, "%s_%d", Map->name, field);
    }
    schema = connection.schemaName;
    if (schema && strlen(schema) > 0) {
	sprintf(buf2, "%s.%s", schema, buf);
	fi->table = G_store(buf2);
    }
    else {
	fi->table = G_store(buf);
    }

    /* Field name */
    if (field_name)
	fi->name = G_store(field_name);
    else
	fi->name = G_store(buf);

    fi->key = G_store(GV_KEY_COLUMN);	/* Should be: id/fid/gfid/... ? */
    fi->database = G_store(connection.databaseName);
    fi->driver = G_store(connection.driverName);

    return fi;
}

/*!
  \brief Get information about link to database.
  
  Variables are substituted by values, link is index to array of
  dblinks.

  \param Map pointer to Map_info structure
  \param link link id

  \return pointer to new field_info structure
 */
struct field_info *Vect_get_dblink(const struct Map_info *Map, int link)
{
    struct field_info *fi;

    G_debug(1, "Vect_get_dblink(): link = %d", link);

    if (link >= Map->dblnk->n_fields) {
	G_warning(_("Requested dblink %d, maximum link number %d"), link,
		  Map->dblnk->n_fields - 1);
	return NULL;
    }

    fi = (struct field_info *)G_malloc(sizeof(struct field_info));
    fi->number = Map->dblnk->field[link].number;

    if (Map->dblnk->field[link].name != NULL)
	fi->name = G_store(Map->dblnk->field[link].name);
    else
	fi->name = NULL;

    fi->table = G_store(Map->dblnk->field[link].table);
    fi->key = G_store(Map->dblnk->field[link].key);
    fi->database = Vect_subst_var(Map->dblnk->field[link].database, Map);
    fi->driver = G_store(Map->dblnk->field[link].driver);

    return fi;
}

/*!
  \brief Get information about link to database (by layer number)
  
  Variables are substituted by values, field is number of requested
  field.
  
  \param Map pointer to Map_info structure
  \param field layer number
  
  \return pointer to new field_info structure
  \return NULL if not found
*/
struct field_info *Vect_get_field(const struct Map_info *Map, int field)
{
    int i;
    struct field_info *fi = NULL;

    G_debug(1, "Vect_get_field(): field = %d", field);

    for (i = 0; i < Map->dblnk->n_fields; i++) {
	if (Map->dblnk->field[i].number == field) {
	    fi = Vect_get_dblink(Map, i);
	    break;
	}
    }

    return fi;
}

/*!
  \brief Get information about link to database (by layer name)
  
  \param Map pointer to Map_info structure
  \param field layer name
  
  \return pointer to new field_info structure
  \return NULL if not found
*/
struct field_info *Vect_get_field_by_name(const struct Map_info *Map, const char *field)
{
    int i;
    struct field_info *fi = NULL;

    G_debug(1, "Vect_get_field_by_name(): field = %s", field);

    for (i = 0; i < Map->dblnk->n_fields; i++) {
	if (strcmp(Map->dblnk->field[i].name, field) == 0) {
	    fi = Vect_get_dblink(Map, i);
	    break;
	}
    }

    return fi;
}

/*!
  \brief Get information about link to database (by layer number or layer name)
  
  Note: if <em>field</em> is -1 then the function returns the first
  dblink or NULL
  
  \param Map pointer to Map_info structure
  \param field layer number or name
  
  \return pointer to new field_info structure
  \return NULL if not found
*/
struct field_info *Vect_get_field2(const struct Map_info *Map, const char *field)
{
    int ifield;
    struct field_info *fi;

    G_debug(1, "Vect_get_field2(): field = %s", field);

    fi = NULL;
    ifield = atoi(field);
    
    if (ifield > 0) {
	fi = Vect_get_field(Map, ifield);
	if (fi)
	    return fi;
    }
    else if (ifield == -1) {
      if (Vect_get_num_dblinks(Map) > 0)
	  return Vect_get_dblink(Map, 0); /* return first */
      else
	  return NULL;
    }
    else if (ifield == 0)
	return Vect_get_field_by_name(Map, field);

    return NULL;
}

/*!
  \brief Get field number of given field

  \param Map pointer to Map_info structure
  \param field layer name

  \return layer number
  \return -1 for all layers
  \return 0 if layer not found
*/
int Vect_get_field_number(const struct Map_info *Map, const char *field)
{
    struct field_info *fi;

    G_debug(1, "Vect_get_field_number(): field = %s", field);

    if (strcmp(field, "-1") == 0)
	return -1;

    if (Vect_get_num_dblinks(Map) == 0)
	return atoi(field);
    
    fi = Vect_get_field2(Map, field);

    if (fi)
	return fi->number;

    return atoi(field);
}

static int read_dblinks_nat(struct Map_info *Map)
{
    FILE *fd;
    char file[1024], buf[2001];
    char tab[1024], col[1024], db[1024], drv[1024], fldstr[1024], *fldname;
    int fld;
    char *c, *path;
    int row, rule;
    struct dblinks *dbl;
    char **tokens;
    int ntok, i;

    dbl = Map->dblnk;

    /* Read dblink for native format */
    path = Vect__get_path(Map);
    fd = G_fopen_old(path, GV_DBLN_ELEMENT, Map->mapset);
    G_free(path);
    if (fd == NULL) {		/* This may be correct, no tables defined */
	G_debug(1, "Cannot open vector database definition file");
	return -1;
    }

    row = 0;
    rule = 0;
    while (G_getl2(buf, 2000, fd)) {
	row++;
	G_chop(buf);
	G_debug(1, "dbln: %s", buf);

	c = (char *)strchr(buf, '#');
	if (c != NULL)
	    *c = '\0';

	if (strlen(buf) == 0)
	    continue;

#ifdef NOT_ABLE_TO_READ_GRASS_6
	int ndef;
	ndef = sscanf(buf, "%s|%s|%s|%s|%s", fldstr, tab, col, db, drv);

        if (ndef < 2 || (ndef < 5 && rule < 1)) {
            G_warning(_("Error in rule on row %d in <%s>"), row, file);
            continue;
        }
#else
	tokens = G_tokenize(buf, " |");
	ntok = G_number_of_tokens(tokens);

	if (ntok < 2 || (ntok < 5 && rule < 1)) {
	    G_warning(_("Error in rule on row %d in <%s>"), row, file);
	    continue;
	}

	strcpy(fldstr, tokens[0]);
	strcpy(tab, tokens[1]);
	if (ntok > 2) {
	    strcpy(col, tokens[2]);
	    if (ntok > 3) {
		strcpy(db, tokens[3]);
		/* allow for spaces in path names */
		for (i=4; i < ntok-1; i++) {
		    strcat(db, " ");
		    strcat(db, tokens[i]);
		}

		strcpy(drv, tokens[ntok-1]);
	    }
	}
	G_free_tokens(tokens);
#endif

	/* get field and field name */
	fldname = strchr(fldstr, '/');
	if (fldname != NULL) {	/* field has name */
	    fldname[0] = 0;
	    fldname++;
	}
	fld = atoi(fldstr);

	Vect_add_dblink(dbl, fld, fldname, tab, col, db, drv);

	G_debug(1,
		"field = %d name = %s, table = %s, key = %s, database = %s, driver = %s",
		fld, fldname, tab, col, db, drv);

	rule++;
    }
    fclose(fd);

    G_debug(1, "Dblinks read");
    
    return rule;
}

static int read_dblinks_ogr(struct Map_info *Map)
{
    struct dblinks *dbl;
    
    dbl = Map->dblnk;
    G_debug(3, "Searching for FID column in OGR DB");
#ifndef HAVE_OGR
    G_warning(_("GRASS is not compiled with OGR support"));
#else
#if GDAL_VERSION_NUM > 1320 && HAVE_OGR /* seems to be fixed after 1320 release */
    int nLayers;
    char *ogr_fid_col;
    
    G_debug(3, "GDAL_VERSION_NUM: %d", GDAL_VERSION_NUM);
    
    if (Map->fInfo.ogr.ds == NULL) {
	/* open the connection to fetch the FID column name */
	OGRRegisterAll();
	
	/* data source handle */
	Map->fInfo.ogr.ds = OGROpen(Map->fInfo.ogr.dsn, FALSE, NULL);
	if (Map->fInfo.ogr.ds == NULL)
	    G_fatal_error(_("Unable to open OGR data source '%s'"),
			  Map->fInfo.ogr.dsn);
    }
    if (Map->fInfo.ogr.layer == NULL) {
	/* get layer number */
	nLayers = OGR_DS_GetLayerCount(Map->fInfo.ogr.ds);	/* Layers = Maps in OGR DB */
	
	G_debug(3, "%d layers (maps) found in data source", nLayers);
	
	G_debug(3, "Trying to open OGR layer: %s", Map->fInfo.ogr.layer_name);
	if (Map->fInfo.ogr.layer_name) {
	    Map->fInfo.ogr.layer = OGR_DS_GetLayerByName(Map->fInfo.ogr.ds, Map->fInfo.ogr.layer_name);
	    if (Map->fInfo.ogr.layer == NULL) {
		OGR_DS_Destroy(Map->fInfo.ogr.ds);
		Map->fInfo.ogr.ds = NULL;
		G_fatal_error(_("Unable to open OGR layer <%s>"),
			      Map->fInfo.ogr.layer_name);
	    }
	}
    }
    
    /* get fid column */
    ogr_fid_col = G_store(OGR_L_GetFIDColumn(Map->fInfo.ogr.layer));
    G_debug(3, "Using FID column <%s> in OGR DB", ogr_fid_col);
    Vect_add_dblink(dbl, 1,  Map->fInfo.ogr.layer_name,
		    Map->fInfo.ogr.layer_name, ogr_fid_col,
		    Map->fInfo.ogr.dsn, "ogr");
#else
	dbDriver *driver;
	dbCursor cursor;
	dbString sql;
	int FID = 0, OGC_FID = 0, OGR_FID = 0, GID = 0;

	G_debug(3, "GDAL_VERSION_NUM: %d", GDAL_VERSION_NUM);

	/* FID is not available for all OGR drivers */
	db_init_string(&sql);

	driver = db_start_driver_open_database("ogr", Map->fInfo.ogr.dsn);

	if (driver == NULL) {
	    G_warning(_("Unable to open OGR DBMI driver"));
	    return -1;
	}

	/* this is a bit stupid, but above FID auto-detection doesn't work yet...: */
	db_auto_print_errors(0);
	sprintf(buf, "select FID from %s where FID > 0",
		Map->fInfo.ogr.layer_name);
	db_set_string(&sql, buf);

	if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) !=
	    DB_OK) {
	    /* FID not available, so we try ogc_fid */
	    G_debug(3, "Failed. Now searching for ogc_fid column in OGR DB");
	    sprintf(buf, "select ogc_fid from %s where ogc_fid > 0",
		    Map->fInfo.ogr.layer_name);
	    db_set_string(&sql, buf);

	    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) !=
		DB_OK) {
		/* Neither FID nor ogc_fid available, so we try ogr_fid */
		G_debug(3,
			"Failed. Now searching for ogr_fid column in OGR DB");
		sprintf(buf, "select ogr_fid from %s where ogr_fid > 0",
			Map->fInfo.ogr.layer_name);
		db_set_string(&sql, buf);

		if (db_open_select_cursor
		    (driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK) {
		    /* Neither FID nor ogc_fid available, so we try gid */
		    G_debug(3,
			    "Failed. Now searching for gid column in OGR DB");
		    sprintf(buf, "select gid from %s where gid > 0",
			    Map->fInfo.ogr.layer_name);
		    db_set_string(&sql, buf);

		    if (db_open_select_cursor
			(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK) {
			/* neither FID nor ogc_fid nor ogr_fid nor gid available */
			G_warning(_("All FID tests failed. Neither 'FID' nor 'ogc_fid' "
				   "nor 'ogr_fid' nor 'gid' available in OGR DB table"));
			db_close_database_shutdown_driver(driver);
			return 0;
		    }
		    else
			GID = 1;
		}
		else
		    OGR_FID = 1;
	    }
	    else
		OGC_FID = 1;
	}
	else
	    FID = 1;

	G_debug(3, "FID: %d, OGC_FID: %d, OGR_FID: %d, GID: %d", FID, OGC_FID,
		OGR_FID, GID);

	db_close_cursor(&cursor);
	db_close_database_shutdown_driver(driver);
	db_auto_print_errors(1);

	if (FID) {
	    G_debug(3, "Using FID column in OGR DB");
	    Vect_add_dblink(dbl, 1, Map->fInfo.ogr.layer_name, Map->fInfo.ogr.layer_name, "FID",
			    Map->fInfo.ogr.dsn, "ogr");
	}
	else {
	    if (OGC_FID) {
		G_debug(3, "Using ogc_fid column in OGR DB");
		Vect_add_dblink(dbl, 1, Map->fInfo.ogr.layer_name, Map->fInfo.ogr.layer_name,
				"ogc_fid", Map->fInfo.ogr.dsn, "ogr");
	    }
	    else {
		if (OGR_FID) {
		    G_debug(3, "Using ogr_fid column in OGR DB");
		    Vect_add_dblink(dbl, 1, Map->fInfo.ogr.layer_name, Map->fInfo.ogr.layer_name,
				    "ogr_fid", Map->fInfo.ogr.dsn, "ogr");
		}
		else {
		    if (GID) {
			G_debug(3, "Using gid column in OGR DB");
			Vect_add_dblink(dbl, 1, Map->fInfo.ogr.layer_name,
					Map->fInfo.ogr.layer_name, "gid",
					Map->fInfo.ogr.dsn, "ogr");
		    }
		}
	    }
	}
#endif /* GDAL_VERSION_NUM > 1320 && HAVE_OGR */
	return 1;
#endif	/* HAVE_GDAL */
}

static int read_dblinks_pg(struct Map_info *Map)
{
#ifdef HAVE_POSTGRES
    char *name;
    struct dblinks *dbl;
    struct Format_info_pg *pg_info;
    
    dbl = Map->dblnk;
    pg_info = &(Map->fInfo.pg);
    
    if (!pg_info->fid_column) {
	G_warning(_("Feature table <%s> has no primary key defined. "
		    "Unable to define DB links."), pg_info->table_name);
	return -1;
    }
    G_debug(3, "Using FID column <%s>", pg_info->fid_column);

    name = NULL;
    if (G_strcasecmp(pg_info->schema_name, "public") != 0)
	G_asprintf(&name, "%s.%s", pg_info->schema_name,
		   pg_info->table_name);
    else
	name = pg_info->table_name;
    
    Vect_add_dblink(dbl, 1, name, name,
		    pg_info->fid_column,
		    pg_info->db_name, "pg");
    if (name != pg_info->table_name)
	G_free(name);
    return 1;
#else
    G_warning(_("GRASS not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
  \brief Read dblinks to existing structure.
  
  Variables are not substituted by values.
  
  \param Map pointer to Map_info structure

  \return number of links read
  \return -1 on error
 */
int Vect_read_dblinks(struct Map_info *Map)
{
    G_debug(1, "Vect_read_dblinks(): map = %s, mapset = %s", Map->name,
	    Map->mapset);
    
    Vect_reset_dblinks(Map->dblnk);

    if (Map->format == GV_FORMAT_NATIVE) {
	return read_dblinks_nat(Map);
    }
    else if (Map->format == GV_FORMAT_OGR || Map->format == GV_FORMAT_OGR_DIRECT) {
	return read_dblinks_ogr(Map);
    }
    else if (Map->format == GV_FORMAT_POSTGIS) {
	return read_dblinks_pg(Map);
    }
    else {
	G_fatal_error(_("Unknown vector map format"));
    }

    return -1;
}

/*!
  \brief Write dblinks to file
  
  \param Map pointer to Map_info structure
  
  \return 0 on success
  \return -1 on error
 */
int Vect_write_dblinks(struct Map_info *Map)
{
    int i;
    FILE *fd;
    char *path, buf[1024];
    struct dblinks *dbl;

    if (Map->format != GV_FORMAT_NATIVE)
	/* nothing to write for non-native formats */
	return 0;
    
    G_debug(1, "Vect_write_dblinks(): map = %s, mapset = %s", Map->name,
	    Map->mapset);

    dbl = Map->dblnk;

    path = Vect__get_path(Map);
    fd = G_fopen_new(path, GV_DBLN_ELEMENT);
    G_free(path);
    if (fd == NULL) {		/* This may be correct, no tables defined */
	G_warning(_("Unable to create database definition file for vector map <%s>"),
		  Vect_get_name(Map));
	return -1;
    }

    for (i = 0; i < dbl->n_fields; i++) {
	if (dbl->field[i].name != NULL)
	    sprintf(buf, "%d/%s", dbl->field[i].number, dbl->field[i].name);
	else
	    sprintf(buf, "%d", dbl->field[i].number);

	fprintf(fd, "%s|%s|%s|%s|%s\n", buf, dbl->field[i].table,
		dbl->field[i].key, dbl->field[i].database,
		dbl->field[i].driver);
	G_debug(1, "%s|%s|%s|%s|%s", buf, dbl->field[i].table,
		dbl->field[i].key, dbl->field[i].database,
		dbl->field[i].driver);
    }
    fclose(fd);

    G_debug(1, "Dblinks written");
    
    return 0;
}

/*!
  \brief Substitute variable in string
  
  \param in current string
  \param Map pointer to Map_info structure
  
  \return pointer to new string
*/
char *Vect_subst_var(const char *in, const struct Map_info *Map)
{
    char *c;
    char buf[1000], str[1000];

    G_debug(3, "Vect_subst_var(): in = %s, map = %s, mapset = %s", in,
	    Map->name, Map->mapset);

#ifdef __MINGW32__
    char *cin;
    cin = G_str_replace(in, "/", "\\");
    strcpy(str, cin);
    G_free(cin);
#else
    strcpy(str, in);
#endif
    
    strcpy(buf, str);
    c = (char *)strstr(buf, "$GISDBASE");
    if (c != NULL) {
	*c = '\0';
	sprintf(str, "%s%s%s", buf, Map->gisdbase, c + 9);
    }

    strcpy(buf, str);
    c = (char *)strstr(buf, "$LOCATION_NAME");
    if (c != NULL) {
	*c = '\0';
	sprintf(str, "%s%s%s", buf, Map->location, c + 14);
    }

    strcpy(buf, str);
    c = (char *)strstr(buf, "$MAPSET");
    if (c != NULL) {
	*c = '\0';
	sprintf(str, "%s%s%s", buf, Map->mapset, c + 7);
    }

    strcpy(buf, str);
    c = (char *)strstr(buf, "$MAP");
    if (c != NULL) {
	*c = '\0';
	sprintf(str, "%s%s%s", buf, Map->name, c + 4);
    }

    G_debug(3, "  -> %s", str);
    return (G_store(str));
}

/*!
  \brief Rewrite 'dbln' file
  
  Should be used by GRASS modules which update database tables, so
  that other applications know that tables were changed and can reload
  data.
  
  \param Map pointer to Map_info structure
*/
void Vect_set_db_updated(struct Map_info *Map)
{
    if (strcmp(Map->mapset, G_mapset()) != 0 &&
	G_strcasecmp(Map->mapset, "ogr") != 0) {
	G_fatal_error(_("Bug: attempt to update map which is not in current mapset"));
    }

    Vect_write_dblinks(Map);
}
