#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/config.h>

#ifdef HAVE_PROJ_H
#include <proj.h>
#if PROJ_VERSION_MAJOR >= 6
#define USE_PROJDB
#endif
#else
#include <proj_api.h>
#endif

char *get_authority_names(void)
{
    char *authnames;

#ifdef USE_PROJDB
    /* PROJ 6 */
    int i, len;
    PROJ_STRING_LIST authlist = proj_get_authorities_from_database(NULL);

    len = 0;
    for (i = 0; authlist[i]; i++) {
	len += strlen(authlist[i]) + 1;
    }
    if (len > 0) {
	authnames = G_malloc((len + 1) * sizeof(char)); /* \0 */
	*authnames = '\0';
	for (i = 0; authlist[i]; i++) {
	    if (i > 0)
		strcat(authnames, ",");
	    strcat(authnames, authlist[i]);
	}
    }
    else {
	authnames = G_store("");
    }
#else
    /* PROJ 4, 5 */
    /* there are various init files in share/proj/:
     * EPSG,GL27,IGNF,ITRF2000,ITRF2008,ITRF2014,nad27,nad83,esri
     * but they have different formats: bothering only with EPSG here */ 
    authnames = G_store("EPSG");
#endif

    return authnames;
}

void list_codes(char *authname)
{
#ifdef USE_PROJDB
    /* PROJ 6+ */
    int i, crs_cnt;
    PROJ_CRS_INFO **proj_crs_info;
    
    crs_cnt = 0;
    proj_crs_info = proj_get_crs_info_list_from_database(NULL,
			authname, NULL, &crs_cnt);
    if (crs_cnt < 1)
	G_fatal_error(_("No codes found for authority %s"),
		      authname);
	    
    for (i = 0; i < crs_cnt; i++) {
	const char *proj_definition;
	char emptystr;
	PJ *pj;

	emptystr = '\0';
	pj = proj_create_from_database(NULL,
				       proj_crs_info[i]->auth_name,
				       proj_crs_info[i]->code,
				       PJ_CATEGORY_CRS,
				       0, NULL);
	proj_definition = proj_as_proj_string(NULL, pj, PJ_PROJ_5, NULL);
	if (!proj_definition) {
	    /* what to do with a CRS without proj string ? */
	    G_debug(1, "No proj string for %s:%s",
		    proj_crs_info[i]->auth_name,
		    proj_crs_info[i]->code);
	    proj_definition = &emptystr;
	}

	if (proj_definition) {
	    fprintf(stdout, "%s|%s|%s\n", proj_crs_info[i]->code,
					  proj_crs_info[i]->name,
					  proj_definition);
	}

	proj_destroy(pj);
    }
#else
    char pathname[GPATH_MAX];
    char code[GNAME_MAX], name[GNAME_MAX], proj_def[8192];
    FILE *fp;
    char buf[4096];
    int line;

#ifdef HAVE_PROJ_H
    /* PROJ 5 */
    PJ_INIT_INFO init_info;

    if (G_strcasecmp(authname, "EPSG") == 0)
	authname = "epsg";

    init_info = proj_init_info(authname);
    sprintf(pathname, "%s", init_info.filename);
    
    if (access(pathname, F_OK) != 0)
	G_fatal_error(_("Unable to find init file %s"), authname);

#else
    /* PROJ 4 */
    /* can't use pj_find_file() from the old proj api
     * because it does not exist in PROJ 4 */
    char *grass_proj_share;

    if (G_strcasecmp(authname, "EPSG") == 0)
	authname = "epsg";

    grass_proj_share = getenv("GRASS_PROJSHARE");
    if (!grass_proj_share)
	G_fatal_error(_("Environment variable GRASS_PROJSHARE is not set"));
    sprintf(pathname, "%s/%s", grass_proj_share, authname);
    G_convert_dirseps_to_host(pathname);
#endif

    /* PROJ 4 / 5 */
    
    /* the init files do not have a common structure, thus restrict to epsg 
     * see pj_init.c get_init_string() in PROJ 4 / 5 for a 
     * generic init file parser, however without descriptive name */
    if (strcmp(authname, "epsg") != 0)
	G_fatal_error(_("Only epsg file is currently supported"));
    
    /* open the init file */
    fp = fopen(pathname, "r");
    if (!fp) {
	G_fatal_error(_("Unable to open init file <%s>"), authname);
    }

    code[0] = '\0';
    name[0] = '\0';
    proj_def[0] = '\0';
    /* print list of codes, names, definitions */
    for (line = 1; G_getl2(buf, sizeof(buf), fp); line++) {
	int buflen, bufstart;
	int i, j;

	G_strip(buf);
	buflen = strlen(buf);
	    
	if (*buf == '\0' || 
	    (buflen >= 10 && strncmp(buf, "<metadata>", 10) == 0)) {
	    name[0] = '\0'; 
	    continue;
	}

	/* name: could be text following '# ' */
	/* code: <code> 
	 * definition follows code until next '<' */

	if (*buf == '#' && buflen > 2) {
	    sprintf(name, "%s", buf + 2);
	    continue;
	}

	i = 0;
	bufstart = 0;
	while (i < buflen) {
	    
	    if (buf[i] == '<') {
		/* end of section ? */
		if (code[0] != '\0') {
		    G_strip(proj_def);
		    /* the descriptive name may be hidden in proj_def as
		     * +title=
		     * e.g. IGNF */
		    fprintf(stdout, "%s|%s|%s\n", code, name, proj_def);
		    code[0] = '\0';
		    name[0] = '\0';
		    proj_def[0] = '\0';
		}

		/* start of section ? */
		bufstart = i + 1;
		j = bufstart;
		while (j < buflen && buf[j] != '>')
		    j++;

		if (j < buflen) {
		    buf[j] = '\0';
		    sprintf(code, "%s", buf + bufstart);
		}
		i = j + 1;
		bufstart = i;
		continue;
	    }
	    
	    if (buf[i] == '#') {
		/* the remaining content of the line could be the name */
		bufstart = i + 1;
		if (bufstart < buflen) {
		    sprintf(name, "%s", buf + bufstart);
		    G_strip(name);
		}
		i = buflen;
		continue;
	    }

	    if (code[0] != '\0') {
		char stopchar;
		int proj_len;

		/* inside a section definition */
		/* test for '<' or '#' later on in the line */
		j = bufstart;
		while (j < buflen && buf[j] != '<' && buf[j] != '#')
		    j++;
		if (j < buflen) {
		    stopchar = buf[j];
		    buf[j] = '\0';
		    proj_len = strlen(proj_def);
		    proj_def[proj_len] = ' ';
		    proj_def[proj_len + 1] = '\0';
		    strcat(proj_def, buf + bufstart);
		    buf[j] = stopchar;
		    i = j;
		    bufstart = i;
		}
		else {
		    proj_len = strlen(proj_def);
		    proj_def[proj_len] = ' ';
		    proj_def[proj_len + 1] = '\0';
		    strcat(proj_def, buf + bufstart);
		    i = buflen;
		    bufstart = i;
		}
	    }
	    else
		i++;
	}
    }
    fclose(fp);
#endif
}


