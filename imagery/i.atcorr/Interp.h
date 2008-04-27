#ifndef INTERP_H
#define INTERP_H

struct InterpStruct
{
	float romix; 
	float rorayl; 
	float roaero;
	float phaa; 
	float phar; 
	float tsca;
	float tray; 
	float trayp; 
	float taer;
	float taerp; 
	float dtott; 
	float utott;
	float astot; 
	float asray; 
	float asaer;
	float utotr; 
	float utota; 
	float dtotr;
	float dtota;
};

/*
To estimate the different atmospheric functions r(mS,mv,fS,fv), T(q) and S at any
wavelength from the 10 discret computations (subroutine DISCOM).
 */
void interp (const int iaer, const int idatmp, 
			 const float wl, const float taer55, 
			 const float taer55p, const float xmud, 
			 InterpStruct& is);

#endif /* INTERP_H */
