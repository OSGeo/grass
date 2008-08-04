#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"

extern FILE *tracefd;
extern FILE *inputfd;

int input(int level, char *buf, char *help[])
{
    char temp1[10], temp2[3];
    int i;

    if (level && isatty(fileno(inputfd)))
	fprintf(stdout,
		"enter 'help' for help, 'end' when done, 'exit' to quit\n");

    do {
	if (level && isatty(fileno(inputfd)))
	    fprintf(stdout, "%s ", level == 1 ? ">" : ">>>");
	if (!G_getl2(buf, 1024, inputfd)) {
	    if (inputfd != stdin) {
		fclose(inputfd);
		inputfd = stdin;
	    }
	    return 0;
	}

	if (tracefd != NULL) {
	    fprintf(tracefd, "%s\n", buf);
	    fflush(tracefd);
	}

	if (sscanf(buf, "%5s %1s", temp1, temp2) == 1) {
	    if (strcmp(temp1, "end") == 0)
		return 0;
	    if (strcmp(temp1, "exit") == 0)
		exit(0);
	    if (strcmp(temp1, "help") == 0) {
		*buf = '#';
		if (help != NULL) {
		    for (i = 0; help[i] && help[i][0]; i++)
			fprintf(stdout, "%s\n", help[i]);
		    fprintf(stdout,
			    "enter 'end' when done, 'exit' to quit\n");
		}
	    }
	}
    }
    while (*buf == '#');

    if (level)
	add_to_session(level > 1, buf);

    if (*buf == '\\')
	for (i = 0; (buf[i] = buf[i + 1]); i++) ;
    return 1;
}

int gobble_input(void)
{
    char buf[1024];

    if (inputfd != stdin) {
	fclose(inputfd);
	inputfd = stdin;
    }
    else if (!isatty(0))
	while (input(0, buf, (char **)NULL)) ;

    return 0;
}
