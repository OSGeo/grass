/* Adjacency correction 500m long Int bit[31]
 * 00 -> class 1: Not Corrected product
 * 01 -> class 2: Corrected product
 */  
int qc500e(long int pixel) 
{
    long int swabfrom, swabto, qctemp;

    int class;

    swabfrom = pixel;
    swabfrom >> 31;		/* bit no 31 becomes 0 */
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


