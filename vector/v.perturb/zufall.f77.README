* README for zufall random number package
* ------ --- ------ ------ ------ -------
* This package contains a portable random number generator set
* for: uniform (u in [0,1)), normal (<g> = 0, <g^2> = 1), and
* Poisson distributions. The basic module, the uniform generator,
* uses a lagged Fibonacci series generator:
* 
*               t    = u(n-273) + u(n-607)
*               u(n) = t - float(int(t))
* 
* where each number generated, u(k), is floating point. Since
* the numbers are floating point, the left end boundary of the
* range contains zero. This package is nearly portable except
* for the following. (1) It is written in lower case, (2) the
* test package contains a timer (second) which is not portable,
* and (3) there are cycle times (in seconds) in data statements 
* for NEC SX-3, Fujitsu VP2200, and Cray Y-MP. Select your 
* favorite and comment out the others. Replacement functions 
* for 'second' are included - comment out the others. Otherwise 
* the package is portable and returns the same set of floating 
* point numbers up to word precision on any machine. There are 
* compiler directives ($cdir for Cray, *vdir for SX-3, and VOCL 
* for Fujitsu VP2200) which should be otherwise ignored.
* 
* To compile this beast, note that all floating point numbers
* are declared 'double precision'. On Cray X-MP, Y-MP, and C-90
* machines, use the cft77 (cf77) option -dp to run this in 64
* bit mode (not 128 bit double).
* 
* External documentation, "Lagged Fibonacci Random Number Generators
* for the NEC SX-3," is to be published in the International
* Journal of High Speed Computing (1993). Otherwise, ask the
* author: 
* 
*          W. P. Petersen 
*          IPS, RZ F-5
*          ETHZ
*          CH 8092, Zurich
*          Switzerland
* 
* e-mail:  wpp@ips.ethz.ch.
* 
* The package contains the following routines:
* 
* ------------------------------------------------------
* UNIFORM generator routines:
* 
*       subroutine zufalli(seed)
*       integer seed
* c initializes common block containing seeds. if seed=0,
* c the default value is 1802.
* 
*       subroutine zufall(n,u)
*       integer n
*       double precision u(n)
* c returns set of n uniforms u(1), ..., u(n).
* 
*       subroutine zufallsv(zusave)
*       double precision zusave(608)
* c saves buffer and pointer in zusave, for later restarts
* 
*       subroutine zufallrs(zusave)
*       double precision zusave(608)
* c restores seed buffer and pointer from zusave
* ------------------------------------------------------
* 
* NORMAL generator routines:
* 
*       subroutine normalen(n,g)
*       integer n
*       double precision g(n)
* c returns set of n normals g(1), ..., g(n) such that
* c mean <g> = 0, and variance <g**2> = 1.
* 
*       subroutine normalsv(normsv)
*       double precision normsv(1634)
* c saves zufall seed buffer and pointer in normsv
* c buffer/pointer for normalen restart also in normsv
* 
*       subroutine normalrs(normsv)
*       double precision normsv(1634)
* c restores zufall seed buffer/pointer and 
* c buffer/pointer for normalen restart from normsv
* ------------------------------------------------------
* 
* POISSON generator routine:
* 
*       subroutine fische(n,mu,q)
*       integer n,q(n)
*       double precision mu
* c returns set of n integers q, with poisson
* c distribution, density p(q,mu) = exp(-mu) mu**q/q!
* c 
* c USE zufallsv and zufallrs for stop/restart sequence
* c
