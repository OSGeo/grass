/* Atmospheric correction 500m long Int bit[30]
 * 00 -> class 1: Not Corrected product
 * 01 -> class 2: Corrected product
 */  
int qc500d(long int pixel) 
{
    long int swabfrom, swabto, qctemp;

    int class;

    swabfrom = pixel;
    swabfrom >> 30;		/* bit no 30 becomes 0 */
    swab(&swabfrom, &swabto, 1);
    qctemp = swabto;
    if (qctemp & 0x01) {
	class = 2;		/*Corrected */
    }
    else {
	class = 1;		/*Not corrected */
    }
    return class;
}


