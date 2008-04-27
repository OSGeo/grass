%define cvs_y 2004
%define cvs_m 05
%define cvs_d 05
%define cvsver exp_%{cvs_y}_%{cvs_m}_%{cvs_d}
%define version	6.2.1
%define rel 3
%define release %mkrel %rel
#define release %{?_with_cvs:%mkrel -c %{cvs_y}%{cvs_m}%{cvs_d} %rel}%{!?_with_cvs:%mkrel %rel}
%define grassfix 62
#if %mdkversion >= 200710
%define name grass
#Obsoletes: grass%{grassfix}
#else
#define name grass%{?grassfix:%grassfix}
#endif

%{?_with_cvs: %define build_cvs 1}

Summary: 	Geographic Resources Analysis Support System
Name: 		%{name}
Version: 	%{version}
Release: 	%{release}
Group: 		Sciences/Geosciences
License: 	GPL
URL: 		http://grass.itc.it/
%if %{?_with_cvs:1}%{!?_with_cvs:0}
Source: 	http://grass.itc.it/%{name}/source/snapshot/%{name}src_cvs_snapshot_%{cvsver}.tar.gz
Source1: 	http://grass.itc.it/grass62/source/snapshot/grass62src_cvs_snapshot_%{cvsver}.tar.gz
%else
Source:		http://grass.itc.it/grass%{grassfix}/source/grass-%{version}.tar.gz
%endif
Source2: 	grass5_48.png.bz2
Source3: 	grass5_32.png.bz2
Source4: 	grass5_16.png.bz2
Patch2:		grass51-20030614-blas-lapack-libs.patch.bz2
BuildRoot: 	%{_tmppath}/%{name}-%{version}-root

Requires:	xterm 
Requires:       tk 
Requires:       tcl

BuildRequires: 	png-devel 
BuildRequires:  jpeg-devel 
BuildRequires:  tiff-devel 
BuildRequires:  gd-devel >= 2.0 
BuildRequires:  freetype2-devel
BuildRequires: 	MesaGLU-devel 
BuildRequires:  unixODBC-devel 
BuildRequires:  fftw-devel 
BuildRequires:  lesstif-devel
BuildRequires: 	tk tk-devel
BuildRequires:  ncurses-devel 
BuildRequires:  zlib-devel 
BuildRequires:  gdbm-devel 
BuildRequires:  readline-devel 
BuildRequires:  postgresql-devel
BuildRequires:	gcc-gfortran 
BuildRequires:  gdal-devel >= 1.2.0 
BuildRequires:  libblas-devel 
BuildRequires:  flex 
BuildRequires:  bison
BuildRequires:  proj-devel proj >= 4.4.9
BuildRequires:  tcl tcl-devel
BuildRequires:  fftw-devel
BuildRequires:	cfitsio-devel
BuildRequires:	unixODBC-devel
BuildRequires:  mysql-devel
BuildRequires:	termcap-devel
BuildRequires:	ffmpeg-devel
BuildRequires:	freetype-devel
BuildRequires:	python-devel
BuildRequires:	sqlite-devel
%if %mdkversion >= 200700
# deal with Xorg split
BuildRequires:	mesaglw-devel
%endif

Obsoletes:	grass57
Provides:	grass%{version}%{release} = %{version}-%{release}

%description
GRASS (Geographic Resources Analysis Support System) is an 
open source, Free Software Geographical Information System (GIS)
with raster, topological vector, image processing, and graphics
production functionality that operates on various platforms 
through a graphical user interface and shell in X-Window.

%prep
%setup -q %{?_with_cvs:-b1 -n %{name}_%{cvsver}}%{!?_with_cvs:-n grass-%{version}}
#patch2
autoconf

%build
export LDFLAGS="-L/usr/X11R6/%{_lib}"
%configure \
	--with-dbm-includes=%{_includedir}/gdbm/ \
	--with-proj-share=%{_datadir}/proj \
	--with-postgres-includes='%{_includedir}/pgsql %{_includedir}/pgsql/internal' \
	--with-freetype \
	--with-freetype-includes=%{_includedir}/freetype2 \
	--with-motif \
%if %mdkversion >= 200700
	--with-opengl-libs=%{_libdir} \
	--with-motif-libs=%{_libdir} \
%if 0
	--with-glw \
	--with-glw-includes=%{_includedir} \
	--with-glw-libs=%{_libdir} \
%endif
	--with-motif-libs=%{_libdir} \
	--with-motif-includes=%{_includedir} \
%else
	--with-opengl-libs=%{_prefix}/X11R6/%{_lib} \
	--with-motif-includes=%{_prefix}/X11R6/include \
%endif
	--with-gdal  \
	--with-mysql --with-mysql-includes=%{_includedir}/mysql \
	--with-odbc \
	--enable-largefile \
	--with-ffmpeg --with-ffmpeg-includes=%{_includedir}/ffmpeg \
	--with-curses \
	--with-python \
	--with-sqlite \
	--with-cxx \
	--with-nls \
	%{?_with_cvs:--with-grass50=`pwd`/../grass50_%{cvsver}}

#Options that aren't really used
#	--with-blas \
#	--with-lapack \
#	--with-dbm \

#Fix messy grass readline misdetection:
perl -pi -e "s/^READLINELIB .*\$/READLINELIB         =  -lreadline -ltermcap/g" include/Make/Platform.make
perl -pi -e "s/^HISTORYLIB.*\$/HISTORYLIB          =  -lhistory/g" include/Make/Platform.make
perl -pi -e 's,/\* #undef HAVE_READLINE_READLINE_H \*/,#define HAVE_READLINE_READLINE_H 1,g' include/config.h
%if %{?_with_cvs:1}%{!?_with_cvs:0}
make mix
%endif
#r.mapcalc not building first time around:
make||make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"
mkdir -p $RPM_BUILD_ROOT/%{_bindir}
mkdir -p $RPM_BUILD_ROOT/%{_menudir}
#%makeinstall_std INST_DIR=%{_libdir}/grass%{grassfix}
# Actions in make install that don't take into account packaging in a place different to running:
sed -e 's|^GISBASE.*|GISBASE=%{_libdir}/grass%{grassfix}|' \
 bin.%{_target_platform}/grass%{grassfix} > $RPM_BUILD_ROOT/%{_bindir}/grass%{grassfix}
chmod a+x $RPM_BUILD_ROOT/usr/bin/grass%{grassfix}
#cp $RPM_BUILD_PATH/bin.i586-mandrake-linux-gnu/gmake5 $RPM_BUILD_ROOT/usr/bin
#cp $RPM_BUILD_PATH/bin.i586-mandrake-linux-gnu/gmakelinks5 $RPM_BUILD_ROOT/usr/bin

mkdir -p %{buildroot}/%{_libdir}/grass%{grassfix}
cp -a dist.%{_target_platform}/* %{buildroot}/%{_libdir}/grass%{grassfix}

# Add makefiles to includes:
cp -a include/Make %{buildroot}/%{_libdir}/grass%{grassfix}/include/

# Manually bzip2 the man pages:
bzip2 $RPM_BUILD_ROOT/%{_libdir}/grass%{grassfix}/man/man?/*

mkdir $RPM_BUILD_ROOT/%{_libdir}/grass%{grassfix}/locks/

#Menu support:
cat << EOF > $RPM_BUILD_ROOT/%{_menudir}/%{name}
?package(%{name}):command="%{_bindir}/grass%{grassfix}" \
icon="%{name}.png" \
needs="text" \
section="More Applications/Sciences/Geosciences" \
title="Grass%{grassfix}" \
longtitle="Geographic Resources Analysis Support System" \
xdg="true"
EOF

mkdir -p $RPM_BUILD_ROOT%{_liconsdir} $RPM_BUILD_ROOT%{_iconsdir} $RPM_BUILD_ROOT%{_miconsdir}

bzcat %{SOURCE2} > $RPM_BUILD_ROOT%{_liconsdir}/%{name}.png
bzcat %{SOURCE3} > $RPM_BUILD_ROOT%{_iconsdir}/%{name}.png
bzcat %{SOURCE4} > $RPM_BUILD_ROOT%{_miconsdir}/%{name}.png

mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications
cat > $RPM_BUILD_ROOT%{_datadir}/applications/mandriva-%{name}.desktop << EOF
[Desktop Entry]
Name=Grass%{grassfix}
Comment=Geographic Resources Analysis Support System
Exec=%{_bindir}/grass%{grassfix} 
Icon=%{name}
Terminal=true
Type=Application
Categories=X-MandrivaLinux-MoreApplications-Sciences-Geosciences;Science;Geology;
EOF

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

%post
%update_menus

%postun
%clean_menus

%files
%defattr(-,root,root)
%attr(0755,root,root) %{_bindir}/*
%{_libdir}/grass%{grassfix}/
%{_menudir}/%{name}
%{_datadir}/applications/mandriva-grass.desktop
%{_miconsdir}/*.png
%{_liconsdir}/*.png
%{_iconsdir}/*.png
%attr(1777,root,root) %{_libdir}/grass%{grassfix}/locks
%doc AUTHORS COPYING INSTALL README CHANGES


%changelog
* Tue Feb 06 2007 Buchan Milne <bgmilne@mandriva.org> 6.2.1-3mdv2007.0
+ Revision: 116819
- Enable a few more features

* Fri Dec 15 2006 Buchan Milne <bgmilne@mandriva.org> 6.2.1-2mdv2007.1
+ Revision: 97418
- buildrequire termcap-devel
- build system cant handle package name different to module name
-New version 6.2.1
-use versioned name on backports
-obsolete this versioned one in the cooker package to take care of upgrades
-drop irrelevant patches

  + Nicolas LÃ©cureuil <neoclust@mandriva.org>
    - Add BuildRequires

* Wed Aug 30 2006 Buchan Milne <bgmilne@mandriva.org> 6.0.2-1mdv2007.0
+ Revision: 58796
- Import grass



* Tue Aug 29 2006 Buchan Milne <bgmilne@mandriva.org> 6.0.2-1mdv2007.0
- 6.0.2
- fix buildrequires and lib/include paths for xorg 7.0 and later
- disable glw on 2007 and later until I can find the problem
- xdg menu

* Fri Feb 03 2006 Lenny Cartier <lenny@mandriva.com> 6.0.2-0.RC4.1mdk
- 6.0.2RC4

* Mon Jan 02 2006 Oden Eriksson <oeriksson@mandriva.com> 6.0.1-4mdk
- rebuilt against soname aware deps (tcl/tk)
- fix deps

* Mon Oct 03 2005 Nicolas Lécureuil <neoclust@mandriva.org> 6.0.1-3mdk
- Fix BuildRequires

* Mon Oct 03 2005 Nicolas Lécureuil <neoclust@mandriva.org> 6.0.1-2mdk
 Fix BuildRequires

* Sat Oct 01 2005 Lenny Cartier <lenny@mandriva.com> 6.0.1-1mdk
- 6.0.1

* Tue May 03 2005 Nicolas Lécureuil <neoclust@mandriva.org> 6.0.0-3mdk
- Rebuild ( Fix #15790)

* Wed Mar 16 2005 Buchan Milne <bgmilne@linux-mandrake.com> 6.0.0-2mdk
- rebuild

* Tue Mar 15 2005 Buchan Milne <bgmilne@linux-mandrake.com> 6.0.0-1mdk
- 6.0.0
- better requires
- cleanups and merge with changes in grass57
- use mkrel
- obsolete grass57

* Fri Jul 16 2004 Buchan Milne <bgmilne@linux-mandake.com> 5.0.3-3mdk
- rebuild for gcc-3.4

* Sat Dec 20 2003 Per Øyvind Karlsen <peroyvind@linux-mandrake.com> 5.0.3-2mdk
- rebuild to get correct dependencies

* Sun Dec 14 2003 Per Øyvind Karlsen <peroyvind@linux-mandrake.com> 5.0.3-1mdk
- 5.0.3
- drop P1, fixed upstream
- don't bzip2 icons in src.rpm
- fix buildrequires (lib64..)
- add postgresql-devel to buildrequires
- fix build against latest freetype (P2)

* Fri Apr 11 2003 Buchan Milne <bgmilne@linux-mandrake.com> 5.0.2-1mdk
- 5.0.2
- Rebuild for tcl8.4
- Misc fixes (incl xpm->png)
- try and fix misdetection of readline
- Hack for tcl 8.4/nviz, may be broken, will find out on Sunday ...
- Buildrequires flex and bison (the value of a clean install ;-))

* Sat Feb 22 2003 Buchan Milne <bgmilne@linux-mandrake.com> 5.0.0-3mdk
- Require xterm

* Fri Feb 21 2003 Buchan Milne <bgmilne@linux-mandrake.com> 5.0.0-2mdk
- Buildrequire libblas-devel
- Rebuild for postgres

* Wed Sep 04 2002 Buchan Milne <bgmilne@linux-mandrake.com> 5.0.0-1mdk
- 5.0.0
- buildrequire libgd2-devel (24bit PNG driver)

* Sun Aug 18 2002 Buchan Milne <bgmilne@linux-mandrake.com> 5.0.0-0.pre5.3mdk
- Add back gdal, lapack and blas support (aka I build too seldom on klama!).

* Fri Aug 16 2002 Buchan Milne <bgmilne@linux-mandrake.com> 5.0.0-0.pre5.2mdk
- build with dbmi, glw and readline support
- require gcc-g77 for lapack and blas
- gdal,lapack,blas support optional (klama frozen :-().

* Mon Aug 05 2002 Buchan Milne <bgmilne@linux-mandrake.com> 5.0.0-0.pre5.1mdk
- Rebuild
- Buildrequires gdbm, lapack

* Tue Jun 25 2002 Buchan Milne <bgmilne@cae.co.za> 5.0.0-0.pre5.0mdk
- pre5

* Tue May 14 2002 Buchan Milne <bgmilne@cae.co.za> 5.0.0-0.pre4.0mdk
- New prerelase
- PostgreSQL,lapack,gdal support
- Next stop, subpackages

* Mon Apr 14 2002 Buchan Milne <bgmilne@cae.co.za>  5.0.0-0.pre3.0mdk
- First stab at a Mandrake RPM
- No postgres support (doesn't build against 7.2 but does against 7.1)
- Probably need subpackages for nvis, tcltkgrass, odbc, postgres etc
