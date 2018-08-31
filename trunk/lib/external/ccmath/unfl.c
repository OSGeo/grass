/*  unfl.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
static unsigned int a=69069U,c=244045795U;
static unsigned int s,h,sbuf[256];
double unfl()
{ int i;
  i=(int)(s>>24); s=sbuf[i];
  h=a*h+c; sbuf[i]=h;
  return s*2.328306436538696e-10;
}
void setunfl(unsigned int k)
{ int j;
  for(h=k,j=0; j<=256 ;++j){
    h=a*h+c;
    if(j<256) sbuf[j]=h; else s=h;
   }
}
