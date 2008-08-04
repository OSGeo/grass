typedef signed char int8;	/* NB: non-ANSI compilers may not grok */
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;	/* sizeof (uint16) must == 2 */

#if defined(__alpha) || (defined(_MIPS_SZLONG) && _MIPS_SZLONG == 64)
typedef int int32;
typedef unsigned int uint32;	/* sizeof (uint32) must == 4 */
#else
typedef long int32;
typedef unsigned long uint32;	/* sizeof (uint32) must == 4 */
#endif

extern void TIFFSwabShort(uint16 *);
extern void TIFFSwabLong(uint32 *);
extern void TIFFSwabDouble(double *);
extern void TIFFSwabArrayOfShort(uint16 *, unsigned long);
extern void TIFFSwabArrayOfLong(uint32 *, unsigned long);
extern void TIFFSwabArrayOfDouble(double *, unsigned long);
extern void TIFFReverseBits(unsigned char *, unsigned long);
extern const unsigned char *TIFFGetBitRevTable(int);
