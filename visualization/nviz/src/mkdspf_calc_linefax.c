#include "viz.h"

/* Nvision stuff */
#include "interface.h"
#include <stdlib.h>
#include <grass/gis.h>

viz_calc_linefax(linefax, args, nargs, interp)
     cmndln_info *linefax;
     char *args[];
     int nargs;
     Tcl_Interp *interp;
{
    char buf[30];
    int type;
    float interval;		/*increment tvalue for intervals */
    float datarange;		/*diff btwn min and max data tvalues */
    float current_tvalue;	/*the threshold value being computed */
    float min, max;
    int ithresh;
    int i;			/*looping variable */
    char **thresh_values;
    int num_thresh_values;

    /* This subroutine parses thru the command line and assigns tvalues to
     ** the linefax structure.  Different placement of commandline arguments
     ** due to -i -c -r are dealt with individually.  #of thresholds and the
     ** thresholds are calculated.  Note that the maximum nthresber of thresholds
     ** that are computed is 127.  This is to set at cap on the size of the
     ** display file that is created */

    /* Changed this one a bit so it parses the tcl/tk command line arguments
       Mark (9/7/94)
     */

    /* checking to see if threshold type is possible combination */
    /* this does not guarantee that all commandline arguments are valid */

    if (strcmp(args[3], "c") == 0) {
	type = 1;
	if (lit_model(linefax, args[5], interp) != TCL_OK)
	    return (TCL_ERROR);
    }

    else if (strcmp(args[3], "i") == 0) {
	/* Get the list holding the threshold values and figure out how long it is */
	if (Tcl_SplitList(interp, args[4], &num_thresh_values, &thresh_values)
	    != TCL_OK) {
	    pr_commandline(interp);
	    return (TCL_ERROR);
	}

	ithresh = num_thresh_values;

	if (ithresh > 127) {
	    Tcl_AppendResult(interp, "Maximum number of thresholds is 127\n",
			     NULL);
	    Tcl_AppendResult(interp, "Using the first 127 entered\n", NULL);
	    linefax->nthres = 127;
	}
	else
	    linefax->nthres = ithresh;

	type = 2;
	if (lit_model(linefax, args[5], interp) != TCL_OK)
	    return (TCL_ERROR);
    }

    else if (strcmp(args[3], "r") == 0) {
	type = 3;
	if (lit_model(linefax, args[5], interp) != TCL_OK)
	    return (TCL_ERROR);
    }

    else {
	pr_commandline(interp);
	return (TCL_ERROR);
    }

    switch (type) {
    case 1:			/*complete traversal of data at set interval */
	/* Note: maximum nthresber of thresholds is set at 127 */
	{
	    double temp;

	    if (Tcl_GetDouble(interp, args[4], &temp) != TCL_OK) {
		pr_commandline(interp);
		return (TCL_ERROR);
	    }
	    interval = (float)temp;	/* off the command line */
	}

	datarange = Headfax.max - Headfax.min;

	{
	    char temp1[20], temp2[20], temp3[20];

	    sprintf(temp1, "%f", Headfax.max);
	    sprintf(temp2, "%f", Headfax.min);
	    sprintf(temp3, "%f", interval);
	    Tcl_AppendResult(interp, "Max thresh ", temp1, "  Min thresh ",
			     temp2, "\nInterval ", temp3, "\n", NULL);
	}

	if (((datarange) / interval) > 126.0) {	/* means #thres = 127 */
	    char temp[20];

	    interval = datarange / 126.0;
	    sprintf(temp, " %f\n", interval);
	    Tcl_AppendResult(interp,
			     "Maximum number of thresholds exceeded. \n",
			     "New interval", temp, NULL);
	}

	linefax->nthres = (int)((datarange) / interval) + 1;

	{
	    char temp1[20];

	    sprintf(temp1, "%d", linefax->nthres);
	    Tcl_AppendResult(interp, "Number of thresholds ", temp1, "\n",
			     NULL);
	}

	current_tvalue = Headfax.min;	/* assign first threshold */
	for (i = 0; i < linefax->nthres; i++) {
	    linefax->tvalue[i] = current_tvalue;
	    current_tvalue += interval;
	}
	break;

    case 2:			/*isolated thresholds as listed on commandline */
	for (i = 0; i < linefax->nthres; i++)
	    linefax->tvalue[i] = (float)atof(thresh_values[i]);
	Tcl_Free((char *)thresh_values);
	break;

    case 3:			/* min and max given on commandline as well as interval */
	/* Need to extract values from list */
	{
	    int num_list_args;
	    char **list_args;

	    Tcl_SplitList(interp, args[4], &num_list_args, &list_args);
	    interval = (float)atof(list_args[2]);
	    max = (float)atof(list_args[1]);
	    min = (float)atof(list_args[0]);
	    datarange = max - min;

	    Tcl_AppendResult(interp, "Max thresh ", list_args[1],
			     "  Min thresh ", list_args[0], "\nInterval ",
			     list_args[2], "\n", NULL);

	    Tcl_Free((char *)list_args);
	}

	if ((datarange / interval) > 126.0) {
	    char temp1[20];

	    interval = datarange / 126.0;
	    sprintf(temp1, "%f", interval);
	    Tcl_AppendResult(interp,
			     "Maximum number of thresholds exceeded. \n",
			     "New interval is ", temp1, "\n", NULL);
	}

	linefax->nthres = (int)(datarange) / interval + 1;

	{
	    char temp1[20];

	    sprintf(temp1, "%d", linefax->nthres);
	    Tcl_AppendResult(interp, "Number of thresholds ",
			     temp1, "\n", NULL);
	}

	current_tvalue = min;
	for (i = 0; i < linefax->nthres; i++) {
	    linefax->tvalue[i] = current_tvalue;
	    current_tvalue += (float)interval;
	}
	break;

    }

    return (TCL_OK);
}

/****************************** lit_model ************************************/

/****************************** lit_model ************************************/

/****************************** lit_model ************************************/
int lit_model(linefax, typeid, interp)
     cmndln_info *linefax;
     char *typeid;		/*command line argument indicating lighting model */
     Tcl_Interp *interp;
{
    char buf[30];

    strcpy(buf, typeid);

    if (strcmp(buf, "f") == 0) {
	linefax->litmodel = 1;
	return (TCL_OK);
    }


    if (strcmp(buf, "g") == 0) {
	linefax->litmodel = 2;
	return (TCL_OK);
    }

    else {
	pr_commandline(interp);
	return (TCL_ERROR);
    }

}
