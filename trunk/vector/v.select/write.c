#include <grass/vector.h>
#include <grass/glocale.h>

void write_lines(struct Map_info *In, struct field_info *IFi, int *ALines,
                 int *AAreas,
		 struct Map_info *Out, int table_flag, int reverse_flag,
		 int nfields, int *fields, int *ncats, int **cats)
{
    int i, f, j, aline, nalines;
    int atype;
    
    struct line_pnts *APoints;
    struct line_cats *ACats;
    
    APoints = Vect_new_line_struct();
    ACats = Vect_new_cats_struct();
    
    for (i = 0; i < nfields; i++) {
	ncats[i] = 0;
	cats[i] =
	    (int *)G_malloc(Vect_cidx_get_num_cats_by_index(&(In[0]), i) *
			    sizeof(int));
	fields[i] = Vect_cidx_get_field_number(&(In[0]), i);
    }

    nalines = Vect_get_num_lines(In);
    G_message(_("Writing selected features..."));
    for (aline = 1; aline <= nalines; aline++) {
	G_debug(3, "aline = %d ALines[aline] = %d", aline, ALines[aline]);
	G_percent(aline, nalines, 2);
	if ((!reverse_flag && !(ALines[aline])))
	    continue;

	atype = Vect_read_line(&(In[0]), APoints, ACats, aline);

	if ((reverse_flag && ALines[aline])) {
	    if (atype == GV_BOUNDARY && AAreas) {
		int left, right, skipme;

		skipme = 1;
		Vect_get_line_areas(&(In[0]), aline, &left, &right);
		if (left < 0)
		    left = Vect_get_isle_area(&(In[0]), abs(left));
		if (left > 0 && !AAreas[left])
		    skipme = 0;
		if (right < 0)
		    right = Vect_get_isle_area(&(In[0]), abs(right));
		if (right > 0 && !AAreas[right])
		    skipme = 0;
		if (skipme)
		    continue;
	    }
	    else
		continue;
	}


	Vect_write_line(Out, atype, APoints, ACats);

	if (!table_flag && (IFi != NULL)) {
	    for (i = 0; i < ACats->n_cats; i++) {
		f = -1;
		for (j = 0; j < nfields; j++) {	/* find field */
		    if (fields[j] == ACats->field[i]) {
			f = j;
			break;
		    }
		}
		if (f >= 0) {
		    cats[f][ncats[f]] = ACats->cat[i];
		    ncats[f]++;
		}
	    }
	}
    }

    Vect_destroy_line_struct(APoints);
    Vect_destroy_cats_struct(ACats);
}
