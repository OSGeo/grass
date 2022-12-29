#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>

int inpt(FILE * rulefd, char *buf)
{
    do {
	if (!G_getl2(buf, 1024, rulefd))
	    return 0;
	G_strip(buf);
    }
    while (*buf == '#');
    return 1;
}

int key_data(char *buf, char **k, char **d)
{
    char *key, *data;

    for (key = buf; *key; key++)
	if (*key != ' ' && *key != '\t')
	    break;

    if (*key == 0)
	return 0;

    for (data = key + 1; *data; data++)
	if (*data == ' ' || *data == '\t')
	    break;

    if (*data)
	*data++ = 0;

    *k = key;
    *d = data;

    return 1;
}
