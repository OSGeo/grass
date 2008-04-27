#ifdef MAIN
	char *mapset_name[GMAPSET_MAX];
	int nmapsets ;
	int choice[GMAPSET_MAX];
	int nchoices;
	int curr_mapset[GMAPSET_MAX];
	int ncurr_mapsets;
#else
	extern char *mapset_name[];
	extern int nmapsets;
	extern int choice[];
	extern int nchoices;
	extern int curr_mapset[];
	extern int ncurr_mapsets;
#endif

#define	REPLACE	0
#define ADD	1
#define DELETE	2
