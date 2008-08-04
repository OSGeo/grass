
/***************************************************************************
 *            reg_html.c
 *
 *  Fri May 20 18:14:32 2005
 *  Copyright  2005  User
 *  Email
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "globals.h"

void new_ext_html(char *ext, char *gisbase, char **html, int major, int minor,
		  int revision)
{
    int pos1, pos2, pos3;
    int start, end;
    int insert_here;
    char *first_char;
    char *last_char;
    char item[MAXSTR];
    int len;

    pos1 = find_pos("<b>Drivers sections:</b>", html, 0);	/* first go to section on "Drivers" */
    if (pos1 < 0) {
	/* we have a new version of the HTML docs that does not have a "Drivers" section anymore */
	/* let's check for the special GEM comment */
	pos1 =
	    find_pos
	    ("<!-- GEM Extensions StartHTML. Do not delete or change this comment! -->",
	     html, 0);
	if (pos1 < 0) {
	    /* sorry, I can't handle these HTML docs */
	    print_warning
		("Unknown format of index.html. Unable to register HTML man pages.\n");
	    return;
	}
    }
    pos2 = find_pos("<hr>", html, pos1);	/* the horizontal ruler marks the end of the HTML text */
    if (find_pos("<h3>Installed extensions:</h3>", html, pos1) == -1) {
	/* Extensions section does not yet exist: create it now */
	insert_str("<h3>Installed extensions:</h3>\n", pos2, html);
	insert_str("<ul>\n", pos2 + 1, html);
	insert_str("</ul>\n", pos2 + 2, html);
	insert_str("<p>\n", pos2 + 3, html);
    }

    start = find_pos("<h3>Installed extensions:</h3>", html, pos1);
    end = find_pos("</ul>", html, start);
    insert_here = start + 2;
    /* check if this entry already exists and if so: bail out or overwrite, if force mode */
    sprintf(item, "\">%s", ext);
    pos3 = find_pos(item, html, insert_here);
    if (pos3 != -1) {
	/* exists:  */
	print_warning("list item '%s' exists in index.html.\n", ext);
	if ((FORCE) && (UPGRADE)) {
	    sprintf(item,
		    "<li><a href=\"../extensions/%s/index.html\">%s (%i.%i.%i)</a>\n",
		    ext, ext, major, minor, revision);
	    strcpy(html[pos3], item);
	}
	return;
    }

    /* now go through all links in the Extensions section and insert this one in the right
       alphabetical position */
    pos3 = find_pos("<li><a href=", html, start);
    while ((pos3 != -1) && (pos3 < end)) {
	/* extract name of extension at this position */
	first_char = strrchr(html[pos3], '"');
	last_char = strrchr(html[pos3], '<');
	len = (last_char - first_char) / sizeof(char);
	strncpy(item, first_char + 2 * sizeof(char), len);
	item[len - 1] = '\0';	/* get rid of '<' */
	if (strcmp(ext, item) < 0) {
	    insert_here = pos3;
	    break;		/* found our position: let's quit this! */
	}
	/* look for next item */
	start++;
	pos3 = find_pos("<li><a href=", html, start);
    }

    /* now insert new entry for this extension */
    sprintf(item,
	    "<li><a href=\"../extensions/%s/index.html\">%s (%i.%i.%i)</a>\n",
	    ext, ext, major, minor, revision);
    insert_str(item, insert_here, html);

}


void delete_ext_html(char *ext, char *gisbase, char **html)
{
    int pos1, pos2, pos3;
    int start, end;
    char item[MAXSTR];
    int found;
    int i;

    pos1 = find_pos("<b>Drivers sections:</b>", html, 0);	/* first go to section on "Drivers" */
    if (pos1 < 0) {
	/* we have a new version of the HTML docs that does not have a "Drivers" section anymore */
	/* let's check for the special GEM comment */
	pos1 =
	    find_pos
	    ("<!-- GEM Extensions StartHTML. Do not delete or change this comment! -->",
	     html, 0);
	if (pos1 < 0) {
	    /* sorry, I can't handle these HTML docs */
	    print_warning
		("Unknown format of index.html. Unable to de-register HTML man pages.\n");
	    return;
	}
    }

    pos2 = find_pos("<hr>", html, pos1);	/* the horizontal ruler marks the end of the HTML text */
    if (find_pos("<h3>Installed extensions:</h3>", html, pos1) == -1) {
	/* Extensions section does not exist: bail out! */
	print_warning("no extensions section found in index.html.\n");
	return;
    }

    start = find_pos("<h3>Installed extensions:</h3>", html, pos1);
    end = find_pos("</ul>", html, start);
    /* check if the entry exists and if so delete */
    found = 0;
    sprintf(item, "\">%s", ext);
    pos3 = find_pos(item, html, start);
    if (pos3 == -1) {
	/* does not exist: */
	print_warning("extension '%s' not listed in index.html.\n", ext);
	return;
    }

    /* delete item, if it was found in the extensions section */
    if (pos3 < end) {
	delete_str(pos3, html);
    }
    end--;			/* end of extensions section is no one up! */

    /* if no more entries left in the extensions list: delete the entire section */
    pos3 = find_pos("<ul>", html, start);
    if ((pos3 != -1) && (end > pos3) && (end - pos3 < 2)) {
	for (i = 0; i < 4; i++) {
	    delete_str(pos3 - 1, html);
	}
    }
}


void register_html(char *pkg_short_name, char *gisbase, int major, int minor,
		   int revision)
{

    char file[MAXSTR];
    char str[MAXSTR];
    char **line;
    int n_lines, i;
    FILE *f_in, *f_out;

    /* check if index.html exists and is readable */
    sprintf(file, "%s/docs/html/index.html", gisbase);
    f_in = fopen(file, "r");
    if (f_in == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    return;
	}
	else {
	    /* sth. strange happened */
	    fclose(f_in);
	    print_error(ERR_REGISTER_HTML, "checking for file '%s': %s\n",
			file, strerror(errno));
	}
    }

    /* create a temporary index.html copy for write access */
    /* TODO: Do not hardcode temp paths */
    strcpy(TMP_HTML, "/tmp/grass.extensions.db.XXXXXX");	/* TMP_HTML is a global variable */
    mkstemp(TMP_HTML);

    f_out = fopen(TMP_HTML, "w+");
    if (f_out == NULL) {
	print_error(ERR_REGISTER_HTML,
		    "could not create temp file '%s': %s\n \
		Make sure that directory /tmp exists on your system and you have write permission.\n", TMP_HTML, strerror(errno));
    }

    atexit(&exit_db);		/* now need to register an at exit func to remove tmpdb automatically! */

    /* everything fine: create a shell command to install HTML stuff */
    if (VERBOSE) {
	sprintf(str,
		"cp -vf %s %s/docs/html/index.html ; chmod -v a+r %s/docs/html/index.html ;",
		TMP_HTML, gisbase, gisbase);
    }
    else {
	sprintf(str,
		"cp -f %s %s/docs/html/index.html &>%s ; chmod a+r %s/docs/html/index.html &>%s ;",
		TMP_HTML, gisbase, TMP_NULL, gisbase, TMP_NULL);
    }
    strcpy(HTML_CMD, str);

    /* count number of lines in index.html */
    n_lines = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	n_lines++;
    }
    if (n_lines == 0) {
	return;
    }
    rewind(f_in);

    /* create an array large enough to hold all lines in index.html */
    /* plus the entries that are to be added for the extension */
    /* plus one NULL terminator */
    /* and copy all lines from index.html into this */
    line = (char **)calloc(n_lines + 10, sizeof(char *));
    for (i = 0; i < (n_lines + 10); i++) {
	line[i] = NULL;
    }
    i = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	line[i] = (char *)malloc((1 + strlen(str)) * sizeof(char));
	strcpy(line[i], str);
	i++;
    }

    /* create "Extensions" entry in html document if necessary and add a link to */
    /* this extension's HTML man index */
    new_ext_html(pkg_short_name, gisbase, line, major, minor, revision);

    /* write output to tmpfile */
    i = 0;
    while (line[i] != NULL) {
	fprintf(f_out, line[i]);
	i++;
    }
    fflush(f_out);

    /* close files */
    fclose(f_in);
    fclose(f_out);

    /* free memory */
    for (i = 0; i < (n_lines + 10); i++) {
	free(line[i]);
    }
    free(line);
}


void deregister_html(char *pkg_short_name, char *gisbase)
{

    char file[MAXSTR];
    char str[MAXSTR];
    char **line;
    int n_lines, i;
    FILE *f_in, *f_out;

    /* check if index.html exists and is readable */
    sprintf(file, "%s/docs/html/index.html", gisbase);
    f_in = fopen(file, "r");
    if (f_in == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    return;
	}
	else {
	    /* sth. strange happened */
	    fclose(f_in);
	    print_error(ERR_REGISTER_HTML, "checking for file '%s': %s\n",
			file, strerror(errno));
	}
    }

    /* create a temporary index.html copy for write access */
    /* TODO: Do not hardcode temp paths */
    strcpy(TMP_HTML, "/tmp/grass.extensions.db.XXXXXX");	/* TMP_HTML is a global variable */
    mkstemp(TMP_HTML);

    f_out = fopen(TMP_HTML, "w+");
    if (f_out == NULL) {
	print_error(ERR_REGISTER_HTML,
		    "could not create temp file '%s': %s\n \
		Make sure that directory /tmp exists on your system and you have write permission.\n", TMP_HTML, strerror(errno));
    }

    atexit(&exit_db);		/* now need to register an at exit func to remove tmpdb automatically! */

    /* everything fine: create a shell command to copy modified HTML stuff on uninstall */
    if (VERBOSE) {
	sprintf(str,
		"cp -vf %s %s/docs/html/index.html ; chmod -v a+r %s/docs/html/index.html ;",
		TMP_HTML, gisbase, gisbase);
    }
    else {
	sprintf(str,
		"cp -f %s %s/docs/html/index.html &>%s ; chmod a+r %s/docs/html/index.html &>%s ;",
		TMP_HTML, gisbase, TMP_NULL, gisbase, TMP_NULL);
    }
    strcpy(HTML_CMD, str);


    /* count number of lines in index.html */
    n_lines = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	n_lines++;
    }
    if (n_lines == 0) {
	return;
    }
    rewind(f_in);

    /* create an array large enough to hold all lines in index.html */
    /* plus one NULL terminator */
    /* and copy all lines from index.html into this */
    line = (char **)calloc(n_lines + 1, sizeof(char *));
    for (i = 0; i < (n_lines + 1); i++) {
	line[i] = NULL;
    }
    i = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	line[i] = (char *)malloc((1 + strlen(str)) * sizeof(char));
	strcpy(line[i], str);
	i++;
    }

    /* delete link to this extension's HTML manual from index.html */
    delete_ext_html(pkg_short_name, gisbase, line);

    /* write output to tmpfile */
    i = 0;
    while (line[i] != NULL) {
	fprintf(f_out, line[i]);
	i++;
    }
    fflush(f_out);

    /* close files */
    fclose(f_in);
    fclose(f_out);

    /* free memory */
    for (i = 0; i < (n_lines + 1); i++) {
	free(line[i]);
    }
    free(line);
}


/*
   Returns number of restored entries 
 */
int restore_html(char *gisbase)
{
    char str[MAXSTR];
    char idx[MAXSTR];
    char ext_idx[MAXSTR];
    char dir[MAXSTR];
    char subdir[MAXSTR];
    char **line;
    int n_entries, n_lines, i;
    FILE *f_in, *f_out, *f_ext;
    DIR *dirp;
    DIR *subdirp;
    struct dirent *ep;
    int num_restored;
    int n_subdirs;
    int major, minor, revision;


    /* check if index.html exists and is readable */
    sprintf(idx, "%s/docs/html/index.html", gisbase);
    f_in = fopen(idx, "r");
    if (f_in == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    return (0);
	}
	else {
	    /* sth. strange happened */
	    fclose(f_in);
	    print_error(ERR_REGISTER_HTML, "checking for file '%s': %s\n",
			idx, strerror(errno));
	}
    }

    /* create a temporary index.html copy for write access */
    /* TODO: Do not hardcode temp paths */
    strcpy(TMP_HTML, "/tmp/grass.extensions.db.XXXXXX");	/* TMP_HTML is a global variable */
    mkstemp(TMP_HTML);

    f_out = fopen(TMP_HTML, "w+");
    if (f_out == NULL) {
	print_error(ERR_REGISTER_HTML,
		    "could not create temp file '%s': %s\n \
		Make sure that directory /tmp exists on your system and you have write permission.\n", TMP_HTML, strerror(errno));
    }

    /* everything fine: create a shell command to install HTML stuff */
    if (VERBOSE) {
	sprintf(str,
		"cp -vf %s %s/docs/html/index.html ; chmod -v a+r %s/docs/html/index.html ;",
		TMP_HTML, gisbase, gisbase);
    }
    else {
	sprintf(str,
		"cp -f %s %s/docs/html/index.html &>%s ; chmod a+r %s/docs/html/index.html &>%s ;",
		TMP_HTML, gisbase, TMP_NULL, gisbase, TMP_NULL);
    }
    strcpy(HTML_CMD, str);

    atexit(&exit_db);		/* now need to register an at exit func to remove tmpdb automatically! */

    /* allocate a pointer to the directory structure */
    sprintf(dir, "%s/docs/extensions", gisbase);
    dirp = opendir(dir);
    if (dirp == NULL) {
	/* directory does not exist or is not accessible */
	return (0);
    }

    /* PASS 1 */
    /* count number of subdirs in docs/extensions/ each new link will require one entry in index.html */
    n_entries = 0;
    n_subdirs = 0;
    while ((ep = readdir(dirp))) {
	sprintf(subdir, "%s/%s", dir, ep->d_name);
	if ((!strcmp(ep->d_name, ".")) || (!strcmp(ep->d_name, ".."))) {
	    continue;
	}
	subdirp = opendir(subdir);
	if (subdirp == NULL) {
	    continue;
	}
	n_subdirs++;
	closedir(subdirp);
    }
    closedir(dirp);

    /* count number of lines in menu.tcl */
    n_lines = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	n_lines++;
    }
    if (n_lines == 0) {
	return (0);
    }
    rewind(f_in);

    /* create an array large enough to hold all lines in index.html */
    /* plus one new entry to make a link for each extension */
    /* plus space for the new Extensions section */
    /* plus one NULL terminator */
    /* and copy all lines from menu.tcl into this */
    line = (char **)calloc(n_lines + n_subdirs + 10, sizeof(char *));
    for (i = 0; i < (n_lines + n_subdirs + 10); i++) {
	line[i] = NULL;
    }
    i = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	line[i] = (char *)malloc((1 + strlen(str)) * sizeof(char));
	strcpy(line[i], str);
	i++;
    }
    line[i] = NULL;		/* add NULL terminator */

    /* PASS 2: re-create links if necessary */
    dirp = opendir(dir);
    num_restored = 0;
    while ((ep = readdir(dirp))) {
	sprintf(subdir, "%s/%s", dir, ep->d_name);
	if ((!strcmp(ep->d_name, ".")) || (!strcmp(ep->d_name, ".."))) {
	    continue;
	}
	subdirp = opendir(subdir);
	if (subdirp == NULL) {
	    continue;
	}
	closedir(subdirp);

	/* try to open extension's index.html file */
	sprintf(ext_idx, "%s/index.html", subdir);
	f_ext = fopen(ext_idx, "r");
	if (f_ext == NULL) {
	    continue;		/* cannot access index.html: skip to next extension */
	}
	major = 0;
	minor = 0;
	revision = 0;
	/* retrieve version information from extension's index.html */
	i = 0;
	while (fgets(str, MAXSTR, f_ext) != NULL) {
	    if (strstr(str, "<title>") != NULL) {
		i = 1;
		break;		/* this is the title line: that's all we need */
	    }
	}
	if (i == 0) {
	    continue;		/* not a valid index.html: skip to next extension */
	}
	sscanf(strchr(str, '(') + sizeof(char), "%i.%i.%i", &major, &minor,
	       &revision);
	new_ext_html(ep->d_name, gisbase, line, major, minor, revision);
	num_restored++;
	fclose(f_ext);
    }
    closedir(dirp);

    /* write output to tmpfile */
    i = 0;
    while (line[i] != NULL) {
	fprintf(f_out, line[i]);
	i++;
    }
    fflush(f_out);

    /* close remaining files */
    fclose(f_in);
    fclose(f_out);

    /* free memory */
    for (i = 0; i < (n_lines + n_subdirs + 10); i++) {
	free(line[i]);
    }
    free(line);

    return (num_restored);
}
