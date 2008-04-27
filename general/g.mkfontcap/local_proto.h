#ifdef G_MKFONTCAP_MAIN
#  define G_MKFONTCAP_GLOBAL
#else
#  define G_MKFONTCAP_GLOBAL extern
#endif

G_MKFONTCAP_GLOBAL char **searchdirs;
G_MKFONTCAP_GLOBAL int numsearchdirs;

G_MKFONTCAP_GLOBAL struct GFONT_CAP *fontcap;
G_MKFONTCAP_GLOBAL int totalfonts;
G_MKFONTCAP_GLOBAL int maxfonts;

/* freetype_fonts.c */
void find_freetype_fonts(void);

/* stroke_fonts.c */
void find_stroke_fonts(void);
