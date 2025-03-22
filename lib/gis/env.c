/*!
   \file lib/gis/env.c

   \brief GIS library - environment routines

   (C) 2001-2025 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
   \author Updated for GRASS7 by Glynn Clements
 */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h> /* for sleep() */
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

struct bind {
    int loc;
    char *name;
    char *value;
};

struct env {
    struct bind *binds;
    int count;
    int size;
};

static struct state {
    struct env env;
    struct env env2;
    char *gisrc;
    int varmode;
    int init[2];
} state;

static struct state *st = &state;

static int read_env(int);
static int set_env(const char *, const char *, int);
static int unset_env(const char *, int);
static const char *get_env(const char *, int);
static void write_env(int);
static void parse_env(FILE *, int);
static void force_read_env(int);
static FILE *open_env(const char *, int);

/*!
   \brief Set where to find/store variables

   Modes:
   - G_GISRC_MODE_FILE
   - G_GISRC_MODE_MEMORY

   \param mode mode to find/store variables (G_GISRC_MODE_FILE by default)
 */
void G_set_gisrc_mode(int mode)
{
    st->varmode = mode;
}

/*!
   \brief Get info where variables are stored

   \return mode
 */
int G_get_gisrc_mode(void)
{
    return (st->varmode);
}

/*!
   \brief Initialize variables

   \return
 */
void G_init_env(void)
{
    read_env(G_VAR_GISRC);
    read_env(G_VAR_MAPSET);
}

/*!
 * \brief Force to read the mapset environment file VAR
 *
 * The mapset specific VAR file of the mapset set with G_setenv()
 * will be read into memory, ignoring if it was read before.
 * Existing values will be overwritten, new values appended.
 *
 * \return
 */
void G__read_mapset_env(void)
{
    force_read_env(G_VAR_MAPSET);
}

/*!
 * \brief Force to read the GISRC environment file
 *
 * The GISRC file
 * will be read into memory, ignoring if it was read before.
 * Existing values will be overwritten, new values appended.
 *
 * \return
 */
void G__read_gisrc_env(void)
{
    force_read_env(G_VAR_GISRC);
}

/*!
 * \brief Read or read again the GISRC (session) environment variable
 *
 * The GISRC environment variable will be read and its value
 * stored, ignoring if it was read before.
 *
 * Calls G_fatal_error when the GISRC variable is not set.
 */
void G__read_gisrc_path(void)
{
    st->gisrc = getenv("GISRC");
    if (!st->gisrc) {
        G_fatal_error(_("No active GRASS session: "
                        "GISRC environment variable not set"));
    }
}

static void parse_env(FILE *fd, int loc)
{
    /* Account for long lines up to GPATH_MAX.
       E.g. "GISDBASE: GPATH_MAX\n\0" */
    char buf[GPATH_MAX + 16];
    char *name;
    char *value;

    while (G_getl2(buf, sizeof buf, fd)) {
        for (name = value = buf; *value; value++)
            if (*value == ':')
                break;
        if (*value == 0)
            continue;

        *value++ = 0;
        G_strip(name);
        G_strip(value);
        if (*name && *value)
            set_env(name, value, loc);
    }
}

static int read_env(int loc)
{

    FILE *fd;

    if (loc == G_VAR_GISRC && st->varmode == G_GISRC_MODE_MEMORY)
        return 0; /* don't use file for GISRC */

    if (G_is_initialized(&st->init[loc]))
        return 1;

    if ((fd = open_env("r", loc))) {
        parse_env(fd, loc);
        fclose(fd);
    }

    G_initialize_done(&st->init[loc]);
    return 0;
}

/*!
 * \brief Force the reading or the GISRC or MAPSET/VAR files
 * and overwrite/append the specified variables
 *
 */
static void force_read_env(int loc)
{
    FILE *fd;

    if ((fd = open_env("r", loc))) {
        parse_env(fd, loc);
        fclose(fd);
    }
}

static int set_env(const char *name, const char *value, int loc)
{
    int n;
    int empty;
    char *tv;

    /* if value is NULL or empty string, convert into an unsetenv() */
    if (!value || !strlen(value)) {
        unset_env(name, loc);
        return 0;
    }

    tv = G_store(value);
    G_strip(tv);
    if (*tv == 0) {
        G_free(tv);
        unset_env(name, loc);
        return 1;
    }

    /*
     * search the array
     *   keep track of first empty slot
     *   and look for name in the environment
     */
    empty = -1;
    for (n = 0; n < st->env.count; n++) {
        struct bind *b = &st->env.binds[n];

        if (!b->name) /* mark empty slot found */
            empty = n;
        else if (strcmp(b->name, name) == 0 && b->loc == loc) {
            b->value = tv;
            return 1;
        }
    }

    /* add name to env: to empty slot if any */
    if (empty >= 0) {
        struct bind *b = &st->env.binds[empty];

        b->loc = loc;
        b->name = G_store(name);
        b->value = tv;
        return 0;
    }

    /* must increase the env list and add in */
    if (st->env.count >= st->env.size) {
        st->env.size += 20;
        st->env.binds =
            G_realloc(st->env.binds, st->env.size * sizeof(struct bind));
    }

    {
        struct bind *b = &st->env.binds[st->env.count++];

        b->loc = loc;
        b->name = G_store(name);
        b->value = tv;
    }

    return 0;
}

static int unset_env(const char *name, int loc)
{
    int n;

    for (n = 0; n < st->env.count; n++) {
        struct bind *b = &st->env.binds[n];

        if (b->name && strcmp(b->name, name) == 0 && b->loc == loc) {
            G_free(b->name);
            b->name = 0;
            return 1;
        }
    }

    return 0;
}

static const char *get_env(const char *name, int loc)
{
    int n;

    for (n = 0; n < st->env.count; n++) {
        struct bind *b = &st->env.binds[n];

        if (b->name && (strcmp(b->name, name) == 0) && b->loc == loc)
            return b->value;
    }

    return NULL;
}

static void write_env(int loc)
{
    FILE *fd;
    int n;
    char dummy[2];
    void (*sigint)(int);

#ifdef SIGQUIT
    void (*sigquit)(int);
#endif

    if (loc == G_VAR_GISRC && st->varmode == G_GISRC_MODE_MEMORY)
        return; /* don't use file for GISRC */

    /*
     * THIS CODE NEEDS TO BE PROTECTED FROM INTERRUPTS
     * If interrupted, it can wipe out the GISRC file
     */
    sigint = signal(SIGINT, SIG_IGN);
#ifdef SIGQUIT
    sigquit = signal(SIGQUIT, SIG_IGN);
#endif
    if ((fd = open_env("w", loc))) {
        for (n = 0; n < st->env.count; n++) {
            struct bind *b = &st->env.binds[n];

            if (b->name && b->value && b->loc == loc &&
                (sscanf(b->value, "%1s", dummy) == 1))
                fprintf(fd, "%s: %s\n", b->name, b->value);
        }
        fclose(fd);
    }

    signal(SIGINT, sigint);
#ifdef SIGQUIT
    signal(SIGQUIT, sigquit);
#endif
}

static FILE *open_env(const char *mode, int loc)
{
    char buf[GPATH_MAX] = {0}; // initialized

    if (loc == G_VAR_GISRC) {
        if (!st->gisrc)
            G__read_gisrc_path();

        if (!st->gisrc) {
            return NULL;
        }
        G_strlcpy(buf, st->gisrc, sizeof(buf));
    }
    else if (loc == G_VAR_MAPSET) {
        /* Warning: G_VAR_GISRC must be previously read -> */
        /* TODO: better place ? */
        read_env(G_VAR_GISRC);

        sprintf(buf, "%s/%s/VAR", G_location_path(), G_mapset());
    }

    return fopen(buf, mode);
}

/*!
   \brief Get environment variable

   G_fatal_error() is called when variable is not found.

   \param name variable name

   \return char pointer to value for name
 */
const char *G_getenv(const char *name)
{
    const char *value = G_getenv_nofatal(name);

    if (value)
        return value;

    G_fatal_error(_("Incomplete GRASS session: Variable '%s' not set"), name);
    return NULL;
}

/*!
   \brief Get variable from specific place

   Locations:
   - G_VAR_GISRC
   - G_VAR_MAPSET

   G_fatal_error() is called when variable is not found.

   \param name variable name
   \param loc location (G_VAR_GISRC, G_VAR_MAPSET)

   \return variable value
   \return NULL if not found
 */
const char *G_getenv2(const char *name, int loc)
{
    const char *value = G_getenv_nofatal2(name, loc);

    if (value)
        return value;

    G_fatal_error(_("Incomplete GRASS session: Variable '%s' not set"), name);
    return NULL;
}

/*!
   \brief Get environment variable

   \param name variable name

   \return char pointer to value for name
   \return NULL if name not set
 */
const char *G_getenv_nofatal(const char *name)
{
    if (strcmp(name, "GISBASE") == 0)
        return getenv(name);

    read_env(G_VAR_GISRC);

    return get_env(name, G_VAR_GISRC);
}

/*!
   \brief Get environment variable from specific place

   \param name variable name
   \param loc location (G_VAR_GISRC, G_VAR_MAPSET)

   \return char pointer to value for name
   \return NULL if name not set
 */
const char *G_getenv_nofatal2(const char *name, int loc)
{
    if (strcmp(name, "GISBASE") == 0)
        return getenv(name);

    read_env(loc);

    return get_env(name, loc);
}

/*!
   \brief Set environment variable (updates .gisrc)

   If value is NULL, becomes an G_unsetenv().

   \param name variable name
   \param value variable value
 */
void G_setenv(const char *name, const char *value)
{
    read_env(G_VAR_GISRC);
    set_env(name, value, G_VAR_GISRC);
    write_env(G_VAR_GISRC);
}

/*!
   \brief Set environment variable from specific place (updates .gisrc)

   If value is NULL, becomes an G_unsetenv().

   \param name variable name
   \param value variable value
   \param loc location (G_VAR_GISRC, G_VAR_MAPSET)

 */
void G_setenv2(const char *name, const char *value, int loc)
{
    read_env(loc);
    set_env(name, value, loc);
    write_env(loc);
}

/*!
   \brief Set environment name to value (doesn't update .gisrc)

   \param name variable name
   \param value variable value
 */
void G_setenv_nogisrc(const char *name, const char *value)
{
    read_env(G_VAR_GISRC);
    set_env(name, value, G_VAR_GISRC);
}

/*!
   \brief Set environment name to value from specific place (doesn't update
   .gisrc)

   \param name variable name
   \param value variable value
   \param loc location (G_VAR_GISRC, G_VAR_MAPSET)
 */
void G_setenv_nogisrc2(const char *name, const char *value, int loc)
{
    read_env(loc);
    set_env(name, value, loc);
}

/*!
   \brief Remove name from environment

   Updates .gisrc

   \param name variable name
 */
void G_unsetenv(const char *name)
{
    read_env(G_VAR_GISRC);
    unset_env(name, G_VAR_GISRC);
    write_env(G_VAR_GISRC);
}

/*!
   \brief Remove name from environment from specific place

   Updates .gisrc

   \param name variable name
   \param loc location (G_VAR_GISRC, G_VAR_MAPSET)
 */
void G_unsetenv2(const char *name, int loc)
{
    read_env(loc);
    unset_env(name, loc);
    write_env(loc);
}

/*!
   \brief Writes current environment to .gisrc
 */
void G__write_env(void)
{
    if (st->init[G_VAR_GISRC])
        write_env(G_VAR_GISRC);
}

/*!
   \brief Get variable name for index n.

   For example:

   \code
   for (n = 0; ; n++)
   if ((name = G_get_env_name(n)) == NULL)
   break;
   \endcode

   \param n index of variable

   \return pointer to variable name
   \return NULL not found
 */
const char *G_get_env_name(int n)
{
    int i;

    read_env(G_VAR_GISRC);
    if (n >= 0)
        for (i = 0; i < st->env.count; i++)
            if (st->env.binds[i].name && *st->env.binds[i].name && (n-- == 0))
                return st->env.binds[i].name;
    return NULL;
}

/*!
   \brief Initialize init array for G_VAR_GISRC.
 */
void G__read_env(void)
{
    st->init[G_VAR_GISRC] = 0;
}

/*!
   \brief Set up alternative environment variables
 */
void G_create_alt_env(void)
{
    int i;

    /* copy env to env2 */
    st->env2 = st->env;

    st->env.count = 0;
    st->env.size = 0;
    st->env.binds = NULL;

    for (i = 0; i < st->env2.count; i++) {
        struct bind *b = &st->env2.binds[i];

        if (b->name)
            set_env(b->name, b->value, G_VAR_GISRC);
    }
}

/*!
   \brief Switch environments
 */
void G_switch_env(void)
{
    struct env tmp;

    tmp = st->env;
    st->env = st->env2;
    st->env2 = tmp;
}
