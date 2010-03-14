#include <string.h>
#include <grass/glocale.h>
#include "filter.h"
#include "local_proto.h"

FILTER *get_filter(char *name, int *nfilters, char *title)
{
    FILE *fd;
    FILTER *filter, *f;
    char buf[300];
    char temp[100];
    char label[50];
    int row, col;
    int n;
    int have_divisor;
    int have_type;
    int have_start;
    int count;
    double div;

    f = filter = 0;
    count = *nfilters = 0;
    *title = 0;

    fd = fopen(name, "r");
    if (!fd) {
	G_fatal_error(_("Cannot open filter file '%s'"), name);
    }

    while (fgets(buf, sizeof buf, fd)) {
	G_strip(buf);
	if (*buf == '#' || *buf == 0)
	    continue;

	if (sscanf(buf, "%s %[^\n]", label, temp) == 2) {
	    uppercase(label);
	    if (strcmp(label, "TITLE") == 0) {
		G_strip(temp);
		strcpy(title, temp);
		continue;
	    }
	}

	uppercase(buf);
	if (sscanf(buf, "MATRIX %d", &n) == 1) {
	    if (n < 3) {
		G_fatal_error(_("Illegal filter matrix size specified"));
	    }
	    if (n % 2 == 0) {
		G_fatal_error(_("Even filter matrix size specified"));
	    }

	    count++;
	    filter = (FILTER *) G_realloc(filter, count * sizeof(FILTER));
	    f = &filter[count - 1];
	    f->size = n;
	    f->divisor = 1;
	    f->dmatrix = NULL;
	    f->type = PARALLEL;
	    f->start = UL;
	    have_divisor = 0;
	    have_type = 0;
	    have_start = 0;

	    f->matrix = (DCELL **) G_malloc(n * sizeof(DCELL *));
	    for (row = 0; row < n; row++)
		f->matrix[row] = (DCELL *) G_malloc(n * sizeof(DCELL));

	    for (row = 0; row < n; row++)
		for (col = 0; col < n; col++)
		    if (fscanf(fd, "%lf", &f->matrix[row][col]) != 1) {
			G_fatal_error(_("Illegal filter matrix"));
		    }
	    continue;
	}
	if (sscanf(buf, "DIVISOR %lf", &div) == 1)
	    if (sscanf(buf, "%s", label) == 1 &&
		strcmp(label, "DIVISOR") == 0) {
		if (!filter) {
		    G_fatal_error(_("Filter file format error"));
		}
		if (have_divisor) {
		    G_fatal_error(_("Duplicate filter divisor specified"));
		}
		have_divisor = 1;
		if (sscanf(buf, "DIVISOR %lf", &div) == 1) {
		    f->divisor = div;
		    if (div == 0)
			f->dmatrix = f->matrix;
		    continue;
		}
		f->divisor = 0;
		f->dmatrix = (DCELL **) G_malloc(f->size * sizeof(DCELL *));
		for (row = 0; row < f->size; row++)
		    f->dmatrix[row] =
			(DCELL *) G_malloc(f->size * sizeof(DCELL));

		for (row = 0; row < f->size; row++)
		    for (col = 0; col < f->size; col++)
			if (fscanf(fd, "%lf", &f->dmatrix[row][col]) != 1) {
			    G_fatal_error(_("Illegal divisor matrix"));
			}
		continue;
	    }
	if (sscanf(buf, "TYPE %s", temp) == 1) {
	    if (!filter) {
		G_fatal_error(_("Filter file format error"));
	    }
	    if (have_type) {
		G_fatal_error(_("Duplicate filter type specified"));
	    }
	    if (strcmp(temp, "P") == 0)
		f->type = PARALLEL;
	    else if (strcmp(temp, "S") == 0)
		f->type = SEQUENTIAL;
	    else {
		G_fatal_error(_("Illegal filter type specified"));
	    }
	    have_type = 1;
	    continue;
	}
	if (sscanf(buf, "START %s", temp) == 1) {
	    if (!filter) {
		G_fatal_error(_("Filter file format error"));
	    }
	    if (have_start) {
		G_fatal_error(_("Duplicate filter start specified"));
	    }
	    if (strcmp(temp, "UL") == 0)
		f->start = UL;
	    /* disable any other starting corner until the rest of
	     * this program handles them properly
	     */
	    else
		G_warning(_("Filter start %s ignored, using UL"), temp);

	    /* disable these others
	       else if (strcmp (temp, "UR") == 0)
	       f->start  = UR ;
	       else if (strcmp (temp, "LL") == 0)
	       f->start  = LL ;
	       else if (strcmp (temp, "LR") == 0)
	       f->start  = LR ;
	       else
	       {
	       ERROR ("illegal filter type specified");
	       }
	     */
	    have_start = 1;
	    continue;
	}

	/* other lines are ignored */
    }

    if (!filter) {
	G_fatal_error(_("Illegal filter file format"));
    }

    *nfilters = count;
    return filter;
}
