
@Echo OFF 
setlocal
:: 
:: License: GPL 2.0
:: (c) 2013 GKX Associates Inc.
::
:: INSTALLATION INSTRUCTIONS:
:: Modify the definitions of R_HOME and R_ARCH as needed
:: (To get the value of R_HOME go into R and issue this command: 
::        normalizePath(R.home())
:: and use its output as the value here.  If you upgrade R to another
:: version R_HOME will need to be changed. For most users R_ARCH can 
:: be left as set but if you have a 32 bit system then comment out the
:: line setting R_ARCH and uncomment the prior line.)
::
:: RUN INSTRUCTIONS:
:: Create an R script whose first line is:
::  #Rscript2.bat %0 %*
:: and give it a .bat extesion.  If it is called test.bat then call it 
:: like this (adding arguments if any):
::  test.bat 

set R_HOME=C:\Program Files\R\R-3.0.2

:: 32 or 64 bit version of R.  
:: (If you wish to use both versions of R make two versions of this file.)
:: set R_ARCH=i386

set R_ARCH=x64

set R_PATH=%R_HOME%\bin\%R_ARCH%
"%R_PATH%\Rscript.exe" %*
