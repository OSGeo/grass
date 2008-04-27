/* quiet mode returns:   0 - open OK */
/*                      -1 - driver locked */
/*                      -2 - driver not running */
/*                      -3 - can't open fifos */
/*                      -4 - no such monitor */
/*                      -5 - GIS_LOCK undefined in shell environment */
/*                      -6 - couldn't read, write or create lock file */

#define OK       0
#define LOCKED  -1
#define NO_RUN  -2
#define NO_OPEN -3
#define NO_MON  -4
#define NO_KEY  -5
#define LOCK_FAILED -6

