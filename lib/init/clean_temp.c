#include <grass/config.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grass/gis.h>
#include "local_proto.h"

/**************************************************************
 * clean_temp
 *
 *   looks for all files in mapset temp directory
 *   of the form pid.n and removes those which have
 *   been abandoned their processes (pid).
 *
 *   also removes any other file found which is "old"
 *   with an modification time greater then 4 days
 *
 *   2006: Rewritten for GRASS 6 by Roberto Flor, ITC-irst
 *
 **************************************************************/

#include <limits.h>
#include <string.h>
#include <errno.h>
#ifdef PATH_MAX
#define BUF_MAX PATH_MAX
#else
#define BUF_MAX 4096
#endif

#define SLEEP 30		/* 30 seconds */

/* Recursively scan the directory pathname, removing directory and files */

void clean_dir(const char *pathname, uid_t uid, pid_t pid, time_t now,
	       int max_age)
{
    char buf[BUF_MAX];
    DIR *curdir;
    struct dirent *cur_entry;
    struct stat info;
    int n, pathlen;

    curdir = opendir(pathname);
    if (curdir == NULL) {
	G_warning("Can't open directory %s: %s,skipping\n", pathname,
		  strerror(errno));
	return;
    }
    /* loop over current dir */
    while ((cur_entry = readdir(curdir))) {
	if ((G_strcasecmp(cur_entry->d_name, ".") == 0) ||
	    (G_strcasecmp(cur_entry->d_name, "..") == 0))
	    continue;		/* Skip dir and parent dir entries */

	if ((pathlen =
	     G_snprintf(buf, BUF_MAX, "%s/%s", pathname,
			cur_entry->d_name)) >= BUF_MAX)
	    G_fatal_error
		("clean_temp: exceeded maximum pathname length %d, got %d, should'nt happen",
		 BUF_MAX, pathlen);

	if (stat(buf, &info) != 0) {
	    G_warning("Can't stat file %s: %s,skipping\n", buf,
		      strerror(errno));
	    continue;
	}
	if (S_ISDIR(info.st_mode)) {	/* It's a dir, recurring */
	    clean_dir(buf, uid, pid, now, max_age);
	    /* Return here means we have completed the subdir recursion */
	    /* Trying to remove the now empty dir */
	    if (info.st_uid != uid)	/* Not owners of dir */
		continue;
#ifndef DEBUG_CLEAN
	    if (rmdir(buf) != 0) {
		if (errno != ENOTEMPTY) {
		    G_warning
			("Can't remove empty directory %s: %s,skipping\n",
			 buf, strerror(errno));
		}
	    }
#else
	    G_warning("Removing directory %s\n", buf);
#endif
	}
	else {			/* It's a file check it */
	    if (info.st_uid == uid) {	/* Remove only files owned by current user */
		if (sscanf(cur_entry->d_name, "%d.%d", &pid, &n) == 2) {
		    if (!find_process(pid))
#ifndef DEBUG_CLEAN
			if (unlink(buf) != 0)
			    G_warning("Can't remove file %s: %s,skipping\n",
				      buf, strerror(errno));
#else
			G_warning("Removing file %s\n", buf);
#endif
		}
		else {
		    if ((now - info.st_mtime) > max_age)	/* Not modified in 4 days: TODO configurable param */
#ifndef DEBUG_CLEAN
			if (unlink(buf) != 0)
			    G_warning("Can't remove file %s: %s,skipping\n",
				      buf, strerror(errno));
#else
			G_warning("Removing file %s\n", buf);
#endif
		}
	    }
	}
    }
    closedir(curdir);
    return;
}

int main(int argc, char *argv[])
{
    const char *mapset;
    char element[GNAME_MAX];
    char tmppath[BUF_MAX];
    pid_t ppid;
    pid_t pid;
    uid_t uid;
    time_t now;
    long max_age;

    G_gisinit(argv[0]);
    pid = 0;
    ppid = 0;
    if (argc > 1)
	sscanf(argv[1], "%d", &ppid);

    /* Get the mapset temp directory */
    G__temp_element(element);
    G_file_name(tmppath, element, "", mapset = G_mapset());

    /* get user id and current time in seconds */
#ifdef __MINGW32__
    /* TODO */
    uid = -1;
#else
    uid = getuid();
#endif

    now = time(NULL);

    /* set maximum age in seconds (4 days) */
    max_age = 4 * 24 * 60 * 60;

    /*
     * Scan the temp directory and subdirectory for 
     * files owned by the user and of the form pid.n
     * to be removed if the process is not running
     * all "old" files are removed as well
     */

    while (1) {
	if (ppid > 0 && !find_process(ppid))
	    break;
	clean_dir(tmppath, uid, pid, now, max_age);
	if (ppid <= 0)
	    break;
	G_sleep(SLEEP);
    }
    exit(0);
}

int find_process(int pid)
{
#ifdef __MINGW32__
    /* TODO */
    return -1;
#else
    return (kill(pid, 0) == 0 || errno != ESRCH);
#endif
}
