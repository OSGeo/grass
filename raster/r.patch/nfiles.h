/* The number of raster maps that can be patched together.
 *
 * All raster maps will be opened at one time, so this number can not
 * be arbitrarily large.
 *
 * Must be smaller than MAXFILES as defined in lib/gis/G.h which 
 * in turn must be smaller than the operating system's limit.
 *  (Given by `cat /proc/sys/fs/file-max` in Linux 2.4)
 */

#define MAXFILES 200
