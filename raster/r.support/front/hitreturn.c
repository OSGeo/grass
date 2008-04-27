#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/* 
 * hitreturn() - Get carrage return 
 *
 * RETURN: EXIT_SUCCESS
 */
int hitreturn(void)
{
    char buf[127];

    G_message(_("\nHit RETURN to continue -->"));
    G_gets(buf);

    return EXIT_SUCCESS;
}
