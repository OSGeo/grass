/*
 ***********************************************************************
 * prompt for vector maps (digit)
 *
 *   char *
 *   G_ask_vector_new(prompt, name)) 
 *       asks user to input name of a new vector map
 *
 *   char *
 *   G_ask_vector_old(prompt, name) 
 *       asks user to input name of an existing vector map
 *
 *   char *
 *   G_ask_vector_any(prompt, name)
 *       asks user to input any vector name
 *
 *   char *
 *   G_ask_vector_in_mapset(prompt, name)
 *       asks user to input name of an existing vector map
 *       in current mapset
 *
 *   parms:
 *      const char *prompt    optional prompt for user
 *      char *name            buffer to hold name of map found
 *
 *   returns:
 *      char *pointer to a string with name of mapset
 *       where file was found, or NULL if not found
 **********************************************************************/
#include <grass/gis.h>


/*!
 * \brief prompt for a new vector map
 *
 * Asks the user to enter a name for a vector map which does not
 * exist in the current mapset.
 *
 *  \param prompt
 *  \param name
 *  \return char * 
 */

char *G_ask_vector_new(const char *prompt, char *name)
{
    return G_ask_new(prompt, name, "vector", "vector");
}


/*!
 * \brief prompt for an existing vector map
 *
 * Asks the user to enter the name of an existing vector
 * file in any mapset in the database.
 *
 *  \param prompt
 *  \param name
 *  \return char * 
 */

char *G_ask_vector_old(const char *prompt, char *name)
{
    return G_ask_old(prompt, name, "vector", "vector");
}

char *G_ask_vector_any(const char *prompt, char *name)
{
    return G_ask_any(prompt, name, "vector", "vector", 1);
}


/*!
 * \brief prompt for an existing vector map
 *
 * Asks the user to enter the name of an existing vector
 * file in the current mapset.
 *
 *  \param prompt
 *  \param name
 *  \return char * 
 */

char *G_ask_vector_in_mapset(const char *prompt, char *name)
{
    return G_ask_in_mapset(prompt, name, "vector", "vector");
}
