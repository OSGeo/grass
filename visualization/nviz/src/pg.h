#include <grass/Vect.h>
#define QRY_LENGTH 1024

struct Sql
{
    double centX;		/* x coordinate                     */
    double centY;		/* y coordinate                     */
    double permX;		/* permiter easting                 */
    double permY;		/* perimeter north                  */
    double rad2;
    double distance;
    double maxY;		/* northing                         */
    double minY;		/* south                            */
    double minX;		/* west                             */
    double maxX;		/* east                             */
};

char *buildPg(char *, char *, int);
char *buildPgSite(char *, char *, char *);
char *runPg(char *);
char *do_query(char *, struct Sql *);
char *getCat(struct Map_info *, float, float, int *);
int fillSQLstruct(struct Sql *, float, float, int);
