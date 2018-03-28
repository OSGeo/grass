#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

void copy_tabs(const struct Map_info *In, struct Map_info *Out)
{
    int nlines, line, i;
    int ttype, ntabs;
    int **ocats, *nocats, nfields, *fields;
    
    struct field_info *IFi, *OFi;
    struct line_cats *Cats;
    dbDriver *driver;
    
    ntabs = 0;
    
    /* Collect list of output cats */
    Cats = Vect_new_cats_struct();
    nfields = Vect_cidx_get_num_fields(Out);
    ocats = (int **)G_malloc(nfields * sizeof(int *));
    nocats = (int *)G_malloc(nfields * sizeof(int));
    fields = (int *)G_malloc(nfields * sizeof(int));
    for (i = 0; i < nfields; i++) {
	nocats[i] = 0;
	ocats[i] =
	    (int *)G_malloc(Vect_cidx_get_num_cats_by_index(Out, i) *
			    sizeof(int));
	fields[i] = Vect_cidx_get_field_number(Out, i);
    }
    
    nlines = Vect_get_num_lines(Out);
    for (line = 1; line <= nlines; line++) {
	Vect_read_line(Out, NULL, Cats, line);
	
	for (i = 0; i < Cats->n_cats; i++) {
	    int f, j;
	    
	    f = -1;
	    for (j = 0; j < nfields; j++) {	/* find field */
		if (fields[j] == Cats->field[i]) {
		    f = j;
		    break;
		}
	    }
	    if (f >= 0) {
		ocats[f][nocats[f]] = Cats->cat[i];
		nocats[f]++;
	    }
	}
    }
    
    /* Copy tables */
    G_message(_("Writing attributes..."));
    
    /* Number of output tabs */
    for (i = 0; i < Vect_get_num_dblinks(In); i++) {
	int j, f = -1;
	
	IFi = Vect_get_dblink(In, i);
	
	for (j = 0; j < nfields; j++) {	/* find field */
	    if (fields[j] == IFi->number) {
		f = j;
		break;
	    }
	}
	if (f >= 0 && nocats[f] > 0)
	    ntabs++;
    }
    
    if (ntabs > 1)
	ttype = GV_MTABLE;
    else
	ttype = GV_1TABLE;
    
    for (i = 0; i < nfields; i++) {
	int ret;
	
	if (fields[i] == 0)
	    continue;
	if (nocats[i] == 0)
	    continue;
	
	G_verbose_message(_("Writing attributes for layer %d"), fields[i]);
	
	/* Make a list of categories */
	IFi = Vect_get_field(In, fields[i]);
	if (!IFi) {		/* no table */
	    G_message(_("No attribute table for layer %d"), fields[i]);
	    continue;
	}
	
	OFi = Vect_default_field_info(Out, IFi->number, NULL, ttype);
	
	ret = db_copy_table_by_ints(IFi->driver, IFi->database, IFi->table,
				    OFi->driver,
				    Vect_subst_var(OFi->database, Out),
				    OFi->table, IFi->key, ocats[i],
				    nocats[i]);
	
	if (ret == DB_FAILED) {
	    G_warning(_("Unable to copy table <%s>"), IFi->table);
	}
	else {

	    driver = db_start_driver_open_database(OFi->driver,
						   Vect_subst_var(OFi->database,
								    Out));

	    if (!driver) {
		G_warning(_("Unable to open database <%s> with driver <%s>"),
			      OFi->database, OFi->driver);
	    }
	    else {

		/* do not allow duplicate keys */
		if (db_create_index2(driver, OFi->table, IFi->key) != DB_OK) {
		    G_warning(_("Unable to create index"));
		}

		if (db_grant_on_table(driver, OFi->table, DB_PRIV_SELECT,
		     DB_GROUP | DB_PUBLIC) != DB_OK) {
		    G_warning(_("Unable to grant privileges on table <%s>"),
				  OFi->table);
		}

		db_close_database_shutdown_driver(driver);
	    }

	    Vect_map_add_dblink(Out, OFi->number, OFi->name, OFi->table,
				IFi->key, OFi->database, OFi->driver);
	}
    }
}
