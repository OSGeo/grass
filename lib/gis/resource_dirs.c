/*!
   \file lib/gis/resource_dirs.c

   \brief GIS Library - Get paths to resource directories.

   \author Nicklas Larsson

   SPDX-FileCopyrightText: 2025 Nicklas Larsson
   SPDX-FileCopyrightText: GRASS Development Team
   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <grass/config.h>

#if defined(HAVE_DLADDR)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>

#elif defined(_WIN32)
// TODO: add
#else
#error Support for Dynamically Loaded Libraries (DL) is missing
#endif // defined(HAVE_DLADDR)

#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && \
    !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#define RESOURCE_DIRS_ATOMIC 1
#else
#include <stdbool.h>
#define RESOURCE_DIRS_ATOMIC 0
#endif

#include <grass/gis.h>
#include <grass/glocale.h>

typedef enum {
    GRASS_COLORSDIR,
    GRASS_DOCDIR,
    GRASS_ETCBINDIR,
    GRASS_ETCDIR,
    GRASS_FONTSDIR,
    GRASS_GRAPHICSDIR,
    GRASS_GUISCRIPTDIR,
    GRASS_LOCALEDIR,
    GRASS_MISCDIR,
    GRASS_MKDOCSDIR,
} GResourceDirType;

static const char *const g_resource_dir_type_names[] = {"GRASS_COLORSDIR",
                                                        "GRASS_DOCDIR",
                                                        "GRASS_ETCBINDIR",
                                                        "GRASS_ETCDIR",
                                                        "GRASS_FONTSDIR",
                                                        "GRASS_GRAPHICSDIR",
                                                        "GRASS_GUISCRIPTDIR",
                                                        "GRASS_LOCALEDIR",
                                                        "GRASS_MISCDIR",
                                                        "GRASS_MKDOCSDIR",
                                                        NULL};

#define MAX_PAIRS   32
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 512

typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VAL_LEN];
} KVPair;

static KVPair g_config[MAX_PAIRS];
static size_t g_config_count = 0;

static void config_init_if_needed(void);
static const char *get_g_env(GResourceDirType t);

const char *G_config_get(const char *key)
{
    config_init_if_needed();
    for (size_t i = 0; i < g_config_count; i++) {
        if (strcmp(g_config[i].key, key) == 0) {
            return g_config[i].value;
        }
    }
    return NULL; // not found
}

const char *G_colors_dir(void)
{
    return get_g_env(GRASS_COLORSDIR);
}

const char *G_doc_dir(void)
{
    return get_g_env(GRASS_DOCDIR);
}

const char *G_etcbin_dir(void)
{
    return get_g_env(GRASS_ETCBINDIR);
}

const char *G_etc_dir(void)
{
    return get_g_env(GRASS_ETCDIR);
}

const char *G_fonts_dir(void)
{
    return get_g_env(GRASS_FONTSDIR);
}

const char *G_graphics_dir(void)
{
    return get_g_env(GRASS_GRAPHICSDIR);
}

const char *G_gui_resource_dir(void)
{
    return getenv("GRASS_GUIRESDIR");
}

const char *G_gui_script_dir(void)
{
    return get_g_env(GRASS_GUISCRIPTDIR);
}

const char *G_gui_wx_dir(void)
{
    return getenv("GRASS_GUIWXDIR");
}

const char *G_locale_dir(void)
{
    return get_g_env(GRASS_LOCALEDIR);
}

const char *G_misc_dir(void)
{
    return get_g_env(GRASS_MISCDIR);
}

const char *G_mkdoc_dir(void)
{
    return get_g_env(GRASS_MKDOCSDIR);
}

// Internal
static int parse_line(const char *line, char *out_key, char *out_val);
int config_load_file(FILE *fp);
static const char *get_config_file(void);

static int parse_line(const char *line, char *out_key, char *out_val)
{
    // Expect lines like: key = value
    const char *eq = strchr(line, '=');
    if (!eq)
        return 0;

    size_t klen = (size_t)(eq - line);
    if (klen >= MAX_KEY_LEN)
        klen = MAX_KEY_LEN - 1;

    G_strlcpy(out_key, line, klen + 1);

    const char *vstart = eq + 1;
    size_t vlen = strnlen(vstart, MAX_VAL_LEN - 1);
    if (vlen > 0 && vstart[vlen - 1] == '\n')
        vlen--;

    G_strlcpy(out_val, vstart, vlen + 1);

    G_strip(out_key);
    G_strip(out_val);

    return (out_key[0] != '\0'); // valid only if key is non-empty
}

int config_load_file(FILE *fp)
{
    if (!fp)
        return -1;
    g_config_count = 0;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        // Skip comments and blank lines
        char *p = line;
        while (isspace((unsigned char)*p))
            p++;
        if (*p == '\0' || *p == '#' || *p == ';')
            continue;

        char key[MAX_KEY_LEN] = {0}, val[MAX_VAL_LEN] = {0};
        if (parse_line(p, key, val)) {
            // Overwrite if key already exists
            size_t i;
            for (i = 0; i < g_config_count; i++) {
                if (strcmp(g_config[i].key, key) == 0) {
                    G_strlcpy(g_config[i].value, val, MAX_VAL_LEN);
                    break;
                }
            }
            if (i == g_config_count) {
                if (g_config_count < MAX_PAIRS) {
                    G_strlcpy(g_config[g_config_count].key, key, MAX_KEY_LEN);
                    G_strlcpy(g_config[g_config_count].value, val, MAX_VAL_LEN);
                    g_config_count++;
                }
                else {
                    G_warning(_("%s programmers error, reached max number of "
                                "configuration key-value entries."),
                              __func__);
                    return -2;
                }
            }
        }
    }
    return 0;
}

static const char grass_config_anchor = 0;

static const char *get_config_file(void)
{
    char *libfile_name;

#if defined(HAVE_DLADDR)
    Dl_info info;

    if (dladdr((const void *)&grass_config_anchor, &info) == 0) {
        G_warning(_("%s failed to resolve library address via dladdr."),
                  __func__);
        return NULL;
    }
    libfile_name = G_store(info.dli_fname);
#else // _WIN32
    // TODO: add
#endif

    char *resolved_path = realpath(libfile_name, NULL);
    if (!resolved_path) {
        G_warning(_("%s failed to resolv physical path '%s'."), __func__,
                  libfile_name);
        return NULL;
    }
    G_free(libfile_name);

    char *dir = dirname(resolved_path);
    char *library_directory = G_store(dir);
    if (!library_directory) {
        G_warning("%s failed to duplicate directory path.", __func__);
        G_free(resolved_path);
        return NULL;
    }
    G_free(resolved_path);

    char *config_file = NULL;
    char buf[GPATH_MAX] = {0};
    snprintf(buf, GPATH_MAX, "%s%c%s%c%s", library_directory, HOST_DIRSEP,
             "grass", HOST_DIRSEP, "grass.conf");
    G_free(library_directory);

    resolved_path = realpath(buf, NULL);
    if (!resolved_path) {
        int err = errno;
        G_warning(_("%s unable to resolve GRASS config file path '%s', %s."),
                  __func__, buf, strerror(err));
        return NULL;
    }
    if (access(resolved_path, R_OK) == 0) {
        config_file = G_store(resolved_path);
    }
    else {
        G_warning("%s failed to access the config file '%s'.", __func__,
                  resolved_path);
    }
    G_free(resolved_path);

    return config_file;
}

#if RESOURCE_DIRS_ATOMIC
static _Atomic bool g_config_initialized = false;
static atomic_flag g_config_init_lock = ATOMIC_FLAG_INIT;
static char *g_config_file_path = NULL;
static char g_grass_prefix[GPATH_MAX] = {0};

#else
static bool g_config_initialized = false;
static char *g_config_file_path = NULL;
static char g_grass_prefix[GPATH_MAX] = {0};
#endif

static void config_init_if_needed(void)
{
#if RESOURCE_DIRS_ATOMIC
    // Fast path: already initialized
    if (atomic_load_explicit(&g_config_initialized, memory_order_acquire))
        return;

    // Acquire a simple spin lock to perform initialization once
    while (atomic_flag_test_and_set_explicit(&g_config_init_lock,
                                             memory_order_acquire)) {
        // Another thread is initializing; busy-wait until done
    }
#else
    if (g_config_initialized)
        return;
#endif

    // Double-check after acquiring the lock
    if (!g_config_initialized) {
        const char *path = get_config_file();
        if (path) {
            FILE *fp = fopen(path, "r");
            if (fp) {
                if (config_load_file(fp) == 0) {
                    g_config_file_path = G_store(path);
#if RESOURCE_DIRS_ATOMIC
                    atomic_store_explicit(&g_config_initialized, true,
                                          memory_order_release);
#else
                    g_config_initialized = true;
#endif
                }
                fclose(fp);
            }
            else {
                int err = errno;
                G_fatal_error(_("%s unable to open configure file '%s', %s."),
                              __func__, path, strerror(err));
            }

            // Set GRASS prefix path
            for (size_t i = 0; i < g_config_count; i++) {
                if (strcmp(g_config[i].key, "GRASS_PREFIX_RELATIVE") == 0) {
                    char symlinkpath[GPATH_MAX] = {0};
                    snprintf(symlinkpath, sizeof(symlinkpath), "%s%c%s",
                             dirname(g_config_file_path), HOST_DIRSEP,
                             g_config[i].value);

                    char *resolved_path = realpath(symlinkpath, NULL);
                    if (!resolved_path) {
                        int err = errno;
                        G_fatal_error(
                            _("%s nable to resolve GRASS prefix '%s', %s"),
                            __func__, symlinkpath, strerror(err));
                    }
                    if (G_strlcpy(g_grass_prefix, resolved_path, GPATH_MAX) >=
                        sizeof(g_grass_prefix)) {
                        G_fatal_error(_("%s resolved GRASS prefix '%s' is "
                                        "too long."),
                                      __func__, resolved_path);
                    }
                    G_free(resolved_path);
                    break;
                }
            }
        }
    }

#if RESOURCE_DIRS_ATOMIC
    // Release the spin lock
    atomic_flag_clear_explicit(&g_config_init_lock, memory_order_release);
#endif
}

static const char *resolve_and_set_resource_path(const char *dir_type_name)
{
    char buf[GPATH_MAX] = {0};
    const char *value = G_config_get(dir_type_name);
    if (!value || !*value)
        return NULL;

    if (snprintf(buf, GPATH_MAX, "%s%c%s", g_grass_prefix, HOST_DIRSEP,
                 value) >= GPATH_MAX) {
        G_warning("%s file name exceeds allowed length '%s'.", __func__, buf);
        return NULL;
    }
#if defined(_MSC_VER)
    _putenv_s(dir_type_name, buf);
#else
    setenv(dir_type_name, buf, 1);
#endif
    return getenv(dir_type_name);
}

static const char *get_g_env(GResourceDirType t)
{
    config_init_if_needed();

    const char *dir_type_name = g_resource_dir_type_names[t];
    const char *value = getenv(dir_type_name);
    if (value)
        return value;

    return resolve_and_set_resource_path(dir_type_name);
}
