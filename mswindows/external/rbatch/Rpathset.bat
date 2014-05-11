:: Software and documentation is (c) 2013 GKX Associates Inc. and 
:: licensed under [GPL 2.0](http://www.gnu.org/licenses/gpl-2.0.html).

:: Purpose: setup path to use R, Rtools and other utilities from cmd line.
::
:: Makes no permanent system changes.  Does not read or write registry.
:: Temporarily prepends to PATH and sets environment variables for current 
:: Windows cmd line session only.
::
:: Use: Run this each time you launch cmd.exe and want to use R or Rtools.
:: Can also use it like this:  Rpathset Rgui
:: where Rgui may be replaced with R, Rscript, etc.
::
:: Install: Modify set statements appropriately for your installation.
:: and then place this batch script anywhre on your existing path.
:: (The Windows commandline command PATH shows the current PATH.)
:: 
:: In many cases no changes are needed at all in this file.
:: R_HOME and R_ARCH are the most likely that may need to be changed.
::
:: Report bugs to:
:: ggrothendieck at gmail.com
:: 
:: License: GPL 2.0

:: Go into R and issue this command: normalizePath(R.home())
:: and use its output as the value here.  If you upgrade R to another
:: version R_HOME will change.
:: R is available from: http://www.r-project.org
set R_HOME=C:\Program Files\R\R-3.1.0

:: 32 or 64 bit version of R.  
:: (If you wish to use both versions of R make two versions of this file.)
:: set R_ARCH=i386
set R_ARCH=x64

:: If in future R changes where it puts its executables then change accordingly
set R_PATH=%R_HOME%\bin\%R_ARCH%

:: directory path where Rtools was installed.  Usually best to use default
:: which is the one shown below.  Note that different versions of R may 
:: require different versions of Rtools.
:: Rtools is available from: http://cran.r-project.org/bin/windows/Rtools/
set R_TOOLS=C:\Rtools

:: If in future Rtools changes the required paths then modify accordingly.  
:: To check, run the following findstr command which lists the R_TOOLS_PATH 
:: (plus some garbage):
::   findstr {app} %R_TOOLS%\unins000.dat
set R_TOOLS_PATH=%R_TOOLS%\bin;%R_TOOLS%\gcc-4.6.3\bin

:: From within R, the R_USER directory path can be viewed like this:
::    cat(normalizePath('~'))
:: It contains your personal .Rprofile, if any, and unless set otherwise 
:: %R_USER%\R\win-library contains your personal R library of packages 
:: (from CRAN and elsewhere).
set R_USER=%userprofile%\Documents

:: This reduces the verbosity of certain Cygwin tools
:: (Unfortunately, it seems to have no effect on some Windows systems.)
set CYGWIN=nodosfilewarning

:: Displays Rtools version in use
type %R_TOOLS%\version.txt

:: MiKTeX path.  Used to build R packages from source.
:: This is the directory containing pdflatex.exe
:: MiKTeX is available from http://miktex.org
set R_MIKTEX_PATH=C:\Program Files (x86)\MiKTeX 2.9\miktex\bin

:: This is only needed when building RMySQL package from source
:: It is not needed to run RMySQL once its built.
:: set MYSQL_HOME=C:\Program Files\MySQL\MySQL Server 5.1

:: This is only needed to run JGR and Deducer.
:: R_LIBS is the system library.
:: If you have installed at least one package (at which point R will ask to 
::  set up a personal library -- which you should allow) then R_LIBS_USER
::  is similar to output of .libPaths() with first comnponent being your
::  personal library and second compnent being library holding packages that 
::  come with R.  
:: Be sure NOT to store the packages that you downloaded from CRAN
::  in the %R_HOME%\library directory.
:: set R_LIBS=%R_USER%\R\win-library\2.15
:: set R_LIBS_USER=%R_LIBS%;%R_HOME%\library

:: adds directory to path for the remainder of current cmd line session
path %R_TOOLS_PATH%;%R_MIKTEX_PATH%;%PATH%;%R_PATH%

:: if there are no arguments we are done; else run the argument
if "%1"=="" goto:eof
%*
