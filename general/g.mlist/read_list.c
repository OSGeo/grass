/* Copied from general/manage */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "global.h"

/*******************************************************************
read the element list file

   format is:

   # ... comments
   main element:alias:description:menu text
      sub element:description
      sub element:description
	  .
	  .
	  .
******************************************************************/

static int add_element(const char *, const char *);
static int empty(const char *);
static int format_error(const char *, int, const char *);

int read_list(int check_if_empty)
{
    FILE *fd;
    char element_list[600], buf[1024], elem[100], alias[100], desc[100],
	text[100], *env;
    int any, line;

    nlist = 0;
    list = 0;
    any = 0;

    if ((env = G__getenv("ELEMENT_LIST")))
	strcpy(element_list, env);
    else
	sprintf(element_list, "%s/etc/element_list", G_gisbase());
    fd = fopen(element_list, "r");

    if (!fd)
	G_fatal_error("can't open database element list <%s>", element_list);

    line = 0;
    while (G_getl(buf, sizeof(buf), fd)) {
	line++;
	if (*buf == '#')
	    continue;
	if (*buf == ' ' || *buf == '\t') {	/* support element */
	    *desc = 0;
	    if (sscanf(buf, "%[^:]:%[^\n]", elem, desc) < 1)
		continue;
	    if (*elem == '#')
		continue;
	    if (nlist == 0)
		format_error(element_list, line, buf);

	    G_strip(elem);
	    G_strip(desc);
	    add_element(elem, desc);
	}
	else {			/* main element */

	    if (sscanf
		(buf, "%[^:]:%[^:]:%[^:]:%[^\n]", elem, alias, desc,
		 text) != 4)
		format_error(element_list, line, buf);

	    G_strip(elem);
	    G_strip(alias);
	    G_strip(desc);
	    G_strip(text);

	    list =
		(struct list *)G_realloc(list, (nlist + 1) * sizeof(*list));
	    list[nlist].mainelem = G_store(elem);
	    list[nlist].alias = G_store(alias);
	    list[nlist].maindesc = G_store(desc);
	    list[nlist].text = G_store(text);
	    list[nlist].nelem = 0;
	    list[nlist].element = 0;
	    list[nlist].desc = 0;
	    list[nlist].status = 0;
	    if (!check_if_empty || !empty(elem)) {
		list[nlist].status = 1;
		any = 1;
	    }
	    nlist++;
	    add_element(elem, desc);
	}
    }

    fclose(fd);

    return any;
}

static int add_element(const char *elem, const char *desc)
{
    int n;
    int nelem;

    if (*desc == 0)
	desc = elem;

    n = nlist - 1;
    nelem = list[n].nelem++;
    list[n].element =
	(char **)G_realloc(list[n].element, (nelem + 1) * sizeof(char *));
    list[n].element[nelem] = G_store(elem);
    list[n].desc =
	(char **)G_realloc(list[n].desc, (nelem + 1) * sizeof(char *));
    list[n].desc[nelem] = G_store(desc);

    return 0;
}

static int empty(const char *elem)
{
    DIR *dirp;
    struct dirent *dp;
    char dir[1024];
    int any;

    G__file_name(dir, elem, "", G_mapset());

    any = 0;
    if ((dirp = opendir(dir)) != NULL) {
	while (!any && (dp = readdir(dirp)) != NULL) {
	    if (dp->d_name[0] != '.')
		any = 1;
	}
	closedir(dirp);
    }

    return any == 0;
}

static int format_error(const char *element_list, int line, const char *buf)
{
    G_fatal_error(_("Format error: <%s>\nLine: %d\n%s"), element_list, line,
		  buf);

    return 1;
}
