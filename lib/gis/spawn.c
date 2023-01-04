/*!
 * \file lib/gis/spawn.c
 *
 * \brief GIS Library -  Handles process spawning.
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Glynn Clements
 *
 * \date 2004-2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#ifndef _WIN32
#include <sys/wait.h>
#else
#include <windows.h>
#endif
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

/** \def MAX_ARGS Maximum number of arguments */

/** \def MAX_BINDINGS Maximum number of bindings */

/** \def MAX_SIGNALS Maximum number of signals */

/** \def MAX_REDIRECTS Maximum number of redirects */
#define MAX_ARGS      256
#define MAX_BINDINGS  256
#define MAX_SIGNALS   32
#define MAX_REDIRECTS 32

/**
 * \brief Spawns a new process.
 *
 * A more useful alternative to G_system(), which takes the
 * arguments of <b>command</b> as parameters.
 *
 * \param[in] command command to execute
 * \return -1 on error
 * \return process status on success
 */

struct redirect {
    int dst_fd;
    int src_fd;
    const char *file;
    int mode;
};

struct signal {
    int which;
    int action;
    int signum;
    int valid;
#ifndef _WIN32
    struct sigaction old_act;
    sigset_t old_mask;
#endif
};

struct binding {
    const char *var;
    const char *val;
};

struct spawn {
    const char *args[MAX_ARGS];
    int num_args;
    struct redirect redirects[MAX_REDIRECTS];
    int num_redirects;
    struct signal signals[MAX_SIGNALS];
    int num_signals;
    struct binding bindings[MAX_BINDINGS];
    int num_bindings;
    int background;
    const char *directory;
};

static void parse_arglist(struct spawn *sp, va_list va);
static void parse_argvec(struct spawn *sp, const char **va);

#ifdef _WIN32

struct buffer {
    char *str;
    size_t len;
    size_t size;
};

static const int INCREMENT = 50;

static void clear(struct buffer *b)
{
    b->len = 0;
    b->str[b->len] = '\0';
}

static void init(struct buffer *b)
{
    b->str = G_malloc(1);
    b->size = 1;
    clear(b);
}

static char *release(struct buffer *b)
{
    char *p = b->str;

    b->str = NULL;
    b->size = 0;
    b->len = 0;

    return p;
}

static void finish(struct buffer *b)
{
    if (b->str)
        G_free(b->str);
    release(b);
}

static void ensure(struct buffer *b, size_t n)
{
    if (b->size <= b->len + n + 1) {
        b->size = b->len + n + INCREMENT;
        b->str = G_realloc(b->str, b->size);
    }
}

static void append(struct buffer *b, const char *str)
{
    size_t n = strlen(str);

    ensure(b, n);
    memcpy(&b->str[b->len], str, n);
    b->len += n;
    b->str[b->len] = '\0';
}

static void append_char(struct buffer *b, char c)
{
    ensure(b, 1);
    b->str[b->len] = c;
    b->len++;
    b->str[b->len] = '\0';
}

static void escape_arg(struct buffer *result, const char *arg)
{
    struct buffer buf;
    int quote, j;

    init(&buf);

    quote = arg[0] == '\0' || strchr(arg, ' ') || strchr(arg, '\t');

    if (quote)
        append_char(result, '\"');

    for (j = 0; arg[j]; j++) {
        int c = arg[j];
        int k;

        switch (c) {
        case '\\':
            append_char(&buf, '\\');
            break;
        case '\"':
            for (k = 0; k < buf.len; k++)
                append(result, "\\\\");
            clear(&buf);
            append(result, "\\\"");
            break;
        default:
            if (buf.len > 0) {
                append(result, buf.str);
                clear(&buf);
            }
            append_char(result, c);
        }
    }

    if (buf.len > 0)
        append(result, buf.str);

    if (quote) {
        append(result, buf.str);
        append_char(result, '\"');
    }

    finish(&buf);
}

static char *check_program(const char *pgm, const char *dir, const char *ext)
{
    char pathname[GPATH_MAX];

    sprintf(pathname, "%s%s%s%s", dir, *dir ? "\\" : "", pgm, ext);
    return access(pathname, 0) == 0 ? G_store(pathname) : NULL;
}

static char *find_program_ext(const char *pgm, const char *dir, char **pathext)
{
    char *result;
    int i;

    if (result = check_program(pgm, dir, ""), result)
        return result;

    for (i = 0; pathext[i]; i++) {
        const char *ext = pathext[i];

        if (result = check_program(pgm, dir, ext), result)
            return result;
    }

    return NULL;
}

static char *find_program_dir_ext(const char *pgm, char **path, char **pathext)
{
    char *result = NULL;
    int i;

    if (strchr(pgm, '\\') || strchr(pgm, '/')) {
        if (result = find_program_ext(pgm, "", pathext), result)
            return result;
    }
    else {
        if (result = find_program_ext(pgm, ".", pathext), result)
            return result;

        for (i = 0; path[i]; i++) {
            const char *dir = path[i];

            if (result = find_program_ext(pgm, dir, pathext), result)
                return result;
        }
    }

    return NULL;
}

static char *find_program(const char *pgm)
{
    char **path = G_tokenize(getenv("PATH"), ";");
    char **pathext = G_tokenize(getenv("PATHEXT"), ";");
    char *result = find_program_dir_ext(pgm, path, pathext);

    G_free_tokens(path);
    G_free_tokens(pathext);
    return result;
}

static char *make_command_line(int shell, const char *cmd, const char **argv)
{
    struct buffer result;
    int i;

    init(&result);

    if (shell) {
        const char *comspec = getenv("COMSPEC");

        append(&result, comspec ? comspec : "cmd.exe");
        append(&result, " /c \"");
        escape_arg(&result, cmd);
    }

    for (i = shell ? 1 : 0; argv[i]; i++) {
        if (result.len > 0)
            append_char(&result, ' ');
        escape_arg(&result, argv[i]);
    }

    append(&result, "\"");

    return release(&result);
}

static char *make_environment(const char **envp)
{
    struct buffer result;
    int i;

    init(&result);

    for (i = 0; envp[i]; i++) {
        const char *env = envp[i];

        append(&result, env);
        append_char(&result, '\0');
    }

    return release(&result);
}

static HANDLE get_handle(int fd)
{
    HANDLE h1, h2;

    if (fd < 0)
        return INVALID_HANDLE_VALUE;

    h1 = (HANDLE)_get_osfhandle(fd);
    if (!DuplicateHandle(GetCurrentProcess(), h1, GetCurrentProcess(), &h2, 0,
                         TRUE, DUPLICATE_SAME_ACCESS))
        return INVALID_HANDLE_VALUE;

    return h2;
}

static int win_spawn(const char *cmd, const char **argv, const char **envp,
                     const char *cwd, HANDLE handles[3], int background,
                     int shell)
{
    char *args = make_command_line(shell, cmd, argv);
    char *env = make_environment(envp);
    char *program = shell ? NULL : find_program(cmd);
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL result;
    DWORD exitcode;
    int i;

    if (!shell) {
        G_debug(3, "win_spawn: program = %s", program);

        if (!program) {
            G_free(args);
            G_free(env);
            return -1;
        }
    }

    G_debug(3, "win_spawn: args = %s", args);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = handles[0];
    si.hStdOutput = handles[1];
    si.hStdError = handles[2];

    result = CreateProcess(program, /* lpApplicationName */
                           args,    /* lpCommandLine */
                           NULL,    /* lpProcessAttributes */
                           NULL,    /* lpThreadAttributes */
                           1,       /* bInheritHandles */
                           0,       /* dwCreationFlags */
                           env,     /* lpEnvironment */
                           cwd,     /* lpCurrentDirectory */
                           &si,     /* lpStartupInfo */
                           &pi      /* lpProcessInformation */
    );

    G_free(args);
    G_free(env);
    G_free(program);

    if (!result) {
        G_warning(_("CreateProcess() failed: error = %d"), GetLastError());
        return -1;
    }

    CloseHandle(pi.hThread);

    for (i = 0; i < 3; i++)
        if (handles[i] != INVALID_HANDLE_VALUE)
            CloseHandle(handles[i]);

    if (!background) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        if (!GetExitCodeProcess(pi.hProcess, &exitcode))
            return -1;
        CloseHandle(pi.hProcess);
        return (int)exitcode;
    }

    CloseHandle(pi.hProcess);

    return pi.dwProcessId;
}

static void do_redirects(struct redirect *redirects, int num_redirects,
                         HANDLE handles[3])
{
    int i;

    for (i = 0; i < 3; i++)
        handles[i] = get_handle(i);

    for (i = 0; i < num_redirects; i++) {
        struct redirect *r = &redirects[i];

        if (r->dst_fd < 0 || r->dst_fd > 2) {
            if (r->file || r->src_fd >= 0)
                G_warning(_("G_spawn: unable to redirect descriptor %d"),
                          r->dst_fd);
            continue;
        }

        if (r->file) {
            r->src_fd = open(r->file, r->mode, 0666);

            if (r->src_fd < 0) {
                G_warning(_("G_spawn: unable to open file %s"), r->file);
                _exit(127);
            }

            handles[r->dst_fd] = get_handle(r->src_fd);

            close(r->src_fd);
        }
        else if (r->src_fd >= 0) {
            handles[r->dst_fd] = get_handle(r->src_fd);
        }
        else {
            if (r->dst_fd < 3) {
                CloseHandle(handles[r->dst_fd]);
                handles[r->dst_fd] = INVALID_HANDLE_VALUE;
            }
            close(r->dst_fd);
        }
    }
}

static void add_binding(const char **env, int *pnum, const struct binding *b)
{
    char *str = G_malloc(strlen(b->var) + strlen(b->val) + 2);
    int n = *pnum;
    int i;

    sprintf(str, "%s=%s", b->var, b->val);

    for (i = 0; i < n; i++)
        if (G_strcasecmp(env[i], b->var) == 0) {
            env[i] = str;
            return;
        }

    env[n++] = str;
    *pnum = n;
}

static const char **do_bindings(const struct binding *bindings,
                                int num_bindings)
{
    const char **newenv;
    int i, n;

    for (i = 0; _environ[i]; i++)
        ;
    n = i;

    newenv = G_malloc((num_bindings + n + 1) * sizeof(char *));

    for (i = 0; i < n; i++)
        newenv[i] = _environ[i];

    for (i = 0; i < num_bindings; i++)
        add_binding(newenv, &n, &bindings[i]);

    newenv[num_bindings + n] = NULL;

    return newenv;
}

static int do_spawn(struct spawn *sp, const char *command)
{
    HANDLE handles[3];
    const char **env;
    int status;

    do_redirects(sp->redirects, sp->num_redirects, handles);
    env = do_bindings(sp->bindings, sp->num_bindings);

    status = win_spawn(command, sp->args, env, sp->directory, handles,
                       sp->background, 1);

    if (!sp->background && status < 0)
        G_warning(_("G_spawn: unable to execute command"));

    return status;
}

#else /* __MINGW32__ */

static int undo_signals(const struct signal *signals, int num_signals,
                        int which)
{
    int error = 0;
    int i;

    for (i = num_signals - 1; i >= 0; i--) {
        const struct signal *s = &signals[i];

        if (s->which != which)
            continue;

        if (!s->valid)
            continue;

        switch (s->action) {
        case SSA_IGNORE:
        case SSA_DEFAULT:
            if (sigaction(s->signum, &s->old_act, NULL) < 0) {
                G_warning(_("G_spawn: unable to restore signal %d"), s->signum);
                error = 1;
            }
            break;
        case SSA_BLOCK:
        case SSA_UNBLOCK:
            if (sigprocmask(SIG_UNBLOCK, &s->old_mask, NULL) < 0) {
                G_warning(_("G_spawn: unable to restore signal %d"), s->signum);
                error = 1;
            }
            break;
        }
    }

    return !error;
}

static int do_signals(struct signal *signals, int num_signals, int which)
{
    struct sigaction act;
    sigset_t mask;
    int error = 0;
    int i;

    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;

    for (i = 0; i < num_signals; i++) {
        struct signal *s = &signals[i];

        if (s->which != which)
            continue;

        switch (s->action) {
        case SSA_IGNORE:
            act.sa_handler = SIG_IGN;
            if (sigaction(s->signum, &act, &s->old_act) < 0) {
                G_warning(_("G_spawn: unable to reset signal %d"), s->signum);
                error = 1;
            }
            else
                s->valid = 1;
            break;
        case SSA_DEFAULT:
            act.sa_handler = SIG_DFL;
            if (sigaction(s->signum, &act, &s->old_act) < 0) {
                G_warning(_("G_spawn: unable to ignore signal %d"), s->signum);
                error = 1;
            }
            else
                s->valid = 1;
            break;
        case SSA_BLOCK:
            sigemptyset(&mask);
            sigaddset(&mask, s->signum);
            if (sigprocmask(SIG_BLOCK, &mask, &s->old_mask) < 0) {
                G_warning(_("G_spawn: unable to block signal %d"), s->signum);
                error = 1;
            }
            break;
        case SSA_UNBLOCK:
            sigemptyset(&mask);
            sigaddset(&mask, s->signum);
            if (sigprocmask(SIG_UNBLOCK, &mask, &s->old_mask) < 0) {
                G_warning(_("G_spawn: unable to unblock signal %d"), s->signum);
                error = 1;
            }
            else
                s->valid = 1;
            break;
        }
    }

    return !error;
}

static void do_redirects(struct redirect *redirects, int num_redirects)
{
    int i;

    for (i = 0; i < num_redirects; i++) {
        struct redirect *r = &redirects[i];

        if (r->file) {
            r->src_fd = open(r->file, r->mode, 0666);

            if (r->src_fd < 0) {
                G_warning(_("G_spawn: unable to open file %s"), r->file);
                _exit(127);
            }

            if (dup2(r->src_fd, r->dst_fd) < 0) {
                G_warning(_("G_spawn: unable to duplicate descriptor %d to %d"),
                          r->src_fd, r->dst_fd);
                _exit(127);
            }

            close(r->src_fd);
        }
        else if (r->src_fd >= 0) {
            if (dup2(r->src_fd, r->dst_fd) < 0) {
                G_warning(_("G_spawn: unable to duplicate descriptor %d to %d"),
                          r->src_fd, r->dst_fd);
                _exit(127);
            }
        }
        else
            close(r->dst_fd);
    }
}

static void do_bindings(const struct binding *bindings, int num_bindings)
{
    int i;

    for (i = 0; i < num_bindings; i++) {
        const struct binding *b = &bindings[i];
        char *str = G_malloc(strlen(b->var) + strlen(b->val) + 2);

        sprintf(str, "%s=%s", b->var, b->val);
        putenv(str);
    }
}

static int do_spawn(struct spawn *sp, const char *command)
{
    int status = -1;
    pid_t pid;

    if (!do_signals(sp->signals, sp->num_signals, SST_PRE))
        return status;

    pid = fork();
    if (pid < 0) {
        G_warning(_("Unable to create a new process: %s"), strerror(errno));
        undo_signals(sp->signals, sp->num_signals, SST_PRE);

        return status;
    }

    if (pid == 0) {
        if (!undo_signals(sp->signals, sp->num_signals, SST_PRE))
            _exit(127);

        if (!do_signals(sp->signals, sp->num_signals, SST_CHILD))
            _exit(127);

        if (sp->directory)
            if (chdir(sp->directory) < 0) {
                G_warning(_("Unable to change directory to %s"), sp->directory);
                _exit(127);
            }

        do_redirects(sp->redirects, sp->num_redirects);
        do_bindings(sp->bindings, sp->num_bindings);

        execvp(command, (char **)sp->args);
        G_warning(_("Unable to execute command '%s': %s"), command,
                  strerror(errno));
        _exit(127);
    }

    do_signals(sp->signals, sp->num_signals, SST_POST);

    if (sp->background)
        status = (int)pid;
    else {
        pid_t n;

        do
            n = waitpid(pid, &status, 0);
        while (n == (pid_t)-1 && errno == EINTR);

        if (n != pid)
            status = -1;
        else {
            if (WIFEXITED(status))
                status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                status = WTERMSIG(status);
            else
                status = -0x100;
        }
    }

    undo_signals(sp->signals, sp->num_signals, SST_POST);
    undo_signals(sp->signals, sp->num_signals, SST_PRE);

    return status;
}

#endif /* __MINGW32__ */

static void begin_spawn(struct spawn *sp)
{
    sp->num_args = 0;
    sp->num_redirects = 0;
    sp->num_signals = 0;
    sp->num_bindings = 0;
    sp->background = 0;
    sp->directory = NULL;
}

#define NEXT_ARG(var, type) ((type) * (var)++)
#define NEXT_ARG_INT(var)   (int)((intptr_t) * (var)++)

static void parse_argvec(struct spawn *sp, const char **va)
{
    for (;;) {
        const char *arg = NEXT_ARG(va, const char *);
        const char *var, *val;

        if (!arg) {
            sp->args[sp->num_args++] = NULL;
            break;
        }
        else if (arg == SF_REDIRECT_FILE) {
            sp->redirects[sp->num_redirects].dst_fd = NEXT_ARG_INT(va);

            sp->redirects[sp->num_redirects].src_fd = -1;
            sp->redirects[sp->num_redirects].mode = NEXT_ARG_INT(va);
            sp->redirects[sp->num_redirects].file = NEXT_ARG(va, const char *);

            sp->num_redirects++;
        }
        else if (arg == SF_REDIRECT_DESCRIPTOR) {
            sp->redirects[sp->num_redirects].dst_fd = NEXT_ARG_INT(va);
            sp->redirects[sp->num_redirects].src_fd = NEXT_ARG_INT(va);

            sp->redirects[sp->num_redirects].file = NULL;
            sp->num_redirects++;
        }
        else if (arg == SF_CLOSE_DESCRIPTOR) {
            sp->redirects[sp->num_redirects].dst_fd = NEXT_ARG_INT(va);

            sp->redirects[sp->num_redirects].src_fd = -1;
            sp->redirects[sp->num_redirects].file = NULL;
            sp->num_redirects++;
        }
        else if (arg == SF_SIGNAL) {
            sp->signals[sp->num_signals].which = NEXT_ARG_INT(va);
            sp->signals[sp->num_signals].action = NEXT_ARG_INT(va);
            sp->signals[sp->num_signals].signum = NEXT_ARG_INT(va);

            sp->signals[sp->num_signals].valid = 0;
            sp->num_signals++;
        }
        else if (arg == SF_VARIABLE) {
            var = NEXT_ARG(va, const char *);

            val = getenv(var);
            sp->args[sp->num_args++] = val ? val : "";
        }
        else if (arg == SF_BINDING) {
            sp->bindings[sp->num_bindings].var = NEXT_ARG(va, const char *);
            sp->bindings[sp->num_bindings].val = NEXT_ARG(va, const char *);

            sp->num_bindings++;
        }
        else if (arg == SF_BACKGROUND) {
            sp->background = 1;
        }
        else if (arg == SF_DIRECTORY) {
            sp->directory = NEXT_ARG(va, const char *);
        }
        else if (arg == SF_ARGVEC) {
            parse_argvec(sp, NEXT_ARG(va, const char **));
        }
        else
            sp->args[sp->num_args++] = arg;
    }
}

static void parse_arglist(struct spawn *sp, va_list va)
{
    for (;;) {
        const char *arg = va_arg(va, const char *);
        const char *var, *val;

        if (!arg) {
            sp->args[sp->num_args++] = NULL;
            break;
        }
        else if (arg == SF_REDIRECT_FILE) {
            sp->redirects[sp->num_redirects].dst_fd = va_arg(va, int);

            sp->redirects[sp->num_redirects].src_fd = -1;
            sp->redirects[sp->num_redirects].mode = va_arg(va, int);
            sp->redirects[sp->num_redirects].file = va_arg(va, const char *);

            sp->num_redirects++;
        }
        else if (arg == SF_REDIRECT_DESCRIPTOR) {
            sp->redirects[sp->num_redirects].dst_fd = va_arg(va, int);
            sp->redirects[sp->num_redirects].src_fd = va_arg(va, int);

            sp->redirects[sp->num_redirects].file = NULL;
            sp->num_redirects++;
        }
        else if (arg == SF_CLOSE_DESCRIPTOR) {
            sp->redirects[sp->num_redirects].dst_fd = va_arg(va, int);

            sp->redirects[sp->num_redirects].src_fd = -1;
            sp->redirects[sp->num_redirects].file = NULL;
            sp->num_redirects++;
        }
        else if (arg == SF_SIGNAL) {
            sp->signals[sp->num_signals].which = va_arg(va, int);
            sp->signals[sp->num_signals].action = va_arg(va, int);
            sp->signals[sp->num_signals].signum = va_arg(va, int);

            sp->signals[sp->num_signals].valid = 0;
            sp->num_signals++;
        }
        else if (arg == SF_VARIABLE) {
            var = va_arg(va, char *);

            val = getenv(var);
            sp->args[sp->num_args++] = val ? val : "";
        }
        else if (arg == SF_BINDING) {
            sp->bindings[sp->num_bindings].var = va_arg(va, const char *);
            sp->bindings[sp->num_bindings].val = va_arg(va, const char *);

            sp->num_bindings++;
        }
        else if (arg == SF_BACKGROUND) {
            sp->background = 1;
        }
        else if (arg == SF_DIRECTORY) {
            sp->directory = va_arg(va, const char *);
        }
        else if (arg == SF_ARGVEC) {
            parse_argvec(sp, va_arg(va, const char **));
        }
        else
            sp->args[sp->num_args++] = arg;
    }
}

/**
 * \brief Spawn new process based on <b>command</b>.
 *
 * This is a more advanced version of G_spawn().
 *
 * \param[in] command
 * \param[in] args arguments
 * \return -1 on error
 * \return process status on success
 */
int G_vspawn_ex(const char *command, const char **args)
{
    struct spawn sp;

    begin_spawn(&sp);

    parse_argvec(&sp, args);

    return do_spawn(&sp, command);
}

/**
 * \brief Spawn new process based on <b>command</b>.
 *
 * This is a more advanced version of G_spawn().
 *
 * \param[in] command
 * \return -1 on error
 * \return process status on success
 */

int G_spawn_ex(const char *command, ...)
{
    struct spawn sp;
    va_list va;

    begin_spawn(&sp);

    va_start(va, command);
    parse_arglist(&sp, va);
    va_end(va);

    return do_spawn(&sp, command);
}

/**
 * \brief Spawn new process based on <b>command</b>.
 *
 * \param[in] command
 * \return -1 on error
 * \return process status on success
 */

int G_spawn(const char *command, ...)
{
    const char *args[MAX_ARGS];
    int num_args = 0;
    va_list va;
    int status = -1;

    va_start(va, command);

    for (;;) {
        const char *arg = va_arg(va, const char *);

        args[num_args++] = arg;
        if (!arg)
            break;
    }

    va_end(va);

    status =
        G_spawn_ex(command,
#ifndef _WIN32
                   SF_SIGNAL, SST_PRE, SSA_IGNORE, SIGINT, SF_SIGNAL, SST_PRE,
                   SSA_IGNORE, SIGQUIT, SF_SIGNAL, SST_PRE, SSA_BLOCK, SIGCHLD,
#endif
                   SF_ARGVEC, args, NULL);

    return status;
}

int G_wait(int i_pid)
{
#ifdef _WIN32
    DWORD rights = PROCESS_QUERY_INFORMATION | SYNCHRONIZE;
    HANDLE hProcess = OpenProcess(rights, FALSE, (DWORD)i_pid);
    DWORD exitcode;

    if (!hProcess)
        return -1;

    WaitForSingleObject(hProcess, INFINITE);
    if (!GetExitCodeProcess(hProcess, &exitcode))
        exitcode = (DWORD)-1;

    CloseHandle(hProcess);

    return (int)exitcode;
#else
    pid_t pid = (pid_t)i_pid;
    int status = -1;
    pid_t n;

    do
        n = waitpid(pid, &status, 0);
    while (n == (pid_t)-1 && errno == EINTR);

    if (n != pid)
        return -1;
    else {
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            return WTERMSIG(status);
        else
            return -0x100;
    }
#endif
}
