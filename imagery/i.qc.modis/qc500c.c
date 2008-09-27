/* Band-wise Data Quality 500m long Int 
 * bits[2-5][6-9][10-13][14-17][18-21][22-25][26-29]
 * 0000 -> class 1: highest quality
 * 0111 -> class 2: noisy detector
 * 1000 -> class 3: dead detector; data interpolated in L1B
 * 1001 -> class 4: solar zenith >= 86 degrees
 * 1010 -> class 5: solar zenith >= 85 and < 86 degrees
 * 1011 -> class 6: missing input
 * 1100 -> class 7: internal constant used in place of climatological data for at least one atmospheric constant
 * 1101 -> class 8: correction out of bounds, pixel constrained to extreme allowable value
 * 1110 -> class 9: L1B data faulty
 * 1111 -> class 10: not processed due to deep ocean or cloud
 * Class 11: Combination of bits unused
 */  
int qc500c(long int pixel, int bandno) 
{
    long int swabfrom, swabto, qctemp;

    int class;

    swabfrom = pixel;
    swabfrom >> 2 + (4 * bandno - 1);	/* bitshift [] to [0-3] etc. */
    swab(&swabfrom, &swabto, 4);
    qctemp = swabto;
    if (qctemp & 0x08) {	/* 1??? */
	if (qctemp & 0x04) {	/* 11?? */
	    if (qctemp & 0x02) {	/* 111? */
		if (qctemp & 0x01) {	/* 1111 */
		    class = 10;	/*Not proc.ocean/cloud */
		}
		else {		/* 1110 */
		    class = 9;	/*L1B faulty data */
		}
	    }
	    else {		/* 110? */
		if (qctemp & 0x01) {	/* 1101 */
		    class = 8;	/*corr. out of bounds */
		}
		else {		/* 1100 */
		    class = 7;	/*internal constant used */
		}
	    }
	}
	else {
	    if (qctemp & 0x02) {	/* 101? */
		if (qctemp & 0x01) {	/* 1011 */
		    class = 6;	/*missing input */
		}
		else {		/* 1010 */
		    class = 5;	/*solarzen>=85&&<86 */
		}
	    }
	    else {		/* 100? */
		if (qctemp & 0x01) {	/* 1001 */
		    class = 4;	/*solar zenith angle>=86 */
		}
		else {		/* 1000 */
		    class = 3;	/*Dead detector */
		}
	    }
	}
    }
    else {			/* 0??? */
	if (qctemp & 0x04) {	/* 01?? */
	    if (qctemp & 0x02) {	/* 011? */
		if (qctemp & 0x01) {	/* 0111 */
		    class = 2;	/*Noisy detector */
		}
		else {		/* 0110 */
		    class = 11;	/*Unused */
		}
	    }
	    else {		/* 010? */
		if (qctemp & 0x01) {	/* 0101 */
		    class = 11;	/*Unused */
		}
		else {		/* 0100 */
		    class = 11;	/*Unused */
		}
	    }
	}
	else {			/* 00?? */
	    if (qctemp & 0x02) {	/* 001? */
		if (qctemp & 0x01) {	/* 0011 */
		    class = 11;	/*Unused */
		}
		else {		/* 0010 */
		    class = 11;	/*Unused */
		}
	    }
	    else {		/* 000? */
		if (qctemp & 0x01) {	/* 0001 */
		    class = 11;	/*Unused */
		}
		else {		/* 0000 */
		    class = 1;	/*Highest quality */
		}
	    }
	}
    }
    return class;
}


