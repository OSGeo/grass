#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/form.h>

/* Generate form in HTML/TXT format.
*  Pointer to resulting string is stored to 'form'. This string must be freed by application.
*
*  returns: -1 error 
*            0 success
*/
int 
F_generate (char *drvname, char *dbname, char *tblname, char *key, int keyval, 
	    char *frmname, char *frmmapset, 
	    int edit_mode, int format, 
	    char **form) 
{
    int      col, ncols, ctype, sqltype, more; 
    char     buf[5000], buf1[100], *colname;
    dbString sql, html, str;
    dbDriver *driver;
    dbHandle handle;
    dbCursor cursor;
    dbTable  *table;
    dbColumn *column;
    dbValue  *value;

    int i = 0;
    /* see /usr/lib/tcl8.4/encoding/ */
    static char *encoding_list[] = {
	"utf-8",
	"ascii",
	"iso8859-1",
	"iso8859-2",
	"iso8859-15",
	"iso2022-jp",
	"koi8-r",
	"euc-jp",
	NULL
    };
    char *enc_env;

    G__read_env();
    enc_env = G__getenv("GRASS_DB_ENCODING");
    
    /* TODO: support 'format' (txt, html), currently html only */

    G_debug ( 2, "F_generate(): drvname = '%s', dbname = '%s'\n      tblname = '%s', key = '%s', keyval = %d\n"
	         "    form = '%s', form_mapset = '%s'\n      edit_mode = %d", 
		 drvname, dbname, tblname, key, keyval, frmname, frmmapset, edit_mode);
    
    db_init_string (&sql);
    db_init_string (&html); /* here is the result stored */
    db_init_string (&str);

    G_debug ( 2, "Open driver" );
    driver = db_start_driver(drvname);
    if (driver == NULL) { 
	G_warning ("Cannot open driver\n"); 
        sprintf ( buf, "Cannot open driver '%s'<BR>", drvname );
        *form = G_store (buf);
	return -1; 
    }
    G_debug ( 2, "Driver opened" );

    db_init_handle (&handle);
    db_set_handle (&handle, dbname, NULL);
    G_debug ( 2, "Open database" );
    if (db_open_database(driver, &handle) != DB_OK){
	G_warning ("Cannot open database\n"); 
	db_shutdown_driver(driver); 
	sprintf ( buf, "Cannot open database '%s' by driver '%s'<BR>", dbname, drvname);
        *form = G_store (buf);
	return -1;
    }
    G_debug ( 2, "Database opened" );

    /* TODO: test if table exist first, but this should be tested by application befor
    *        F_generate() is called, because it may be correct (connection defined in DB
    *        but table does not exist) */
    
    sprintf (buf, "select * from %s where %s = %d", tblname, key, keyval);
    G_debug ( 2, "%s", buf);
    db_set_string (&sql, buf);  
    if ( db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK) {
	G_warning ("Cannot open select cursor\n"); 
        db_close_database(driver);
	db_shutdown_driver(driver); 
	sprintf ( buf, "Cannot open select cursor:<BR>'%s'<BR>on database '%s' by driver '%s'<BR>", 
		        db_get_string(&sql), dbname, drvname);
        *form = G_store (buf);
	return -1;
    }
    G_debug ( 2, "Select Cursor opened" );
	
    table = db_get_cursor_table (&cursor);

    if ( db_fetch (&cursor, DB_NEXT, &more ) != DB_OK ) {
	G_warning ("Cannot fetch next record\n"); 
	db_close_cursor(&cursor);
	db_close_database(driver);
	db_shutdown_driver(driver); 
        *form = G_store ("Cannot fetch next record");
	return -1;
    }

    if ( !more ) {
	G_warning ( "No database record" );
	if ( format == F_HTML ) {
            *form = G_store ("No record selected.<BR>");
	} else {
            *form = G_store ("No record selected.");
	}
    } else {
	ncols = db_get_table_number_of_columns (table);

	/* Start form */
	if ( format == F_HTML ) {
	    if ( edit_mode == F_EDIT ) {
		db_append_string (&html, "<FORM>");	

		sprintf (buf, "<INPUT type=hidden name=%s value=\"%s\">", F_DRIVER_FNAME, drvname );
		db_append_string (&html, buf);
		/* Note: because html_library.tcl failes to parse
		*  <INPUT name=abc value='dbname=xxx'> and returnes
		*  name="xxx" value="dbname=xxx" order of value and name parameters is changed */
		sprintf (buf, "<INPUT type=hidden value=\"%s\" name=%s>", dbname, F_DATABASE_FNAME );
		db_append_string (&html, buf);
		sprintf (buf, "<INPUT type=hidden name=%s value=\"%s\">", F_TABLE_FNAME, tblname );
		db_append_string (&html, buf);
		sprintf (buf, "<INPUT type=hidden name=%s value=\"%s\">", F_KEY_FNAME, key );
		db_append_string (&html, buf);

	    }
	    
	    for( col = 0; col < ncols; col++) {
		column = db_get_table_column(table, col);
		sqltype = db_get_column_sqltype (column);
		ctype = db_sqltype_to_Ctype(sqltype);
		value  = db_get_column_value(column);
		db_convert_value_to_string( value, sqltype, &str);
		colname = db_get_column_name (column);

		G_debug ( 2, "%s: %s", colname, db_get_string (&str) );

		if ( edit_mode == F_VIEW ) {
		    sprintf (buf, "<B>%s : </B> %s <BR>", colname, db_get_string(&str) );
		    db_append_string (&html, buf);
		} else {
		    sprintf (buf, "<B>%s : </B>", colname );
		    db_append_string (&html, buf);

		    if ( G_strcasecmp (colname,key) == 0 ) {
			sprintf (buf, "%s<BR> <INPUT type=hidden name=%s value=\"%s\">", 
				   db_get_string(&str), colname, db_get_string(&str) );
		    } else {
			switch ( ctype ) {
			    case DB_C_TYPE_INT:
				sprintf (buf1, "20" );
				break;		    
			    case DB_C_TYPE_DOUBLE:
				sprintf (buf1, "30" );
				break;	    
			    case DB_C_TYPE_STRING:
				sprintf (buf1, "%d", db_get_column_length (column) );
				break;
			    case DB_C_TYPE_DATETIME:
				sprintf (buf1, "20" );
				break;
			}		
			sprintf (buf, "<INPUT type=text size=%s name=%s value=\"%s\"><BR>", 
				       buf1, colname, db_get_string(&str) );
		    }
		    db_append_string (&html, buf);	
		}
	    } 

	    if ( edit_mode == F_EDIT ) {
		sprintf(buf, "<HR>   Assume data encoding as:<BR><BR><SELECT NAME=%s SIZE=4><HR><BR>",
			F_ENCODING);
		db_append_string(&html, buf);

		i = 0;
		while (encoding_list[i] != NULL) {

		    if (G_strcasecmp(encoding_list[i], enc_env) == 0)
			sprintf(buf, "<OPTION VALUE=\"%s\" SELECTED>%s",
				encoding_list[i], encoding_list[i]);
		    else
			sprintf(buf, "<OPTION VALUE=\"%s\">%s", encoding_list[i],
				encoding_list[i]);
		    ++i;
		    db_append_string(&html, buf);
		}

		sprintf(buf, "</SELECT>");
		db_append_string(&html, buf);
	    }
	    
	    /* Close form */
	    if ( edit_mode == F_EDIT ) {
		db_append_string (&html, "</FORM>");	
	    }
	} else { /* F_TXT */
	    for( col = 0; col < ncols; col++) {
		column = db_get_table_column(table, col);
		sqltype = db_get_column_sqltype (column);
		ctype = db_sqltype_to_Ctype(sqltype);
		value  = db_get_column_value(column);
		db_convert_value_to_string( value, sqltype, &str);
		colname = db_get_column_name (column);

		G_debug ( 2, "%s: %s", colname, db_get_string (&str) );

		sprintf (buf, "%s : %s\n", colname, db_get_string(&str) );
		db_append_string (&html, buf);
	    }
	}
    }
    G_debug ( 2, "FORM STRING:\n%s\n", db_get_string(&html) );

    db_close_cursor(&cursor);
    db_close_database(driver);
    db_shutdown_driver(driver); 

    *form = G_store ( db_get_string(&html) );

    db_free_string (&sql);
    db_free_string (&html);
    db_free_string (&str);

    return 0;
}


