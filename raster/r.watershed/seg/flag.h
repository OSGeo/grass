#ifndef __FLAG_H__
#define __FLAG_H__

/* a set of routines that allow the programmer to "flag" cells in a
 * raster map. A flag is of type unsigned char, i.e. 8 bits can be set. 
 *
 * int flag_set(flag, bitno)
 *     sets the flag at position bitno to one.
 *
 * int flag_unset(flag, bitno)
 *     sets the flag at position bitno to zero.
 *
 * int flag_get(flag, bitno)
 *     checks if the flag is set at position bitno.
 *
 * Examples:
 * set flag at position 0: FLAG_SET(flag, 0)
 * unset (clear) flag at position 7: FLAG_UNSET(flag, 7)
 * check flag at position 5: is_set_at_5 = FLAG_GET(flag, 5)
 */

#define FLAG_SET(flag,bitno) ((flag) |= (1 << (bitno)))

#define FLAG_UNSET(flag,bitno) ((flag) &= ~(1 << (bitno)))

#define FLAG_GET(flag,bitno) ((flag) & (1 << (bitno)))

#endif /* __FLAG_H__ */
