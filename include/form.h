#define F_VIEW 1
#define F_EDIT 2

#define F_HTML 1
#define F_TXT  2

#define F_DRIVER_FNAME "_grass_internal_driver_name"
#define F_DATABASE_FNAME "_grass_internal_database_name"
#define F_TABLE_FNAME "_grass_internal_table_name"
#define F_KEY_FNAME "_grass_internal_key_name"
#define F_ENCODING "_grass_internal_database_encoding"

int F_generate(char *driver, char *database, char *table, char *key,
	       int keyval, char *frmname, char *frmmapset, int edit_mode,
	       int format, char **form);
int F_open(char *title, char *html);
void F_clear(void);
void F_close(void);
