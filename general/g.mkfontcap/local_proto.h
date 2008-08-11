extern char **searchdirs;
extern int numsearchdirs;

extern struct GFONT_CAP *fontcap;
extern int totalfonts;
extern int maxfonts;

/* freetype_fonts.c */
void find_freetype_fonts(void);

/* stroke_fonts.c */
void find_stroke_fonts(void);
