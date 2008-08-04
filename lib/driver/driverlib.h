
#define DEF_WIDTH  640
#define DEF_HEIGHT 480

#include "driver.h"

extern const struct driver *driver;

/* Utility Functions */

/* Font.c */
int font_is_freetype(void);

/* Text2.c */
void drawchar(double, double, double, double, unsigned char);
void soft_text_ext(int x, int, double, double, double, const char *);
void get_text_ext(int *, int *, int *, int *);
void soft_text(int, int, double, double, double, const char *);
void onechar(int, int, double, double, double, unsigned char);

/* Text3.c */
void soft_text_freetype(int, int, double, double, double, const char *);
void soft_text_ext_freetype(int, int, double, double, double, const char *);
void get_text_ext_freetype(int *, int *, int *, int *);

/* font2.c */
int font_init(const char *);
int get_char_vects(unsigned char, int *, unsigned char **, unsigned char **);

/* font_freetype.c */
int font_init_freetype(const char *, int);
int font_init_charset(const char *);
const char *font_get_freetype_name(void);
const char *font_get_charset(void);
int font_get_index(void);

/* connect_sock.c */
int get_connection_sock(int, int *, int *, int);
int prepare_connection_sock(const char *);

/* command.c */
void command_init(int, int);
int get_command(char *);
int process_command(int);

/* parse_ftcap.c */
extern int font_exists(const char *name);
extern struct GFONT_CAP *parse_freetypecap(void);
extern void free_freetypecap(struct GFONT_CAP *ftcap);
extern void free_font_list(char **fonts, int num_fonts);
