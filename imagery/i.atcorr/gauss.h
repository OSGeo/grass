#ifndef MY_GAUSS_H
#define MY_GAUSS_H

const long int mu	= 25;
const long int np	= 49;

/* index standard interval [-1,1] */
#define STDI(X) ((X)+mu)

struct Gauss
{
private:
	static float angmu[10];
	static float angphi[13];

public:
	/* [a,b] = [0,2*Pi] */
	float rp[np];			/* gaussian angles */
	float gp[np];			/* gaussian weights */

	// [a,b] = [-1,1]
	float rm[2*mu+1];		/* shifted gaussian angles */
	float gb[2*mu+1];		/* shifted gaussian weights */
					/* with the ends zeroed as well as the center */
					/* [0 ? ? ? ? 0 ? ? ? ? 0] */

    /*  preliminary computations for gauss integration */
	void init();

	/*	Compute for a given n, the gaussian quadrature (the n gaussian angles and the
	their respective weights). The gaussian quadrature is used in numerical integration involving the
	cosine of emergent or incident direction zenith angle. */
	static void gauss (float a, float b, float *x, float *w, long int n);
};

#endif /* MY_GAUSS_H */
