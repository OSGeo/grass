/* setup:  Program for GRASS user to first use.  Sets environment
 * variables:  LOCATION_NAME MAPSET GISDBASE
 */


/* these defines come from the Gmakefile, but are defined here to allow
 * debugging with saber
 */
#ifndef D_LOCATION_NAME
#define D_LOCATION_NAME "spearfish"
#endif
#ifndef D_GISDBASE
#define D_GISDBASE "/data"
#endif
#ifndef GRASS_VERSION_NUMBER
#define GRASS_VERSION_NUMBER ""
#endif



static char *intro[] = {
    "                   PLEASE SET SESSION INFORMATION",
    "",
    "DATABASE: A directory (folder) on disk to contain all GRASS maps and data.",
    "",
    "LOCATION: This is the name of a geographic location. It is defined by a",
    "          co-ordinate system and a rectangular boundary.",
    "",
    "MAPSET:   Each GRASS session runs under a particular MAPSET. This consists of",
    "          a rectangular REGION and a set of maps. Every LOCATION contains at",
    "          least a MAPSET called PERMANENT, which is readable by all sessions.",
    "",
    "         The REGION defaults to the entire area of the chosen LOCATION.",
    "         You may change it later with the command: g.region",
    "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ",
    0
};
static char *loc_text =
    "LOCATION:                              (enter list for a list of locations)";
static char *map_text =
    "MAPSET:                                (or mapsets within a location)";

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vask.h>
#include <grass/edit.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    char version[80];
    char gisdbase[70];
    char location_name[41];
    char location[1024];
    char mapset[41];
    int line;
    int yes;
    struct Cell_head window;

    char *GISDBASE;
    char *LOCATION_NAME;
    char *MAPSET;

    int repeat;

    /* GISBASE
     *   comes from the unix environment and must be set
     *   make sure it is NOT in the .gisrc file
     *   note: G_getenv() make a special case of this variable
     */
    G_no_gisinit();

    G_unsetenv("GISBASE");	/* this cleans the variable */
    G_getenv("GISBASE");	/* this reads it from the unix environment */

    /* Gather all existing variables ********************************************* */
    GISDBASE = G__getenv("GISDBASE");
    LOCATION_NAME = G__getenv("LOCATION_NAME");
    MAPSET = G__getenv("MAPSET");

    if (!MAPSET)
	MAPSET = G_whoami();
    if (!LOCATION_NAME)
	LOCATION_NAME = D_LOCATION_NAME;
    if (!GISDBASE)
	GISDBASE = D_GISDBASE;

    strcpy(mapset, MAPSET);
    strcpy(location_name, LOCATION_NAME);
    strcpy(gisdbase, GISDBASE);
    G__setenv("GISDBASE", gisdbase);

    sprintf(version, "                            GRASS %s",
	    GRASS_VERSION_NUMBER);

    for (repeat = 1; repeat; hit_return()) {
	V_clear();
	V_line(0, version);
	for (line = 1; intro[line]; line++)
	    V_line(line, intro[line]);

	line++;
	V_line(line, loc_text);
	V_ques(location_name, 's', line++, 12, 25);

	V_line(line, map_text);
	V_ques(mapset, 's', line++, 12, 25);

	line++;
	V_line(line, "DATABASE:");
	V_ques(gisdbase, 's', line++, 10, sizeof(gisdbase) - 1);

	V_intrpt_ok();
	if (!V_call())
	    exit(1);

	G_strip(gisdbase);
	if (*gisdbase == 0) {
	    fprintf(stderr, "No DATABASE specified\n");
	    strcpy(gisdbase, D_GISDBASE);
	    continue;
	}
#ifdef __MINGW32__
	if (*gisdbase == '/') {
	    char tmp[200], *p;

	    sprintf(tmp, "%s", getenv("WD"));
	    for (p = tmp + strlen(tmp); --p >= tmp && *p == '\\';) ;
	    for (; --p >= tmp && *p != '\\';) ;
	    for (; --p >= tmp && *p == '\\';) ;
	    *(p + 1) = 0;
	    for (p = tmp; *p; p++)
		if (*p == '\\')
		    *p = '/';
	    strcat(tmp, gisdbase);
	    strcpy(gisdbase, tmp);
	}
	if (!gisdbase[1] || gisdbase[1] != ':')
#else
	if (*gisdbase != '/')
#endif
	{
	    char temp[200];

	    fprintf(stderr, "DATABASE <%s> - must start with /\n", gisdbase);
	    sprintf(temp, " '%s'", gisdbase);
	    strcpy(gisdbase, temp);
	    continue;
	}
	if (access(gisdbase, 0) != 0) {
	    fprintf(stderr, "DATABASE <%s> - not found\n", gisdbase);
	    continue;
	}
	G__setenv("GISDBASE", gisdbase);

	/* take only first word of responses */
	first_word(location_name);
	first_word(mapset);

	if (*location_name && (G_legal_filename(location_name) < 0)) {
	    fprintf(stderr, "LOCATION <%s> - illegal name\n", location_name);
	    continue;
	}
	if (*mapset && (G_legal_filename(mapset) < 0)) {
	    fprintf(stderr, "MAPSET <%s> - illegal name\n", mapset);
	    continue;
	}

	if (*location_name == 0 || strcmp(location_name, "list") == 0) {
	    list_locations(gisdbase);
	    *location_name = 0;
	    continue;
	}
	sprintf(location, "%s/%s", gisdbase, location_name);
	if (access(location, 0) != 0) {
	    fprintf(stderr, "LOCATION <%s> - doesn't exist\n", location_name);
	    list_locations(gisdbase);
	    if (!can_make_location(gisdbase, location_name))
		continue;

	    fprintf(stderr, "\nWould you like to create location <%s> ? ",
		    location_name);
	    if (G_yes("", 1)) {
		if (make_location(gisdbase, location_name)) {

		    G__setenv("LOCATION_NAME", location_name);
		    G__setenv("MAPSET", "PERMANENT");
		    G__write_env();


		    if (system("g.setproj")) {
			G_get_default_window(&window);
			if (E_edit_cellhd(&window, -1) < 0)
			    fprintf(stderr,
				    "WARNING: You did not provide default region for %s!\n",
				    location_name);

			G__put_window(&window, "", "DEFAULT_WIND");
			G__put_window(&window, "", "WIND");
			fprintf(stderr, "LOCATION <%s> created\n",
				location_name);
			fprintf(stderr,
				"\nBut the PROJECTION information files were not created!\n");
			fprintf(stderr,
				"You must run g.setproj successfully before projection software will work%c%c%c\n",
				7, 7, 7);
			continue;
		    }
		    else {
			G_get_default_window(&window);
			if (E_edit_cellhd(&window, -1) < 0)
			    fprintf(stderr,
				    "WARNING: You did not provide default region for %s!\n",
				    location_name);
			G__put_window(&window, "", "DEFAULT_WIND");
			G__put_window(&window, "", "WIND");
			fprintf(stderr, "LOCATION <%s> created!\n",
				location_name);
			continue;
		    }
		}
		fprintf(stderr, "LOCATION <%s> NOT created\n", location_name);
	    }
	    continue;
	}
	G__setenv("LOCATION_NAME", location_name);
	if (*mapset == 0 || strcmp(mapset, "list") == 0) {
	    list_mapsets(location_name, location);
	    *mapset = 0;
	    continue;
	}
	G__setenv("MAPSET", mapset);

	repeat = 0;

	switch (mapset_permissions(mapset)) {
	case -1:
	    if (strcmp(mapset, G_whoami()) == 0) {
		yes = 1;
	    }
	    else {
		repeat = 1;
		fprintf(stderr, "\n\nMapset <<%s>> is not available\n",
			mapset);
		list_mapsets(location_name, location);
		fprintf(stderr,
			"\nWould you like to create < %s > as a new mapset? ",
			mapset);
		yes = G_yes("", 1);
	    }

	    if (yes && (make_mapset(location, mapset) == 0))
		repeat = 0;
	    break;
	case 0:
	    fprintf(stderr, "\n\nSorry, no access to <<%s>>.\n", mapset);
	    list_mapsets(location_name, location);
	    repeat = 1;
	    break;
	case 1:
	    mapset_message(mapset);
	    if (!mapset_question(mapset))
		repeat = 1;
	    break;
	}
	if (!repeat)
	    break;
    }

    G__write_env();
    exit(0);
}

int list_locations(const char *gisdbase)
{
    fprintf(stderr, "\nAvailable locations:\n");
    fprintf(stderr, "----------------------\n");
    G_ls(gisdbase, stderr);
    fprintf(stderr, "----------------------\n");
    return 0;
}

int list_mapsets(const char *location_name, const char *location)
{
    char **mapsets;
    int i, num_mapsets;
    int any, ok, any_ok;
    int len, tot_len;

    fprintf(stderr, "\nMapsets in location <%s>\n", location_name);
    fprintf(stderr, "----------------------\n");
    mapsets = G__ls(location, &num_mapsets);
    any = 0;
    any_ok = 0;
    tot_len = 0;
    if (num_mapsets > 0) {
	for (i = 0; i < num_mapsets; i++) {
	    any = 1;
	    len = strlen(mapsets[i]) + 1;
	    len /= 20;
	    len = (len + 1) * 20;
	    tot_len += len;
	    if (tot_len > 75) {
		fprintf(stderr, "\n");
		tot_len = len;
	    }
	    if ((ok = (mapset_permissions(mapsets[i]) == 1)))
		any_ok = 1;
	    fprintf(stderr, "%s%-*s", ok ? "(+)" : "   ", len, mapsets[i]);
	}
	if (tot_len)
	    fprintf(stderr, "\n");
	if (any_ok)
	    fprintf(stderr,
		    "\nnote: you only have access to mapsets marked with (+)\n");
	else if (any)
	    fprintf(stderr,
		    "\nnote: you do not have access to any of these mapsets\n");
    }
    fprintf(stderr, "----------------------\n");

    return 0;
}

int first_word(char *buf)
{
    char temp[GNAME_MAX];

    *temp = 0;
    sscanf(buf, "%s", temp);
    strcpy(buf, temp);

    return 0;
}

int hit_return(void)
{
    char buf[80];

    fprintf(stderr, "\nHit RETURN -->");
    G_gets(buf);

    return 0;
}
