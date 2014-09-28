#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


double *Cdhc_shapiro_wilk(double *x, int n)
{
    static double y[2];
    double a[25], s2, *xcopy;
    double sumb = 0.0, sumx = 0.0, sumx2 = 0.0;
    int i, k;

    if ((xcopy = (double *)malloc(n * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in Cdhc_shapiro_wilk\n");
	exit(EXIT_FAILURE);
    }

    k = n / 2;
    for (i = 0; i < n; ++i) {
	xcopy[i] = x[i];
	sumx += x[i];
	sumx2 += x[i] * x[i];
    }
    s2 = sumx2 - sumx * sumx / n;

    qsort(xcopy, n, sizeof(double), Cdhc_dcmp);

    if (n == 3) {
	a[0] = (double).7071;
    }
    else if (n == 4) {
	a[0] = (double).6872;
	a[1] = (double).1677;
    }
    else if (n == 5) {
	a[0] = (double).6646;
	a[1] = (double).2413;
    }
    else if (n == 6) {
	a[0] = (double).6431;
	a[1] = (double).2806;
	a[2] = (double).0875;
    }
    else if (n == 7) {
	a[0] = (double).6233;
	a[1] = (double).3031;
	a[2] = (double).1401;
    }
    else if (n == 8) {
	a[0] = (double).6052;
	a[1] = (double).3164;
	a[2] = (double).1743;
	a[3] = (double).0561;
    }
    else if (n == 9) {
	a[0] = (double).5888;
	a[1] = (double).3244;
	a[2] = (double).1976;
	a[3] = (double).0947;
    }
    else if (n == 10) {
	a[0] = (double).5739;
	a[1] = (double).3291;
	a[2] = (double).2141;
	a[3] = (double).1224;
	a[4] = (double).0399;
    }
    else if (n == 11) {
	a[0] = (double).5601;
	a[1] = (double).3315;
	a[2] = (double).226;
	a[3] = (double).1429;
	a[4] = (double).0695;
    }
    else if (n == 12) {
	a[0] = (double).5475;
	a[1] = (double).3325;
	a[2] = (double).2347;
	a[3] = (double).1586;
	a[4] = (double).0922;
	a[5] = (double).0303;
    }
    else if (n == 13) {
	a[0] = (double).5359;
	a[1] = (double).3325;
	a[2] = (double).2412;
	a[3] = (double).1707;
	a[4] = (double).1099;
	a[5] = (double).0539;
    }
    else if (n == 14) {
	a[0] = (double).5251;
	a[1] = (double).3318;
	a[2] = (double).246;
	a[3] = (double).1802;
	a[4] = (double).124;
	a[5] = (double).0727;
	a[6] = (double).024;
    }
    else if (n == 15) {
	a[0] = (double).515;
	a[1] = (double).3306;
	a[2] = (double).2495;
	a[3] = (double).1878;
	a[4] = (double).1353;
	a[5] = (double).088;
	a[6] = (double).0433;
    }
    else if (n == 16) {
	a[0] = (double).5056;
	a[1] = (double).329;
	a[2] = (double).2521;
	a[3] = (double).1939;
	a[4] = (double).1447;
	a[5] = (double).1005;
	a[6] = (double).0593;
	a[7] = (double).0196;
    }
    else if (n == 17) {
	a[0] = (double).4968;
	a[1] = (double).3273;
	a[2] = (double).254;
	a[3] = (double).1988;
	a[4] = (double).1524;
	a[5] = (double).1109;
	a[6] = (double).0725;
	a[7] = (double).0359;
    }
    else if (n == 18) {
	a[0] = (double).4886;
	a[1] = (double).3253;
	a[2] = (double).2553;
	a[3] = (double).2027;
	a[4] = (double).1587;
	a[5] = (double).1197;
	a[6] = (double).0837;
	a[7] = (double).0496;
	a[8] = (double).0163;
    }
    else if (n == 19) {
	a[0] = (double).4808;
	a[1] = (double).3232;
	a[2] = (double).2561;
	a[3] = (double).2059;
	a[4] = (double).1641;
	a[5] = (double).1271;
	a[6] = (double).0932;
	a[7] = (double).0612;
	a[8] = (double).0303;
    }
    else if (n == 20) {
	a[0] = (double).4734;
	a[1] = (double).3211;
	a[2] = (double).2565;
	a[3] = (double).2085;
	a[4] = (double).1686;
	a[5] = (double).1334;
	a[6] = (double).1013;
	a[7] = (double).0711;
	a[8] = (double).0422;
	a[9] = (double).014;
    }
    else if (n == 21) {
	a[0] = (double).4643;
	a[1] = (double).3185;
	a[2] = (double).2578;
	a[3] = (double).2119;
	a[4] = (double).1736;
	a[5] = (double).1399;
	a[6] = (double).1092;
	a[7] = (double).0804;
	a[8] = (double).053;
	a[9] = (double).0263;
    }
    else if (n == 22) {
	a[0] = (double).459;
	a[1] = (double).3156;
	a[2] = (double).2571;
	a[3] = (double).2131;
	a[4] = (double).1764;
	a[5] = (double).1443;
	a[6] = (double).115;
	a[7] = (double).0878;
	a[8] = (double).0618;
	a[9] = (double).0368;
	a[10] = (double).0122;
    }
    else if (n == 23) {
	a[0] = (double).4542;
	a[1] = (double).3126;
	a[2] = (double).2563;
	a[3] = (double).2139;
	a[4] = (double).1787;
	a[5] = (double).148;
	a[6] = (double).1201;
	a[7] = (double).0941;
	a[8] = (double).0696;
	a[9] = (double).0459;
	a[10] = (double).0228;
    }
    else if (n == 24) {
	a[0] = (double).4493;
	a[1] = (double).3098;
	a[2] = (double).2554;
	a[3] = (double).2145;
	a[4] = (double).1807;
	a[5] = (double).1512;
	a[6] = (double).1245;
	a[7] = (double).0997;
	a[8] = (double).0764;
	a[9] = (double).0539;
	a[10] = (double).0321;
	a[11] = (double).0107;
    }
    else if (n == 25) {
	a[0] = (double).445;
	a[1] = (double).3069;
	a[2] = (double).2543;
	a[3] = (double).2148;
	a[4] = (double).1822;
	a[5] = (double).1539;
	a[6] = (double).1283;
	a[7] = (double).1046;
	a[8] = (double).0823;
	a[9] = (double).061;
	a[10] = (double).0403;
	a[11] = (double).02;
    }
    else if (n == 26) {
	a[0] = (double).4407;
	a[1] = (double).3043;
	a[2] = (double).2533;
	a[3] = (double).2151;
	a[4] = (double).1836;
	a[5] = (double).1563;
	a[6] = (double).1316;
	a[7] = (double).1089;
	a[8] = (double).0876;
	a[9] = (double).0672;
	a[10] = (double).0476;
	a[11] = (double).0284;
	a[12] = (double).0094;
    }
    else if (n == 27) {
	a[0] = (double).4366;
	a[1] = (double).3018;
	a[2] = (double).2522;
	a[3] = (double).2152;
	a[4] = (double).1848;
	a[5] = (double).1584;
	a[6] = (double).1346;
	a[7] = (double).1128;
	a[8] = (double).0923;
	a[9] = (double).0728;
	a[10] = (double).054;
	a[11] = (double).0358;
	a[12] = (double).0178;
    }
    else if (n == 28) {
	a[0] = (double).4328;
	a[1] = (double).2992;
	a[2] = (double).251;
	a[3] = (double).2151;
	a[4] = (double).1857;
	a[5] = (double).1601;
	a[6] = (double).1372;
	a[7] = (double).1162;
	a[8] = (double).0965;
	a[9] = (double).0778;
	a[10] = (double).0598;
	a[11] = (double).0424;
	a[12] = (double).0253;
	a[13] = (double).0084;
    }
    else if (n == 29) {
	a[0] = (double).4291;
	a[1] = (double).2968;
	a[2] = (double).2499;
	a[3] = (double).215;
	a[4] = (double).1864;
	a[5] = (double).1616;
	a[6] = (double).1395;
	a[7] = (double).1192;
	a[8] = (double).1002;
	a[9] = (double).0822;
	a[10] = (double).065;
	a[11] = (double).0483;
	a[12] = (double).032;
	a[13] = (double).0159;
    }
    else if (n == 30) {
	a[0] = (double).4254;
	a[1] = (double).2944;
	a[2] = (double).2487;
	a[3] = (double).2148;
	a[4] = (double).187;
	a[5] = (double).163;
	a[6] = (double).1415;
	a[7] = (double).1219;
	a[8] = (double).1036;
	a[9] = (double).0862;
	a[10] = (double).0697;
	a[11] = (double).0537;
	a[12] = (double).0381;
	a[13] = (double).0227;
	a[14] = (double).0076;
    }
    else if (n == 31) {
	a[0] = (double).422;
	a[1] = (double).2921;
	a[2] = (double).2475;
	a[3] = (double).2145;
	a[4] = (double).1874;
	a[5] = (double).1641;
	a[6] = (double).1433;
	a[7] = (double).1243;
	a[8] = (double).1066;
	a[9] = (double).0899;
	a[10] = (double).0739;
	a[11] = (double).0585;
	a[12] = (double).0435;
	a[13] = (double).0289;
	a[14] = (double).0144;
    }
    else if (n == 32) {
	a[0] = (double).4188;
	a[1] = (double).2898;
	a[2] = (double).2463;
	a[3] = (double).2141;
	a[4] = (double).1878;
	a[5] = (double).1651;
	a[6] = (double).1449;
	a[7] = (double).1265;
	a[8] = (double).1093;
	a[9] = (double).0931;
	a[10] = (double).0777;
	a[11] = (double).0629;
	a[12] = (double).0485;
	a[13] = (double).0344;
	a[14] = (double).0206;
	a[15] = (double).0068;
    }
    else if (n == 33) {
	a[0] = (double).4156;
	a[1] = (double).2876;
	a[2] = (double).2451;
	a[3] = (double).2137;
	a[4] = (double).188;
	a[5] = (double).166;
	a[6] = (double).1463;
	a[7] = (double).1284;
	a[8] = (double).1118;
	a[9] = (double).0961;
	a[10] = (double).0812;
	a[11] = (double).0669;
	a[12] = (double).053;
	a[13] = (double).0395;
	a[14] = (double).0262;
	a[15] = (double).0131;
    }
    else if (n == 34) {
	a[0] = (double).4127;
	a[1] = (double).2854;
	a[2] = (double).2439;
	a[3] = (double).2132;
	a[4] = (double).1882;
	a[5] = (double).1667;
	a[6] = (double).1475;
	a[7] = (double).1301;
	a[8] = (double).114;
	a[9] = (double).0988;
	a[10] = (double).0844;
	a[11] = (double).0706;
	a[12] = (double).0572;
	a[13] = (double).0441;
	a[14] = (double).0314;
	a[15] = (double).0187;
	a[16] = (double).0062;
    }
    else if (n == 35) {
	a[0] = (double).4096;
	a[1] = (double).2834;
	a[2] = (double).2427;
	a[3] = (double).2127;
	a[4] = (double).1883;
	a[5] = (double).1673;
	a[6] = (double).1487;
	a[7] = (double).1317;
	a[8] = (double).116;
	a[9] = (double).1013;
	a[10] = (double).0873;
	a[11] = (double).0739;
	a[12] = (double).061;
	a[13] = (double).0484;
	a[14] = (double).0361;
	a[15] = (double).0239;
	a[16] = (double).0119;
    }
    else if (n == 36) {
	a[0] = (double).4068;
	a[1] = (double).2813;
	a[2] = (double).2415;
	a[3] = (double).2121;
	a[4] = (double).1883;
	a[5] = (double).1678;
	a[6] = (double).1496;
	a[7] = (double).1331;
	a[8] = (double).1179;
	a[9] = (double).1036;
	a[10] = (double).09;
	a[11] = (double).077;
	a[12] = (double).0645;
	a[13] = (double).0523;
	a[14] = (double).0404;
	a[15] = (double).0287;
	a[16] = (double).0172;
	a[17] = (double).0057;
    }
    else if (n == 37) {
	a[0] = (double).404;
	a[1] = (double).2794;
	a[2] = (double).2403;
	a[3] = (double).2116;
	a[4] = (double).1883;
	a[5] = (double).1683;
	a[6] = (double).1505;
	a[7] = (double).1344;
	a[8] = (double).1196;
	a[9] = (double).1056;
	a[10] = (double).0924;
	a[11] = (double).0798;
	a[12] = (double).0677;
	a[13] = (double).0559;
	a[14] = (double).0444;
	a[15] = (double).0331;
	a[16] = (double).022;
	a[17] = (double).011;
    }
    else if (n == 38) {
	a[0] = (double).4015;
	a[1] = (double).2774;
	a[2] = (double).2391;
	a[3] = (double).211;
	a[4] = (double).1881;
	a[5] = (double).1686;
	a[6] = (double).1513;
	a[7] = (double).1356;
	a[8] = (double).1211;
	a[9] = (double).1075;
	a[10] = (double).0947;
	a[11] = (double).0824;
	a[12] = (double).0706;
	a[13] = (double).0592;
	a[14] = (double).0481;
	a[15] = (double).0372;
	a[16] = (double).0264;
	a[17] = (double).0158;
	a[18] = (double).0053;
    }
    else if (n == 39) {
	a[0] = (double).3989;
	a[1] = (double).2755;
	a[2] = (double).238;
	a[3] = (double).2104;
	a[4] = (double).188;
	a[5] = (double).1689;
	a[6] = (double).152;
	a[7] = (double).1366;
	a[8] = (double).1225;
	a[9] = (double).1092;
	a[10] = (double).0967;
	a[11] = (double).0848;
	a[12] = (double).0733;
	a[13] = (double).0622;
	a[14] = (double).0515;
	a[15] = (double).0409;
	a[16] = (double).0305;
	a[17] = (double).0203;
	a[18] = (double).0101;
    }
    else if (n == 40) {
	a[0] = (double).3964;
	a[1] = (double).2737;
	a[2] = (double).2368;
	a[3] = (double).2098;
	a[4] = (double).1878;
	a[5] = (double).1691;
	a[6] = (double).1526;
	a[7] = (double).1376;
	a[8] = (double).1237;
	a[9] = (double).1108;
	a[10] = (double).0986;
	a[11] = (double).087;
	a[12] = (double).0759;
	a[13] = (double).0651;
	a[14] = (double).0546;
	a[15] = (double).0444;
	a[16] = (double).0343;
	a[17] = (double).0244;
	a[18] = (double).0146;
	a[19] = (double).0049;
    }
    else if (n == 41) {
	a[0] = (double).394;
	a[1] = (double).2719;
	a[2] = (double).2357;
	a[3] = (double).2091;
	a[4] = (double).1876;
	a[5] = (double).1693;
	a[6] = (double).1531;
	a[7] = (double).1384;
	a[8] = (double).1249;
	a[9] = (double).1123;
	a[10] = (double).1004;
	a[11] = (double).0891;
	a[12] = (double).0782;
	a[13] = (double).0677;
	a[14] = (double).0575;
	a[15] = (double).0476;
	a[16] = (double).0379;
	a[17] = (double).0283;
	a[18] = (double).0188;
	a[19] = (double).0094;
    }
    else if (n == 42) {
	a[0] = (double).3917;
	a[1] = (double).2701;
	a[2] = (double).2345;
	a[3] = (double).2085;
	a[4] = (double).1874;
	a[5] = (double).1694;
	a[6] = (double).1535;
	a[7] = (double).1392;
	a[8] = (double).1259;
	a[9] = (double).1136;
	a[10] = (double).102;
	a[11] = (double).0909;
	a[12] = (double).0804;
	a[13] = (double).0701;
	a[14] = (double).0602;
	a[15] = (double).0506;
	a[16] = (double).0411;
	a[17] = (double).0318;
	a[18] = (double).0227;
	a[19] = (double).0136;
	a[20] = (double).0045;
    }
    else if (n == 43) {
	a[0] = (double).3894;
	a[1] = (double).2684;
	a[2] = (double).2334;
	a[3] = (double).2078;
	a[4] = (double).1871;
	a[5] = (double).1695;
	a[6] = (double).1539;
	a[7] = (double).1398;
	a[8] = (double).1269;
	a[9] = (double).1149;
	a[10] = (double).1035;
	a[11] = (double).0927;
	a[12] = (double).0824;
	a[13] = (double).0724;
	a[14] = (double).0628;
	a[15] = (double).0534;
	a[16] = (double).0442;
	a[17] = (double).0352;
	a[18] = (double).0263;
	a[19] = (double).0175;
	a[20] = (double).0087;
    }
    else if (n == 44) {
	a[0] = (double).3872;
	a[1] = (double).2667;
	a[2] = (double).2323;
	a[3] = (double).2072;
	a[4] = (double).1868;
	a[5] = (double).1695;
	a[6] = (double).1542;
	a[7] = (double).1405;
	a[8] = (double).1278;
	a[9] = (double).116;
	a[10] = (double).1049;
	a[11] = (double).0943;
	a[12] = (double).0842;
	a[13] = (double).0745;
	a[14] = (double).0651;
	a[15] = (double).056;
	a[16] = (double).0471;
	a[17] = (double).0383;
	a[18] = (double).0296;
	a[19] = (double).0211;
	a[20] = (double).0126;
	a[21] = (double).0042;
    }
    else if (n == 45) {
	a[0] = (double).385;
	a[1] = (double).2651;
	a[2] = (double).2313;
	a[3] = (double).2065;
	a[4] = (double).1865;
	a[5] = (double).1695;
	a[6] = (double).1545;
	a[7] = (double).141;
	a[8] = (double).1286;
	a[9] = (double).117;
	a[10] = (double).1062;
	a[11] = (double).0959;
	a[12] = (double).086;
	a[13] = (double).0765;
	a[14] = (double).0673;
	a[15] = (double).0584;
	a[16] = (double).0497;
	a[17] = (double).0412;
	a[18] = (double).0328;
	a[19] = (double).0245;
	a[20] = (double).0163;
	a[21] = (double).0081;
    }
    else if (n == 46) {
	a[0] = (double).383;
	a[1] = (double).2635;
	a[2] = (double).2302;
	a[3] = (double).2058;
	a[4] = (double).1862;
	a[5] = (double).1695;
	a[6] = (double).1548;
	a[7] = (double).1415;
	a[8] = (double).1293;
	a[9] = (double).118;
	a[10] = (double).1073;
	a[11] = (double).0972;
	a[12] = (double).0876;
	a[13] = (double).0783;
	a[14] = (double).0694;
	a[15] = (double).0607;
	a[16] = (double).0522;
	a[17] = (double).0439;
	a[18] = (double).0357;
	a[19] = (double).0277;
	a[20] = (double).0197;
	a[21] = (double).0118;
	a[22] = (double).0039;
    }
    else if (n == 47) {
	a[0] = (double).3808;
	a[1] = (double).262;
	a[2] = (double).2291;
	a[3] = (double).2052;
	a[4] = (double).1859;
	a[5] = (double).1695;
	a[6] = (double).155;
	a[7] = (double).142;
	a[8] = (double).13;
	a[9] = (double).1189;
	a[10] = (double).1085;
	a[11] = (double).0986;
	a[12] = (double).0892;
	a[13] = (double).0801;
	a[14] = (double).0713;
	a[15] = (double).0628;
	a[16] = (double).0546;
	a[17] = (double).0465;
	a[18] = (double).0385;
	a[19] = (double).0307;
	a[20] = (double).0229;
	a[21] = (double).0153;
	a[22] = (double).0076;
    }
    else if (n == 48) {
	a[0] = (double).3789;
	a[1] = (double).2604;
	a[2] = (double).2281;
	a[3] = (double).2045;
	a[4] = (double).1855;
	a[5] = (double).1693;
	a[6] = (double).1551;
	a[7] = (double).1423;
	a[8] = (double).1306;
	a[9] = (double).1197;
	a[10] = (double).1095;
	a[11] = (double).0998;
	a[12] = (double).0906;
	a[13] = (double).0817;
	a[14] = (double).0731;
	a[15] = (double).0648;
	a[16] = (double).0568;
	a[17] = (double).0489;
	a[18] = (double).0411;
	a[19] = (double).0335;
	a[20] = (double).0259;
	a[21] = (double).0185;
	a[22] = (double).0111;
	a[23] = (double).0037;
    }
    else if (n == 49) {
	a[0] = (double).377;
	a[1] = (double).2589;
	a[2] = (double).2271;
	a[3] = (double).2038;
	a[4] = (double).1851;
	a[5] = (double).1692;
	a[6] = (double).1553;
	a[7] = (double).1427;
	a[8] = (double).1312;
	a[9] = (double).1205;
	a[10] = (double).1105;
	a[11] = (double).101;
	a[12] = (double).0919;
	a[13] = (double).0832;
	a[14] = (double).0748;
	a[15] = (double).0667;
	a[16] = (double).0588;
	a[17] = (double).0511;
	a[18] = (double).0436;
	a[19] = (double).0361;
	a[20] = (double).0288;
	a[21] = (double).0215;
	a[22] = (double).0143;
	a[23] = (double).0071;
    }
    else if (n == 50) {
	a[0] = (double).3751;
	a[1] = (double).2574;
	a[2] = (double).226;
	a[3] = (double).2032;
	a[4] = (double).1847;
	a[5] = (double).1691;
	a[6] = (double).1554;
	a[7] = (double).143;
	a[8] = (double).1317;
	a[9] = (double).1212;
	a[10] = (double).1113;
	a[11] = (double).102;
	a[12] = (double).0932;
	a[13] = (double).0846;
	a[14] = (double).0764;
	a[15] = (double).0685;
	a[16] = (double).0608;
	a[17] = (double).0532;
	a[18] = (double).0459;
	a[19] = (double).0386;
	a[20] = (double).0314;
	a[21] = (double).0244;
	a[22] = (double).0174;
	a[23] = (double).0104;
	a[24] = (double).0035;
    }

    if (n > 50 || n < 3) {
#ifdef NOISY
	fprintf(stdout,
		"  THIS IS THE SHAPIRO-WILK TEST FOR SMALL SAMPLES\n");
	fprintf(stdout,
		"  THE SAMPLE SIZE MUST BE LESS THAN OR EQUAL TO 50\n");
#endif /* NOISY */

	y[0] = y[1] = 0.0;
    }
    else {
	for (i = 1; i <= k; ++i)
	    sumb += a[i - 1] * (x[n - i + 1] - x[i]);

	y[0] = sumb * sumb / s2;
	y[1] = s2;

#ifdef NOISY
	fprintf(stdout, "  TEST13 SW(N)  =%10.4f\n", y[0]);
#endif /* NOISY */
    }

    free(xcopy);

    return y;
}
