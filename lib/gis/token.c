
/*!
  \file lib/gis/token.c
  
  \brief GIS Library - Tokenize strings
  
  (C) 2001-2008, 2011-2013 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author USA CERL and others
*/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static char **tokenize(const char *, const char *, const char *);

/*!
  \brief Tokenize string
  
  Given a string, <em>buf</em>, turn delimiter, <em>delim</em>, into
  '\0' (NULL) and place pointers to tokens in tokens. <em>buf</em>
  must not contain a new line (\n). <em>delim</em> may consist of more
  than one character. G_free_tokens() must be called when finished
  with tokens to release memory.
  
  Example:
  \code
  char **tokens;
  int ntok, i;
  tokens = G_tokenize(buf, " |:,");
  ntok = G_number_of_tokens(tokens);
  for (i=0; i < ntok; i++) {
     G_debug(1, "%d=[%s]", i, tokens[i]);
  }
  G_free_tokens(tokens);
  \endcode
  
  \param buf input string
  \param delim string delimiter
  
  \return pointer to string token
*/
char **G_tokenize(const char *buf, const char *delim)
{
    return tokenize(buf, delim, NULL);
}

/*!
  \brief Tokenize string
  
  This function behaves similarly to G_tokenize().

  It introduces <em>valchar</em> which defines borders of token. Within
  token <em>delim</em> is ignored.
  
  Example:
  \code
  char *str = "a,'b,c',d";

  char **tokens1, **tokens2;
  int ntok1, ntok2; 
  
  tokens1 = G_tokenize(str, ",");
  ntok1 = G_number_of_tokens(tokens1);

  tokens1 = G_tokenize2(str, ",", "'");
  ntok2 = G_number_of_tokens(tokens2);
  \endcode

  In this example <em>ntok1</em> will be 4, <em>ntok2</em> only 3,
  i.e. { "a", "'b, c'", "d"}

  \param buf input string
  \param delim string delimiter
  \param valchar character defining border of token

  \return pointer to string token
*/
char **G_tokenize2(const char *buf, const char *delim, const char *valchar)
{
    return tokenize(buf, delim, valchar);
}

char **tokenize(const char *buf, const char *delim, const char *inchar)
{
    int i;
    char **tokens;
    const char *p;
    char *q;
    enum {
	S_START,
	S_IN_QUOTE,
	S_AFTER_QUOTE,
    };
    enum {
	A_NO_OP,
	A_ADD_CHAR,
	A_NEW_FIELD,
	A_END_RECORD,
	A_ERROR
    };
    int state;
    int quo = inchar ? *inchar : -1;

    /* do not modify buf, make a copy */
    p = q = G_store(buf);

    i = 0;
    tokens = (char **)G_malloc(2 * sizeof(char *));

    /* always one token */
    tokens[i++] = q;

    for (state = S_START; ; p++) {
	int c = *p;
	int action = A_NO_OP;
	switch (state) {
	case S_START:
	    if (c == quo)
		state = S_IN_QUOTE;
	    else if (c == '\0')
		action = A_END_RECORD;
	    else if (strchr(delim, c))
		action = A_NEW_FIELD;
	    else
		action = A_ADD_CHAR;
	    break;
	case S_IN_QUOTE:
	    if (c == quo)
		state = S_AFTER_QUOTE;
	    else if (c == '\0')
		action = A_ERROR;
	    else
		action = A_ADD_CHAR;
	    break;
	case S_AFTER_QUOTE:
	    if (c == quo)
		state = S_IN_QUOTE, action = A_ADD_CHAR;
	    else if (c == '\0')
		action = A_END_RECORD;
	    else if (strchr(delim, c))
		state = S_START, action = A_NEW_FIELD;
	    else
		action = A_ERROR;
	    break;
	}

	switch (action) {
	case A_NO_OP:
	    break;
	case A_ADD_CHAR:
	    *q++ = *p;
	    break;
	case A_NEW_FIELD:
	    *q++ = '\0';
	    tokens[i++] = q;
	    tokens = G_realloc(tokens, (i + 2) * sizeof(char *));
	    break;
	case A_END_RECORD:
	    *q++ = '\0';
	    tokens[i++] = NULL;
	    return tokens;
	case A_ERROR:
	    G_warning(_("parse error"));
	    *q++ = '\0';
	    tokens[i++] = NULL;
	    return tokens;
	}
    }
}

/*!
  \brief Return number of tokens
  
  \param tokens
  
  \return number of tokens
*/

int G_number_of_tokens(char **tokens)
{
    int n;

    n = 0;
    for (n = 0; tokens[n] != NULL; n++)
      ;
    
    return n;
}

/*!
  \brief Free memory allocated to tokens.
  
  <b>Note:</b> <i>G_free_tokens()</i> must be called when finished with 
  tokens to release memory.
  
  \param[out] tokens
*/
void G_free_tokens(char **tokens)
{
    if (tokens[0] != NULL)
	G_free(tokens[0]);
    G_free(tokens);
}
