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
    char code[GNAME_MAX], name[GNAME_MAX], proj_def[GNAME_MAX];
    FILE *fp;
    char buf[4096];
    int line;

#ifdef HAVE_PROJ_H
    /* PROJ 5 */
    PJ_INIT_INFO init_info;

    if (G_strcasecmp(authname, "EPSG") == 0)
	authname = "epsg";

    init_info = proj_init_info(authname);
    sprintf(pathname, init_info.filename);
    
    if (access(pathname, F_OK) != 0)
	G_fatal_error(_("Unable to find init file %s"), authname);

#else
    /* PROJ 4 */
    /* can't use pj_find_file() from the old proj api
     * because it does not exist in PROJ 4 */
    char *grass_proj_share;
    
    authname = listcodes->answer;
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
    name[0] = '\0';
    /* print list of codes, names, definitions */
    for (line = 1; G_getl2(buf, sizeof(buf), fp); line++) {
	int buflen;

	G_strip(buf);
	buflen = strlen(buf);
	    
	if (*buf == '\0' || 
	    (buflen >= 10 && strncmp(buf, "<metadata>", 10) == 0)) {
	    name[0] = '\0'; 
	    continue;
	}

	if (strncmp(buf, "<metadata>", strlen("<metadata>")) == 0)
	    continue;

	/* name: line starts with '# ' */
	/* code and definition in next line */

	if (*buf == '#' && buflen > 2) {
	    sprintf(name, buf + 2);
	    continue;
	}

	if (*buf == '<') {
	    int i, j;
	    
	    i = 0;
	    while (i < buflen && buf[i] != '>')
		i++;
	    buf[i] = '\0';
	    sprintf(code, buf + 1);
	    i++;
	    j = i;
	    while (i < buflen && buf[i] != '<')
		i++;
	    if (i < buflen && buf[i] == '<')
		buf[i] = '\0';
	    sprintf(proj_def, buf + j);
	    G_strip(proj_def);

	    fprintf(stdout, "%s|%s|%s\n", code, name, proj_def);
	    name[0] = '\0';
	}
    }
    fclose(fp);
#endif
}


