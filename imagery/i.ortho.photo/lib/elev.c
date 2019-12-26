
/**********************************************************
* I_get_group_elev (group, elev_layer, mapset_elev, tl, math_exp, units, nd);
* I_put_group_elev (group, elev_layer, mapset_elev, tl, math_exp, units, nd);
**********************************************************/
#include <stdio.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/ortholib.h>
#include <grass/glocale.h>
#include "orthophoto.h"


#define IN_BUF 200


/* Put the "elev" name into the block file "ELEV" */
int I_put_group_elev(char *group, char *elev, char *mapset_elev, char *tl,
		     char *math_exp, char *units, char *nd)
{
    FILE *fd;

    /*    G_suppress_warnings(1); */
    fd = (FILE *) I_fopen_group_elev_new(group);
    /*    G_suppress_warnings(0); */
    if (fd == NULL)
	return 0;

    fprintf(fd, "elevation layer :%s\n", elev);
    fprintf(fd, "mapset elevation:%s\n", mapset_elev);
    fprintf(fd, "location        :%s\n", tl);
    fprintf(fd, "math expression :%s\n", math_exp);
    fprintf(fd, "units           :%s\n", units);
    fprintf(fd, "no data values  :%s\n", nd);

    return 0;
}


/* Return the elev name from the block file ELEV
    returns 0 on fail,  1 on success */
int I_get_group_elev(char *group, char *elev, char *mapset_elev, char *tl,
		     char *math_exp, char *units, char *nd)
{
    char *err;
    char buf[IN_BUF];
    FILE *fd;

    if (!I_find_group_elev_file(group)) {
	G_warning(
	    _("Unable to find elevation file for group <%s> in mapset <%s>"),
	      group, G_mapset());
	return 0;
    }

    G_suppress_warnings(1);
    fd = I_fopen_group_elev_old(group);
    G_suppress_warnings(0);

    if (!fd) {
	G_warning(
	    _("Unable to open elevation file for group <%s> in mapset <%s>"),
	      group, G_mapset());
	G_sleep(3);

	return 0;
    }

    err=fgets(buf, IN_BUF, fd);
    if(err==NULL) G_fatal_error(_("Unable to read elevation parameter file"));
    sscanf(buf, "elevation layer :%s\n", elev);
    err=fgets(buf, IN_BUF, fd);
    if(err==NULL) G_fatal_error(_("Unable to read elevation parameter file"));
    sscanf(buf, "mapset elevation:%s\n", mapset_elev);
    err=fgets(buf, IN_BUF, fd);
    if(err==NULL) G_fatal_error(_("Unable to read elevation parameter file"));
    sscanf(buf, "location        :%s\n", tl);
    err=fgets(buf, IN_BUF, fd);
    if(err==NULL) G_fatal_error(_("Unable to read elevation parameter file"));
    sscanf(buf, "math expression :%s\n", math_exp);
    err=fgets(buf, IN_BUF, fd);
    if(err==NULL) G_fatal_error(_("Unable to read elevation parameter file"));
    sscanf(buf, "units           :%s\n", units);
    err=fgets(buf, IN_BUF, fd);
    if(err==NULL) G_fatal_error(_("Unable to read elevation parameter file"));
    sscanf(buf, "no data values  :%s\n", nd);
    fclose(fd);

    return 1;
}
