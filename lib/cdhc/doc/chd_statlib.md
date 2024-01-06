# CDH statlib Source

Released in 1994 by Paul Johnson.

Retrieved from the Internet Archive for
[http://sunsite.univie.ac.at/statlib/general/cdh](https://web.archive.org/web/20001006232323/http://sunsite.univie.ac.at/statlib/general/cdh)
-- 2000-10-06

<!-- markdownlint-disable -->
```txt
          TESTS OF COMPOSITE DISTRIBUTIONAL HYPOTHESES FOR
          THE ANALYSIS OF BIOLOGICAL & ENVIRONMENTAL DATA

This library contains FORTRAN subroutines for testing the hypothesis
of normality or the hypothesis of exponentiality.  Lognormality can be
tested by carrying out the tests of normality on the log transformed
data.  Some tests are general goodness-of-fit tests that allow any
distribution to be tested simply by basing the distributional
components of the test statistic on the hypothesized distribution.  The
set of subroutines consist of 24 separate tests.  All tests or a subset
of tests can be called from the main program.   The following step may be
used to access the subroutines.  Create a FORTRAN main program
including one or more call statements,such as:

        CALL TEST#(X,Y,N)

        where # is the test number (see Table)
                  X is the input vector of length N = n sample points
                  Y is the output vector - test statistic(s), df's
                  N is an integer value,number of sample points.


  TEST                                                                                                             TEST
   #               TEST NAME                                                                           OUTPUT
   1     Omnibus Moments Test for Normality
   2     Geary's Test of Normality
   3     Studentized Range for Testing Normality
   4     D'Agostino's D-Statistic Test of Normality
   5     Kuiper V-Statistic Modified to Test Normality
   6     Watson U^2-Statistic Modified to Test Normality
   7     Durbin's Exact Test (Normal Distribution
   8     Anderson-Darling Statistic Modified to Test Normality
   9     Cramer-Von Mises W^2-Statistic to Test Normality               *
   10    Kolmogorov-Smirnov D-Statistic to Test Normality               *
   11    Kolmogorov-Smirnov D-Statistic ( Lilliefors Critical Values)
   12    Chi-Square Test of Normality (Equal Probability Classes)
   13    Shapiro-Wilk W Test of Normality for Small Samples
   14    Shapiro-Francia W' Test of Normality for Large Samples
   15    Shapiro-Wilk W Test of Exponentiality
   16    Cramer-Von Mises W^2-Statistic to Test Exponentiality          *
   17    Kolmogorov-Smirnov D-Statistic to Test Exponentiality          *
   18    Kuiper V-Statistic Modified to Test Exponentiality
   19    Watson U^2-Statistic Modified to Test Exponentiality
   20    Anderson-Darling Statistic Modified to Test Exponentiality
   21    Chi-Square Test for Exponentiality(with E.P.C.)
   22    Modified Maximum Likelihood Ratio Test                         **
   23    Coefficient of Variation Test
   24    Kotz Separate-Families Test for Lognormality vs. Normality

*  indicates the test statistic is modified
** indicates the test is for normality vs. lognormality
E.P.C. --- Equal Probability Classes

   These subroutines can be freely distributed and can be freely used for
   non-commercial purposes.
```
```fortran

C
C  FORTRAN SUBROUTINES TO TEST FOR NORMALITY,LOGNORMALITY,EXPONENTIALITY AND
C  GENERAL GOODNESS-OF-FIT TESTS THAT ALLOW ANY DISTRIBUTION TO BE TESTED
C****************************************************************************
C
C       TESTS OF COMPOSITE DISTRIBUTIONAL HYPOTHESES
C       --------------------------------------------
C
C  INPUT:  X [The vector of observed values]
C          N [The number of input observations]
C
C  OUTPUT: Y [The Composite Distributional Test Statistics]
C
C  SUBROUTINES:
C  ------------
C
C   ID               TEST NAME
C
C  TEST1  Omnibus Moments Test for Normality
C  TEST2  Geary's Test of Normality
C  TEST3  Studentized Range for Testing Normality
C  TEST4  D'Agostino's D-Statistic Test of Normality
C  TEST5  Kuiper V-Statistic Modified to Test Normality
C  TEST6  Watson U^2-Statistic Modified to Test Normality
C  TEST7  Durbin's Exact Test (Normal Distribution,Simple Hypothesis)
C  TEST8  Anderson-Darling Statistic Modified to Test Normality
C  TEST9  Cramer-Von Mises W^2-Statistic Modified to Test Normality
C  TEST10 Kolmogorov-Smirnov D-Statistic Modified to Test Normality
C  TEST11 Kolmogorov-Smirnov D-Statistic with Lilliefors Critical Values
C  TEST12 Chi-Square Test of Normality (with Equal Probability Classes)
C  TEST13 Shapiro-Wilk W Test of Normality for Small Samples
C  TEST14 Shapiro-Francia W' Test of Normality for Large Samples
C  TEST15 Shapiro-Wilk W Test of Exponentiality
C  TEST16 Cramer-Von Mises W^2-Statistic Modified to Test Exponentiality
C  TEST17 Kolmogorov-Smirnov D-Statistic Modified to Test Exponentiality
C  TEST18 Kuiper V-Statistic Modified to Test Exponentiality
C  TEST19 Watson U^2-Statistic Modified to Test Exponentiality
C  TEST20 Anderson-Darling Statistic Modified to Test Exponentiality
C  TEST21 Chi-Square Test of Exponentiality (with Equal Probability Classes)
C  TEST22 Modified Maximum Likelihood Ratio Test for Normality vs. Lognormality
C  TEST23 Coefficient of Variation Test
C  TEST24 Kotz Separate-Families Test for Lognormality vs. Normality
C
C
C    USAGE:    CALL TEST#(X,Y,N)   with # = 1,2,3...........,24
C
C    EXAMPLE:  CALL TEST20(X,Y,10) for an input vector X consisting of
C              10 observations results in the output vector Y where
C              Y(1) = AD(E).[The Shapiro-Wilk W Test of Exponentiality].
C
C    REFERENCES:
C
C    Anderson ,T.W. and D.A. Darling.1954.A Test of Goodness of Fit.
C    JASA 49:765-69.
C    D'Agostino,R.B. and E.S. Pearson.1973.Tests for Departure from Normality.
C    Biometrika 60(3):613-22.
C    D'Agostino,R.B. and B. Rosman.1974.The Power of Geary's Test of
C    Normality.Biometrika 61(1):181-84.
C    Durbin,J.1961.Some Methods of Constructing Exact Tests.
C    Biometrika 48(1&2):41-55.
C    Durbin,J.1973.Distribution Theory Based on the Sample Distribution
C    Function.SIAM.Philadelphia.
C    Geary,R.C.1947.Testing for Normality.Biometrika 36:68-97.
C    Kotz,S. 1973. Normality vs. Lognormality with Applications.
C    Communications in Statistics 1(2):113-32.
C    Lehmann,E.L.1986.Testing Statistical Hypotheses.John Wiley & Sons.
C    New York.
C    Linnet,K.1988.Testing Normality of Transformed Data.
C    Applied Statistics 32(2):180-186.
C    SAS [Statistical Analysis System] User's Guide:Basics.Version 5.1985.
C    SAS User's Guide:Statistics.Version 6.Volumes 1 and 2.1993.
C    Shapiro,S.S. and R.S.Francia.1972.An Approximate Analysis of Variance
C    Test for Normality.JASA 67(337):215-216.
C    Shapiro,S.S.,M.B.Wilk and H.J.Chen.1968.A Comparative Study of Various
C    Tests for Normality.JASA 63:1343-72.
C    Weiss,M.S. 1978.Modification of the Kolmogorov-Smirnov Statistic for Use
C    with Correlated Data.JASA 73(364):872-75.
C
C
C****************************************************************************
C
C    Note if the sample size N >= 150 then increase the DIMENSION statement
C    for the TESTS of interest in each subroutine.
C    The subroutines can be run as a group or individually e.g., as a group:
C    suppose TESTS 4,12 and 21 are required and the input vector consists of
C    20 observations; then in the MAIN program write:
C
C                    CALL TEST4(X,Y,20)
C                    CALL TEST12(X,Y,20)
C                    CALL TEST21(X,Y,20)
C
C****************************************************************************
C
C
C****************************************************************************
C
C    Subroutines By:        Paul Johnson
C                           1420 Lake Blvd #29
C                           Davis,California 95616
C                           EZ006244@ALCOR.UCDAVIS.EDU
C
C    These subroutines can be freely used for non-commercial purposes and can
C    be freely distributed.
C
C                                        Copyright   1994, Paul Johnson
C
C****************************************************************************
C
C
      SUBROUTINE TEST1(X,Y,N)
      DIMENSION X(150),Y(2),Z(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)-MEAN)**3
      DO 30 I = 1,N
   30 Z(I)=(X(I)-MEAN)**2
      DO 40 I = 1,N
   40 SUM3=SUM3+Z(I)
      SUM5=SUM3
      SUM3=SUM3**1.5
      TSSM=(N**0.5)*SUM2/SUM3
      DO 50 I = 1,N
   50 SUM4=SUM4+(X(I)-MEAN)**4
      FSSM=(N*SUM4)/(SUM5*SUM5)
      Y(1) = TSSM
      Y(2) = FSSM
      WRITE(6,300)
  300 FORMAT(1X,' ')
      WRITE(6,301)
  301 FORMAT(10X,'TESTS OF COMPOSITE DISTRIBUTIONAL HYPOTHESES')
      WRITE(6,302)
  302 FORMAT(1X,' ')

      WRITE(6,100) Y(1),Y(2)
  100 FORMAT(/,2X,'TEST1  TSM    =',F10.4,'   FSM    =',F10.4)
      RETURN
      END
      SUBROUTINE TEST2(X,Y,N)
      DIMENSION X(150),Y(2),Z(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      DO 10 I = 1,N
   10 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      DO 20 I = 1,N
   20 Z(I)=ABS(X(I)-MEAN)
      DO 30 I = 1,N
   30 SUM2=SUM2+Z(I)
      DO 40 I = 1,N
   40 SUM3=SUM3+(X(I)-MEAN)**2
      S=N*SUM3
      S1=SQRT(S)
      Y(1)=SUM2/S1
      Y(2)=(Y(1)-0.7979)*SQRT(N*1.0)/0.2123
      WRITE(6,100) Y(1),Y(2)
  100 FORMAT(/,2X,'TEST2  GTN    =',F10.4,'   Z(GTN) =',F10.4)
      RETURN
      END
      SUBROUTINE TEST3(X,Y,N)
      DIMENSION X(150),Y(2),X1(150)
      DO 10 I=1,N
   10 X1(I)=X(I)
      CALL SORT(N,X)
      XS=X(1)
      XM=X(N)
      DO 15 I = 1,N
   15 IF (XS .GE. X(I)) XS=X(I)
      DO 20 I = 1,N
   20 IF (XM .LE. X(I)) XM=X(I)
      SUM1=0
      SUM2=0
      DO 30 I = 1,N
      SUM1=SUM1+X(I)
      SUM2=SUM2+(X(I)*X(I))
   30 CONTINUE
      XBAR=SUM1/N
      S1 =SUM2-((SUM1*SUM1)/N)
      S2=S1/(N-1)
      S3=SQRT(S2)
      Y(1)=(XM-XS)/S3
      DO 40 I=1,N
   40 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST3  U      =',F10.4)
      RETURN
      END
      SUBROUTINE TEST4(A,B,K)
      DIMENSION A(150),B(2),A1(150)
      REAL M2
      REAL MN
      S1=0
      T=0
      MN=0
      E=1
      DO 10 I=1,K
   10 A1(I) = A(I)
      CALL SORT(K,A1)
      DO 20 I =1,K
      T = T + (I - .5*(K+1))*A1(I)
   20 CONTINUE
      DO 30 I = 1,K
      MN=MN+A1(I)
   30 CONTINUE
      M2=MN/K
      DO 40 I=1,K
      S1=S1+(A1(I)-M2)**2
   40 CONTINUE
      S2=S1/K
      S =SQRT(S2)
      D = T/(K**2*S)
      B(1)=(D-1.0/(2*SQRT(3.141592654)))*SQRT(K*1.0)/0.02998598
      WRITE(6,100) B(1)
  100 FORMAT(/,2X,'TEST4  DAGN   =',F10.4)
      RETURN
      END
      SUBROUTINE TEST5(X,Y,N)
      DIMENSION X(150),Y(2),FX(150),FN1(150),FN2(150)
      DIMENSION T(150),Z(150),D(2),FN3(150),X1(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      X(I)=(X(I)-XBAR)/SDX
      FN1(I)=FLOAT(I)/R
      FN2(I)=FLOAT((2*I)-1)/FLOAT(2*N)
      FN3(I)=FLOAT(I)/R
      FX(I)=.5+(ENORMP(X(I)/SQRT(2.0))/2.0)
      IF (FX(I) .LE. 0.0) FX(I) = 0.00001
   30 IF (FX(I) .GE. 1.0) FX(I) = 0.99999
      DO 40 J=1,N
   40 Z(J) = (FN3(J) - FX(J))
      CALL SORT(N,Z)
      D1=Z(N)
      DO 50 J=1,N
   50 T(J) = FX(J)-((J-1)/R)
      CALL SORT(N,T)
      D2=T(N)
      D(1)=D1
      D(2)=D2
      CALL SORT(2,D)
      V=D(1)+D(2)
      V=V*(SQRT(R)+(.82/SQRT(R))+0.05)
      Y(1)=V
      DO 60 I=1,N
   60 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST5  KV(N)  =',F10.4)
      RETURN
      END
      SUBROUTINE TEST6(X,Y,N)
      DIMENSION X(150),Y(2),FX(150),FN1(150),FN2(150)
      DIMENSION X1(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      X(I)=(X(I)-XBAR)/SDX
      FN1(I)=FLOAT(I)/R
      FN2(I)=FLOAT((2*I)-1)/FLOAT(2*N)
      FX(I)=.5+(ENORMP(X(I)/SQRT(2.0))/2.0)
      IF (FX(I) .LE. 0.0) FX(I) = 0.00001
   30 IF (FX(I) .GE. 1.0) FX(I) = 0.99999
      DO 40 I=1,N
      A=((2*I)-1)*ALOG(FX(I))
      B=((2*I)-1)*ALOG(1.0-FX(N+1-I))
      SUM3=SUM3+A+B
      SUM4=SUM4+((-FN2(I)+FX(I))**2)
      FN1(I)=ABS(FN1(I)-FX(I))
   40 CONTINUE
      CVM=1./FLOAT(12*N)+SUM4
      DO 50 I=1,N
      SUM5=SUM5+FX(I)
   50 CONTINUE
      ZBAR=SUM5/R
      W=CVM-R*(ZBAR-0.5)*(ZBAR-0.5)
      W=W*(1.0+0.5/R)
      Y(1)=W
      DO 60 I=1,N
   60 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST6  WU2(N) =',F10.4)
      RETURN
      END
      SUBROUTINE TEST7(X,Y,N)
      DIMENSION X(150),C(150),X1(150),G(150),Z(150),B(150),Y(2)
      R=FLOAT(N)
      SUMX=0
      SUMX2=0
      DO 10 I=1,N
      SUMX=SUMX+X(I)
      SUMX2=SUMX2+X(I)**2
   10 X1(I)=X(I)
      S2=(SUMX2-SUMX**2/N)/(N-1)
      DO 15 I=1,N
      X(I)=(X(I)-SUMX/N)/SQRT(S2)
      B(I)=0.5+(ENORMP(X(I)/SQRT(2.0))/2.0)
   15 CONTINUE
      CALL SORT(N,B)
      DO 20 I=2,N
      C(I)=B(I)-B(I-1)
   20 CONTINUE
      C(1)=B(1)
      C(N+1)=1-B(N)
      CALL SORT(N+1,C)
      DO 30 J=2,N
      G(J)=(N+2-J)*(C(J)-C(J-1))
   30 CONTINUE
      G(1)=(N+1)*C(1)
      G(N+1)=C(N+1)-C(N)
      DO 60 I=1,N
      SUM1=0
      DO 50 J=1,I
      SUM1=SUM1+G(J)
   50 CONTINUE
      Z(I)=(I/R)-SUM1
   60 CONTINUE
      CALL SORT(N,Z)
      R=Z(N)
      Y(1)=R
      DO 70 I=1,N
   70 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST7  DRB(N) =',F10.4)
      RETURN
      END
      SUBROUTINE TEST8(X,Y,N)
      DIMENSION X(150),Y(2),FX(150),FN1(150),FN2(150)
      DIMENSION X1(150)
      EXTERNAL enormp
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      X(I)=(X(I)-XBAR)/SDX
      FN1(I)=FLOAT(I)/R
      FN2(I)=FLOAT((2*I)-1)/FLOAT(2*N)
      FX(I)=.5+(enormp(X(I)/SQRT(2.0))/2.0)
      IF (FX(I) .LE. 0.0) FX(I) = 0.00001
   30 IF (FX(I) .GE. 1.0) FX(I) = 0.99999
      DO 40 I=1,N
      A=((2*I)-1)*ALOG(FX(I))
      B=((2*I)-1)*ALOG(1.0-FX(N+1-I))
      SUM3=SUM3+A+B
      SUM4=SUM4+((-FN2(I)+FX(I))**2)
      FN1(I)=ABS(FN1(I)-FX(I))
   40 CONTINUE
      ADB=SUM3/R
      ADSTAT=-R-ADB
      ADMOD=ADSTAT*(1.0+(.75/R)+(2.25/FLOAT(N**2)))
      Y(1)=ADMOD
      DO 50 I=1,N
   50 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST8  AD(N)  =',F10.4)
      RETURN
      END
      SUBROUTINE TEST9(X,Y,N)
      DIMENSION X(150),Y(2),FX(150),FN1(150),FN2(150)
      DIMENSION X1(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      X(I)=(X(I)-XBAR)/SDX
      FN1(I)=FLOAT(I)/R
      FN2(I)=FLOAT((2*I)-1)/FLOAT(2*N)
      FX(I)=.5+(ENORMP(X(I)/SQRT(2.0))/2.0)
      IF (FX(I) .LE. 0.0) FX(I) = 0.00001
   30 IF (FX(I) .GE. 1.0) FX(I) = 0.99999
      DO 40 I=1,N
      A=((2*I)-1)*ALOG(FX(I))
      B=((2*I)-1)*ALOG(1.0-FX(N+1-I))
      SUM3=SUM3+A+B
      SUM4=SUM4+((-FN2(I)+FX(I))**2)
      FN1(I)=ABS(FN1(I)-FX(I))
   40 CONTINUE
      CVM=1./FLOAT(12*N)+SUM4
      CVMOD=CVM*(1.+(.5/R))
      Y(1)=CVMOD
      DO 50 I=1,N
   50 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST9  CVM(N) =',F10.4)
      RETURN
      END
      SUBROUTINE TEST10(X,Y,N)
      DIMENSION X(150),Y(2),FX(150),FN1(150),FN2(150)
      DIMENSION T(150),Z(150),D(2),FN3(150),X1(150)
      REAL MEAN
      REAL KS
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      X(I)=(X(I)-XBAR)/SDX
      FN1(I)=FLOAT(I)/R
      FN2(I)=FLOAT((2*I)-1)/FLOAT(2*N)
      FN3(I)=FLOAT(I)/R
      FX(I)=.5+(ENORMP(X(I)/SQRT(2.0))/2.0)
      IF (FX(I) .LE. 0.0) FX(I) = 0.00001
   30 IF (FX(I) .GE. 1.0) FX(I) = 0.99999
      DO 40 J=1,N
   40 Z(J)=(FN3(J)-FX(J))
      CALL SORT(N,Z)
      D1=Z(N)
      DO 50 J=1,N
   50 T(J)=FX(J)-((J-1)/R)
      CALL SORT(N,T)
      D2=T(N)
      D(1)=D1
      D(2)=D2
      CALL SORT(2,D)
      DMAX=D(2)
      KS=DMAX*(SQRT(R)+(.85/SQRT(R))-.01)
      Y(1)=KS
      DO 60 I=1,N
   60 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST10 KSD(N) =',F10.4)
      RETURN
      END
      SUBROUTINE TEST11(X,Y,N)
      DIMENSION X(150),Y(2),FX(150),FN1(150),FN2(150)
      DIMENSION T(150),Z(150),D(2),FN3(150),X1(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      X(I)=(X(I)-XBAR)/SDX
      FN1(I)=FLOAT(I)/R
      FN2(I)=FLOAT((2*I)-1)/FLOAT(2*N)
      FN3(I)=FLOAT(I)/R
      FX(I)=.5+(ENORMP(X(I)/SQRT(2.0))/2.0)
      IF (FX(I) .LE. 0.0) FX(I) = 0.00001
   30 IF (FX(I) .GE. 1.0) FX(I) = 0.99999
      DO 40 J=1,N
   40 Z(J)=(FN3(J)-FX(J))
      CALL SORT(N,Z)
      D1=Z(N)
      DO 50 J=1,N
   50 T(J)=FX(J)-((J-1)/R)
      CALL SORT(N,T)
      D2=T(N)
      D(1)=D1
      D(2)=D2
      CALL SORT(2,D)
      Y(1)=D(2)
      DO 60 I=1,N
   60 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST11 KSD    =',F10.4)
      RETURN
      END
      SUBROUTINE TEST12(X,Y,N)
      DIMENSION X(150),Y(2),V(150),V2(150),F(150)
      DIMENSION P(150),Z(150)
      EXTERNAL xinormal
      EXTERNAL dl
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      A=4*((0.75*(N-1)*(N-1))**0.2)
      K1=A
      C=A-K1
      IF(C .GT. 0.5) K1=K1+1
   10 R=N/K1
      IF(R .LT. 5) K1=K1-1
      IF(R .LT.5) GOTO 10
      K2=K1-1
      DO 15 I=1,K1
   15 F(I)=0
      DO 20 I=1,N
   20 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      DO 25 I = 1,N
   25 SUM2=SUM2+(X(I)-MEAN)*(X(I)-MEAN)
      S1=SQRT(SUM2/(N-1))
      DO 30 I=1,K2
   30 P(I)=FLOAT(I)/K1
      DO 40 I=1,K2
   40 Z(I)=XINORMAL(P(I))
      DO 50 I=1,K2
   50 V(I)=MEAN+(Z(I)*S1)
      DO 51 I=1,K2
   51 V2(I+1)=V(I)+0.0001
      DO 55 I=1,N
      DO 45 J=2,K2
      IF(X(I) .GE. V2(J) .AND. X(I) .LE. V(J)) F(J)=1+F(J)
   45 CONTINUE
      IF(X(I) .GE. V2(K2+1)) F(K2+1)=F(K2+1)+1
      IF(X(I) .LE. V(1)) F(1)=F(1)+1
   55 CONTINUE
      DO 65 I=1,K1
   65 SUM3=SUM3+F(I)*F(I)
      Y(1)=SUM3*K1/N-N
      Y(2)=FLOAT(K1)-3
      WRITE(6,100) Y(1),Y(2)
  100 FORMAT(/,2X,'TEST12 CS(N)  =',F10.4,'   DOF  =',F10.4)
      RETURN
      END
      SUBROUTINE TEST13(X,Y,N)
      DIMENSION X(150),X1(150),Y(2),A(25)
      SUMB=0
      SUMX=0
      SUMX2=0
      K=N/2
      IF (N .GT. 50) GO TO 115
      DO 15 I =1,N
      SUMX=SUMX+X(I)
      SUMX2=SUMX2+X(I)**2
      A(I) = 0.0
   15 X1(I)=X(I)
      S2=SUMX2-SUMX**2/N
      CALL SORT(N,X)
      IF (N .GE. 20) GO TO 53
      IF (N .GE. 10) GO TO 33
      IF (N .NE. 3) GO TO  21
      A(1)=0.7071
      GO TO 115
   21 IF (N .NE. 4) GO TO 23
      A(1)=0.6872
      A(2)=0.1677
      GO TO 115
   23 IF (N .NE. 5) GO TO 25
      A(1)=0.6646
      A(2)=0.2413
      GO TO 115
   25 IF (N .NE. 6) GO TO 27
      A(1)=0.6431
      A(2)=0.2806
      A(3)=0.0875
      GO TO 115
   27 IF (N .NE. 7) GO TO 29
      A(1)=0.6233
      A(2)=0.3031
      A(3)=0.1401
      GO TO 115
   29 IF (N .NE. 8) GO TO 31
      A(1)=0.6052
      A(2)=0.3164
      A(3)=0.1743
      A(4)=0.0561
      GO TO 115
   31 IF (N .NE. 9) GO TO 33
      A(1)=0.5888
      A(2)=0.3244
      A(3)=0.1976
      A(4)=0.0947
      GO TO 115
   33 IF (N .NE. 10) GO TO 35
      A(1)=0.5739
      A(2)=0.3291
      A(3)=0.2141
      A(4)=0.1224
      A(5)=0.0399
      GO TO 115
   35 IF (N .NE. 11) GO TO 37
      A(1)=0.5601
      A(2)=0.3315
      A(3)=0.2260
      A(4)=0.1429
      A(5)=0.0695
      GO TO 115
   37 IF (N .NE. 12) GO TO 39
      A(1)=0.5475
      A(2)=0.3325
      A(3)=0.2347
      A(4)=0.1586
      A(5)=0.0922
      A(6)=0.0303
      GO TO 115
   39 IF (N .NE. 13) GO TO 41
      A(1)=0.5359
      A(2)=0.3325
      A(3)=0.2412
      A(4)=0.1707
      A(5)=0.1099
      A(6)=0.0539
      GO TO 115
   41 IF (N .NE. 14) GO TO 43
      A(1)=0.5251
      A(2)=0.3318
      A(3)=0.2460
      A(4)=0.1802
      A(5)=0.1240
      A(6)=0.0727
      A(7)=0.0240
      GO TO 115
   43 IF (N .NE. 15) GO TO 45
      A(1)=0.5150
      A(2)=0.3306
      A(3)=0.2495
      A(4)=0.1878
      A(5)=0.1353
      A(6)=0.0880
      A(7)=0.0433
      GO TO 115
   45 IF (N .NE. 16) GO TO 47
      A(1)=0.5056
      A(2)=0.3290
      A(3)=0.2521
      A(4)=0.1939
      A(5)=0.1447
      A(6)=0.1005
      A(7)=0.0593
      A(8)=0.0196
      GO TO 115
   47 IF (N .NE. 17) GO TO 49
      A(1)=0.4968
      A(2)=0.3273
      A(3)=0.2540
      A(4)=0.1988
      A(5)=0.1524
      A(6)=0.1109
      A(7)=0.0725
      A(8)=0.0359
      GO TO 115
   49 IF (N .NE. 18) GO TO 51
      A(1)=0.4886
      A(2)=0.3253
      A(3)=0.2553
      A(4)=0.2027
      A(5)=0.1587
      A(6)=0.1197
      A(7)=0.0837
      A(8)=0.0496
      A(9)=0.0163
      GO TO 115
   51 IF (N .NE. 19) GO TO 53
      A(1)  = 0.4808
      A(2)  = 0.3232
      A(3)  = 0.2561
      A(4)  = 0.2059
      A(5)  = 0.1641
      A(6)  = 0.1271
      A(7)  = 0.0932
      A(8)  = 0.0612
      A(9)  = 0.0303
      GO TO 115
   53 IF (N .NE. 20) GO TO 55
      A(1)  = 0.4734
      A(2)  = 0.3211
      A(3)  = 0.2565
      A(4)  = 0.2085
      A(5)  = 0.1686
      A(6)  = 0.1334
      A(7)  = 0.1013
      A(8)  = 0.0711
      A(9)  = 0.0422
      A(10) = 0.0140
      GO TO 115
   55 IF (N .NE. 21) GO TO 57
      A(1)  = 0.4643
      A(2)  = 0.3185
      A(3)  = 0.2578
      A(4)  = 0.2119
      A(5)  = 0.1736
      A(6)  = 0.1399
      A(7)  = 0.1092
      A(8)  = 0.0804
      A(9)  = 0.0530
      A(10) = 0.0263
      GO TO 115
   57 IF (N .NE. 22) GO TO 59
      A(1)  = 0.4590
      A(2)  = 0.3156
      A(3)  = 0.2571
      A(4)  = 0.2131
      A(5)  = 0.1764
      A(6)  = 0.1443
      A(7)  = 0.1150
      A(8)  = 0.0878
      A(9)  = 0.0618
      A(10) = 0.0368
      A(11) = 0.0122
      GO TO 115
   59 IF (N .NE. 23) GO TO 61
      A(1)  = 0.4542
      A(2)  = 0.3126
      A(3)  = 0.2563
      A(4)  = 0.2139
      A(5)  = 0.1787
      A(6)  = 0.1480
      A(7)  = 0.1201
      A(8)  = 0.0941
      A(9)  = 0.0696
      A(10) = 0.0459
      A(11) = 0.0228
      GO TO 115
   61 IF (N .NE. 24) GO TO 63
      A(1)  = 0.4493
      A(2)  = 0.3098
      A(3)  = 0.2554
      A(4)  = 0.2145
      A(5)  = 0.1807
      A(6)  = 0.1512
      A(7)  = 0.1245
      A(8)  = 0.0997
      A(9)  = 0.0764
      A(10) = 0.0539
      A(11) = 0.0321
      A(12) = 0.0107
      GO TO 115
   63 IF (N .NE. 25) GO TO 65
      A(1)  = 0.4450
      A(2)  = 0.3069
      A(3)  = 0.2543
      A(4)  = 0.2148
      A(5)  = 0.1822
      A(6)  = 0.1539
      A(7)  = 0.1283
      A(8)  = 0.1046
      A(9)  = 0.0823
      A(10) = 0.0610
      A(11) = 0.0403
      A(12) = 0.0200
      GO TO 115
   65 IF (N .NE. 26) GO TO 67
      A(1)  = 0.4407
      A(2)  = 0.3043
      A(3)  = 0.2533
      A(4)  = 0.2151
      A(5)  = 0.1836
      A(6)  = 0.1563
      A(7)  = 0.1316
      A(8)  = 0.1089
      A(9)  = 0.0876
      A(10) = 0.0672
      A(11) = 0.0476
      A(12) = 0.0284
      A(13) = 0.0094
      GO TO 115
   67 IF (N .NE. 27) GO TO 69
      A(1)  = 0.4366
      A(2)  = 0.3018
      A(3)  = 0.2522
      A(4)  = 0.2152
      A(5)  = 0.1848
      A(6)  = 0.1584
      A(7)  = 0.1346
      A(8)  = 0.1128
      A(9)  = 0.0923
      A(10) = 0.0728
      A(11) = 0.0540
      A(12) = 0.0358
      A(13) = 0.0178
      GO TO 115
   69 IF (N .NE. 28) GO TO 71
      A(1)  = 0.4328
      A(2)  = 0.2992
      A(3)  = 0.2510
      A(4)  = 0.2151
      A(5)  = 0.1857
      A(6)  = 0.1601
      A(7)  = 0.1372
      A(8)  = 0.1162
      A(9)  = 0.0965
      A(10) = 0.0778
      A(11) = 0.0598
      A(12) = 0.0424
      A(13) = 0.0253
      A(14) = 0.0084
      GO TO 115
   71 IF (N .NE. 29) GO TO 73
      A(1)  = 0.4291
      A(2)  = 0.2968
      A(3)  = 0.2499
      A(4)  = 0.2150
      A(5)  = 0.1864
      A(6)  = 0.1616
      A(7)  = 0.1395
      A(8)  = 0.1192
      A(9)  = 0.1002
      A(10) = 0.0822
      A(11) = 0.0650
      A(12) = 0.0483
      A(13) = 0.0320
      A(14) = 0.0159
      GO TO 115
   73 IF (N .NE. 30) GO TO 75
      A(1)  = 0.4254
      A(2)  = 0.2944
      A(3)  = 0.2487
      A(4)  = 0.2148
      A(5)  = 0.1870
      A(6)  = 0.1630
      A(7)  = 0.1415
      A(8)  = 0.1219
      A(9)  = 0.1036
      A(10) = 0.0862
      A(11) = 0.0697
      A(12) = 0.0537
      A(13) = 0.0381
      A(14) = 0.0227
      A(15) = 0.0076
      GO TO 115
   75 IF (N .NE. 31) GO TO 77
      A(1)  = 0.4220
      A(2)  = 0.2921
      A(3)  = 0.2475
      A(4)  = 0.2145
      A(5)  = 0.1874
      A(6)  = 0.1641
      A(7)  = 0.1433
      A(8)  = 0.1243
      A(9)  = 0.1066
      A(10) = 0.0899
      A(11) = 0.0739
      A(12) = 0.0585
      A(13) = 0.0435
      A(14) = 0.0289
      A(15) = 0.0144
      GO TO 115
   77 IF (N .NE. 32) GO TO 79
      A(1)  = 0.4188
      A(2)  = 0.2898
      A(3)  = 0.2463
      A(4)  = 0.2141
      A(5)  = 0.1878
      A(6)  = 0.1651
      A(7)  = 0.1449
      A(8)  = 0.1265
      A(9)  = 0.1093
      A(10) = 0.0931
      A(11) = 0.0777
      A(12) = 0.0629
      A(13) = 0.0485
      A(14) = 0.0344
      A(15) = 0.0206
      A(16) = 0.0068
      GO TO 115
   79 IF (N .NE. 33) GO TO 81
      A(1)  = 0.4156
      A(2)  = 0.2876
      A(3)  = 0.2451
      A(4)  = 0.2137
      A(5)  = 0.1880
      A(6)  = 0.1660
      A(7)  = 0.1463
      A(8)  = 0.1284
      A(9)  = 0.1118
      A(10) = 0.0961
      A(11) = 0.0812
      A(12) = 0.0669
      A(13) = 0.0530
      A(14) = 0.0395
      A(15) = 0.0262
      A(16) = 0.0131
      GO TO 115
   81 IF (N .NE. 34) GO TO 83
      A(1)  = 0.4127
      A(2)  = 0.2854
      A(3)  = 0.2439
      A(4)  = 0.2132
      A(5)  = 0.1882
      A(6)  = 0.1667
      A(7)  = 0.1475
      A(8)  = 0.1301
      A(9)  = 0.1140
      A(10) = 0.0988
      A(11) = 0.0844
      A(12) = 0.0706
      A(13) = 0.0572
      A(14) = 0.0441
      A(15) = 0.0314
      A(16) = 0.0187
      A(17) = 0.0062
      GO TO 115
   83 IF (N .NE. 35) GO TO 85
      A(1)  = 0.4096
      A(2)  = 0.2834
      A(3)  = 0.2427
      A(4)  = 0.2127
      A(5)  = 0.1883
      A(6)  = 0.1673
      A(7)  = 0.1487
      A(8)  = 0.1317
      A(9)  = 0.1160
      A(10) = 0.1013
      A(11) = 0.0873
      A(12) = 0.0739
      A(13) = 0.0610
      A(14) = 0.0484
      A(15) = 0.0361
      A(16) = 0.0239
      A(17) = 0.0119
      GO TO 115
   85 IF (N .NE. 36) GO TO 87
      A(1)  = 0.4068
      A(2)  = 0.2813
      A(3)  = 0.2415
      A(4)  = 0.2121
      A(5) = 0.1883
      A(6) = 0.1678
      A(7) = 0.1496
      A(8) = 0.1331
      A(9) = 0.1179
      A(10) = 0.1036
      A(11) = 0.0900
      A(12) = 0.0770
      A(13) = 0.0645
      A(14) = 0.0523
      A(15) = 0.0404
      A(16) = 0.0287
      A(17) = 0.0172
      A(18) = 0.0057
      GO TO 115
   87 IF (N .NE. 37) GO TO 89
      A(1)  = 0.4040
      A(2)  = 0.2794
      A(3)  = 0.2403
      A(4)  = 0.2116
      A(5)  = 0.1883
      A(6)  = 0.1683
      A(7)  = 0.1505
      A(8)  = 0.1344
      A(9)  = 0.1196
      A(10) = 0.1056
      A(11) = 0.0924
      A(12) = 0.0798
      A(13) = 0.0677
      A(14) = 0.0559
      A(15) = 0.0444
      A(16) = 0.0331
      A(17) = 0.0220
      A(18) = 0.0110
      GO TO 115
   89 IF (N .NE. 38) GO TO 91
      A(1)  = 0.4015
      A(2)  = 0.2774
      A(3)  = 0.2391
      A(4)  = 0.2110
      A(5)  = 0.1881
      A(6)  = 0.1686
      A(7)  = 0.1513
      A(8)  = 0.1356
      A(9)  = 0.1211
      A(10) = 0.1075
      A(11) = 0.0947
      A(12) = 0.0824
      A(13) = 0.0706
      A(14) = 0.0592
      A(15) = 0.0481
      A(16) = 0.0372
      A(17) = 0.0264
      A(18) = 0.0158
      A(19) = 0.0053
      GO TO 115
   91 IF (N .NE. 39) GO TO 93
      A(1)  = 0.3989
      A(2)  = 0.2755
      A(3)  = 0.2380
      A(4)  = 0.2104
      A(5)  = 0.1880
      A(6)  = 0.1689
      A(7)  = 0.1520
      A(8)  = 0.1366
      A(9)  = 0.1225
      A(10) = 0.1092
      A(11) = 0.0967
      A(12) = 0.0848
      A(13) = 0.0733
      A(14) = 0.0622
      A(15) = 0.0515
      A(16) = 0.0409
      A(17) = 0.0305
      A(18) = 0.0203
      A(19) = 0.0101
      GO TO 115
   93 IF (N .NE. 40) GO TO 95
      A(1)  = 0.3964
      A(2)  = 0.2737
      A(3)  = 0.2368
      A(4)  = 0.2098
      A(5)  = 0.1878
      A(6)  = 0.1691
      A(7)  = 0.1526
      A(8)  = 0.1376
      A(9)  = 0.1237
      A(10) = 0.1108
      A(11) = 0.0986
      A(12) = 0.0870
      A(13) = 0.0759
      A(14) = 0.0651
      A(15) = 0.0546
      A(16) = 0.0444
      A(17) = 0.0343
      A(18) = 0.0244
      A(19) = 0.0146
      A(20) = 0.0049
      GO TO 115
   95 IF (N .NE. 41) GO TO 97
      A(1)  = 0.3940
      A(2)  = 0.2719
      A(3)  = 0.2357
      A(4)  = 0.2091
      A(5)  = 0.1876
      A(6)  = 0.1693
      A(7)  = 0.1531
      A(8)  = 0.1384
      A(9)  = 0.1249
      A(10) = 0.1123
      A(11) = 0.1004
      A(12) = 0.0891
      A(13) = 0.0782
      A(14) = 0.0677
      A(15) = 0.0575
      A(16) = 0.0476
      A(17) = 0.0379
      A(18) = 0.0283
      A(19) = 0.0188
      A(20) = 0.0094
      GO TO 115
   97 IF (N .NE. 42) GO TO 99
      A(1)  = 0.3917
      A(2)  = 0.2701
      A(3)  = 0.2345
      A(4)  = 0.2085
      A(5)  = 0.1874
      A(6)  = 0.1694
      A(7)  = 0.1535
      A(8)  = 0.1392
      A(9)  = 0.1259
      A(10) = 0.1136
      A(11) = 0.1020
      A(12) = 0.0909
      A(13) = 0.0804
      A(14) = 0.0701
      A(15) = 0.0602
      A(16) = 0.0506
      A(17) = 0.0411
      A(18) = 0.0318
      A(19) = 0.0227
      A(20) = 0.0136
      A(21) = 0.0045
      GO TO 115
   99 IF (N .NE. 43) GO TO 101
      A(1)  = 0.3894
      A(2)  = 0.2684
      A(3)  = 0.2334
      A(4)  = 0.2078
      A(5)  = 0.1871
      A(6)  = 0.1695
      A(7)  = 0.1539
      A(8)  = 0.1398
      A(9)  = 0.1269
      A(10) = 0.1149
      A(11) = 0.1035
      A(12) = 0.0927
      A(13) = 0.0824
      A(14) = 0.0724
      A(15) = 0.0628
      A(16) = 0.0534
      A(17) = 0.0442
      A(18) = 0.0352
      A(19) = 0.0263
      A(20) = 0.0175
      A(21) = 0.0087
      GO TO 115
  101 IF (N .NE. 44) GO TO 103
      A(1)  = 0.3872
      A(2)  = 0.2667
      A(3)  = 0.2323
      A(4)  = 0.2072
      A(5)  = 0.1868
      A(6)  = 0.1695
      A(7)  = 0.1542
      A(8)  = 0.1405
      A(9)  = 0.1278
      A(10) = 0.1160
      A(11) = 0.1049
      A(12) = 0.0943
      A(13) = 0.0842
      A(14) = 0.0745
      A(15) = 0.0651
      A(16) = 0.0560
      A(17) = 0.0471
      A(18) = 0.0383
      A(19) = 0.0296
      A(20) = 0.0211
      A(21) = 0.0126
      A(22) = 0.0042
      GO TO 115
  103 IF (N .NE. 45) GO TO 105
      A(1)  = 0.3850
      A(2)  = 0.2651
      A(3)  = 0.2313
      A(4)  = 0.2065
      A(5)  = 0.1865
      A(6)  = 0.1695
      A(7)  = 0.1545
      A(8)  = 0.1410
      A(9)  = 0.1286
      A(10) = 0.1170
      A(11) = 0.1062
      A(12) = 0.0959
      A(13) = 0.0860
      A(14) = 0.0765
      A(15) = 0.0673
      A(16) = 0.0584
      A(17) = 0.0497
      A(18) = 0.0412
      A(19) = 0.0328
      A(20) = 0.0245
      A(21) = 0.0163
      A(22) = 0.0081
      GO TO 115
  105 IF (N .NE. 46) GO TO 107
      A(1)  = 0.3830
      A(2)  = 0.2635
      A(3)  = 0.2302
      A(4)  = 0.2058
      A(5)  = 0.1862
      A(6)  = 0.1695
      A(7)  = 0.1548
      A(8)  = 0.1415
      A(9)  = 0.1293
      A(10) = 0.1180
      A(11) = 0.1073
      A(12) = 0.0972
      A(13) = 0.0876
      A(14) = 0.0783
      A(15) = 0.0694
      A(16) = 0.0607
      A(17) = 0.0522
      A(18) = 0.0439
      A(19) = 0.0357
      A(20) = 0.0277
      A(21) = 0.0197
      A(22) = 0.0118
      A(23) = 0.0039
      GO TO 115
  107 IF (N .NE. 47) GO TO 109
      A(1)  = 0.3808
      A(2)  = 0.2620
      A(3)  = 0.2291
      A(4)  = 0.2052
      A(5)  = 0.1859
      A(6)  = 0.1695
      A(7)  = 0.1550
      A(8)  = 0.1420
      A(9)  = 0.1300
      A(10) = 0.1189
      A(11) = 0.1085
      A(12) = 0.0986
      A(13) = 0.0892
      A(14) = 0.0801
      A(15) = 0.0713
      A(16) = 0.0628
      A(17) = 0.0546
      A(18) = 0.0465
      A(19) = 0.0385
      A(20) = 0.0307
      A(21) = 0.0229
      A(22) = 0.0153
      A(23) = 0.0076
      GO TO 115
  109 IF (N .NE. 48) GO TO 111
      A(1)  = 0.3789
      A(2)  = 0.2604
      A(3)  = 0.2281
      A(4)  = 0.2045
      A(5)  = 0.1855
      A(6)  = 0.1693
      A(7)  = 0.1551
      A(8)  = 0.1423
      A(9)  = 0.1306
      A(10) = 0.1197
      A(11) = 0.1095
      A(12) = 0.0998
      A(13) = 0.0906
      A(14) = 0.0817
      A(15) = 0.0731
      A(16) = 0.0648
      A(17) = 0.0568
      A(18) = 0.0489
      A(19) = 0.0411
      A(20) = 0.0335
      A(21) = 0.0259
      A(22) = 0.0185
      A(23) = 0.0111
      A(24) = 0.0037
      GO TO 115
  111 IF (N .NE. 49) GO TO 113
      A(1)  = 0.3770
      A(2)  = 0.2589
      A(3)  = 0.2271
      A(4)  = 0.2038
      A(5)  = 0.1851
      A(6)  = 0.1692
      A(7)  = 0.1553
      A(8)  = 0.1427
      A(9)  = 0.1312
      A(10) = 0.1205
      A(11) = 0.1105
      A(12) = 0.1010
      A(13) = 0.0919
      A(14) = 0.0832
      A(15) = 0.0748
      A(16) = 0.0667
      A(17) = 0.0588
      A(18) = 0.0511
      A(19) = 0.0436
      A(20) = 0.0361
      A(21) = 0.0288
      A(22) = 0.0215
      A(23) = 0.0143
      A(24) = 0.0071
      GO TO 115
  113 IF (N .NE. 50) GO TO 115
      A(1)  = 0.3751
      A(2)  = 0.2574
      A(3)  = 0.2260
      A(4)  = 0.2032
      A(5)  = 0.1847
      A(6)  = 0.1691
      A(7)  = 0.1554
      A(8)  = 0.1430
      A(9)  = 0.1317
      A(10) = 0.1212
      A(11) = 0.1113
      A(12) = 0.1020
      A(13) = 0.0932
      A(14) = 0.0846
      A(15) = 0.0764
      A(16) = 0.0685
      A(17) = 0.0608
      A(18) = 0.0532
      A(19) = 0.0459
      A(20) = 0.0386
      A(21) = 0.0314
      A(22) = 0.0244
      A(23) = 0.0174
      A(24) = 0.0104
      A(25) = 0.0035
  115 IF (N .GT. 50) WRITE(6,202)
  202 FORMAT(1X,' ')
      IF (N .GT. 50) WRITE(6,203)
  203 FORMAT(1X,'THIS IS THE SHAPIRO-WILK TEST FOR SMALL SAMPLES')
      IF (N .GT. 50) WRITE (6,204)
  204 FORMAT(1X,'THE SAMPLE SIZE MUST BE LESS THAN OR EQUAL TO 50')
      IF (N .GT. 50) GO TO 160
      DO 140 I=1,K
      J=N-I+1
  140 SUMB=SUMB+A(I)*(X(J)-X(I))
      Y(1)=SUMB**2/S2
      DO 150 I=1,N
  150 X(I)=X1(I)
      WRITE(6,205) Y(1)
  205 FORMAT(/,2X,'TEST13 SW(N)  =',F10.4)
  160 CONTINUE
      RETURN
      END
      SUBROUTINE TEST14(X,Y,N)
      DIMENSION X(150),X1(150),Y(2),Z(150),P(150)
      EXTERNAL xinormal
      EXTERNAL dl
      SUMA=0
      SUMB=0
      SUMC=0
      SUMD=0
      DO 10 I =1,N
   10 X1(I)=X(I)
      CALL SORT(N,X)
      DO 20 I = 1,N
   20 P(I)=(FLOAT(I)-.375)/(0.25+N)
      DO 25 I=1,N
   25 Z(I)=XINORMAL(P(I))
      DO 30 I=1,N
      SUMA=SUMA+(Z(I)*X(I))
      SUMB=SUMB+(Z(I)**2)
      SUMC=SUMC+X(I)
   30 SUMD=SUMD+X(I)**2
      Y(1)=(SUMA**2/SUMB)/(SUMD-SUMC**2/N)
      DO 40 I=1,N
   40 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST14 SF(N)  =',F10.4)
      RETURN
      END
      SUBROUTINE TEST15(X,Y,N)
      DIMENSION X(150),Y(2),X1(150)
      REAL MEAN
      DO 10 I = 1,N
   10 X1(I)=X(I)
      R=FLOAT(N)
      CALL SORT(N,X)
      XS=X(1)
      SUM1=0
      SUM2=0
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=SUM2-((SUM1*SUM1)/N)
      B1=SQRT(R/(R-1))
      B=(XBAR-XS)*B1
      Y(1)=(B*B)/S1
      DO 30 I=1,N
   30 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST15 SW(E)  =',F10.4)
      RETURN
      END
      SUBROUTINE TEST16(X,Y,N)
      DIMENSION X(150),Y(2),FX(150),FN1(150),FN2(150)
      DIMENSION X1(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      FN2(I)=FLOAT((2*I)-1)/FLOAT(2*N)
      FX(I)=1-EXP(X(I)*(-1.0)/XBAR)
   30 CONTINUE
      DO 40 I=1,N
      A=((2*I)-1)*ALOG(FX(I))
      B=((2*I)-1)*ALOG(1.0-FX(N+1-I))
      SUM3=SUM3+A+B
      SUM4=SUM4+((-FN2(I)+FX(I))**2)
      FN1(I)=ABS(FN1(I)-FX(I))
   40 CONTINUE
      CVM=1./FLOAT(12*N)+SUM4
      CVMOD=CVM*(1.+(.16/R))
      Y(1)=CVMOD
      DO 50 I=1,N
   50 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST16 CVM(E) =',F10.4)
      RETURN
      END
      SUBROUTINE TEST17(X,Y,N)
      DIMENSION X(150),Y(2),FX(150)
      DIMENSION T(150),Z(150),D(2),FN3(150),X1(150)
      REAL MEAN
      REAL KS
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      FN3(I)=FLOAT(I)/R
      FX(I)=1-EXP(X(I)*(-1.0)/XBAR)
   30 CONTINUE
      DO 40 J=1,N
   40 Z(J)=(FN3(J)-FX(J))
      CALL SORT(N,Z)
      D1=Z(N)
      DO 50 J=1,N
   50 T(J)=FX(J)-((J-1)/R)
      CALL SORT(N,T)
      D2=T(N)
      D(1)=D1
      D(2)=D2
      CALL SORT(2,D)
      DMAX=D(2)
      KS=(DMAX-0.2/R)*(SQRT(R)+(.5/SQRT(R))+0.26)
      Y(1)=KS
      DO 60 I=1,N
   60 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST17 KSD(E) =',F10.4)
      RETURN
      END
      SUBROUTINE TEST18(X,Y,N)
      DIMENSION X(150),Y(2),FX(150)
      DIMENSION T(150),Z(150),D(2),FN3(150),X1(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      FN3(I)=FLOAT(I)/R
      FX(I)=1-EXP(X(I)*(-1.0)/XBAR)
   30 CONTINUE
      DO 40 J=1,N
   40 Z(J)=(FN3(J)-FX(J))
      CALL SORT(N,Z)
      D1=Z(N)
      DO 50 J=1,N
   50 T(J)=FX(J)-((J-1)/R)
      CALL SORT(N,T)
      D2=T(N)
      D(1)=D1
      D(2)=D2
      V=D(1)+D(2)
      V=(V-0.2/R)*(SQRT(R)+(.35/SQRT(R))+0.24)
      Y(1)=V
      DO 60 I=1,N
   60 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST18 KV(E)  =',F10.4)
      RETURN
      END
      SUBROUTINE TEST19(X,Y,N)
      DIMENSION X(150),Y(2),FX(150),FN1(150),FN2(150)
      DIMENSION X1(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      FN2(I)=FLOAT((2*I)-1)/FLOAT(2*N)
      FX(I)=1-EXP(X(I)*(-1.0)/XBAR)
   30 CONTINUE
      DO 40 I=1,N
      A=((2*I)-1)*ALOG(FX(I))
      B=((2*I)-1)*ALOG(1.0-FX(N+1-I))
      SUM3=SUM3+A+B
      SUM4=SUM4+((-FN2(I)+FX(I))**2)
      FN1(I)=ABS(FN1(I)-FX(I))
   40 CONTINUE
      CVM=1./FLOAT(12*N)+SUM4
      DO 50 I=1,N
      SUM5=SUM5+FX(I)
   50 CONTINUE
      ZBAR=SUM5/R
      W=CVM-R*(ZBAR-0.5)*(ZBAR-0.5)
      W=W*(1.0+0.16/R)
      Y(1)=W
      DO 60 I=1,N
   60 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST19 WU2(E) =',F10.4)
      RETURN
      END
      SUBROUTINE TEST20(X,Y,N)
      DIMENSION X(150),Y(2),FX(150),FN1(150),FN2(150)
      DIMENSION X1(150)
      REAL MEAN
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 10 I = 1,N
   10 X1(I)=X(I)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      XBAR=MEAN
      DO 20 I = 1,N
   20 SUM2=SUM2+(X(I)*X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      SDX=SQRT(S1)
      R=FLOAT(N)
      CALL SORT(N,X)
      DO 30 I=1,N
      FN2(I)=FLOAT((2*I)-1)/FLOAT(2*N)
      FX(I)=1-EXP(X(I)*(-1.0)/XBAR)
   30 CONTINUE
      DO 40 I=1,N
      A=((2*I)-1)*ALOG(FX(I))
      B=((2*I)-1)*ALOG(1.0-FX(N+1-I))
      SUM3=SUM3+A+B
      SUM4=SUM4+((-FN2(I)+FX(I))**2)
      FN1(I)=ABS(FN1(I)-FX(I))
   40 CONTINUE
      ADB=SUM3/R
      ADSTAT=-R-ADB
      ADEXP=ADSTAT*(1.0+(0.3/R))
      Y(1)=ADEXP
      DO 50 I=1,N
   50 X(I)=X1(I)
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST20 AD(E)  =',F10.4)
      RETURN
      END
      SUBROUTINE TEST21(X,Y,N)
      DIMENSION X(150),Y(2),V(150),V2(150),F(150),P(150)
      REAL MEAN
      SUM1=0
      SUM3=0
      A=4*((0.75*(N-1)*(N-1))**2)
      K1=A
      C=A-K1
      IF(C .GT. 0.5) K1=K1+1
   10 R=N/K1
      IF(R .LT. 5) K1=K1-1
      IF(R .LT.5) GOTO 10
      K2=K1-1
      DO 15 I=1,K1
   15 F(I)=0
      DO 20 I=1,N
   20 SUM1=SUM1+X(I)
      MEAN=N/SUM1
      DO 30 I=1,K2
   30 P(I)=FLOAT(I)/K1
      DO 40 I=1,K2
   40 V(I)=(-1.0/MEAN)*ALOG(1-P(I))
      DO 41 I=1,K2
   41 V2(I+1)=V(I)+0.0001
      DO 55 I=1,N
      DO 45 J=2,K2
      IF(X(I) .GE. V2(J) .AND. X(I) .LE. V(J)) F(J)=1+F(J)
   45 CONTINUE
      IF(X(I) .GE. V2(K2+1)) F(K2+1)=F(K2+1)+1
      IF(X(I) .LE. V(1)) F(1)=F(1)+1
   55 CONTINUE
      DO 65 I=1,K1
   65 SUM3=SUM3+F(I)*F(I)
      Y(1)=SUM3*K1/N-N
      Y(2)=FLOAT(K1)-2
      WRITE(6,100) Y(1),Y(2)
  100 FORMAT(/,2X,'TEST21 CS(E)  =',F10.4,'   DOF  =',F10.4)
      RETURN
      END
      SUBROUTINE TEST22(X,Y,N)
      DIMENSION X(150),Y(2)
      REAL MEAN
      REAL M1
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      MEAN=SUM1/N
      DO 20 I = 1,N
   20 SUM2=SUM2+ALOG(X(I))
      S1=((N*SUM2)-(SUM1*SUM1))/(N*(N-1))
      M1=SUM2/N
      DO 25 I=1,N
   25 SUM4=SUM4+(ALOG(X(I))-M1)*(ALOG(X(I))-M1)
      S2=SUM4/N
      DO 30 I=1,N
   30 SUM3=SUM3+(X(I)-MEAN)**3
      IF(SUM3 .GE. 0) GO TO 40
      WRITE(6,100) (X(I),I=1,N)
  100 FORMAT(4(1X,F10.4,2X))
      WRITE(6,200)
  200 FORMAT(1X,'THIRD SAMPLE MOMENT ABOUT THE MEAN IS LESS THAN ZERO')
      WRITE(6,300)
  300 FORMAT(1X,'HENCE WE ACCEPT THE NULL HYPOTHESIS OF NORMALITY')
      Y(1)=0
      GO TO 50
   40 CONTINUE
      DO 45 I=1,N
   45 SUM5=SUM5+(X(I)-MEAN)*(X(I)-MEAN)
      S1=SQRT(SUM5/N)
      S3=SQRT(S2)
      E1=EXP(M1)
      Y(1)=S1/(S3*E1)
   50 WRITE(6,400) Y(1)
  400 FORMAT(/,2X,'TEST22 LR(NL) =',F10.4)
      RETURN
      END
      SUBROUTINE TEST23(X,Y,N)
      DIMENSION X(150),Y(2)
      SUM2=0
      SUM4=0
      DO 20 I = 1,N
   20 SUM2=SUM2+ALOG(X(I))
      DO 25 I=1,N
   25 SUM4=SUM4+(ALOG(X(I))-(SUM2/N))*(ALOG(X(I))-(SUM2/N))
      S2=SUM4/(N-1)
      S3=EXP(S2)-1
      S4=SQRT(S3)
      Y(1)=S4
      WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST23 CV(L)  =',F10.4)
      RETURN
      END
      SUBROUTINE TEST24(X,Y,N)
      DIMENSION X(150),Y(2)
      SUM1=0
      SUM2=0
      SUM3=0
      SUM4=0
      SUM5=0
      R=FLOAT(N)
      DO 15 I =1,N
   15 SUM1=SUM1+X(I)
      B1=SUM1/N
      DO 20 I = 1,N
   20 SUM2=SUM2+ALOG(X(I))
      A1=SUM2/N
      DO 25 I=1,N
   25 SUM4=SUM4+(ALOG(X(I))-A1)*(ALOG(X(I))-A1)
      A2=SUM4/N
      B3=EXP(2*A1+A2)*(EXP(A2)-1)
      C1=ALOG(A2/B3)
      C2=(EXP(4*A2)+(2*EXP(3*A2))-4)/4-A2+(0.75*EXP(A2))
      C3=A2*(2*EXP(A2)-1)*(2*EXP(A2)-1)
      C4=2*(EXP(A2)-1)*(EXP(A2)-1)
      C5=C3/C4
      IF(C2 .LT. C5) GO TO 30
      C6=SQRT(C2-C5)*2*(SQRT(R))
      Y(1)=C1/C6
      GO TO 40
   30 WRITE(6,35)
      Y(1)=999999999
   35 FORMAT(/,2X,'WARNING!!! STATISTICS FOR THE NEXT TEST WILL',
     +' NOT BE CALCULATED DUE TO SMALL LOGVARIANCE')
   40 WRITE(6,100) Y(1)
  100 FORMAT(/,2X,'TEST24 KT(LN) =',F10.4)
      RETURN
      END
      SUBROUTINE SORT(NN,X)
      INTEGER I,J,K,L,IJ,NN,M,IU(16),IL(16)
      REAL Y,YY,X(500)
      M=1
      I=1
      J=NN
  150 IF (I .GE. J) GO TO 220
  160 K=I
      IJ=(J+I)/2
      Y=X(IJ)
      IF (X(I) .LE. Y) GO TO 170
      X(IJ)=X(I)
      X(I)=Y
      Y=X(IJ)
  170 L=J
      IF (X(J) .GE. Y) GO TO 190
      X(IJ)=X(J)
      X(J)=Y
      Y=X(IJ)
      IF (X(I) .LE. Y) GO TO 190
      X(IJ)=X(I)
      X(I)=Y
      Y=X(IJ)
      GO TO 190
  180 X(L)=X(K)
      X(K)=YY
  190 L=L-1
      IF (X(L) .GT. Y) GO TO 190
      YY=X(L)
  200 K=K+1
      IF (X(K) .LT. Y) GO TO 200
      IF (K .LE. L) GO TO 180
      IF (L-I .LE. J-K) GO TO 210
      IL(M)=I
      IU(M)=L
      I=K
      M=M+1
      GO TO 230
  210 IL(M)=K
      IU(M)=J
      J=L
      M=M+1
      GO TO 230
  220 M=M-1
      IF (M .EQ. 0) GO TO 260
      I=IL(M)
      J=IU(M)
  230 IF (J-I .GE. 1) GO TO 160
      I=I-1
  240 I=I+1
      IF (I .EQ. J) GO TO 220
      Y=X(I+1)
      IF (X(I) .LE. Y) GO TO 240
      K=I
  250 X(K+I)=X(K)
      K=K-1
      IF (Y .LT. X(K)) GO TO 250
      X(K+1)=Y
      GO TO 240
  260 CONTINUE
      RETURN
      END
      FUNCTION enormp(x)
      REAL x
      DOUBLE PRECISION x1,x2,x3,x4,yy1,yy2
      xp1 = 0.771058495001320D-04
      xp2 = -0.00133733772997339D0
      xp3 = 0.0323076579225834D0
      xp4 = 0.0479137145607681D0
      xp5 = 0.128379167095513D0
      xq1 = 0.00301048631703895D0
      xq2 = 0.0538971687740286D0
      xq3 = 0.375795757275549D0
      xr1 = -1.36864857382717D-07
      xr2 = 0.564195517478974D0
      xr3 = 7.21175825088309D0
      xr4 = 43.1622272220567D0
      xr5 = 152.989285046940D0
      xr6 = 339.320816734344D0
      xr7 = 451.918953711873D0
      xr8 = 300.459261020162D0
      xs1 = 1.0D0
      xs2 = 12.7827273196294D0
      xs3 = 77.0001529352295D0
      xs4 = 277.585444743988D0
      xs5 = 638.980264465631D0
      xs6 = 931.354094850610D0
      xs7 = 790.950925327898D0
      xs8 = 300.459260956983D0
      xt1 = 2.10144126479064D0
      xt2 = 26.2370141675169D0
      xt3 = 21.3688200555087D0
      xt4 = 4.65807828718470D0
      xt5 = 0.282094791773523D0
      xu1 = 94.1537750555460D0
      xu2 = 187.114811799590D0
      xu3 = 99.0191814623914D0
      xu4 = 18.0124575948747D0
      x3  = 0.564189583547756D0
      x1 = abs(x)
      IF (x1.GT.0.5D0) GO TO 10
      x4 = x*x
      yy1 = ((((xp1*x4+xp2)*x4+xp3)*x4+xp4)*x4+xp5) +1.0D0
      yy2 = (((xq1*x4+xq2)*x4+xq3)*x4) + 1.0D0
      enormp = x* (yy1/yy2)
      RETURN
   10 IF (x1.GT.4.0D0) GO TO 20
      yy1 = ((((((xr1*x1+xr2)*x1+xr3)*x1+xr4)*x1+xr5)*x1
     +    + xr6)*x1+xr7)*x1+xr8
      yy2 = ((((((xs1*x1+xs2)*x1+xs3)*x1+xs4)*x1+xs5)*x1
     +    + xs6)*x1+xs7)*x1+xs8
      enormp =  1.0D0-exp(-x*x)*yy1/yy2
      IF (x .LT. 0.0D0) enormp = -enormp
      RETURN
   20 x2 = x*x
      x4=1.0D0*x4
      yy1 = ((((xt1*x4+xt2)*x4+xt3)*x4+xt4)*x4) + xt5
      yy2 = ((((xu1*x4+xu2)*x4+xu3)*x4+xu4)*x4) + 1.0D0
      enormp = (x3/x1)-(yy1*x1)/(x2*yy2)
      enormp = 1.0D0-exp(-x2)*enormp
      IF (x .LT. 0.0D0) enormp = -enormp
      RETURN
      END
      FUNCTION xinormal(p)
      DOUBLE PRECISION px,pw,f0
      p0=-0.322232431088D0
      p1=-1.0D0
      p2=-0.342242088547D0
      p3=-0.0204231210245D0
      p4=-0.0000453642210148D0
      q0=0.099348462606D0
      q1=0.588581570495D0
      q2=0.531103462366D0
      q3=0.10353775285D0
      q4=0.0038560700634D0
      pind=p
      IF (p .LT. 1.0E-10) GO TO 10
      GO TO 20
   10 xinormal = -10
      RETURN
   20 IF (p .GE. 1.0) GO TO 30
      GO TO 40
   30 xinormal = 10
      RETURN
      IF (p .EQ. 0.5D0) GO TO 35
      GO TO 40
   35 xinormal = 0.5
      RETURN
   40 IF (p .GT. 0.5D0) p=p-1
      pw=SQRT(ALOG(1/(p*p)))
      f0=(((pw*q4+q3)*pw+q2)*pw+q1)*pw+q0
      px=pw+((((pw*p4+p3)*pw+p2)*pw+p1)*pw+p0)/f0
      if (pind .LT. 0.5D0) px=-px
      xinormal = px
      RETURN
      END
```
<!-- markdownlint-enable -->
