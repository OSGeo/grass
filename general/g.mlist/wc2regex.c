#include <grass/gis.h>

char *wc2regex(const char *wc)
{
    int i, j;
    char *regex;

    for (i = 0, j = 2; wc[i]; i++, j++) {
	switch (wc[i]) {
	case '.':
	case '*':
	    j++;
	    break;
	}
    }
    regex = (char *)G_malloc(j + 1);
    j = 0;
    regex[j++] = '^';
    for (i = 0; wc[i]; i++) {
	switch (wc[i]) {
	case '.':
	    regex[j++] = '\\';
	    break;
	case '*':
	    regex[j++] = '.';
	    break;
	case '?':
	    regex[j++] = '.';
	    continue;
	case '{':
	    regex[j++] = '(';
	    continue;
	case '}':
	    regex[j++] = ')';
	    continue;
	case ',':
	    regex[j++] = '|';
	    continue;
	}
	regex[j++] = wc[i];
    }
    regex[j++] = '$';
    regex[j] = 0;

    return regex;
}
