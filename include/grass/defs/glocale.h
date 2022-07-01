#ifndef GRASS_GLOCALEDEFS_H
#define GRASS_GLOCALEDEFS_H

extern void G_init_locale(void);
extern char *G_gettext(const char *, const char *) __attribute__((format_arg (2)));
extern char *G_ngettext(const char *, const char *, const char *, unsigned long int) __attribute__((format_arg (2), format_arg (3)));

#endif
