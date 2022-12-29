#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include "local_proto.h"

int get_double(const struct proj_parm *parm, const struct proj_desc *desc,
	       double *val)
{
    char answer[200];

    sprintf(answer, "Enter %s ", desc->desc);
    *val = prompt_num_double(answer, parm->deflt, 1);
    return 1;
}

int get_int(const struct proj_parm *parm, const struct proj_desc *desc,
	    int *val)
{
    char answer[200];

    sprintf(answer, "Enter %s ", desc->desc);
    *val = prompt_num_int(answer, (int)parm->deflt, 1);
    return 1;
}

int get_zone(void)
{
    char answer[200];
    int first_time = 1;

    zone = -1;
    while ((zone < 0) || (zone > 60)) {
	if (first_time)
	    first_time = 0;
	else
	    fprintf(stdout, "Invalid zone! Try Again:\n");
	sprintf(answer, "Enter Zone");
	zone = prompt_num_int(answer, 0, 0);
    }
    return (1);
}


/*
 *    Get the Prime Meridian value and std parallel value
 **** */
int get_LL_stuff(const struct proj_parm *parm, const struct proj_desc *desc,
		 int lat, double *val)
{
    char answer[200];
    char buff[256];

    /*  get LONCEN value arguments */
    if (parm->def_exists == 1) {
	if (lat == 1) {
	    G_format_northing(parm->deflt, buff, PROJECTION_LL);
	    fprintf(stderr, "\n    Enter %s (%s) :", desc->desc, buff);
	}
	else {
	    G_format_easting((parm->deflt), buff, PROJECTION_LL);
	    fprintf(stderr, "\n    Enter %s (%s) :", desc->desc, buff);
	}
	G_gets(answer);
	if (strlen(answer) == 0) {
	    *val = parm->deflt;
	    return (1);
	}
    }
    else {
	fprintf(stderr, "\n    Enter %s :", desc->desc);
	G_gets(answer);
	if (strlen(answer) == 0) {
	    *val = 0.0;
	    return (0);
	}
    }
    if (lat == 1) {
	if (!get_deg(answer, 1)) {
	    return (0);
	}
    }
    else {
	if (!get_deg(answer, 0)) {
	    return (0);
	}
    }
    sscanf(answer, "%lf", val);
    return (1);
}

double prompt_num_double(char *str, double deflt, int is_default)
{
    char answer[300];
    double tmp;

    while (1) {
	if (is_default)
	    fprintf(stderr, "\n%s [%.10f]: ", str, deflt);
	else
	    fprintf(stderr, "\n%s: ", str);

	G_gets(answer);
	G_strip(answer);
	if (strlen(answer) == 0 && is_default)
	    return deflt;
	else if (sscanf(answer, "%lf", &tmp) == 1)
	    break;
    }
    return tmp;
}


int prompt_num_int(char *str, int deflt, int is_default)
{
    char answer[300];
    int tmp;

    while (1) {
	if (is_default)
	    fprintf(stderr, "\n%s [%d]: ", str, deflt);
	else
	    fprintf(stderr, "\n%s: ", str);

	G_gets(answer);
	G_strip(answer);
	if (strlen(answer) == 0 && is_default)
	    return deflt;
	else if (1 == sscanf(answer, "%d", &tmp))
	    break;
    }
    return tmp;
}
