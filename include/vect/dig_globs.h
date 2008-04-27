#ifdef CONTROL
#define DIG_GLOBS_GLOBAL
#else
#define DIG_GLOBS_GLOBAL extern
#endif

DIG_GLOBS_GLOBAL int Lines_In_Memory;
DIG_GLOBS_GLOBAL char *Mem_Line_Ptr;
DIG_GLOBS_GLOBAL char *Mem_curr_position;

/* Added undef 2/13/1999 WB Hughes */
#undef DIG_GLOBS_GLOBAL
