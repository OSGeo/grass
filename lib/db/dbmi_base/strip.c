/*!
   \fn 
   \brief 
   \return 
   \param 
 */
/*  db_strip(buf)
 *     char *buf         buffer to be worked on
 *
 *  'buf' is rewritten in place with leading and trailing white
 *  space removed.
 */


void db_strip(char *buf)
{
    char *a, *b;

    /* remove leading white space */
    for (a = b = buf; *a == ' ' || *a == '\t'; a++) ;
    if (a != b)
	while ((*b++ = *a++)) ;

    /* remove trailing white space */
    for (a = buf; *a; a++) ;
    if (a != buf) {
	for (a--; *a == ' ' || *a == '\t'; a--) ;
	a++;
	*a = 0;
    }
}
