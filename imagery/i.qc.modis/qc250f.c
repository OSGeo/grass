/* Different orbit from 500m product, 250m Unsigned Int bit[14]
 * 00 -> class 1: same orbit as 500m
 * 01 -> class 2: different orbit from 500m
 */  
int qc250f(unsigned int pixel) 
{
    unsigned int swabfrom, swabto, qctemp;

    int class;

    swabfrom = pixel;
    swabfrom >> 14;		/* bit no 14 becomes 0 */
    swab(&swabfrom, &swabto, 4);
    qctemp = swabto;
    if (qctemp & 0x01) {
	class = 2;		/*different orbit */
    }
    else {
	class = 1;		/*same orbit */
    }
    return class;
}


