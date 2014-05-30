#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

void list_available_mapsets(const char **mapset_name, int nmapsets, const char* fs)
{
    int  n;
    G_message(_("Available mapsets:"));
    
    for (n = 0; n < nmapsets; n++) {
        fprintf(stdout, "%s", mapset_name[n]);
        if (n < nmapsets-1) {
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

void list_accessible_mapsets(const char* fs)
{
    int n;
    const char *name;
    
    G_message(_("Accessible mapsets:"));
    for (n = 0; (name = G_get_mapset_name(n)); n++) {
        /* match each mapset to its numeric equivalent */
        fprintf(stdout, "%s", name);
        if (G_get_mapset_name(n+1)) {
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
