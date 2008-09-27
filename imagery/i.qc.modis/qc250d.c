/* Atmospheric correction 250m Unsigned Int bit[12]
 * 00 -> class 1: Not Corrected product
 * 01 -> class 2: Corrected product
 */  
int qc250d(unsigned int pixel) 
{
    unsigned int swabfrom, swabto, qctemp;

    int class;

    swabfrom = pixel;
    swabfrom >> 12;		/* bit no 12 becomes 0 */
    swab(&swabfrom, &swabto, 4);
    qctemp = swabto;
    if (qctemp & 0x01) {
	class = 2;		/*Corrected */
    }
    else {
	class = 1;		/*Not corrected */
    }
    return class;
}


