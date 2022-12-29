/*!
  \file db/dbmi_base/token.c
  
  \brief DBMI Library (base) - tokens management
  
  (C) 1999-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
*/
#include <grass/dbmi.h>

/* these routines manage a mapping between tokens (ints) and memory addresses */
#define NONE ( (dbAddress) NULL )

static dbAddress *list = NONE;
static dbToken count = 0;

/*!
  \brief Find token

  \param token pointer to dbToken

  \return dbAddress
  \return NULL if no token found
*/
dbAddress db_find_token(dbToken token)
{
    if (token >= 0 && token < count)
	return list[token];
    return (NONE);
}

/*!
  \brief Drop token

  \param token pointer to dbToken
*/
void db_drop_token(dbToken token)
{
    if (token >= 0 && token < count)
	list[token] = NONE;
}

/*!
  \brief Add new token

  \param address dbAddress of token to be added

  \return dbToken
 */
dbToken db_new_token(dbAddress address)
{
    dbToken token;
    dbAddress *p;

    for (token = 0; token < count; token++)
	if (list[token] == NONE) {
	    list[token] = address;
	    return token;
	}

    p = (dbAddress *) db_realloc((void *)list, sizeof(*list) * (count + 1));
    if (p == NULL)
	return -1;

    list = p;
    token = count++;
    list[token] = address;
    return (token);
}
