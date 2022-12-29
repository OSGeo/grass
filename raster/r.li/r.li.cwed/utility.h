
 /*
  * \brief Utility to concat ans split strings.
  * implemented in utility.c
  *
  *  \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
  *                     Commission from Faunalia Pontedera (PI) www.faunalia.it
  *
  *   This program is free software under the GPL (>=v2)
  *   Read the COPYING file that comes with GRASS for details.
  *      
  */

/*if occurred an error returns NULL */
char *concatena(const char *, const char *);

/*if occurred an error returns NULL */
char **split_arg(char *, char, long *);
