#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/config.h>

#include <proj.h>

char *get_authority_names(void)
{
    char *authnames;
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

    return authnames;
}

void list_codes(char *authname)
{
    int i, crs_cnt;
    PROJ_CRS_INFO **proj_crs_info;

    crs_cnt = 0;
    proj_crs_info =
        proj_get_crs_info_list_from_database(NULL, authname, NULL, &crs_cnt);
    if (crs_cnt < 1)
        G_fatal_error(_("No codes found for authority %s"), authname);

    for (i = 0; i < crs_cnt; i++) {
        const char *proj_definition;
        char emptystr;
        PJ *pj;

        emptystr = '\0';
        pj = proj_create_from_database(NULL, proj_crs_info[i]->auth_name,
                                       proj_crs_info[i]->code, PJ_CATEGORY_CRS,
                                       0, NULL);
        proj_definition = proj_as_proj_string(NULL, pj, PJ_PROJ_5, NULL);
        if (!proj_definition) {
            /* what to do with a CRS without proj string ? */
            G_debug(1, "No proj string for %s:%s", proj_crs_info[i]->auth_name,
                    proj_crs_info[i]->code);
            proj_definition = &emptystr;
        }

        if (proj_definition) {
            fprintf(stdout, "%s|%s|%s\n", proj_crs_info[i]->code,
                    proj_crs_info[i]->name, proj_definition);
        }

        proj_destroy(pj);
    }
}
