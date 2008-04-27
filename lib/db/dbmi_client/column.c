/****************************************************************************
*
* MODULE:       DBMI Library - dbmi_client column lib
*   	    	
* AUTHOR(S):    Radim Blazek (original contributor)
*               Glynn Clement <glynn gclements.plus.com>,
*               Martin Landa <landa.martin gmail.com>
*
* PURPOSE:      Higher level functions for DBMI DataBase Management Interface
*
* COPYRIGHT:    (C) 2001-2007 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/*!
  \fn int db_column_sqltype (dbDriver *driver, char *tab, char *col)
  \brief Get column sqltype
  
  See db_sqltype_name ()
  \return column sqltype or -1 on error
  \param driver DB driver
  \param tab table name
  \param col column name
*/
int
db_column_sqltype (dbDriver *driver, char *tab, char *col)
{
    dbTable *table;
    dbString table_name;
    dbColumn *column;
    int ncol, cl, type;

    db_init_string(&table_name);
    db_set_string(&table_name, tab);
    
    if(db_describe_table (driver, &table_name, &table) != DB_OK)
       return -1;
    
    db_free_string ( &table_name );
    ncol = db_get_table_number_of_columns(table);
    for (cl = 0; cl < ncol; cl++) {
	column = db_get_table_column (table, cl);
	if ( strcmp (  db_get_column_name(column), col ) == 0 ) {
	    type = db_get_column_sqltype(column);
	    return type;
	}
    }
    
    return -1;
}

/*!
  \fn int db_column_Ctype (dbDriver *driver, char *tab, char *col)
  \brief Get column Ctype

  See db_sqltype_to_Ctype()
  \return column Ctype or -1 on error
  \param driver DB driver
  \param tab table name
  \param col column name
*/
int
db_column_Ctype (dbDriver *driver, char *tab, char *col)
{
    int type;
    if ( ( type = db_column_sqltype ( driver, tab, col ) ) >= 0 ) {
	type = db_sqltype_to_Ctype(type); 
	return type;
    }

    return -1;
}

/*!
  \fn int db_get_column ( dbDriver *Driver, char *tname, char *cname, dbColumn **Column )
  \brief Get column structure by table and column name.
  
  Column is set to new dbColumn structure or NULL if column was not found
  \return DB_OK on success, DB_FAILED on error
  \param Driver DB driver
  \param tname table name
  \param cname column name
  \param Column column structure to store within
*/
int
db_get_column ( dbDriver *Driver, char *tname, char *cname, dbColumn **Column )
{
    int   i, ncols;
    dbTable *Table;
    dbColumn *Col, *NCol;
    dbString tabname;

    db_init_string(&tabname);
    db_set_string(&tabname, tname);

    if(db_describe_table (Driver, &tabname, &Table) != DB_OK) {
	 G_warning(_("Cannot describe table <%s>"), tname);
	 return DB_FAILED;
    }

    *Column = NULL;

    ncols = db_get_table_number_of_columns(Table);
    G_debug (3, "ncol = %d", ncols );
	     
    for (i = 0; i < ncols; i++) {
        Col = db_get_table_column (Table, i);
	if ( G_strcasecmp ( db_get_column_name(Col), cname ) == 0 ) {
	    NCol = (dbColumn *) malloc ( sizeof ( dbColumn ) );
            db_init_column ( NCol );
	    db_set_string ( &(NCol->columnName), db_get_column_name(Col) );
	    db_set_string ( &(NCol->description), db_get_column_description(Col) );
	    NCol->sqlDataType = Col->sqlDataType;
	    NCol->hostDataType = Col->hostDataType;
	    db_copy_value ( &(NCol->value), &(Col->value) );
	    NCol->dataLen = Col->dataLen;
	    NCol->precision = Col->precision;
	    NCol->scale = Col->scale;
	    NCol->nullAllowed = Col->nullAllowed;
	    NCol->hasDefaultValue = Col->hasDefaultValue;
	    NCol->useDefaultValue = Col->useDefaultValue;
	    db_copy_value ( &(NCol->defaultValue), &(Col->defaultValue) );
	    NCol->select = Col->select;
	    NCol->update = Col->update;

	    *Column = NCol;
	    return DB_OK;
	}
    }
    return DB_OK;
}
