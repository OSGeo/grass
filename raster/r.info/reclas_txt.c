#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"


int reclass_text(char *text, const CELL cat, struct Reclass *reclass,
		 const int next)
{
    int i;
    int n;
    int first;
    CELL min;

    *text = 0;

    n = reclass->num;
    min = reclass->min;

    first = -1;
    for (i = next; i < n; i++) {
	if (reclass->table[i] == cat) {
	    if (first < 0)
		first = i;
	}
	else if (first >= 0) {
	    do_text(text, (long)first + min, (long)i - 1 + min);
	    first = -1;
	    if (strlen(text) > 20)
		return i;
	}
    }
    if (first >= 0)
	do_text(text, (long)first + min, (long)i - 1 + min);

    return -1;
}


int do_text(char *text, const long first, const long last)
{
    char work[40];

    if (*text)
	strcat(text, " ");

    if (first == last)
	sprintf(work, "%ld", first);
    else
	sprintf(work, "%ld-%ld", first, last);

    strcat(text, work);

    return 0;
}
