#include<stdio.h>
#include<math.h>
#include<stdlib.h>

/*GARI: green atmospherically resistant vegetation index */ 
double ga_ri(double redchan,double nirchan,double bluechan,double greenchan) 
{
    double result;
    {
        result = (nirchan - (greenchan - (bluechan - redchan)))/(nirchan - (greenchan + (bluechan - redchan)));
    }
    return result;
}


