/*
 *   \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "utility.h"

/*if occurred an error returns NULL */
char *concatena(const char *str1, const char *str2)
{
    char *conc = (char *)G_malloc(strlen(str1) + strlen(str2) + 1);

    if (conc == NULL) {
	return NULL;
    }
    strcpy(conc, str1);
    strcat(conc, str2);
    return conc;
}


/* split_arg  returns  the array of token find in linea separated by separatore presente
 * and write in argc the nummer of find token */
/*if occurred an error returns NULL */
char **split_arg(char *linea, char separatore, long *numerotoken)
{
    char **argv;		/* token array */
    char *copialinea;		/* line copy */

    long i;			/* find token number */
    long it;			/* iterator */
    long num;

    int term;			/* =0 if last token has not /0 */


    i = 0;
    term = 0;

    if (linea == NULL || linea[0] == '\0') {
	*numerotoken = 0;
	return NULL;
    }

    /* copy string */
    copialinea = (char *)G_malloc(strlen(linea) + 1);
    if (copialinea == NULL) {
	return NULL;
    }
    strcpy(copialinea, linea);


    argv = (char **)G_malloc(sizeof(char *));
    if (argv == NULL) {
	return NULL;
    }

    num = 0;
    for (it = 0; it < strlen(copialinea); it++) {
	if (copialinea[it] == separatore) {
	    while (copialinea[it + 1] == separatore)
		it++;
	    if (i != 0) {
		argv[i - 1] =
		    G_realloc(argv[i - 1], (num + 1) * sizeof(char));
		argv[i - 1][num] = '\0';
		argv = (char **)G_realloc(argv, (i + 1) * sizeof(char *));
		num = 0;
	    }
	    if ((it + 1) == strlen(copialinea))
		term = 1;
	}
	else {
	    if (num == 0) {
		i++;
		argv[i - 1] = G_malloc(sizeof(char));
		if (argv[i - 1] == NULL) {
		    return NULL;
		}
	    }
	    else {
		argv[i - 1] =
		    G_realloc(argv[i - 1], (num + 1) * sizeof(char));
	    }

	    argv[i - 1][num] = copialinea[it];
	    num++;
	}
    }

    if (!term) {
	argv[i - 1] = G_realloc(argv[i - 1], (num + 1) * sizeof(char));
	argv[i - 1][num] = '\0';
    }


    *numerotoken = i;

    G_free(copialinea);

    return argv;
}
