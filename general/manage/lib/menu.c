#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "list.h"

static int count;
static char **text;
static int first, last;

int menu(int type)
{
    int i;
    int n;
    int x;
    char buf[100];

    build_menu();

    first = 0;

    while (1) {
	last = first + 30;
	if (last > count)
	    last = count;

	G_clear_screen();
	switch (type) {
	case RENAME:
	    fprintf(stdout, "RENAME");
	    break;
	case REMOVE:
	    fprintf(stdout, "REMOVE");
	    break;
	case COPY:
	    fprintf(stdout, "COPY");
	    break;
	case LIST:
	    fprintf(stdout, "LIST");
	    break;
	}
	fprintf(stdout, " FACILITY\n\n");
	fprintf(stdout, "This program allows you to ");
	switch (type) {
	case RENAME:
	    fprintf(stdout, "rename files found in your mapset");
	    break;
	case REMOVE:
	    fprintf(stdout, "remove files found in your mapset");
	    break;
	case COPY:
	    fprintf(stdout, "copy files from other mapsets into your mapset");
	    break;
	case LIST:
	    fprintf(stdout, "list files from mapsets in your search path");
	    break;
	}
	fprintf(stdout, "\n\n");
	fprintf(stdout, "Please select the type of file to be ");
	switch (type) {
	case RENAME:
	    fprintf(stdout, "renamed");
	    break;
	case REMOVE:
	    fprintf(stdout, "removed");
	    break;
	case COPY:
	    fprintf(stdout, "copied");
	    break;
	case LIST:
	    fprintf(stdout, "listed");
	    break;
	}

	fprintf(stdout, "\n\n");
	display_menu();
	fprintf(stdout, "\n");

	if (first > 0)
	    fprintf(stdout, "  -    to see previous menu page\n");
	if (last < count)
	    fprintf(stdout, "  +    to see next menu page\n");
	fprintf(stdout, "RETURN to exit\n\n");

	fprintf(stdout, "> ");

	if (!G_gets(buf))
	    continue;
	if (*buf == 0) {
	    free_menu();
	    return -1;
	}
	G_strip(buf);
	if (first > 0 && strcmp(buf, "-") == 0) {
	    first -= 30;
	    if (first < 0)
		first = 0;
	    continue;
	}
	if (last < count && strcmp(buf, "+") == 0) {
	    first = last;
	    continue;
	}
	if (sscanf(buf, "%d", &x) != 1)
	    continue;
	i = 1;
	for (n = 0; n < nlist; n++)
	    if (list[n].status && (i++ == x)) {
		free_menu();
		return n;
	    }
    }
}

int build_menu(void)
{
    char buf[100];
    int n;

    count = 0;
    text = 0;
    for (n = 0; n < nlist; n++)
	if (list[n].status) {
	    sprintf(buf, "%3d %-.30s", ++count, list[n].text);
	    text = (char **)G_realloc(text, count * sizeof(char *));
	    text[count - 1] = G_store(buf);
	}

    return 0;
}

int free_menu(void)
{
    while (count-- > 0)
	G_free(text[count]);

    return 0;
}
int display_menu(void)
{
    int left, right;
    int i;

    left = first;
    right = first + 15;

    for (i = 0; i < 15; i++) {
	if (left >= last)
	    break;
	if (right >= last)
	    fprintf(stdout, "  %-35.35s\n", text[left]);
	else
	    fprintf(stdout, "  %-35.35s  %-35.35s\n", text[left],
		    text[right]);

	left++;
	right++;
    }

    return 0;
}
