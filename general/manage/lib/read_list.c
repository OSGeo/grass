#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "list.h"

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

static int format_error (char *,int,char *);

int 
read_list (int check_if_empty)
{
    FILE *fd;
    char element_list[600];
    char buf[1024];
    char elem[100];
    char alias[100];
    char desc[100];
    char text[100];
    int any;
    int line;
    char *env;

    nlist = 0;
    list = 0;
    any = 0;

    if ((env = G__getenv ("ELEMENT_LIST")))
	G_strcpy (element_list, env);
    else
	sprintf (element_list, "%s/etc/element_list", G_gisbase());
    fd = fopen (element_list, "r");

    if (!fd)
	G_fatal_error("can't open database element list <%s>", element_list);

    line = 0;
    while (G_getl (buf, sizeof (buf), fd))
    {
	line++;
	if (*buf == '#') continue;
	if (*buf == ' ' || *buf == '\t')	/* support element */
	{
	    *desc = 0;
	    if (sscanf (buf, "%[^:]:%[^\n]", elem, desc) < 1) continue;
	    if (*elem == '#') continue;
	    if (nlist == 0)
		format_error (element_list, line, buf);

	    G_strip (elem);
	    G_strip (desc);
	    add_element (elem, desc);
	}
	else					/* main element */
	{
	    if (sscanf (buf, "%[^:]:%[^:]:%[^:]:%[^\n]", elem, alias, desc, text) != 4)
		format_error (element_list, line, buf);

	    G_strip (elem);
	    G_strip (alias);
	    G_strip (desc);
	    G_strip (text);

	    list = (struct list *) G_realloc (list, (nlist+1) * sizeof (*list));
	    list[nlist].mainelem = G_store (elem);
	    list[nlist].alias = G_store (alias);
	    list[nlist].maindesc = G_store (desc);
	    list[nlist].text = G_store (text);
	    list[nlist].nelem = 0;
	    list[nlist].element = 0;
	    list[nlist].desc = 0;
	    list[nlist].status = 0;
	    if (!check_if_empty || !empty (elem))
	    {
		list[nlist].status = 1;
		any = 1;
	    }
	    nlist++;
	    add_element (elem,desc);
	}
    }

    fclose (fd);

    return any;
}

static int format_error (char *element_list,int line, char *buf)
{
    G_fatal_error (_("Format error: <%s>\nLine: %d\n%s"), element_list, line, buf);

    return 1;
}
