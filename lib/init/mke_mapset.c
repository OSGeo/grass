#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
extern int errno;

#include <grass/gis.h>

int make_mapset (char *location, char *mapset)
{
	char buffer[2048] ;
	char *buffer2;
	FILE *fd ;
	struct Cell_head window;

/* create the mapset directory */
	sprintf(buffer,"%s/%s",location, mapset) ;
	G_mkdir(buffer) ;

/* give the mapset a default window for the entire location */
	G_get_default_window(&window);
	G_put_window(&window);

/* generate DB settings file in new mapset */
	G_asprintf(&buffer2,"%s/%s/VAR", location, mapset);
	if((fd=fopen(buffer2,"w"))==NULL){
		perror("fopen");
		G_fatal_error("Cannot create <%s> file in new mapset", buffer2);
	}
	fprintf (fd, "DB_DRIVER: dbf\n");
	fprintf (fd, "DB_DATABASE: $GISDBASE/$LOCATION_NAME/$MAPSET/dbf/\n");
	fclose (fd);
	G_free(buffer2);
	
/* Make the dbf/ subdirectory */
	sprintf( buffer, "%s/%s/dbf", location, mapset );
	if( G_mkdir( buffer ) != 0 )
		return -1;

	return(0) ;
}
