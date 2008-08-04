
/***************************************************************************
 *            reg_entries.c
 *
 *  Fri May 13 11:35:33 2005
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <dirent.h>
#include "globals.h"

int menu_created;


 /*
    Returns 1, if the string is a submenu item, 0 otherwise.
    Very primitive
  */
int is_submenu(char *item)
{
    if (strchr(item, '[') == NULL) {
	return (0);
    }
    if (strrchr(item, ']') == NULL) {
	return (0);
    }
    if (strchr(item, '[') > strrchr(item, ']')) {
	return (0);
    }
    return (1);
}


/*
   If "Extensions" menu does not yet exist, it will be created immediately
   to the left of the "Help" menu in GIS Manager's main menu bar.
   Returns the line nr after which the "Extension" menu entries should be
   inserted.
 */
int check_ext_menu(char **tcl_lines)
{
    int idx;

    /* check if "Extensions" menu exists */
    idx = find_pos("\"&Xtns\" all options 1", tcl_lines, 0);
    if (idx == -1) {		/* does not exist: create a new one */
	idx = find_pos("\"&Help\" all options", tcl_lines, 0);
	if (idx == -1) {	/* Help menu does not exist: place at far right */
	    idx = find_pos("}]", tcl_lines, 0);
	    if (idx == -1) {
		print_error(ERR_REGISTER_ENTRIES_GISMAN,
			    "could not parse 'menu.tcl'.\n");
	    }
	    insert_str(" \"&Xtns\" all options 1 {\n", idx, tcl_lines);
	    idx++;
	    insert_str(" }\n", idx, tcl_lines);
	    return (idx);
	}
	insert_str(" \"&Xtns\" all options 1 {\n", idx, tcl_lines);
	idx++;
	insert_str(" }\n", idx, tcl_lines);
    }
    return (idx);
}


 /*
    Creates a new submenu for this extension under "Extensions" in GIS Managers
    main menu bar.
    Returns line no. of submenu, after which additional entries should be appended
    to menu.tcl.
  */
int new_submenu(char *pkg_short_name, char *menu, char **tcl_lines)
{
    char tmp[MAXSTR];
    char tmp2[MAXSTR];
    char searchstr[MAXSTR];
    int idx, idx2;
    int insert_here;
    int last;
    char *first_quote;
    char *second_quote;
    int len;
    int terminator;

    /* Store the position of the "Extensions" entry and start looking for submenu from there */
    idx = find_pos("\"&Xtns\" all options", tcl_lines, 0);
    last = find_pos("\" all options", tcl_lines, idx + 1) - 1;	/* find end of "Extensions" menu */
    if (last == -1) {
	last = find_pos("}]", tcl_lines, 0);	/* end of menu.tcl */
    }

    /* check if the line is a valid submenu specifier */
    if (!is_submenu(menu)) {
	print_warning
	    ("first line not a submenu specifier in 'entries-gisman'.\n");
	return (-1);
    }
    else {
	/* check if a submenu with this name does already exist */
	len = (strrchr(menu, ']') - strchr(menu, '[')) / sizeof(char);
	strncpy(tmp, strchr(menu, '[') + sizeof(char), len);
	tmp[len - 1] = '\0';	/* get rid of [] */
	sprintf(searchstr, "{cascad \"%s\"", tmp);	/* this is what we need to look for */

	idx2 = find_pos(searchstr, tcl_lines, idx);
	if ((idx2 != -1) && (idx2 < last)) {
	    print_warning("submenu '%s' exists in GIS Manager's Xtns menu.\n",
			  tmp);
	    return (-1);
	}

	/* ELSE: create a new submenu in the right place */
	insert_here = idx + 1;	/* by default: place submenu in first line after "Extensions" menu */
	idx2 = find_pos("{cascad ", tcl_lines, idx);	/* go through all submenus in "Extensions" */
	while ((idx2 != -1) && (idx2 < last)) {
	    /* check for alphabetical order */
	    first_quote = strchr(tcl_lines[idx2], '\"');	/* get name of submenu w/o quotation marks */
	    second_quote = strchr(first_quote + sizeof(char), '\"');
	    len = (second_quote - first_quote) / sizeof(char);
	    strncpy(tmp2, first_quote + sizeof(char), len);
	    tmp2[len - 1] = '\0';	/* get rid of "" */
	    if (strcmp(tmp, tmp2) < 0) {
		insert_here = idx2;
		break;
	    }
	    idx++;
	    idx2 = find_pos("{cascad ", tcl_lines, idx);
	}

	/* create new submenu and return line no in menu.tcl for insertion of entries */
	sprintf(tmp, " \t\t\t%s {} \"\" 1 {\n", searchstr);
	insert_str(tmp, insert_here, tcl_lines);
	insert_str(" \t\t\t}}\n", insert_here + 1, tcl_lines);

	/* create an uninstall entry in menu.tcl */
	terminator = find_pos("}]", tcl_lines, 0);
	/* this is just a comment that tells the uninstall function which submenu to remove */
	sprintf(tmp, "#(DO_NOT_REMOVE_THIS_COMMENT) <%s> %s {} \"\" 1 {\n",
		pkg_short_name, searchstr);
	insert_str(tmp, terminator + 1, tcl_lines);

	/* return next line for insertion of menu entries and commands! */
	return (insert_here + 1);
    }

    return (-1);
}


/*
   Inserts a new menu entry into the extension's own submenu under "Extensions".
   Returns the line number in menu.tcl after which the next entry should be
   inserted.
 */
int new_item(char *item, char *menu, char **tcl_lines, int line_no)
{
    char *token;
    int num_tokens;
    char entry[MAXSTR];
    char command[MAXSTR];
    char tmp[MAXSTR];


    /* remove dangling white spaces and EOL chars */
    chop(item);

    token = strtok(item, ";");
    if (token == NULL) {
	print_warning("invalid token in 'entries-gisman'.\n");
	return (-1);
    }

    strcpy(entry, token);	/* get menu entry string */

    num_tokens = 0;
    while (token != NULL) {
	token = strtok(NULL, ";");
	if (token != NULL) {
	    strcpy(command, token);	/* get command string */
	}
	num_tokens++;
    }

    if (num_tokens > 2) {
	print_warning("invalid number of tokens (%i) in 'entries-gisman'.\n",
		      num_tokens);
	return (-1);
    }

    /* just a separator or a 'real' menu entry? */
    if ((!strcmp(entry, "-")) && (!strcmp(entry, "-"))) {
	sprintf(tmp, " \t\t\t {separator}\n");
    }
    else {
	sprintf(tmp, " \t\t\t {command \"%s\" {} \"%s\" {} -command {%s }}\n",
		entry, command, command);
    }
    insert_str(tmp, line_no, tcl_lines);

    /* return line no. for next entry */
    line_no++;
    return (line_no);
}


 /* 
    Checks if there are any entries in entries-gisman.
    Reads GISBASE/etc/dm/menu.tcl into an array of strings.
    Adds a new item "Extensions" to the menu bar, if it is missing.
    Adds new submenus and menu items to the GIS manager, as stated in entries-gisman
    Writes the new menu structure to a temporary file which will then be installed
    using the 'su' function.
  */
void register_entries_gisman(char *pkg_short_name, char *gisbase)
{

    char file[MAXSTR];
    char str[MAXSTR];
    char menu[MAXSTR];
    int len;
    char **line;
    int n_entries, n_lines, i;
    int n_lines_org, n_lines_new;
    int line_no;
    FILE *f_gisman, *f_in, *f_out;

    /* check if entries-gisman exists and is readable */
    sprintf(file, "../entries-gisman");
    f_gisman = fopen(file, "r");
    if (f_gisman == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    return;
	}
	else {
	    /* sth. strange happened */
	    fclose(f_gisman);
	    print_error(ERR_REGISTER_ENTRIES_GISMAN,
			"checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }

    /* check if menu.tcl exists and is readable */
    sprintf(file, "%s/etc/dm/menu.tcl", gisbase);
    f_in = fopen(file, "r");
    if (f_in == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    return;
	}
	else {
	    /* sth. strange happened */
	    fclose(f_in);
	    print_error(ERR_REGISTER_ENTRIES_GISMAN,
			"checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }

    /* create a temporary menu.tcl file for write access */
    /* TODO: Do not hardcode temp paths */
    strcpy(TMP_GISMAN, "/tmp/grass.extensions.db.XXXXXX");	/* TMP_GISMAN is a global variable */
    mkstemp(TMP_GISMAN);

    f_out = fopen(TMP_GISMAN, "w+");
    if (f_out == NULL) {
	print_error(ERR_REGISTER_ENTRIES_GISMAN,
		    "could not create temp file '%s': %s\n \
			Make sure that directory /tmp exists on your system and you have write permission.\n", TMP_GISMAN, strerror(errno));
    }
    atexit(&exit_db);		/* now need to register an at exit func to remove tmpdb automatically! */

    /* everything fine: create a shell command to install gisman-entries and modified menu.tcl */
    /* this also creates a backup-copy of menu.tcl */
    if (VERBOSE) {
	sprintf(str,
		"mkdir --verbose %s/etc/dm/gem-entries ; cp -vf ../entries-gisman %s/etc/dm/gem-entries/%s ; \
					cp -vf %s/etc/dm/menu.tcl %s/etc/dm/menu.tcl.gem.bak ; \
					cp -vf %s %s/etc/dm/menu.tcl ; chmod -v a+r %s/etc/dm/menu.tcl ;",
		gisbase, gisbase, pkg_short_name, gisbase, gisbase, TMP_GISMAN, gisbase, gisbase);
    }
    else {
	sprintf(str,
		"mkdir %s/etc/dm/gem-entries &> %s ; cp -f ../entries-gisman %s/etc/dm/gem-entries/%s &> %s ; \
					cp -f %s/etc/dm/menu.tcl %s/etc/dm/menu.tcl.gem.bak &> %s ; \
					cp -f %s %s/etc/dm/menu.tcl &> %s ; chmod a+r %s/etc/dm/menu.tcl &> %s ;",
		gisbase, TMP_NULL, gisbase, pkg_short_name, TMP_NULL, gisbase, gisbase, TMP_NULL, TMP_GISMAN, gisbase, TMP_NULL, gisbase, TMP_NULL);
    }
    strcpy(GISMAN_CMD, str);

    /* count number of lines in entries-gisman */
    n_entries = 0;
    while (fgets(str, MAXSTR, f_gisman) != NULL) {
	n_entries++;
    }
    if (n_entries == 0) {
	return;
    }
    rewind(f_gisman);

    /* count number of lines in menu.tcl */
    n_lines = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	n_lines++;
    }
    if (n_lines == 0) {
	return;
    }
    n_lines_org = n_lines;
    rewind(f_in);

    /* create an array large enough to hold all lines in menu.tcl */
    /* plus the entries that are to be added from entries-gisman */
    /* plus one NULL terminator */
    /* and copy all lines from menu.tcl into this */
    line = (char **)calloc(n_lines + (n_entries * 2) + 6, sizeof(char *));
    for (i = 0; i < (n_lines + (n_entries * 2) + 6); i++) {
	line[i] = NULL;
    }
    i = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	line[i] = (char *)malloc((1 + strlen(str)) * sizeof(char));
	strcpy(line[i], str);
	i++;
    }

    check_ext_menu(line);	/* create "Extensions" menu if necessary */

    /* read all lines from entries-gisman and add to menus */
    i = 1;
    while (nc_fgets_nb(str, MAXSTR, f_gisman) != NULL) {
	if (i == 1) {
	    /* store name of menu item */
	    len = (strrchr(str, ']') - strchr(str, '[')) / sizeof(char);
	    strncpy(menu, strchr(str, '[') + sizeof(char), len);
	    menu[len - 1] = '\0';	/* get rid of [] */
	    line_no = new_submenu(pkg_short_name, str, line);
	    if (line_no < 0) {
		print_warning("no GIS Manager menu entries created.\n");
		break;
	    }
	    i++;
	}
	else {
	    line_no = new_item(str, menu, line, line_no);
	    if (line_no < 0) {
		print_warning("error creating GIS Manager menu entries.\n");
		break;
	    }
	    i++;
	}
    }

    /* write output to tmpfile */
    i = 0;
    while (line[i] != NULL) {
	fprintf(f_out, line[i]);
	i++;
    }
    fflush(f_out);

    /* check for accidental corruption of menu.tcl: if tmpfile has less lines than installed
       menu.tcl, we did sth. wrong and should leave the orginal file untouched! */
    rewind(f_out);
    n_lines_new = 0;
    while (fgets(str, MAXSTR, f_out) != NULL) {
	n_lines_new++;
    }
    if ((n_lines_new == 0) || (n_lines_new < n_lines_org)) {
	print_warning
	    ("file truncation detected. Retaining orginal file 'menu.tcl'.\n");
	strcpy(GISMAN_CMD, "");
    }

    /* close files */
    fclose(f_in);
    fclose(f_gisman);
    fclose(f_out);

    /* free memory */
    for (i = 0; i < (n_lines + (n_entries * 2) + 6); i++) {
	free(line[i]);
    }
    free(line);
}


/*
   This version is for Michael Barton's new version of the GIS Manager (gis.m)
   It is much simpler and more flexible, because gis.m can build menus
   from files at runtime.
   All we have to do is make sure there is a folder 'Xtns' in $GISBASE/etc/gm
   and we copy 'entries-gisman2' (if provided) into that folder using a
   filename 'extension name'.gem!
 */
void register_entries_gisman2(char *pkg_short_name, char *gisbase)
{
    char file[MAXSTR];
    FILE *f_gisman2;
    char str[MAXSTR];

    /* check if entries-gisman2 exists and is readable */
    sprintf(file, "../entries-gisman2");
    f_gisman2 = fopen(file, "r");
    if (f_gisman2 == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    return;
	}
	else {
	    /* sth. strange happened */
	    fclose(f_gisman2);
	    print_error(ERR_REGISTER_ENTRIES_GISMAN2,
			"checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }

    /* let's just blindly create an 'Xtns' dir: if it exists already: no problem */
    /* and then copy file into it! */
    if (VERBOSE) {
	sprintf(str,
		"mkdir --verbose -p %s/etc/gm/Xtns ; cp -fv ../entries-gisman2 %s/etc/gm/Xtns/%s.gem ; ",
		gisbase, gisbase, pkg_short_name);
    }
    else {
	sprintf(str,
		"mkdir -p %s/etc/gm/Xtns ; cp -f ../entries-gisman2 %s/etc/gm/Xtns/%s.gem ; ",
		gisbase, gisbase, pkg_short_name);
    }
    strcpy(GISMAN2_CMD, str);
}


/*
   Checks for a comment left by the new_submenu () function in menu.tcl.        
   If it exists, the submenu specified in that comment will be removed along
   with all its entries.
   Returns -1 on failure, number of removed entries otherwise.
   If no more submenu entries exist, this will also remove the "Xtns" menu.
 */
int deregister_entries_gisman(char *pkg_short_name, char *gisbase)
{
    char file[MAXSTR];
    char str[MAXSTR];
    char tmp[MAXSTR];
    char **line;
    int n_lines, i;
    int n_lines_org, n_lines_new;
    FILE *f_in, *f_out;
    int pos;
    int start, end;
    int start_sub, end_sub;
    char *lq, *rq;
    int num_removed;

    /* check if menu.tcl exists and is readable */
    sprintf(file, "%s/etc/dm/menu.tcl", gisbase);
    f_in = fopen(file, "r");
    if (f_in == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    return (0);
	}
	else {
	    /* sth. strange happened */
	    fclose(f_in);
	    print_error(ERR_DEREGISTER_ENTRIES_GISMAN,
			"checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }

    /* create a temporary menu.tcl file for write access */
    /* TODO: Do not hardcode temp paths */
    strcpy(TMP_GISMAN, "/tmp/grass.extensions.db.XXXXXX");	/* TMP_GISMAN is a global variable */
    mkstemp(TMP_GISMAN);

    f_out = fopen(TMP_GISMAN, "w+");
    if (f_out == NULL) {
	print_error(ERR_REGISTER_ENTRIES_GISMAN,
		    "could not create temp file '%s': %s\n \
			Make sure that directory /tmp exists on your system and you have write permission.\n", TMP_GISMAN, strerror(errno));
    }
    atexit(&exit_db);		/* now need to register an at exit func to remove tmpdb automatically! */

    /* everything fine: create a shell command to copy modified menu.tcl on uninstall */
    if (VERBOSE) {
	sprintf(str, "cp -vf %s/etc/dm/menu.tcl %s/etc/dm/menu.tcl.gem.bak ; \
						cp -vf %s %s/etc/dm/menu.tcl ; chmod -v a+r %s/etc/dm/menu.tcl ;", gisbase, gisbase, TMP_GISMAN, gisbase, gisbase);
    }
    else {
	sprintf(str,
		"cp -f %s/etc/dm/menu.tcl %s/etc/dm/menu.tcl.gem.bak &> %s ; \
						cp -f %s %s/etc/dm/menu.tcl &> %s ; chmod a+r %s/etc/dm/menu.tcl &> %s ;", gisbase, gisbase, TMP_NULL, TMP_GISMAN, gisbase, TMP_NULL, gisbase, TMP_NULL);
    }
    strcpy(GISMAN_CMD, str);


    /* count number of lines in menu.tcl */
    n_lines = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	n_lines++;
    }
    if (n_lines == 0) {
	return (-1);
    }
    rewind(f_in);
    n_lines_org = n_lines;

    /* create an array large enough to hold all lines in menu.tcl */
    /* plus one NULL terminator */
    /* and copy all lines from menu.tcl into this */
    line = (char **)calloc(n_lines + 1, sizeof(char *));
    for (i = 0; i < n_lines + 1; i++) {
	line[i] = NULL;
    }
    i = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	line[i] = (char *)malloc((1 + strlen(str)) * sizeof(char));
	strcpy(line[i], str);
	i++;
    }

    /* search for uninstall comment */
    sprintf(str, "#(DO_NOT_REMOVE_THIS_COMMENT) <%s> {cascad",
	    pkg_short_name);
    pos = find_pos(str, line, 0);
    if (pos == -1) {
	print_warning
	    ("could not find uninstall information in 'menu.tcl'.\n");
	return (-1);
    }

    /* copy name of submenu to search for */
    lq = strchr(line[pos], '\"');
    lq++;
    rq = strchr(lq, '\"');
    strcpy(tmp, lq);
    tmp[(rq - lq) / sizeof(char)] = '\0';

    /* now find "Xtns" menu start and end */
    start = find_pos("\"&Xtns\" all options 1", line, 0);
    end = find_pos("\" all options", line, start + 1) - 1;
    if (end == -1) {
	end = find_pos("}]", line, 0);	/* end of menu.tcl */
    }

    if (start == -1) {
	print_warning("menu 'Xtns' does not exist.\n");
	return (-1);
    }

    /* now find our submenu and set the search range to it */
    sprintf(str, "{cascad \"%s\"", tmp);
    start_sub = find_pos(str, line, start);
    if ((start_sub == -1) || (start_sub > end)) {
	print_warning("could not find submenu entry '%s' in 'menu.tcl'.\n",
		      tmp);
	return (-1);
    }
    end_sub = find_pos(" \t\t\t}}", line, start_sub);
    if ((end_sub == -1) || (end_sub > end)) {
	print_warning
	    ("could not find end of submenu entry '%s' in 'menu.tcl'.\n",
	     tmp);
	return (-1);
    }

    num_removed = 0;

    /* now kill every line in between start and end! */
    for (i = 0; i < ((end_sub - start_sub) + 1); i++) {
	delete_str(start_sub, line);
	num_removed++;
    }

    /* now kill the uninstall comment */
    sprintf(str, "#(DO_NOT_REMOVE_THIS_COMMENT) <%s> {cascad",
	    pkg_short_name);
    pos = find_pos(str, line, 0);
    delete_str(pos, line);
    num_removed++;

    /* check if there are any submenus left in "Xtns" and if not: remove Xtns menu */
    start = find_pos("\"&Xtns\" all options 1", line, 0);
    end = find_pos("\" all options", line, start + 1) - 1;
    if (end - start < 3) {
	for (i = 0; i < ((end - start) + 1); i++) {
	    delete_str(start, line);
	    num_removed++;
	}
    }

    /* write output to tmpfile */
    i = 0;
    while (line[i] != NULL) {
	fprintf(f_out, line[i]);
	i++;
    }
    fflush(f_out);

    /* check for accidental corruption of menu.tcl: if tmpfile is empty (=0 lines), 
       we did sth. wrong and should leave the orginal file untouched! */
    rewind(f_out);
    n_lines_new = 0;
    while (fgets(str, MAXSTR, f_out) != NULL) {
	n_lines_new++;
    }
    if ((n_lines_new == 0)) {
	print_warning
	    ("file truncation detected. Retaining orginal file 'menu.tcl'.\n");
	strcpy(GISMAN_CMD, "");
    }

    /* close files */
    fclose(f_in);
    fclose(f_out);

    /* free memory */
    for (i = 0; i < n_lines + 1; i++) {
	free(line[i]);
    }
    free(line);

    return (num_removed);
}


/*
   This version is for Michael Barton's new GIS Manager.
   In this case, all we have to do is delete the .gem file!
 */
void deregister_entries_gisman2(char *pkg_short_name, char *gisbase)
{
    char file[MAXSTR];
    FILE *f_gisman2;
    char str[MAXSTR];

    /* check if entries-gisman2 exists and is readable */
    sprintf(file, "%s/etc/gm/Xtns/%s.gem", gisbase, pkg_short_name);
    f_gisman2 = fopen(file, "r");
    if (f_gisman2 == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    return;
	}
	else {
	    /* sth. strange happened */
	    fclose(f_gisman2);
	    print_error(ERR_DEREGISTER_ENTRIES_GISMAN2,
			"checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }

    if (VERBOSE) {
	sprintf(str, "rm -vf %s/etc/gm/Xtns/%s.gem ; ",
		gisbase, pkg_short_name);
    }
    else {
	sprintf(str, "rm -f %s/etc/gm/Xtns/%s.gem ; ",
		gisbase, pkg_short_name);

    }
    strcpy(GISMAN_CMD, str);
}


/*
   Returns number of restored entries 
 */
int restore_entries_gisman(char *gisbase)
{
    char str[MAXSTR];
    char menu[MAXSTR];
    char file[MAXSTR];
    char dir[MAXSTR];
    char pkg_short_name[MAXSTR];
    int len;
    char **line;
    int n_entries, n_lines, i;
    int line_no;
    FILE *f_gisman, *f_in, *f_out;
    DIR *dirp;
    struct dirent *ep;
    int num_restored;
    int n_files;

    /* check if menu.tcl exists and is readable */
    sprintf(file, "%s/etc/dm/menu.tcl", gisbase);
    f_in = fopen(file, "r");
    if (f_in == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    return (0);
	}
	else {
	    /* sth. strange happened */
	    fclose(f_in);
	    print_error(ERR_REGISTER_ENTRIES_GISMAN,
			"checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }

    /* create a temporary menu.tcl file for write access */
    /* TODO: Do not hardcode temp paths */
    strcpy(TMP_GISMAN, "/tmp/grass.extensions.db.XXXXXX");	/* TMP_GISMAN is a global variable */
    mkstemp(TMP_GISMAN);

    f_out = fopen(TMP_GISMAN, "w+");
    if (f_out == NULL) {
	print_error(ERR_REGISTER_ENTRIES_GISMAN,
		    "could not create temp file '%s': %s\n \
			Make sure that directory /tmp exists on your system and you have write permission.\n", TMP_GISMAN, strerror(errno));
    }
    atexit(&exit_db);		/* now need to register an at exit func to remove tmpdb automatically! */

    /* everything fine: create a shell command to copy modified menu.tcl on uninstall */
    if (VERBOSE) {
	sprintf(str, "cp -vf %s/etc/dm/menu.tcl %s/etc/dm/menu.tcl.gem.bak ; \
						cp -vf %s %s/etc/dm/menu.tcl ; chmod -v a+r %s/etc/dm/menu.tcl ;", gisbase, gisbase, TMP_GISMAN, gisbase, gisbase);
    }
    else {
	sprintf(str,
		"cp -f %s/etc/dm/menu.tcl %s/etc/dm/menu.tcl.gem.bak &> %s ; \
						cp -f %s %s/etc/dm/menu.tcl &> %s ; chmod a+r %s/etc/dm/menu.tcl &> %s ;", gisbase, gisbase, TMP_NULL, TMP_GISMAN, gisbase, TMP_NULL, gisbase, TMP_NULL);
    }
    strcpy(GISMAN_CMD, str);

    /* allocate a pointer to the directory structure */
    sprintf(dir, "%s/etc/dm/gem-entries", gisbase);
    dirp = opendir(dir);
    if (dirp == NULL) {
	/* directory does not exist or is not accessible */
	return (0);
    }

    /* PASS 1 */
    /* open all files in gem-entries and count the number of lines each has */
    n_entries = 0;
    n_files = 0;
    while ((ep = readdir(dirp))) {
	sprintf(file, "%s/%s", dir, ep->d_name);
	f_gisman = fopen(file, "r");
	if ((!strcmp(ep->d_name, ".")) || (!strcmp(ep->d_name, ".."))) {
	    fclose(f_gisman);
	    continue;
	}
	if (f_gisman == NULL) {
	    fclose(f_gisman);
	    continue;
	}
	/* count number of lines in file */
	while (fgets(str, MAXSTR, f_gisman) != NULL) {
	    n_entries++;
	}
	n_files++;
	fclose(f_gisman);
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

    /* create an array large enough to hold all lines in menu.tcl */
    /* plus the entries that are to be added from the files in gem-entries/ */
    /* plus space for uninstall comments */
    /* plus one NULL terminator */
    /* and copy all lines from menu.tcl into this */
    line =
	(char **)calloc(n_lines + (n_entries * 2) + (n_files * 5) + 1,
			sizeof(char *));
    for (i = 0; i < (n_lines + (n_entries * 2) + (n_files * 5) + 1); i++) {
	line[i] = NULL;
    }
    i = 0;
    while (fgets(str, MAXSTR, f_in) != NULL) {
	line[i] = (char *)malloc((1 + strlen(str)) * sizeof(char));
	strcpy(line[i], str);
	i++;
    }
    line[i] = NULL;		/* add NULL terminator */

    check_ext_menu(line);	/* create "Extensions" menu if necessary */

    /* PASS 2: re-create submenus and all menu items if necessary */
    dirp = opendir(dir);
    num_restored = 0;
    while ((ep = readdir(dirp))) {
	sprintf(file, "%s/%s", dir, ep->d_name);
	if ((!strcmp(ep->d_name, ".")) || (!strcmp(ep->d_name, ".."))) {
	    continue;
	}
	f_gisman = fopen(file, "r");
	if (f_gisman == NULL) {
	    continue;
	}
	/* read all lines from entries-gisman and add to menus */
	i = 1;
	while (nc_fgets_nb(str, MAXSTR, f_gisman) != NULL) {
	    if (i == 1) {
		/* store name of menu item */
		len = (strrchr(str, ']') - strchr(str, '[')) / sizeof(char);
		strncpy(menu, strchr(str, '[') + sizeof(char), len);
		menu[len - 1] = '\0';	/* get rid of [] */
		line_no = new_submenu(pkg_short_name, str, line);
		if (line_no < 0) {
		    break;
		}
		i++;
		num_restored++;
	    }
	    else {
		line_no = new_item(str, menu, line, line_no);
		if (line_no < 0) {
		    break;
		}
		i++;
		num_restored++;
	    }
	}
	fclose(f_gisman);
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
    for (i = 0; i < (n_lines + (n_entries * 2) + (n_files * 5) + 1); i++) {
	free(line[i]);
    }
    free(line);

    return (num_restored);
}
