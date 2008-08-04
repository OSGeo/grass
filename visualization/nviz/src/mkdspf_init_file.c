#include "viz.h"

/* Nvision includes */
#include "interface.h"

char *rindex();

viz_init_file(args, interp)
     char *args[];
     Tcl_Interp *interp;
{
    /* compares the suffix to see if .grid3 (returns a 1) or .sds (returns a 2) */
    char buf[200], *p = NULL;

    strcpy(buf, args[1]);

    p = rindex(buf, '.');	/*sets pointer p to last occurrence of '.' */
    if (p == NULL) {
	Tcl_AppendResult(interp, "rindex returned a NULL\n", NULL);
	pr_commandline(interp);	/* program will exit */
	return (TCL_ERROR);
    }

    strcpy(buf, p);		/*truncates buf to only include the file name suffix */

    if (strcmp(buf, ".grid3") == 0)
	Headfax.token = 1;

    else if (strcmp(buf, ".sds") == 0)
	Headfax.token = 2;

    else {
	pr_commandline(interp);	/* program will exit */
	return (TCL_ERROR);
    }
}
