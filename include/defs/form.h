#ifndef GRASS_FORMDEFS_H
#define GRASS_FORMDEFS_H

int F_generate(char *driver, char *database, char *table, char *key,
	       int keyval, char *frmname, char *frmmapset, int edit_mode,
	       int format, char **form);
int F_open(char *title, char *html);
void F_clear(void);
void F_close(void);

#endif
