/* Cloud State 250m Unsigned Int bits[2-3]
 * 00 -> class 1: Clear -- No clouds
 * 01 -> class 2: Cloudy
 * 10 -> class 3: Mixed
 * 11 -> class 4: Not Set ; Assumed Clear
 */  
int qc250b(unsigned int pixel) 
{
    unsigned int swabfrom, swabto, qctemp;

    int class;

    swabfrom = pixel;
    swabfrom >> 2;		/*bits [2-3] become [0-1] */
    swab(&swabfrom, &swabto, 4);
    qctemp = swabto;
    if (qctemp & 0x02) {	/* 1 ? */
	if (qctemp & 0x01) {
	    class = 4;		/*Not Set ; Assumed Clear */
	}
	else {
	    class = 3;		/*Mixed clouds */
	}
    }
    else {			/* 0 ? */
	if (qctemp & 0x01) {
	    class = 2;		/*Cloudy */
	}
	else {
	    class = 1;		/*Clear -- No Clouds */
	}
    }
    return class;
}


