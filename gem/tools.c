
/***************************************************************************
 *            tools.c
 *
 *  Mon Apr 18 15:00:13 2005
 *  Copyright  2005  Benjamin Ducke
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

#include "globals.h"


/* the following are for cross-platform compatibility. They may already exist
   on a Linux or BSD system, but maybe absent e.g. in Win32, so we will define
   them here. */


char *basename(char *path)
{
    char *copy;
    char *element;
    char *backup;

    copy = strdup(path);

    backup = NULL;
    element = strtok(copy, "/");
    if (element == NULL) {
	if (copy != NULL) {
	    free(copy);
	}
	return (NULL);
    }

    backup = strdup(element);
    while (element != NULL) {
	element = strtok(NULL, "/");
	if ((backup != NULL) && (element != NULL)) {
	    free(backup);
	}
	if ((element != NULL) && (*element != '\0')) {
	    backup = strdup(element);
	}
    }

    if (copy != NULL) {
	free(copy);
    }

    return (backup);
}


/*      A shell version of mkdir () 
   needed because the MINGW one does no accept unix style MOD specifications.
   THIS DOES NOT CHECK FOR ERRORS !
 */
void mkdir_s(char *pathname, char *mode)
{
    char tmp[5000];

    sprintf(tmp, "mkdir %s --mode=%s -p", pathname, mode);
    system(tmp);

}

/* 
   Removes all dangling white-spaces and EOL chars from the end of a string.
   Returns total number of chopped-off characters.
 */
int chop(char *string)
{
    int i;
    int chopped;
    int stop = 0;

    chopped = 0;
    i = strlen(string) - 1;
    while (i >= 0) {
	stop = 1;
	if ((string[i] == '\n') || (string[i] == '\t') ||
	    (string[i] == ' ') || (string[i] == '\f') ||
	    (string[i] == '\r')) {
	    chopped++;
	    stop = 0;
	}
	if (stop == 1) {
	    /* got a non white-space char: stop chopping! */
	    break;
	}
	i--;
    }

    /* chop string */
    string[strlen(string) - chopped] = '\0';

    return (chopped);
}


/* 
   Inserts a string into an array of n strings; positions start at 0
   The size of the array will be increased by one char*. 
   Returns new size of array.
   String array must be NULL-terminated.
   There must be at least one slots left after the NULL-terminator to
   hold the new string! This function will not resize the string
   array to accomodate the new string!
 */
int insert_str(char *str, int pos, char **strarr)
{
    char save[MAXSTR];
    char insert[MAXSTR];
    char last[MAXSTR];
    int n, j;
    int len;

    /* check for valid pos */
    n = 0;
    while (strarr[n] != NULL) {
	n++;
    }

    if ((pos < 0) || (pos > (n))) {
	print_error(ERR_REGISTER_ENTRIES_GISMAN,
		    "insert: invalid line number %i.\n", pos);
    }

    /* if a new string is to be added to the end of the array: */
    if (pos == n) {
	len = (1 + strlen(str)) * sizeof(char);
	strarr[n] = malloc(len);
	strcpy(strarr[n], str);
	n = n + 2;
	strarr[n - 1] = NULL;
	return (n);
    }

    strcpy(last, strarr[n - 1]);
    strcpy(insert, strarr[pos]);
    free(strarr[pos]);
    strarr[pos] = malloc((1 + strlen(str)) * sizeof(char));
    strcpy(strarr[pos], str);
    /* now move all strings > pos up one */
    for (j = pos; j < n - 1; j++) {
	strcpy(save, strarr[j + 1]);	/* save string to be overwritten */
	free(strarr[j + 1]);	/* overwrite string */
	len = (1 + strlen(insert)) * sizeof(char);	/* make room for string to be inserted */
	strarr[j + 1] = malloc(len);
	strcpy(strarr[j + 1], insert);	/* insert string */

	strcpy(insert, save);	/* set saved string to next to be inserted */

    }

    /* overwrite NULL terminator with last item */
    strarr[n] = malloc((1 + strlen(last)) * sizeof(char));
    strcpy(strarr[n], last);

    /* increase size of array by one */
    n = n + 2;
    strarr[n - 1] = NULL;	/* set last element of array to NULL */

    return (n);
}


/* 
   Delete a string at position pos (o to n) from an array of n strings; 
   positions start at 0
   The size of the array will be decreased by one char*.
   Returns new size of array.
   String array must be NULL-terminated.
 */
int delete_str(int pos, char **strarr)
{
    int i;

    /* check for valid pos */
    i = 0;
    while (strarr[i] != NULL) {
	i++;
    }

    if ((pos < 0) || (pos > (i))) {
	print_error(ERR_REGISTER_ENTRIES_GISMAN,
		    "delete: invalid line number %i.\n", pos);
    }

    /* now move all strings > pos down one */
    i = pos;
    while (strarr[i] != NULL) {
	free(strarr[i]);
	if (strarr[i + 1] != NULL) {
	    strarr[i] = malloc((1 + (strlen(strarr[i + 1]))) * sizeof(char));
	    strcpy(strarr[i], strarr[i + 1]);
	}
	i++;
    }

    /* decrease size of array by one */
    i = i - 1;
    strarr[i] = NULL;		/* set last element of array to NULL */

    return (i);
}


 /*
    Returns the first line number in which *str is found.
    Search starts after line number 'start'.
    Returns -1 if string not found.
    String array must be NULL-terminated.
  */
int find_pos(char *str, char **strarr, int start)
{
    int i, j;

    /* check for valid pos */
    i = 0;
    while (strarr[i] != NULL) {
	i++;
    }

    if ((start < 0) || (start > (i))) {
	exit(ERR_REGISTER_ENTRIES_GISMAN);
    }

    for (j = start; j < (i); j++) {
	if (strstr(strarr[j], str) != NULL) {
	    return (j);
	}
    }

    return (-1);
}


 /*
    Dumps an array of strings to a file.
    String array must be NULL-terminated.
  */
void dump_str(FILE * f, char **strarr)
{
    int i;

    i = 0;
    while (strarr[i] != NULL) {
	fprintf(f, "%i: %s", i, strarr[i]);
	i++;
    }
}

/*
   Get package name. Copies the package name as found in
   the 'name' info file into the char array *name.
 */
void get_package_name(char *path, char *name)
{
    FILE *f;
    char file[MAXSTR];
    char tmp[MAXSTR];

    sprintf(file, "%s/%s", path, "name");

    /* get extension name */
    f = fopen(file, "r");
    if (f == NULL) {
	print_error(ERR_INVALID_EXT, "'name' file not readable.\n");
    }
    else {
	if (nc_fgets_nb(tmp, MAXSTR, f) == NULL) {
	    fclose(f);
	    print_error(ERR_INVALID_EXT,
			"invalid or missing extension name.\n");
	}
	else {
	    chop(tmp);
	    strcpy(name, tmp);
	}
    }
    fclose(f);
}

/*
   A replacement function for fgets to filter out comments and blank lines.
   Useful for parsing settings files.
   Returns a line from a file only if it does not start with '#'.
   Returns only the part of the line left of '#'.
   Otherwise, tries to read the next line from the file or returns NULL on EOF.
 */
char *nc_fgets(char *s, int size, FILE * stream)
{
    char *hashmark;
    char *tmp;

    if (fgets(s, size, stream) == NULL) {
	return (NULL);
    }

    hashmark = strchr(s, '#');

    if (hashmark != NULL) {
	if (s - hashmark == 0) {
	    /* line starts with a hashmark: recursively call nc_fgets() */
	    return (nc_fgets(s, size, stream));
	}
	else {
	    /* return only the part before '#' */
	    tmp = malloc(sizeof(char) * MAXSTR);
	    strcpy(tmp, s);
	    tmp = strtok(tmp, "#");
	    sprintf(s, "%s\n", tmp);
	    free(tmp);
	}
    }
    return (s);
}


/*
   Same as nc_fgets (). Additionally, this filters for HTML tags.
 */
char *nc_fgets_html(char *s, int size, FILE * stream)
{
    char *hashmark;
    char *tmp;
    char *tag;
    char *tag_2;
    char *tag_insert;
    char *tag_content;
    char *pos;
    char *insert;
    int space;

    if (fgets(s, size, stream) == NULL) {
	return (NULL);
    }

    /* look for HTML tags: this discards all text inside the tags except for:
       <br> becomes \n
       <p> becomes \n\n
     */
    tmp = malloc(sizeof(char) * (strlen(s) + 1));
    tag_content = malloc(sizeof(char) * (strlen(s) + 1));

    insert = tmp;
    pos = s;

    while (*pos != '\0') {
	if (*pos == '<') {	/* possibly an html open tag */
	    tag = pos;
	    tag_insert = tag_content;
	    pos--;
	    if (pos >= s) {
		if (*pos == 32) {
		    space = 1;
		}
		else {
		    space = 0;
		}
	    }
	    while (*tag != '\0') {
		*tag_insert = *tag;
		(*tag_insert)++;
		if (*tag == '>') {	/* OK, we got a tag */
		    *tag_insert = '\0';

		    /* only add additional newlines, if this is not the end of the line already! */
		    tag_2 = tag;
		    tag_2++;
		    if (*tag_2 != '\n') {
			if (strstr(tag_content, "<br>") != NULL) {
			    /* a <br> at the start of a line produce no additional linefeed! */
			    if (insert > tmp) {
				*insert = '\n';
				(*insert)++;
			    }
			}
			if (strstr(tag_content, "<BR>") != NULL) {
			    if (insert > tmp) {
				*insert = '\n';
				(*insert)++;
			    }
			}
			if (strstr(tag_content, "<p>") != NULL) {
			    if (insert > tmp) {
				*insert = '\n';
				(*insert)++;
			    }
			    *insert = '\n';
			    (*insert)++;
			}
			if (strstr(tag_content, "<P>") != NULL) {
			    if (insert > tmp) {
				*insert = '\n';
				(*insert)++;
			    }
			    *insert = '\n';
			    (*insert)++;
			}
		    }

		    pos = tag;	/* skip this */

		    /* if the next character is a space and there was none
		       before the tag: skip that, too 
		     */
		    if (*pos == 32) {
			if (space == 1) {
			    pos++;
			    space = 0;
			}
		    }
		    break;
		}
		tag++;
	    }
	}
	if (*pos != '>') {
	    *insert = *pos;
	    insert++;
	}
	pos++;
    }

    *insert = '\0';

    strcpy(s, tmp);

    free(tmp);
    free(tag_content);

    hashmark = strchr(s, '#');

    if (hashmark != NULL) {
	if (s - hashmark == 0) {
	    /* line starts with a hashmark: recursively call nc_fgets_html() */
	    return (nc_fgets_html(s, size, stream));
	}
	else {
	    /* return only the part before '#' */
	    tmp = malloc(sizeof(char) * MAXSTR);
	    strcpy(tmp, s);
	    tmp = strtok(tmp, "#");
	    sprintf(s, "%s\n", tmp);
	    free(tmp);
	}
    }

    return (s);
}


/*
   Checks whether a string actually contains text or is a blank line/whitespace only.
   Returns 1 if text , 0 if only blank/whitespace.
 */
int is_text(char *s)
{
    int i;
    int nonws;

    /* check for a blank or white-space only line */
    nonws = 0;
    for (i = strlen(s) - 1; i >= 0; i--) {
	if ((s[i] == ' ') || (s[i] == '\t') || (s[i] == '\n') ||
	    (s[i] == '\f') || (s[i] == '\r')) {
	    nonws = 0;
	}
	else {
	    nonws = 1;
	    break;		/* break at first non-ws char! */
	}
    }

    return (nonws);
}


/*
   Same as nc_fgets() but also skips over blank lines and those that contain only 
   whitespace chars.
 */
char *nc_fgets_nb(char *s, int size, FILE * stream)
{
    char *hashmark;
    char *tmp;

    if (fgets(s, size, stream) == NULL) {
	return (NULL);
    }


    if (is_text(s) == 0) {
	/* line is ws only: recursively call nc_fgets() to get next line */
	return (nc_fgets_nb(s, size, stream));
    }

    hashmark = strchr(s, '#');

    if (hashmark != NULL) {
	if (s - hashmark == 0) {
	    /* line starts with a hashmark: recursively call nc_fgets() */
	    return (nc_fgets_nb(s, size, stream));
	}
	else {
	    /* return only the part before '#' */
	    tmp = malloc(sizeof(char) * MAXSTR);
	    strcpy(tmp, s);
	    tmp = strtok(tmp, "#");
	    sprintf(s, "%s\n", tmp);
	    free(tmp);
	}
    }
    return (s);
}


/*
   This just dumps an ASCII file to stdout, line by line, but
   skips over comments.
 */
void dump_ascii(char *file, char *heading)
{
    char tmp[MAXSTR];
    FILE *f;

    fprintf(stdout, "%s\n", heading);
    f = fopen(file, "r");
    if (f == NULL) {
	fprintf(stdout, "  No information available.\n");
    }
    else {
	while (nc_fgets_html(tmp, MAXSTR, f) != NULL) {
	    fprintf(stdout, "  %s", tmp);
	}
	fprintf(stdout, "\n");
	fclose(f);
    }
}


/* 
   Dumps the contents of an ASCII file without comment lines but with blank lines
   to a temporary file.
 */
void dump_plain(char *file, char *tmpfile)
{
    char tmp[MAXSTR];
    FILE *f_in;
    FILE *f_out;

    /* create a temporary menu.tcl file for write access */
    /* TODO: Do not hardcode temp path */
    strcpy(tmpfile, "/tmp/grass.extensions.db.XXXXXX");	/* TMP_GISMAN is a global variable */
    mkstemp(tmpfile);

    f_out = fopen(tmpfile, "w+");
    if (f_out == NULL) {
	print_error(ERR_DUMP_PLAIN_TXT,
		    "could not create temp file '%s': %s\n \
		Make sure that directory /tmp exists on your system and you have write permission.\n", tmpfile, strerror(errno));
    }

    atexit(&exit_db);		/* now need to register an at exit func to remove tmpfile automatically! */

    f_in = fopen(file, "r");
    while (nc_fgets(tmp, MAXSTR, f_in) != NULL) {
	fprintf(f_out, tmp);
    }

    fclose(f_in);
    fclose(f_out);
}


/* 
   Same as dump_plain() but adds a <br> after each newline in the stream.
   Also replaces blank lines with a <p>.
 */
void dump_html(char *file, char *tmpfile)
{
    char tmp[MAXSTR];
    char line[MAXSTR];
    FILE *f_in;
    FILE *f_out;
    int fd;

    /* create a temporary menu.tcl file for write access */
    /* TODO: Do not hardcode temp path */
    strcpy(tmpfile, "/tmp/grass.extensions.db.XXXXXX");	/* TMP_GISMAN is a global variable */
    mkstemp(tmpfile);

    f_out = fopen(tmpfile, "w+");
    if (f_out == NULL) {
	print_error(ERR_DUMP_PLAIN_TXT,
		    "could not create temp file '%s': %s\n \
		Make sure that directory /tmp exists on your system and you have write permission.\n", tmpfile, strerror(errno));
    }

    atexit(&exit_db);		/* now need to register an at exit func to remove tmpfile automatically! */

    f_in = fopen(file, "r");
    while (nc_fgets(line, MAXSTR, f_in) != NULL) {
	chop(line);
	if (!is_text(line)) {	/* replace blank lines with a <p> */
	    fprintf(f_out, "<p>\n");
	}
	else {
	    sprintf(tmp, "%s <br>\n", line);
	    fprintf(f_out, tmp);
	}
    }

    fclose(f_in);
    fclose(f_out);
    close(fd);
}


/* 
   A pretty dumb function: this just lists all directories which are directly below
   top level and not called "src". It is assumed that these contain binary distributions
   which can be installed by just doing a "make install" in the respective directory.
 */
void list_binaries(char *package)
{
    char tmp[MAXSTR];
    struct stat buf;
    DIR *dir;
    struct dirent *dir_entry;
    int n_dirs = 0;

    fprintf(stdout, "Binary installation files\n");

    dir = opendir(package);
    if (dir == NULL) {
	fprintf(stdout, "  None.\n\n");
	return;
    }

    dir_entry = readdir(dir);
    while (dir_entry != NULL) {
	if ((strcmp(dir_entry->d_name, ".")) &&
	    (strcmp(dir_entry->d_name, "..")) &&
	    (strcmp(dir_entry->d_name, "src"))
	    ) {
	    /* check if it is a directory */
	    sprintf(tmp, "%s/%s", package, dir_entry->d_name);
	    stat(tmp, &buf);
	    if (S_ISDIR(buf.st_mode)) {
		if (n_dirs == 0) {
		    fprintf(stdout, "  %s", dir_entry->d_name);
		}
		else {
		    fprintf(stdout, ", %s", dir_entry->d_name);
		}
		n_dirs++;
	    }
	}
	dir_entry = readdir(dir);
    }
    if (n_dirs == 0) {
	fprintf(stdout, "  None.");
    }
    fprintf(stdout, "\n\n");
}


/* 
   Just as dumb: checks, if a specified directory 'binaries' exists in addition to 'src'
   Returns 1 if so, 0 otherwise 
 */
int binaries_exist(char *package, char *binaries)
{
    char tmp[MAXSTR];
    struct stat buf;
    DIR *dir;
    struct dirent *dir_entry;

    dir = opendir(package);
    if (dir == NULL) {
	return (0);
    }

    dir_entry = readdir(dir);
    while (dir_entry != NULL) {
	if ((strcmp(dir_entry->d_name, ".")) &&
	    (strcmp(dir_entry->d_name, "..")) &&
	    (strcmp(dir_entry->d_name, "src"))
	    ) {
	    /* check if it is a directory */
	    sprintf(tmp, "%s/%s", package, dir_entry->d_name);
	    stat(tmp, &buf);
	    if (S_ISDIR(buf.st_mode)) {
		if (!strcmp(dir_entry->d_name, binaries)) {
		    return (1);	/* found it */
		}
	    }
	}
	dir_entry = readdir(dir);
    }
    return (0);
}


/* 
   Tests for a known file-extension:
   .tar.gz, .tgz
   .tar.bz2, .tbz
   .zip
   Returns:
   0 = unknown (TYPE_UNKNOWN)
   1 = tar file with gzip compression (TAR_GZIP)
   2 = tar file with bzip2 compression (TAR_BZIP2)
   3 = zip archive (ZIP)
   4 = plain tar archive, uncompressed (TAR)

   VERY primitive!
 */
int check_filetype(char *myfile)
{

    if (strstr(myfile, ".tar.gz") != NULL) {
	return (1);
    }
    if (strstr(myfile, ".tgz") != NULL) {
	return (1);
    }
    if (strstr(myfile, ".tar.bz2") != NULL) {
	return (2);
    }
    if (strstr(myfile, ".tbz") != NULL) {
	return (2);
    }
    if (strstr(myfile, ".zip") != NULL) {
	return (3);
    }
    if (strstr(myfile, ".tar") != NULL) {
	return (4);
    }

    return (0);
}

/*
   Retrieves an extension from the WWW using wget and
   stores the file in the current directory.
 */
void wget_extension(char *url)
{
    char str[MAXSTR];
    int error;

    fprintf(stdout, "Downloading...");

    if (VERBOSE) {
	sprintf(str, "wget -N %s", url);
    }
    else {
	sprintf(str, "wget -N -q %s", url);
    }
    error = system(str);
    if (error == -1) {
	print_error(ERR_DOWNLOAD,
		    "could not run 'wget' to download extension. Is it installed?\n");
    }
    if (error > 0) {
	print_error(ERR_DOWNLOAD, "running command '%s'.\n", str);
    }

    print_done();
}

/*
   This function checks if there is write access to the GRASS directory.
   If not, it aborts with an error message.
   Otherwise, 'cmd' is simply executed as currently running user.
 */
void su(char *gisbase, char *cmd)
{
    char tmpfile[MAXSTR];
    int error;
    static unsigned long next;
    FILE *f;

    /* check for permission  */
    next = next * 1103515245 + 54321;	/* generate a random file name */
    next = (next / 65536) % 32768;
    srand(next);
    sprintf(tmpfile, "%s/gem.test.%i", gisbase, rand());

    f = fopen(tmpfile, "w+");

    if (errno == EACCES) {	/* permission denied */
	print_error(ERR_INSTALL_EXT,
		    "You don't have write access to your local GRASS installation.\nPlease consult your system administrator.\n");
    }
    else {
	remove(tmpfile);
	fclose(f);
	error = system(cmd);
	if (error != 0) {
	    print_error(ERR_MISSING_CMD, "could not run '%s'.\n", cmd);
	}
    }
}


/*
   Compares two version descriptions consisting of three ints each
   returns: -1, if major.minor.revision < major2.minor2.revision2,
   0, if equal
   1, if major.minor.revision > major2.minor2.revision2,
 */
int vercmp(int major, int minor, int revision, int major2, int minor2,
	   int revision2)
{
    if ((major == major2) && (minor == minor2) && (revision == revision2)) {
	return (0);
    }
    if (major2 > major) {
	return (-1);
    }
    if (major2 < major) {
	return (1);
    }
    if (minor2 > minor) {
	return (-1);
    }
    if (minor2 < minor) {
	return (1);
    }
    if (revision2 > revision) {
	return (-1);
    }
    if (revision2 < revision) {
	return (1);
    }

    return (0);
}
