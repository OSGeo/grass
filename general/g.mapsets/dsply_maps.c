#include <string.h>
#include <stdio.h>
#include "externs.h"

static int display1(void);
static int display2(void);

int display_available_mapsets(int verbose)
{
    if (verbose)
	display1();
    else
	display2();

    return 0;
}

static int display1(void)
{
    int n;

    fprintf(stdout, "Available mapsets:");
    for (n = 0; n < nmapsets; n++) {
	if (n % 4)
	    fprintf(stdout, " ");
	else
	    fprintf(stdout, "\n");
	fprintf(stdout, "%2d %-15s", n + 1, mapset_name[n]);
    }
    fprintf(stdout, "\n");
    if (nmapsets == 0)
	fprintf(stdout, "** no mapsets **\n");
    fprintf(stdout, "\n");

    return 0;
}

static int display2(void)
{
    int nleft, len, n;
    char *name;

    nleft = 78;
    for (n = 0; n < nmapsets; n++) {
	len = strlen(name = mapset_name[n]);
	if (len > nleft) {
	    fprintf(stdout, "\n");
	    nleft = 78;
	}
	fprintf(stdout, "%s ", name);
	nleft -= (len + 1);
    }
    fprintf(stdout, "\n");

    return 0;
}
