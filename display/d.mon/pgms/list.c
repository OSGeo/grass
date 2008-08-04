/* list.mon - list entries in monitorcap file */

#include <grass/raster.h>
#include <stdio.h>
#include <grass/monitors.h>

int main(int argc, char *argv[])
{
    struct MON_CAP *cap;
    struct MON_CAP *R_parse_monitorcap();
    int n;
    char *fmt1 = "%-15s %-30s\n";
    char *fmt2 = "%-15s %-30s (%s)\n";

    n = 0;
    while ((cap = R_parse_monitorcap(MON_NEXT, "")) != NULL) {
	if (n++ == 0) {
	    fprintf(stdout, fmt1, "name", "description");
	    fprintf(stdout, fmt1, "----", "-----------");
	}
	if (*(cap->tty) != '\0')
	    fprintf(stdout, fmt2, cap->name, cap->comment, cap->where);
	else
	    fprintf(stdout, fmt1, cap->name, cap->comment);
    }
    if (!n)
	fprintf(stdout, "     no known monitors\n");

    return 0;
}
