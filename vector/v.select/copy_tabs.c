#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "proto.h"

void copy_tabs(struct Map_info *In, struct Map_info *Out,
	       int nfields, int *fields, int *ncats, int **cats)
{
    int i, ttype, ntabs;

    struct field_info *IFi, *OFi;
    
    ntabs = 0;
    
    G_message(_("Writing attributes..."));
    
    /* Number of output tabs */
    for (i = 0; i < Vect_get_num_dblinks(In); i++) {
	int f, j;
	
	IFi = Vect_get_dblink(In, i);
	
	for (j = 0; j < nfields; j++) {	/* find field */
	    if (fields[j] == IFi->number) {
		f = j;
		break;
	    }
	}
	if (ncats[f] > 0)
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
	
	/* Make a list of categories */
	IFi = Vect_get_field(In, fields[i]);
	if (!IFi) {		/* no table */
	    G_warning(_("No table for layer %d"), fields[i]);
	    continue;
	}
	
	OFi =
	    Vect_default_field_info(Out, IFi->number, IFi->name, ttype);
	
	ret =
	    db_copy_table_by_ints(IFi->driver, IFi->database, IFi->table,
				  OFi->driver,
				  Vect_subst_var(OFi->database, Out),
				  OFi->table, IFi->key, cats[i],
				  ncats[i]);
	
	if (ret == DB_FAILED) {
	    G_warning(_("Unable to copy table for layer %d"), fields[i]);
	}
	else {
	    Vect_map_add_dblink(Out, OFi->number, OFi->name, OFi->table,
				IFi->key, OFi->database, OFi->driver);
	}
    }
}
