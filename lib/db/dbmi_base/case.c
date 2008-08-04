#include <grass/dbmi.h>

/*!
   \fn void db_char_to_lowercase (char *s)
   \brief 
   \return 
   \param 
 */
void db_char_to_lowercase(char *s)
{
    if (*s >= 'A' && *s <= 'Z')
	*s -= 'A' - 'a';
}

/*!
   \fn void db_char_to_uppercase (char *s)
   \brief 
   \return 
   \param 
 */
void db_char_to_uppercase(char *s)
{
    if (*s >= 'a' && *s <= 'z')
	*s += 'A' - 'a';
}

/*!
   \fn void db_Cstring_to_lowercase (char *s)
   \brief 
   \return 
   \param 
 */
void db_Cstring_to_lowercase(char *s)
{
    while (*s)
	db_char_to_lowercase(s++);
}

/*!
   \fn void db_Cstring_to_uppercase (char *s)
   \brief 
   \return 
   \param 
 */
void db_Cstring_to_uppercase(char *s)
{
    while (*s)
	db_char_to_uppercase(s++);
}

/*!
   \fn int db_nocase_compare (char *a, char *b)
   \brief 
   \return 
   \param 
 */
int db_nocase_compare(const char *a, const char *b)
{
    char s, t;

    while (*a && *b) {
	s = *a++;
	t = *b++;
	db_char_to_uppercase(&s);
	db_char_to_uppercase(&t);
	if (s != t)
	    return 0;
    }
    return (*a == 0 && *b == 0);
}
