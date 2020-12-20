#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

void list_available_subprojects(const char **subproject_name, int nsubprojects, const char* fs)
{
    int  n;
    G_message(_("Available subprojects:"));
    
    for (n = 0; n < nsubprojects; n++) {
        fprintf(stdout, "%s", subproject_name[n]);
        if (n < nsubprojects-1) {
            if (strcmp(fs, "newline") == 0)
                fprintf(stdout, "\n");
            else if (strcmp(fs, "space") == 0)
                fprintf(stdout, " ");
            else if (strcmp(fs, "comma") == 0)
                fprintf(stdout, ",");
            else if (strcmp(fs, "tab") == 0)
                fprintf(stdout, "\t");
            else
                fprintf(stdout, "%s", fs);
        }
    }
    fprintf(stdout, "\n");
}

void list_accessible_subprojects(const char* fs)
{
    int n;
    const char *name;
    
    G_message(_("Accessible subprojects:"));
    for (n = 0; (name = G_get_subproject_name(n)); n++) {
        /* match each subproject to its numeric equivalent */
        fprintf(stdout, "%s", name);
        if (G_get_subproject_name(n+1)) {
            if (strcmp(fs, "newline") == 0)
                fprintf(stdout, "\n");
            else if (strcmp(fs, "space") == 0)
                fprintf(stdout, " ");
            else if (strcmp(fs, "comma") == 0)
                fprintf(stdout, ",");
            else if (strcmp(fs, "tab") == 0)
                fprintf(stdout, "\t");
            else
                fprintf(stdout, "%s", fs);
        }
    }
    fprintf(stdout, "\n");
}
