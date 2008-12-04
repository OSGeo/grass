
/**
 * \file spawn.c
 *
 * \brief GIS Library -  Handles process spawning.
 *
 * (C) 2001-2008 by the GRASS Development Team
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

#ifndef __MINGW32__
#include <sys/wait.h>
#endif
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

/** \def MAX_ARGS Maximum number of arguments */

/** \def MAX_BINDINGS Maximum number of bindings */

/** \def MAX_SIGNALS Maximum number of signals */

/** \def MAX_REDIRECTS Maximum number of redirects */
#define MAX_ARGS 256
#define MAX_BINDINGS 256
#define MAX_SIGNALS 32
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

#ifdef __MINGW32__

int G_spawn(const char *command, ...)
{
    va_list va;
    const char *args[MAX_ARGS];
    int num_args = 0;

    va_start(va, command);

    for (num_args = 0; num_args < MAX_ARGS;) {
	const char *arg = va_arg(va, const char *);

	args[num_args++] = arg;
	if (!arg)
	    break;
    }

    va_end(va);

    if (num_args >= MAX_ARGS) {
	G_warning(_("Too many arguments"));
	return -1;
    }

    return _spawnv(_P_WAIT, command, args);
}

#else

int G_spawn(const char *command, ...)
{
    va_list va;
    char *args[MAX_ARGS];
    int num_args = 0;
    struct sigaction act, intr, quit;
    sigset_t block, oldmask;
    int status = -1;
    pid_t pid;

    va_start(va, command);

    for (num_args = 0; num_args < MAX_ARGS;) {
	char *arg = va_arg(va, char *);

	args[num_args++] = arg;
	if (!arg)
	    break;
    }

    va_end(va);

    if (num_args >= MAX_ARGS) {
	G_warning(_("Too many arguments"));
	return -1;
    }

    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;

    act.sa_handler = SIG_IGN;
    if (sigaction(SIGINT, &act, &intr) < 0)
	goto error_1;
    if (sigaction(SIGQUIT, &act, &quit) < 0)
	goto error_2;

    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &block, &oldmask) < 0)
	goto error_3;

    pid = fork();

    if (pid < 0) {
	G_warning(_("Unable to create a new process"));
	goto error_4;
    }

    if (pid == 0) {
	sigaction(SIGINT, &intr, NULL);
	sigaction(SIGQUIT, &quit, NULL);

	execvp(command, args);
	G_warning(_("Unable to execute command"));
	_exit(127);
    }
    else {
	pid_t n;

	do
	    n = waitpid(pid, &status, 0);
	while (n == (pid_t) - 1 && errno == EINTR);

	if (n != pid)
	    status = -1;
    }

  error_4:
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
  error_3:
    sigaction(SIGQUIT, &quit, NULL);
  error_2:
    sigaction(SIGINT, &intr, NULL);
  error_1:
    return status;
}

#endif /*__MINGW32__*/

struct redirect
{
    int dst_fd;
    int src_fd;
    const char *file;
    int mode;
};

struct signal
{
    int which;
    int action;
    int signum;
    int valid;
#ifndef __MINGW32__
    struct sigaction old_act;
    sigset_t old_mask;
#endif
};

struct binding
{
    const char *var;
    const char *val;
};

struct spawn
{
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

#ifdef __MINGW32__

static void do_redirects(struct redirect *redirects, int num_redirects)
{
    if (num_redirects > 0)
	G_fatal_error
	    ("G_spawn_ex: redirection not (yet) supported on Windows");
}

static char **do_bindings(char **env, struct binding *bindings,
			  int num_bindings)
{
    if (num_bindings > 0)
	G_fatal_error
	    ("G_spawn_ex: redirection not (yet) supported on Windows");

    return env;
}

static int do_spawn(struct spawn *sp, const char *command)
{
    char **env;
    int status;

    do_redirects(sp->redirects, sp->num_redirects);
    env = do_bindings(_environ, sp->bindings, sp->num_bindings);

    status =
	spawnvpe(sp->background ? _P_NOWAIT : _P_WAIT, command, sp->args, env);

    if (!sp->background && status < 0)
	G_warning(_("Unable to execute command"));

    return status;
}

#else /* __MINGW32__ */

static int undo_signals(struct signal *signals, int num_signals, int which)
{
    int error = 0;
    int i;

    for (i = num_signals - 1; i >= 0; i--) {
	struct signal *s = &signals[i];

	if (s->which != which)
	    continue;

	if (!s->valid)
	    continue;

	switch (s->action) {
	case SSA_IGNORE:
	case SSA_DEFAULT:
	    if (sigaction(s->signum, &s->old_act, NULL) < 0) {
		G_warning(_("G_spawn: unable to restore signal %d"),
			  s->signum);
		error = 1;
	    }
	    break;
	case SSA_BLOCK:
	case SSA_UNBLOCK:
	    if (sigprocmask(SIG_UNBLOCK, &s->old_mask, NULL) < 0) {
		G_warning(_("G_spawn: unable to restore signal %d"),
			  s->signum);
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
		G_warning(_("G_spawn: unable to ignore signal %d"),
			  s->signum);
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
		G_warning(_("G_spawn: unable to unblock signal %d"),
			  s->signum);
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

static void do_bindings(struct binding *bindings, int num_bindings)
{
    int i;

    for (i = 0; i < num_bindings; i++) {
	struct binding *b = &bindings[i];
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
	G_warning(_("Unable to create a new process"));
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
	G_warning(_("Unable to execute command"));
	_exit(127);
    }

    do_signals(sp->signals, sp->num_signals, SST_POST);

    if (sp->background)
	status = (int)pid;
    else {
	pid_t n;

	do
	    n = waitpid(pid, &status, 0);
	while (n == (pid_t) - 1 && errno == EINTR);

	if (n != pid)
	    status = -1;
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

#define NEXT_ARG(var, type) ((type) *(var)++)

static void parse_argvec(struct spawn *sp, const char **va)
{
    for (;;) {
	const char *arg = NEXT_ARG(va, const char *);
	const char *var, *val;

	switch ((int)arg) {
	case 0:
	    sp->args[sp->num_args++] = NULL;
	    break;
	case ((int)SF_REDIRECT_FILE):
	    sp->redirects[sp->num_redirects].dst_fd = NEXT_ARG(va, int);

	    sp->redirects[sp->num_redirects].src_fd = -1;
	    sp->redirects[sp->num_redirects].mode = NEXT_ARG(va, int);
	    sp->redirects[sp->num_redirects].file = NEXT_ARG(va, const char *);

	    sp->num_redirects++;
	    break;
	case ((int)SF_REDIRECT_DESCRIPTOR):
	    sp->redirects[sp->num_redirects].dst_fd = NEXT_ARG(va, int);
	    sp->redirects[sp->num_redirects].src_fd = NEXT_ARG(va, int);

	    sp->redirects[sp->num_redirects].file = NULL;
	    sp->num_redirects++;
	    break;
	case ((int)SF_CLOSE_DESCRIPTOR):
	    sp->redirects[sp->num_redirects].dst_fd = NEXT_ARG(va, int);

	    sp->redirects[sp->num_redirects].src_fd = -1;
	    sp->redirects[sp->num_redirects].file = NULL;
	    sp->num_redirects++;
	    break;
	case ((int)SF_SIGNAL):
	    sp->signals[sp->num_signals].which = NEXT_ARG(va, int);
	    sp->signals[sp->num_signals].action = NEXT_ARG(va, int);
	    sp->signals[sp->num_signals].signum = NEXT_ARG(va, int);

	    sp->signals[sp->num_signals].valid = 0;
	    sp->num_signals++;
	    break;
	case ((int)SF_VARIABLE):
	    var = NEXT_ARG(va, const char *);

	    val = getenv(var);
	    sp->args[sp->num_args++] = val ? val : "";
	    break;
	case ((int)SF_BINDING):
	    sp->bindings[sp->num_bindings].var = NEXT_ARG(va, const char *);
	    sp->bindings[sp->num_bindings].val = NEXT_ARG(va, const char *);

	    sp->num_bindings++;
	    break;
	case ((int)SF_BACKGROUND):
	    sp->background = 1;
	    break;
	case ((int)SF_DIRECTORY):
	    sp->directory = NEXT_ARG(va, const char *);

	    break;
	case ((int)SF_ARGVEC):
	    parse_argvec(sp, NEXT_ARG(va, const char **));

	    break;
	default:
	    sp->args[sp->num_args++] = arg;
	    break;
	}

	if (!arg)
	    break;
    }
}

static void parse_arglist(struct spawn *sp, va_list va)
{
    for (;;) {
	const char *arg = va_arg(va, const char *);
	const char *var, *val;

	switch ((int)arg) {
	case 0:
	    sp->args[sp->num_args++] = NULL;
	    break;
	case ((int)SF_REDIRECT_FILE):
	    sp->redirects[sp->num_redirects].dst_fd = va_arg(va, int);

	    sp->redirects[sp->num_redirects].src_fd = -1;
	    sp->redirects[sp->num_redirects].mode = va_arg(va, int);
	    sp->redirects[sp->num_redirects].file = va_arg(va, const char *);

	    sp->num_redirects++;
	    break;
	case ((int)SF_REDIRECT_DESCRIPTOR):
	    sp->redirects[sp->num_redirects].dst_fd = va_arg(va, int);
	    sp->redirects[sp->num_redirects].src_fd = va_arg(va, int);

	    sp->redirects[sp->num_redirects].file = NULL;
	    sp->num_redirects++;
	    break;
	case ((int)SF_CLOSE_DESCRIPTOR):
	    sp->redirects[sp->num_redirects].dst_fd = va_arg(va, int);

	    sp->redirects[sp->num_redirects].src_fd = -1;
	    sp->redirects[sp->num_redirects].file = NULL;
	    sp->num_redirects++;
	    break;
	case ((int)SF_SIGNAL):
	    sp->signals[sp->num_signals].which = va_arg(va, int);
	    sp->signals[sp->num_signals].action = va_arg(va, int);
	    sp->signals[sp->num_signals].signum = va_arg(va, int);

	    sp->signals[sp->num_signals].valid = 0;
	    sp->num_signals++;
	    break;
	case ((int)SF_VARIABLE):
	    var = va_arg(va, char *);

	    val = getenv(var);
	    sp->args[sp->num_args++] = val ? val : "";
	    break;
	case ((int)SF_BINDING):
	    sp->bindings[sp->num_bindings].var = va_arg(va, const char *);
	    sp->bindings[sp->num_bindings].val = va_arg(va, const char *);

	    sp->num_bindings++;
	    break;
	case ((int)SF_BACKGROUND):
	    sp->background = 1;
	    break;
	case ((int)SF_DIRECTORY):
	    sp->directory = va_arg(va, const char *);

	    break;
	case ((int)SF_ARGVEC):
	    parse_argvec(sp, va_arg(va, const char **));

	    break;
	default:
	    sp->args[sp->num_args++] = arg;
	    break;
	}

	if (!arg)
	    break;
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
