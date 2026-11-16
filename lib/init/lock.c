#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "local_proto.h"
#include <grass/gis.h>
#include <grass/glocale.h>
#else
#include <windows.h>
#endif

/******************************************************************
 *lock file pid
 *
 *   this programs "locks" the file for process pid:
 *
 *   1. if file exists, the pid is read out of the file. if this
 *      process is still running, the file is considered locked.
 *      exit(2).
 *   2. something weird happened. G_fatal_error() aka exit(1)
 *   3. if file does not exist, or if file exists but process is not
 *      running (ie, lock was not removed), the file is locked for
 *      process pid by writing pid into the file.
 *      exit(0).
 ******************************************************************/

#include <errno.h>

int find_process(int pid);

int main(int argc, char *argv[])
{
    int pid;
    int lockpid;
#ifndef _WIN32
    int lock;
    int locked;
#endif

    if (argc != 3 || sscanf(argv[2], "%d", &lockpid) != 1) {
#ifdef _WIN32
        fprintf(stderr, "Usage: %s file pid\n", argv[0]);
        return 1;
#else
        G_fatal_error(_("Usage: %s file pid"), argv[0]);
#endif
    }
#define file argv[1]

#ifdef _WIN32
    for (;;) {
        HANDLE lock_file = CreateFileA(
            file, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (lock_file != INVALID_HANDLE_VALUE) {
            DWORD written;
            int result = WriteFile(lock_file, &lockpid, sizeof lockpid, &written,
                                   NULL) &&
                          written == sizeof lockpid;
            CloseHandle(lock_file);
            if (!result) {
                fprintf(stderr, "Unable to write lockfile %s\n", file);
                DeleteFileA(file);
                return 1;
            }
            return 0;
        }
        if (GetLastError() != ERROR_FILE_EXISTS)
            break;

        lock_file = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (lock_file == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                continue;
            break;
        }
        DWORD read;
        int has_pid = ReadFile(lock_file, &pid, sizeof pid, &read, NULL) &&
                      read == sizeof pid;
        CloseHandle(lock_file);
        if (has_pid && find_process(pid))
            return 2;
        if (!DeleteFileA(file) && GetLastError() != ERROR_FILE_NOT_FOUND)
            break;
    }
    fprintf(stderr, "%s: unable to create lock file\n", file);
    return 1;
#else
    locked = 0;
    if ((lock = open(file, 0)) >= 0) { /* file exists */
        G_sleep(1); /* allow time for file creator to write its pid */
        if (read(lock, &pid, sizeof pid) == sizeof pid)
            locked = find_process(pid);
        close(lock);
    }
    if (locked)
        exit(2);

    if ((lock = creat(file, 0666)) < 0) {
        perror(file);
        G_fatal_error("%s: ", argv[0]);
    }
    if (write(lock, &lockpid, sizeof lockpid) != sizeof lockpid)
        G_fatal_error(_("Unable to write lockfile %s (%s)"), file,
                      strerror(errno));
    close(lock);
    exit(0);
#endif
}

int find_process(int pid)
{
    /* attempt to kill pid with NULL signal. if success, then
       process pid is still running. otherwise, must check if
       kill failed because no such process, or because user is
       not owner of process
     */
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    DWORD exit_code;
    int running;
    if (process == NULL)
        return GetLastError() == ERROR_ACCESS_DENIED;
    running = !GetExitCodeProcess(process, &exit_code) ||
              exit_code == STILL_ACTIVE;
    CloseHandle(process);
    return running;
#else
    if (kill(pid, 0) == 0)
        return 1;
    return errno != ESRCH;
#endif
}
