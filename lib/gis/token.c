
/**
 * \file token.c
 *
 * \brief GIS Library - Token functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <stdlib.h>
#include <grass/gis.h>


/**
 * \brief Tokenize string.
 *
 * Given a string, <b>buf</b>, turn delimiter, <b>delim</b>, into '\0' 
 * (NULL) and place pointers to tokens in tokens.  <b>buf</b> must not 
 * contain a new line (\n).
 *
 * \param[in] buf input string
 * \param[in] delim string delimiter
 * \return Pointer to string token
 */

char **G_tokenize(const char *buf, const char *delim)
{
    int i;
    char **tokens;
    char *p;

    i = 0;
    while (!G_index(delim, *buf) && (*buf == ' ' || *buf == '\t'))	/* needed for G_free () */
	buf++;

    p = G_store(buf);

    tokens = (char **)G_malloc(sizeof(char *));

    while (1) {
	while (!G_index(delim, *p) && (*p == ' ' || *p == '\t'))
	    p++;
	if (*p == 0)
	    break;
	tokens[i++] = p;
	tokens = (char **)G_realloc((char *)tokens, (i + 1) * sizeof(char *));

	while (*p && (G_index(delim, *p) == NULL))
	    p++;
	if (*p == 0)
	    break;
	*p++ = 0;
    }
    tokens[i] = NULL;

    return (tokens);
}


/**
 * \brief Return number of tokens.
 *
 * <b>Note:</b> Function is incomplete.
 *
 * \param[in] tokens
 * \return number of tokens
 */

int G_number_of_tokens(char **tokens)
{
    int n;

    for (n = 0; tokens[n] != NULL; n++) {
	/* nothing */
    }

    return n;
}


/**
 * \brief Free memory allocated to tokens.
 *
 * <b>Note:</b> <i>G_free_tokens()</i> must be called when finished with 
 * tokens to release memory.
 *
 * \param[in,out] tokens
 * \return always returns 0
 */

int G_free_tokens(char **tokens)
{
    if (tokens[0] != NULL)
	G_free(tokens[0]);
    G_free(tokens);

    return (0);
}
