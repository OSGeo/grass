/*
 * $Id$
 *
 ****************************************************************************
 *
 * MODULE:       d.save
 * AUTHOR(S):    David Satnik - Central Washington University
 * PURPOSE:      Output all commands that have been used to create the 
 *               current display graphics with the help of the internal Pad 
 *               contents. 
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *    	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "locals.h"

/* globals !!! */
int Sheight, Swidth;
char Scurwin[100];

int Wtop, Wbot, Wleft, Wright;
char Wcell[100]="", Wdig[100], Wsite[100], Wcolor[25]="";

int Mtype;
int proj;
struct Cell_head *Mwind; 
char  Nstr[20], Sstr[20], Estr[20], Wstr[20], EWRESstr[20], NSRESstr[20];

struct list_struct *List = NULL, *List_last = NULL;

int in_frame_list(struct Option *, char *);
int init_globals(void);
int which_item(char *);
int set_item(char *, char **);
int process_list(char *, char **, int);
int process_items(char **, int);
int process_pad(char ***, int *);

int main (int argc, char **argv)
{
	char **pads;
	char **items;
	char **list;
	int npads;
	int nitems;
	int p;
	int stat;
	int i, j;
	int redraw;
	int total_rno, *rno;
	int total_mno, **mno;
	int nlists, *live;
	int from, to, tmp;
	struct list_struct *temp_list;
	struct Flag *all_flag;
	struct Flag *cur_frame;
	struct Flag *only_object;
	struct Option *opt1;
	struct Option *opt2;
	struct Option *opt3;
	struct GModule *module;
	char buff[1024];
	char current_frame[64];
	float Ftop, Fbot, Fleft, Fright;


	G_gisinit(argv[0]);

        module = G_define_module();
        module->keywords = _("display");
	module->description = 
	  _("Creates a list of commands for recreating screen graphics.");
	

	opt1 = G_define_option();
	opt1->key = "frame";
	opt1->description = _("Name of frame(s) to save");
	opt1->type = TYPE_STRING;
	opt1->required = NO;
	opt1->multiple = YES;
	/* Conditionalize so "help" and "--interface-description" work */
	R__open_quiet();  /* don't let library code make us die */

	if (R_open_driver() == 0)
	{
		Sheight = R_screen_bot() - R_screen_top();
		Swidth = R_screen_rite() - R_screen_left();

		Mwind = (struct Cell_head *) G_malloc(sizeof(struct Cell_head));
		R_pad_list (&pads, &npads);

		/* process the screen pad */
		p = -1;
		stat = R_pad_select ("");
		if (stat) {
			R_pad_perror ("echo     ERROR", stat);
			fprintf (stdout,"exit -1\n\n");
		}
		else process_pad(&items, &nitems);


		opt1->answer = Scurwin;
		buff[0] = '\0';
		for (p=npads-1; p>=0; p--) {
			strcat(buff, pads[p]);
			if (p != 0)  strcat(buff, ",");
		}
		opt1->options = buff;
	}
	else
		stat = -1; /* used to exit, if driver open failed */

	opt2 = G_define_option();
	opt2->key = "remove";
	opt2->description =
	  _("List of object numbers to remove which are displayed after \"#\". "
	  "-1 for the last object.");
	opt2->type = TYPE_INTEGER;
	opt2->required = NO;
	opt2->multiple = YES;

	opt3 = G_define_option();
	opt3->key = "move";
	opt3->description = _("List of object numbers to move "
	  "(\"from\" to \"to\"). remove= option will be done first, if any.");
	opt3->type = TYPE_INTEGER;
	opt3->required = NO;
	opt3->key_desc = "from,to";
	opt3->multiple = YES;

	cur_frame = G_define_flag();
	cur_frame->key = 'c';
	cur_frame->description = _("Save current frame");
	cur_frame->answer = 0;

	all_flag = G_define_flag();
	all_flag->key = 'a';
	all_flag->description = _("Save all the frames");
	all_flag->answer = 0;

	only_object = G_define_flag();
	only_object->key = 'o';
	only_object->description =
	  _("Only map objects without extra header and tailer");
	only_object->answer = 0;

	if (G_parser(argc, argv))
	    exit(EXIT_FAILURE);


	if (stat) /* Check we have monitor */
	    G_fatal_error(_("No monitor selected"));

	total_rno = 0;
	if (opt2->answers)
	{
		for (total_rno=0; opt2->answers[total_rno]; total_rno++);
		if (total_rno)
		{
			rno = (int *) G_malloc(total_rno*sizeof(int));
			for (i=0; i<total_rno; i++)
				rno[i] = atoi(opt2->answers[i]);
		}
	}

	total_mno = 0;
	if (opt3->answers)
	{
		for (total_mno=0; opt3->answers[total_mno]; total_mno++);
		total_mno /= 2;
		if (total_mno)
		{
			mno = (int **) G_malloc(total_mno*sizeof(int *));
			for (i=0,j=0; i<total_mno; i++,j+=2)
			{
				mno[i] = (int *) G_malloc(2*sizeof(int));
				mno[i][0] = atoi(opt3->answers[j]);
				mno[i][1] = atoi(opt3->answers[j+1]);
			}
		}
	}

	if (cur_frame->answer)
	{
		D_get_cur_wind(current_frame) ;
		opt1->answer = current_frame ;
	}

	if(!only_object->answer)
		fprintf (stdout,":\n# Shell Script created by d.save %s\n\n", G_date());

	G_get_window ( Mwind );

	redraw = 0;
	/* now start at the end (the earliest made window) and process them */
	for (p = npads-1; p >= 0; p--) {
		if (all_flag->answer || in_frame_list(opt1, pads[p]))
		{
			init_globals();
			if(!cur_frame->answer && !only_object->answer)
				fprintf (stdout,"\n# Here are the commands to create window: %s\n", pads[p]);
			stat = R_pad_select (pads[p]);
			if (stat) {
				R_pad_perror ("echo     ERROR", stat);
				fprintf (stdout,"exit -1\n\n");
				continue;
			}

			if (total_rno || total_mno)
			{
				stat = R_pad_get_item("list", &list, &nlists);
				if (stat || !nlists) {
					R_pad_perror ("echo     ERROR", stat);
					fprintf (stdout,"exit -1\n\n");
					continue;
				}

				R_pad_delete_item("list");

				live = (int *) G_malloc(nlists*sizeof(int));
				for (i=0; i<nlists; i++)
					live[i] = i;

				if (total_rno)
				{
					for (i=0; i<total_rno; i++)
					{
						if (rno[i]<-1 || rno[i]>nlists)
							continue;

						redraw = 1;

						rno[i] = (rno[i]==-1 ?
							  nlists:(rno[i]==0 ?
							   1:rno[i]));

						live[nlists-rno[i]] = -1;
					}
					G_free(rno);
				}

				if (total_mno)
				{
					for (i=0; i<total_mno; i++)
					{
						from = (mno[i][0]==-1 ?
							  nlists:(mno[i][0]==0 ?
							   1:mno[i][0]));
						to = (mno[i][1]==-1 ?
							  nlists:(mno[i][1]==0 ?
							   1:mno[i][1]));
						if (from<1		||
						    from>nlists		||
						    to<1		||
						    to>nlists		||
						    from==to		||
						    live[nlists-from]<0)
						{
							G_free(mno[i]);
							continue;
						}

						redraw = 1;

						tmp = live[nlists-to];
						live[nlists-to] = live[nlists-from];
						if (from<to)
						{
							for (j=nlists-from; j>=nlists-to+2; j--)
								live[j] = live[j-1];
							live[nlists-to+1] = tmp;
						}
						else
						{
							for (j=nlists-from; j<=nlists-to-2; j++)
								live[j] = live[j+1];
							live[nlists-to-1] = tmp;
						}
						G_free(mno[i]);
					}
					G_free(mno);
				}

				for (i=0;i<nlists;i++)
				{
					if (live[i]>=0)
						D_add_to_list(list[live[i]]);
				}
				G_free(live);
				R_pad_freelist (list, nlists);
			}

			if (process_pad(&items, &nitems) != 0) continue;

			Ftop = (100.0 * Wtop)/Sheight;
			Fbot = (100.0 * Wbot)/Sheight;
			Fleft = (100.0 * Wleft)/Swidth;
			Fright = (100.0 * Wright)/Swidth;
			if (Ftop<0) Ftop=0;
			if (Fbot<0) Fbot=0;
			if (Fleft<0) Fleft=0;
			if (Fright<0) Fright=0;

			if(!cur_frame->answer && !only_object->answer)
			{
				if (all_flag->answer && p==npads-1)
					fprintf (stdout,"d.frame -ec frame=%s at=%.4f,%.4f,%.4f,%.4f\n", pads[p],
						100-Fbot, 100-Ftop, Fleft, Fright);
				else
					fprintf (stdout,"d.frame -c frame=%s at=%.4f,%.4f,%.4f,%.4f\n", pads[p],
						100-Fbot, 100-Ftop, Fleft, Fright);
			}

			if(!only_object->answer) {
				if (Wcolor[0] == '\0')
					fprintf (stdout,"d.erase\n");
				else
					fprintf (stdout,"d.erase color=%s\n", Wcolor);

				if (Mtype != -1) {
					fprintf (stdout,"g.region n=%s s=%s e=%s w=%s nsres=%s ewres=%s\n",
					    Nstr, Sstr, Estr, Wstr, NSRESstr, EWRESstr);
				}
				fprintf (stdout,"\n");
			}

/* List already has commands to draw these maps.
			if (Wcell[0]!='\0')
				fprintf (stdout,"d.rast map=%s\n", Wcell);

			if (Wdig[0]!='\0')
				fprintf (stdout,"d.vect map=%s\n", Wdig);

			if (Wsite[0]!='\0')
				fprintf (stdout,"d.sites sitefile=%s\n", Wsite);
*/

			/* print out the list */
			i=0;
			temp_list = List;
			while (temp_list!=NULL) {
				i++;
				temp_list = temp_list->ptr;
			}

			while (List!=NULL) {
				fprintf (stdout,"%-70s # %d\n", List->string, i--);
				temp_list = List;
				List = List->ptr;
				G_free (temp_list->string);
				G_free (temp_list);
			}
			List_last = NULL;
		}
		if (! all_flag->answer && ! strcmp(opt1->answer, pads[p])) break;
	}
	if (!only_object->answer && (all_flag->answer || in_frame_list(opt1, Scurwin)))
		fprintf (stdout,"\nd.frame -s frame=%s\n", Scurwin);

	R_close_driver();

	if (redraw)
		G_system("d.redraw");

	exit(EXIT_SUCCESS);
}

/* return 1 if the padname is in the opt->answers list of frame names */
int 
in_frame_list (struct Option *opt, char *padname)
{
	int n;
	if (opt->answers)
		for (n=0; opt->answers[n] != NULL; n++)
			if (! strcmp(opt->answers[n], padname)) return(1);

	return(0);
}

int 
init_globals (void)
{
	Wtop = Wbot = Wleft = Wright = 0;
	Wcell[0] = '\0';
	Wdig[0] = '\0';
	Wsite[0] = '\0';
	Wcolor[0] = '\0';

	Mtype = Mwind->zone = -1;
	Mwind->ew_res = Mwind->ns_res = Mwind->north = Mwind->south 
	             = Mwind->east = Mwind->west = 0.0;

	return 0;
}



/* this array of strings defines the possible item types */
#define ITEM_TYPES 12
#define ITEM_SIZE 10
char Known_items[ITEM_TYPES][ITEM_SIZE] = {
	"cur_w",
	"d_win",
	"m_win",
	"time",
	"list",
	"erase",
	"cell",
	"dig",
	"site",
	"cell_list",
	"dig_list",
	"site_list"
};


/* this function returns the position in the Known_items array that
   the given item string matches or -1 if no match. */
int 
which_item (char *itemstr)
{
	int i;

	for(i=0; i<ITEM_TYPES; i++) {
		if (!strcmp(itemstr, Known_items[i])) return (i);
	}
	return (-1);
}

/* this function sets the global variable(s) associated with an item */
int 
set_item (char *item, char **list)
{
	char *err;
		
	if (!strcmp(item, "list")) process_list(item, list, 1);
	else {
		switch (which_item(item)) {
		case 0: /* cur_w */
			strcpy(Scurwin, list[0]);
			break;
		case 1:  /* d_win */
			sscanf(list[0]," %d %d %d %d ", &Wtop, &Wbot, &Wleft, &Wright);
			break;
		case 2:  /* m_win */
			sscanf(list[0], " %d %d %s %s %s %s %d %d ", 
			    &Mtype, &(Mwind->zone),
			    Estr, Wstr, Nstr, Sstr, 
			    &(Mwind->rows), &(Mwind->cols));
                        proj = G_projection();
			G_scan_easting(Estr, &(Mwind->east), proj);
			G_scan_easting(Wstr, &(Mwind->west), proj);
			G_scan_northing(Nstr, &(Mwind->north), proj);
			G_scan_northing(Sstr, &(Mwind->south), proj);
			if ( (err = G_adjust_Cell_head (Mwind, 1, 1)) ) {
			    G_fatal_error ( err );
			}
			G_format_resolution (Mwind->ew_res,  EWRESstr,  proj);
			G_format_resolution (Mwind->ns_res,  NSRESstr,  proj);
			break;
		case 3: /* time */
			break;
		case 5: /* d.erase color */
			sscanf(list[0]," %s ", Wcolor);
			break;
		case 6: /* cell */
			sscanf(list[0]," %s ", Wcell);
			break;
		case 7: /* dig */
			sscanf(list[0]," %s ", Wdig);
			break;
		case 8: /* site */
			sscanf(list[0]," %s ", Wsite);
			break;
		case 9:
		case 10:
		case 11:
			break;
		default:
			G_warning(_("Unknown item type in pad: %s"), item);
			break;
		}
	}

	return 0;
}

/* this function processes items which have multiple lines */
int 
process_list (char *item, char **list, int count)
{
	int n;
	struct list_struct *new_list;

	switch (which_item(item)) {
	case 4: /* list */
		for (n = 0; n < count; n++) {
			new_list = (struct list_struct *) G_malloc(sizeof(struct list_struct));
			new_list->ptr = NULL;
			new_list->string = (char *) G_malloc(strlen(list[n])+1);
			strcpy(new_list->string, list[n]);
			if (List == NULL)  /* nothing on the list yet */
				List = new_list;
			else
				List_last->ptr = new_list;

			List_last = new_list;
		}
		break;
	case 9:
	case 10:
	case 11:
		break;
	default: /* otherwise */
		G_warning(_("Unknown item type in pad: %s"), item);
		break;
	}


	return 0;
}


/* this function processes the items in a pad */
int 
process_items (char **items, int nitems)
{
	int count;
	char **list;
	int i;
	int stat ;

	for (i = nitems-1; i >= 0; i--)
	{
		stat = R_pad_get_item (items[i], &list, &count);
		if (stat) {
			R_pad_perror ("#          ERROR", stat);
			fprintf (stdout,"exit -1\n\n");
			continue;
		}
		if (count==1) set_item(items[i],list);
		else process_list(items[i],list, count);

		R_pad_freelist (list,count);
	}

	return 0;
}

/* this function processes a pad */
int 
process_pad (char ***items, int *nitems)
{
	int stat;

	stat = R_pad_list_items (items, nitems);
	if (stat) {
		R_pad_perror ("echo     ERROR", stat);
		fprintf (stdout,"exit -1\n\n");
		return(-1);
	}

	process_items(*items, *nitems);
	return(0);
}
