/* MODLAND QA Bits 250m Unsigned Int bits[0-1]
 * 00 -> class 1: Corrected product produced at ideal quality -- all bands
 * 01 -> class 2: Corrected product produced at less than idel quality -- some or all bands
 * 10 -> class 3: Corrected product NOT produced due to cloud effect -- all bands
 * 11 -> class 4: Corrected product NOT produced due to other reasons -- some or all bands mayb be fill value (Note that a value of [11] overrides a value of [01])
 */  
int qc250a(unsigned int pixel) 
{
    unsigned int swabfrom, swabto, qctemp;

    int class;

    swabfrom = pixel;
    swab(&swabfrom, &swabto, 4);
    qctemp = swabto;
    if (qctemp & 0x02) {
	
	    /* Non-Corrected product */ 
	    if (qctemp & 0x01) {
	    class = 4;		/*other reasons */
	}
	else {
	    class = 3;		/*because of clouds */
	}
    }
    else {
	
	    /* Corrected product */ 
	    if (qctemp & 0x01) {
	    class = 2;		/*less than ideal quality */
	}
	else {
	    class = 1;		/*ideal quality */
	}
    }
    return class;
}


