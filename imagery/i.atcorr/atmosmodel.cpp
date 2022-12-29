extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "common.h"
#include "atmosmodel.h"

void AtmosModel::tropic()
{
    static const float z1[34] =
	{ 
	    0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 
	    12.f, 13.f, 14.f, 15.f, 16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 
	    22.f, 23.f, 24.f, 25.f, 30.f, 35.f, 40.f, 45.f, 50.f, 70.f, 100.f, 99999.f
	};
	
    static const float p1[34] =
	{ 
	    1013.f, 904.f, 805.f, 715.f, 633.f, 559.f, 492.f, 432.f, 378.f, 
	    329.f, 286.f, 247.f, 213.f, 182.f, 156.f, 132.f, 111.f, 93.7f,
	    78.9f, 66.6f, 56.5f, 48.f, 40.9f, 35.f, 30.f, 25.7f, 12.2f, 6.f, 
	    3.05f, 1.59f, .854f, .0579f, 3e-4f, 0.f
	};

    static const float t1[34] =
	{ 
	    300.f, 294.f, 288.f, 284.f, 277.f, 270.f, 264.f, 257.f, 250.f, 
	    244.f, 237.f, 230.f, 224.f, 217.f, 210.f, 204.f, 197.f, 195.f,
	    199.f, 203.f, 207.f, 211.f, 215.f, 217.f, 219.f, 221.f, 232.f, 
	    243.f, 254.f, 265.f, 270.f, 219.f, 210.f, 210.f
	};

    static const float wh1[34] =
	{ 
	    19.f, 13.f, 9.3f, 4.7f, 2.2f, 1.5f, .85f, .47f, .25f, .12f, .05f, 


	    .017f, .006f, .0018f, .001f, 7.6e-4f, 6.4e-4f, 5.6e-4f, 5e-4f,
	    4.9e-4f, 4.5e-4f, 5.1e-4f, 5.1e-4f, 5.4e-4f, 6e-4f, 6.7e-4f, 
	    3.6e-4f, 1.1e-4f, 4.3e-5f, 1.9e-5f, 6.3e-6f, 1.4e-7f, 1e-9f, 0.f
	};

    static const float wo1[34] =
	{ 
	    5.6e-5f, 5.6e-5f, 5.4e-5f, 5.1e-5f, 4.7e-5f, 4.5e-5f,
	    4.3e-5f, 4.1e-5f, 3.9e-5f, 3.9e-5f, 3.9e-5f, 4.1e-5f, 4.3e-5f, 4.5e-5f,
	    4.5e-5f, 4.7e-5f, 4.7e-5f, 6.9e-5f, 9e-5f, 1.4e-4f, 1.9e-4f, 2.4e-4f,
	    2.8e-4f, 3.2e-4f, 3.4e-4f, 3.4e-4f, 2.4e-4f, 9.2e-5f, 4.1e-5f, 1.3e-5f,
	    4.3e-6f, 8.6e-8f, 4.3e-11f, 0.f
	};

    /* model: tropical mc clatchey */
    for (int i = 0; i < 34; i++)
    {
	z[i] = z1[i];
	p[i] = p1[i];
	t[i] = t1[i];
	wh[i] = wh1[i];
	wo[i] = wo1[i];
    }
}

void AtmosModel::midsum()
{
    static const float z1[34] =
	{ 
	    0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f,
	    12.f, 13.f, 14.f, 15.f, 16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
	    24.f, 25.f, 30.f, 35.f, 40.f, 45.f, 50.f, 70.f, 100.f, 99999.f
	};

    static const float p1[34] =
	{ 
	    1013.f, 902.f, 802.f, 710.f, 628.f, 554.f, 487.f, 426.f,
	    372.f, 324.f, 281.f, 243.f, 209.f, 179.f, 153.f, 130.f, 111.f, 95.f,
	    81.2f, 69.5f, 59.5f, 51.f, 43.7f, 37.6f, 32.2f, 27.7f, 13.2f, 6.52f, 
	    3.33f, 1.76f, .951f, .0671f, 3e-4f, 0.f
	};

    static const float t1[34] =
	{ 
	    294.f, 290.f, 285.f, 279.f, 273.f, 267.f, 261.f, 255.f,
	    248.f, 242.f, 235.f, 229.f, 222.f, 216.f, 216.f, 216.f, 216.f, 216.f,
	    216.f, 217.f, 218.f, 219.f, 220.f, 222.f, 223.f, 224.f, 234.f, 245.f, 258.f,
	    270.f, 276.f, 218.f, 210.f, 210.f
	};

    static const float wh1[34] =
	{ 
	    14.f, 9.3f, 5.9f, 3.3f, 1.9f, 1.f, .61f, .37f, .21f, .12f,
	    .064f, .022f, .006f, .0018f, .001f, 7.6e-4f, 6.4e-4f, 5.6e-4f, 5e-4f,
	    4.9e-4f, 4.5e-4f, 5.1e-4f, 5.1e-4f, 5.4e-4f, 6e-4f, 6.7e-4f, 3.6e-4f,
	    1.1e-4f, 4.3e-5f, 1.9e-5f, 1.3e-6f, 1.4e-7f, 1e-9f, 0.f
	};

    static const float wo1[34] =
	{ 
	    6e-5f, 6e-5f, 6e-5f, 6.2e-5f, 6.4e-5f, 6.6e-5f, 6.9e-5f,
	    7.5e-5f, 7.9e-5f, 8.6e-5f, 9e-5f, 1.1e-4f, 1.2e-4f, 1.5e-4f, 1.8e-4f,
	    1.9e-4f, 2.1e-4f, 2.4e-4f, 2.8e-4f, 3.2e-4f, 3.4e-4f, 3.6e-4f, 3.6e-4f,
	    3.4e-4f, 3.2e-4f, 3e-4f, 2e-4f, 9.2e-5f, 4.1e-5f, 1.3e-5f, 4.3e-6f,
	    8.6e-8f, 4.3e-11f, 0.f
	};

    /* model: midlatitude summer mc clatchey */
    for (int i = 0; i < 34; i++)
    {
	z[i] = z1[i];
	p[i] = p1[i];
	t[i] = t1[i];
	wh[i] = wh1[i];
	wo[i] = wo1[i];
    }
}

void AtmosModel::midwin()
{
    static const float z1[34] =
	{ 
	    0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f,
	    12.f, 13.f, 14.f, 15.f, 16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
	    24.f, 25.f, 30.f, 35.f, 40.f, 45.f, 50.f, 70.f, 100.f, 99999.f
	};

    static const float p1[34] =
	{ 
	    1018.f, 897.3f, 789.7f, 693.8f, 608.1f, 531.3f, 462.7f,
	    401.6f, 347.3f, 299.2f, 256.8f, 219.9f, 188.2f, 161.f, 137.8f, 117.8f,
	    100.7f, 86.1f, 73.5f, 62.8f, 53.7f, 45.8f, 39.1f, 33.4f, 28.6f, 24.3f,
	    11.1f, 5.18f, 2.53f, 1.29f, .682f, .0467f, 3e-4f, 0.f
	};

    static const float t1[34] =
	{ 
	    272.2f, 268.7f, 265.2f, 261.7f, 255.7f, 249.7f, 243.7f,
	    237.7f, 231.7f, 225.7f, 219.7f, 219.2f, 218.7f, 218.2f, 217.7f, 217.2f,
	    216.7f, 216.2f, 215.7f, 215.2f, 215.2f, 215.2f, 215.2f, 215.2f, 215.2f,
	    215.2f, 217.4f, 227.8f, 243.2f, 258.5f, 265.7f, 230.7f, 210.2f, 210.f
	};	

    static const float wh1[34] =
	{ 
	    3.5f, 2.5f, 1.8f, 1.2f, .66f, .38f, .21f, .085f, .035f,
	    .016f, .0075f, .0069f, .006f, .0018f, .001f, 7.6e-4f, 6.4e-4f, 5.6e-4f,
	    5e-4f, 4.9e-4f, 4.5e-4f, 5.1e-4f, 5.1e-4f, 5.4e-4f, 6e-4f, 6.7e-4f,
	    3.6e-4f, 1.1e-4f, 4.3e-5f, 1.9e-5f, 6.3e-6f, 1.4e-7f, 1e-9f, 0.f
	};

    static const float wo1[34] = 
	{ 
	    6e-5f, 5.4e-5f, 4.9e-5f, 4.9e-5f, 4.9e-5f, 5.8e-5f,
	    6.4e-5f, 7.7e-5f, 9e-5f, 1.2e-4f, 1.6e-4f, 2.1e-4f, 2.6e-4f, 3e-4f,
	    3.2e-4f, 3.4e-4f, 3.6e-4f, 3.9e-4f, 4.1e-4f, 4.3e-4f, 4.5e-4f, 4.3e-4f,
	    4.3e-4f, 3.9e-4f, 3.6e-4f, 3.4e-4f, 1.9e-4f, 9.2e-5f, 4.1e-5f, 1.3e-5f,
	    4.3e-6f, 8.6e-8f, 4.3e-11f, 0.f
	};

    /* model: midlatitude winter mc clatchey */
    for (int i = 0; i < 34; i++)
    {
	z[i] = z1[i];
	p[i] = p1[i];
	t[i] = t1[i];
	wh[i] = wh1[i];
	wo[i] = wo1[i];
    }
}

void AtmosModel::subsum()
{
    static const float z1[34] =
	{ 
	    0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f,
	    12.f, 13.f, 14.f, 15.f, 16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
	    24.f, 25.f, 30.f, 35.f, 40.f, 45.f, 50.f, 70.f, 100.f, 99999.f
	};

    static const float p1[34] =
	{ 
	    1010.f, 896.f, 792.9f, 700.f, 616.f, 541.f, 473.f, 413.f,
	    359.f, 310.7f, 267.7f, 230.f, 197.7f, 170.f, 146.f, 125.f, 108.f, 92.8f,
	    79.8f, 68.6f, 58.9f, 50.7f, 43.6f, 37.5f, 32.27f, 27.8f, 13.4f, 6.61f,
	    3.4f, 1.81f, .987f, .0707f, 3e-4f, 0.f
	};

    static const float t1[34] =
	{ 
	    287.f, 282.f, 276.f, 271.f, 266.f, 260.f, 253.f, 246.f,
	    239.f, 232.f, 225.f, 225.f, 225.f, 225.f, 225.f, 225.f, 225.f, 225.f,
	    225.f, 225.f, 225.f, 225.f, 225.f, 225.f, 226.f, 228.f, 235.f, 247.f, 262.f,
	    274.f, 277.f, 216.f, 210.f, 210.f
	};

    static const float wh1[34] =
	{ 
	    9.1f, 6.f, 4.2f, 2.7f, 1.7f, 1.f, .54f, .29f, .13f, .042f,
	    .015f, .0094f, .006f, .0018f, .001f, 7.6e-4f, 6.4e-4f, 5.6e-4f, 5e-4f,
	    4.9e-4f, 4.5e-4f, 5.1e-4f, 5.1e-4f, 5.4e-4f, 6e-4f, 6.7e-4f, 3.6e-4f,
	    1.1e-4f, 4.3e-5f, 1.9e-5f, 6.3e-6f, 1.4e-7f, 1e-9f, 0.f
	};

    static const float wo1[34] = 
	{ 
	    4.9e-5f, 5.4e-5f, 5.6e-5f, 5.8e-5f, 6e-5f, 6.4e-5f,
	    7.1e-5f, 7.5e-5f, 7.9e-5f, 1.1e-4f, 1.3e-4f, 1.8e-4f, 2.1e-4f, 2.6e-4f,
	    2.8e-4f, 3.2e-4f, 3.4e-4f, 3.9e-4f, 4.1e-4f, 4.1e-4f, 3.9e-4f, 3.6e-4f,
	    3.2e-4f, 3e-4f, 2.8e-4f, 2.6e-4f, 1.4e-4f, 9.2e-5f, 4.1e-5f, 1.3e-5f,
	    4.3e-6f, 8.6e-8f, 4.3e-11f, 0.f
	};

    /* model: subarctique summer mc clatchey */
    for (int i = 0; i < 34; i++)
    {
	z[i] = z1[i];
	p[i] = p1[i];
	t[i] = t1[i];
	wh[i] = wh1[i];
	wo[i] = wo1[i];
    }
}

void AtmosModel::subwin()
{
    static const float z1[34] =
	{ 
	    0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f,
	    12.f, 13.f, 14.f, 15.f, 16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
	    24.f, 25.f, 30.f, 35.f, 40.f, 45.f, 50.f, 70.f, 100.f, 99999.f
	};

    static const float p1[34] =
	{ 
	    1013.f, 887.8f, 777.5f, 679.8f, 593.2f, 515.8f, 446.7f,
	    385.3f, 330.8f, 282.9f, 241.8f, 206.7f, 176.6f, 151.f, 129.1f, 110.3f,
	    94.31f, 80.58f, 68.82f, 58.75f, 50.14f, 42.77f, 36.47f, 31.09f, 26.49f,
	    22.56f, 10.2f, 4.701f, 2.243f, 1.113f, .5719f, .04016f, 3e-4f, 0.f
	};

    static const float t1[34] =
	{ 
	    257.1f, 259.1f, 255.9f, 252.7f, 247.7f, 240.9f, 234.1f,
	    227.3f, 220.6f, 217.2f, 217.2f, 217.2f, 217.2f, 217.2f, 217.2f, 217.2f,
	    216.6f, 216.f, 215.4f, 214.8f, 214.1f, 213.6f, 213.f, 212.4f, 211.8f,
	    211.2f, 216.f, 222.2f, 234.7f, 247.f, 259.3f, 245.7f, 210.f, 210.f
	};

    static const float wh1[34] =
	{ 
	    1.2f, 1.2f, .94f, .68f, .41f, .2f, .098f, .054f, .011f,
	    .0084f, .0055f, .0038f, .0026f, .0018f, .001f, 7.6e-4f, 6.4e-4f, 5.6e-4f,
	    5e-4f, 4.9e-4f, 4.5e-4f, 5.1e-4f, 5.1e-4f, 5.4e-4f, 6e-4f, 6.7e-4f,
	    3.6e-4f, 1.1e-4f, 4.3e-5f, 1.9e-5f, 6.3e-6f, 1.4e-7f, 1e-9f, 0.f
	};

    static const float wo1[34] =
	{ 
	    4.1e-5f, 4.1e-5f, 4.1e-5f, 4.3e-5f, 4.5e-5f, 4.7e-5f,
	    4.9e-5f, 7.1e-5f, 9e-5f, 1.6e-4f, 2.4e-4f, 3.2e-4f, 4.3e-4f, 4.7e-4f,
	    4.9e-4f, 5.6e-4f, 6.2e-4f, 6.2e-4f, 6.2e-4f, 6e-4f, 5.6e-4f, 5.1e-4f,
	    4.7e-4f, 4.3e-4f, 3.6e-4f, 3.2e-4f, 1.5e-4f, 9.2e-5f, 4.1e-5f, 1.3e-5f,
	    4.3e-6f, 8.6e-8f, 4.3e-11f, 0.f
	};

    /* model: subarctique winter mc clatchey */
    for (int i = 0; i < 34; i++)
    {
	z[i] = z1[i];
	p[i] = p1[i];
	t[i] = t1[i];
	wh[i] = wh1[i];
	wo[i] = wo1[i];
    }
}

void AtmosModel::us62()
{
    static const float z1[34] =
	{ 
	    0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f,
	    12.f, 13.f, 14.f, 15.f, 16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
	    24.f, 25.f, 30.f, 35.f, 40.f, 45.f, 50.f, 70.f, 100.f, 99999.f
	};

    static const float p1[34] =
	{ 
	    1013.f, 898.6f, 795.f, 701.2f, 616.6f, 540.5f, 472.2f,
	    411.1f, 356.5f, 308.f, 265.f, 227.f, 194.f, 165.8f, 141.7f, 121.1f,
	    103.5f, 88.5f, 75.65f, 64.67f, 55.29f, 47.29f, 40.47f, 34.67f, 29.72f, 25.49f,
	    11.97f, 5.746f, 2.871f, 1.491f, .7978f, .0552f, 3.008e-4f, 0.f
	};

    static const float t1[34] =
	{ 
	    288.1f, 281.6f, 275.1f, 268.7f, 262.2f, 255.7f, 249.2f,
	    242.7f, 236.2f, 229.7f, 223.2f, 216.8f, 216.6f, 216.6f, 216.6f, 216.6f,
	    216.6f, 216.6f, 216.6f, 216.6f, 216.6f, 217.6f, 218.6f, 219.6f, 220.6f,
	    221.6f, 226.5f, 236.5f, 253.4f, 264.2f, 270.6f, 219.7f, 210.f, 210.f
	};

    static const float wh1[34] =
	{ 
	    5.9f, 4.2f, 2.9f, 1.8f, 1.1f, .64f, .38f, .21f, .12f,
	    .046f, .018f, .0082f, .0037f, .0018f, 8.4e-4f, 7.2e-4f, 6.1e-4f, 5.2e-4f,
	    4.4e-4f, 4.4e-4f, 4.4e-4f, 4.8e-4f, 5.2e-4f, 5.7e-4f, 6.1e-4f, 6.6e-4f,
	    3.8e-4f, 1.6e-4f, 6.7e-5f, 3.2e-5f, 1.2e-5f, 1.5e-7f, 1e-9f, 0.f
	};
	
    static const float wo1[34] = 
	{ 
	    5.4e-5f, 5.4e-5f, 5.4e-5f, 5e-5f, 4.6e-5f, 4.6e-5f,
	    4.5e-5f, 4.9e-5f, 5.2e-5f, 7.1e-5f, 9e-5f, 1.3e-4f, 1.6e-4f, 1.7e-4f,
	    1.9e-4f, 2.1e-4f, 2.4e-4f, 2.8e-4f, 3.2e-4f, 3.5e-4f, 3.8e-4f, 3.8e-4f,
	    3.9e-4f, 3.8e-4f, 3.6e-4f, 3.4e-4f, 2e-4f, 1.1e-4f, 4.9e-5f, 1.7e-5f,
	    4e-6f, 8.6e-8f, 4.3e-11f, 0.f
	};

    /* model: us standard 62 mc clatchey */
    for (int i = 0; i < 34; i++)
    {
        z[i] = z1[i];
        p[i] = p1[i];
        t[i] = t1[i];
        wh[i] = wh1[i];
        wo[i] = wo1[i];
    }
}


void AtmosModel::parse()
{
    cin >> idatm;
    cin.ignore(numeric_limits<int>::max(),'\n'); /* read the rest of the scraps, like comments */

    uw = 0.;
    uo3 = 0.;

    switch(idatm)
    {
    case 0: us62();	    break;
    case 1: tropic();	break;
    case 2: midsum();	break;
    case 3: midwin();	break; 
    case 4: subsum();	break;
    case 5: subwin();	break;
    case 6: us62();	    break;
    case 7: 
    {
	/* read input */
	for(int i = 0; i < 34; i++)
	{
	    cin >> z[i];
	    cin >> p[i];
	    cin >> t[i];
	    cin >> wh[i];
	    cin >> wo[i];
	    cin.ignore(numeric_limits<int>::max(),'\n'); /* read the rest of the scraps, like comments */
	}
	break;
    }
    case 8: 
    {
	cin >> uw;
	cin >> uo3;
	cin.ignore(numeric_limits<int>::max(),'\n'); /* read the rest of the scraps, like comments */
	us62();
	break;
    }
    default: G_warning(_("Unknown atmospheric model!"));
    }
}

/* --- atmospheric model ---- */
void AtmosModel::print()
{	
    static const string head(" atmospheric model description  ");
    static const string line(" -----------------------------  ");
    Output::Begin(); Output::Repeat(22,' '); Output::Print(head); Output::End();
    Output::Begin(); Output::Repeat(22,' '); Output::Print(line); Output::End();

    if(idatm < 7) 
    {
	static const string atmid[7] = {
	    string("no absorption computed                             "),
	    string("tropical            (uh2o=4.12g/cm2,uo3=.247cm-atm)"),
	    string("midlatitude summer  (uh2o=2.93g/cm2,uo3=.319cm-atm)"),
	    string("midlatitude winter  (uh2o=.853g/cm2,uo3=.395cm-atm)"),
	    string("subarctic  summer   (uh2o=2.10g/cm2,uo3=.480cm-atm)"),
	    string("subarctic  winter   (uh2o=.419g/cm2,uo3=.480cm-atm)"),
	    string("us  standard 1962   (uh2o=1.42g/cm2,uo3=.344cm-atm)")
	};

	Output::Begin(); 
	Output::Repeat(10,' ');
	Output::Print(" atmospheric model identity : ");
	Output::End();

	Output::Begin(); 
	Output::Repeat(15,' ');
	Output::Print(atmid[idatm]);
	Output::End();
    }
    else if(idatm == 7)
    {
	Output::Begin();
	Output::Print(" atmospheric model identity : ");
	Output::End();

	Output::Begin();
	Output::Repeat(12, ' ');
	Output::Print(" user defined atmospheric model  ");
	Output::End();

	Output::Begin();
	Output::Repeat(12, ' ');
	Output::Print("*altitude  *pressure  *temp.     *h2o dens. *o3 dens.  ");
	Output::End();

	for(int i = 0; i < 34; i++)
	{
	    Output::Begin();
	    Output::Repeat(12, ' ');
	    ostringstream s;
	    s.setf(ios::fixed, ios::floatfield);
	    s << setprecision(4);
	    s << setw(9) << z[i] << "  ";
	    s << setw(9) << p[i] << "  ";
	    s << setw(9) << t[i] << "  ";
	    s << setw(9) << wh[i] << "  ";
	    s << setw(9) << wo[i] << "  ";
	    s << ends;
	    Output::Print(s.str());
	    Output::End();
	}
    }
    else 
    {
	Output::Begin();
	Output::Repeat(10, ' ');
	Output::Print(" atmospheric model identity :  ");
	Output::End();

	Output::Begin();
	Output::Repeat(12, ' ');
	ostringstream s1;
	s1.setf(ios::fixed, ios::floatfield);
	s1 << setprecision(3);
	s1 << " user defined water content : uh2o=" << setw(9) << uw << " g/cm2 ";
	Output::Print(s1.str());
	Output::End();

	Output::Begin();
	Output::Repeat(12, ' ');
	ostringstream s2;
	s2.setf(ios::fixed, ios::floatfield);
	s2 << setprecision(3);
	s2 << " user defined ozone content : uo3 =" << setw(9) << uo3 << " cm-atm";
	Output::Print(s2.str());
	Output::End();
    }

    Output::Begin(); Output::End();
}

AtmosModel AtmosModel::Parse()
{
    AtmosModel atms;
    atms.parse();
    return atms;
}
