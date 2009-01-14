%define shortver 63

Summary:	GRASS - Geographic Resources Analysis Support System
Name:		grass
Version:	6.3.0
Release:	1
License:	GPL
Group:		Applications/GIS
URL:		http://grass.osgeo.org/
Source:		grass-%{version}.tar.gz
Patch:          grass-%{version}_wx-vdigit.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Packager:       Otto Dassau <otto.dassau@gmx.de>
Requires:       libgdal >= 1.5.0
Requires:       tcl >= 8.4
Requires:       tk >= 8.4
Requires:       proj4 >= 4.5.0
Requires:       sqlite >= 3
Requires:       xterm
Requires:       unixODBC
Requires:       fftw3
Requires:       netcdf
Requires:       libpng
Requires:       libtiff
Requires:       libjpeg
Requires:       readline
Requires:	wxPython2.8-gtk2-unicode
Requires:       python-numeric
BuildRequires:  bison
BuildRequires:  gcc-c++
BuildRequires:  flex
BuildRequires:  freetype2-devel
BuildRequires:  libjpeg-devel
BuildRequires:  libgdal-devel
BuildRequires:  libpng-devel >= 1.2.12
BuildRequires:  man
BuildRequires:  readline-devel
BuildRequires:  libpng-devel
BuildRequires:  libtiff-devel
%if 0%{?suse_version} >= 1030
BuildRequires:  libnetcdf-devel
%else
BuildRequires:  netcdf
%endif
BuildRequires:  libjpeg-devel
BuildRequires:  tcl-devel
BuildRequires:  tk-devel
BuildRequires:  libproj-devel proj4
BuildRequires:  ncurses-devel >= 5.5
BuildRequires:  zlib-devel
BuildRequires:  libtiff-devel
BuildRequires:  xorg-x11-Mesa-devel
BuildRequires:  sqlite-devel
BuildRequires:  unixODBC-devel
BuildRequires:  postgresql-devel
BuildRequires:  mysql mysql-devel
BuildRequires:  fftw3 fftw3-devel
BuildRequires:  fdupes perl
BuildRequires:  python-devel swig
BuildRequires:	wxPython2.8-devel-gtk2-unicode

%package docs
Summary:        Documentation for grass
Group:          Applications/GIS
Requires:       grass = %{version}

%package devel
Summary:        Development files for grass
Group:          Development/Libraries
Requires:       grass = %{version}

%debug_package

%description
GRASS (Geographic Resources Analysis Support System), commonly
referred to as GRASS, is a Geographic Information System
(GIS) used for geospatial data management and analysis, image
processing, graphics/maps production, spatial modeling, and
visualization. GRASS is currently used in academic and commercial
settings around the world, as well as by many governmental agencies
and environmental consulting companies.

%description devel
This package contains the development files for grass

%description docs
This package contains the HTML documentation files for grass

%prep
%setup -n grass-%{version}
%patch -p1
%define grasver -@GRASS_VERSION_MAJOR@.@GRASS_VERSION_MINOR@.@GRASS_VERSION_RELEASE@
%define grasver2 '-${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE}'

sed s/%{grasver}//g include/Make/Platform.make.in >_tmp
mv _tmp include/Make/Platform.make.in

sed s/%{grasver}//g grass.pc.in >_tmp
mv _tmp grass.pc.in

sed s/%{grasver2}//g configure >_tmp
mv _tmp configure
chmod +x configure

sed s/%{grasver2}//g Makefile >_tmp
mv _tmp Makefile

%define _prefix /opt
%define grassdir %{_prefix}/grass
%define grasslib %{_prefix}/grass/lib

#configure with shared libs:
CFLAGS="-O2" ./configure \
	--prefix=%{_prefix} \
	--enable-shared \
	--enable-largefile \
	--with-proj-share=/usr/share/proj \
	--with-cxx \
	--with-gdal=/usr/bin/gdal-config \
	--with-postgres --with-postgres-includes=/usr/include/pgsql \
	--with-mysql --with-mysql-includes=/usr/include/mysql \
	--with-fftw \
	--with-readline \
	--with-netcdf \
	--with-curses \
	--with-nls \
	--with-sqlite \
	--with-freetype \
	--with-freetype-includes=/usr/include/freetype2 \
	--with-odbc \
	--with-python \
	--with-wxwidgets=/usr/lib/wxPython-2.8.7.1-gtk2-unicode/bin/wx-config	

%build
make prefix=%{_prefix} BINDIR=%{_bindir} PREFIX=%{_prefix}

%install
make prefix=%{buildroot}%{_prefix} BINDIR=%{buildroot}%{_bindir} \
PREFIX=%{buildroot}%{_prefix} install

# changing GISBASE in startup script (deleting %{buildroot} from path)
mkdir %{buildroot}/usr/bin -p
cat %{buildroot}%{_prefix}/bin/grass%{shortver} |
	sed s:%{buildroot}::g > %{buildroot}/usr/bin/grass%{shortver}
rm %{buildroot}%{_prefix}/bin/grass%{shortver}
chmod +x %{buildroot}/usr/bin/grass%{shortver}
ln -s grass%{shortver} %{buildroot}/usr/bin/grass

# make grass libraries available on the system
install -d %{buildroot}/etc/ld.so.conf.d
echo %{grasslib} >> %{buildroot}/etc/ld.so.conf.d/grass-%{version}.conf
pushd %{buildroot}%{grassdir}/man
  gzip */*
popd
pushd %{buildroot}%{grassdir}/etc/gem/skeleton/
chmod +x uninstall
chmod +x post
popd

# this is possibly dangerous
# make sure that no changeable files are linked
fdupes -r %{buildroot}%{grassdir} | perl -ne \
'chomp;if(!$_){my $f=shift @a;while($_=shift @a){system "ln -f $f $_";}}else{push @a,$_;}'

%clean
rm -rf %{buildroot}

%files devel
%defattr(-,root,root)
%{grassdir}/include
%{grasslib}/*.a

%files docs
%defattr(-,root,root)
%{grassdir}/docs
%{grassdir}/man

%files
%defattr(-,root,root)
/etc/ld.so.conf.d/grass-%{version}.conf
%{grassdir}/bin/*
%{grassdir}/etc/*
%{grassdir}/scripts/*
%{grassdir}/bwidget/*
#%{grassdir}/locale/*
%lang(ar) %{grassdir}/locale/ar/
%lang(cs) %{grassdir}/locale/cs/
%lang(de) %{grassdir}/locale/de/
%lang(el) %{grassdir}/locale/el/
%lang(es) %{grassdir}/locale/es/
%lang(fr) %{grassdir}/locale/fr/
%lang(hi) %{grassdir}/locale/hi/
%lang(it) %{grassdir}/locale/it/
%lang(ja) %{grassdir}/locale/ja/
%lang(ko) %{grassdir}/locale/ko/
%lang(lv) %{grassdir}/locale/lv/
%lang(mr) %{grassdir}/locale/mr/
%lang(pl) %{grassdir}/locale/pl/
%lang(pt) %{grassdir}/locale/pt/
%lang(pt_br) %{grassdir}/locale/pt_br/
%lang(ru) %{grassdir}/locale/ru/
%lang(sl) %{grassdir}/locale/sl/
%lang(th) %{grassdir}/locale/th/
%lang(tr) %{grassdir}/locale/tr/
%lang(vi) %{grassdir}/locale/vi/
%lang(zh) %{grassdir}/locale/zh/
%{grassdir}/driver/*
%{grassdir}/fonts/*
%{grasslib}/*.so
/usr/bin/grass*
#%exclude %{grasslib}/*[a-zA-Z].so
#%doc AUTHORS COPYING GPL.TXT README REQUIREMENTS.html
%{grassdir}/AUTHORS
%{grassdir}/CHANGES
%{grassdir}/COPYING
%{grassdir}/GPL.TXT
%{grassdir}/REQUIREMENTS.html
%defattr(755,root,root)
%{_prefix}/bin/*

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%changelog
* Thu Apr 23 2008 Otto Dassau <otto.dassau@gmx.de> 6.3.0.1
- update to 6.3 with new wxpython as standard gui
- patch to comment wxgui vdigit in Makefile
* Thu Dec 20 2007 Otto Dassau 6.2.3
- added support for netcdf, readline and LFS
* Mon Aug 13 2007 Dirk Stöcker 6.2.2
- adapted for openSUSE build service
* Mon Jul 16 2007 Otto Dassau 6.2.2
- first build of grass 6.2.2 bugfix release
* Fri Jan 05 2007 Otto Dassau 6.2.1
- split into devel and docs packages
- removed ld.so.conf entry and added ld.so.conf.d/grass* 
* Tue Dec 19 2006 Otto Dassau 6.2.1
- updated to SuSE 10.2
* Wed Jan 25 2006 Otto Dassau 6.0.2RC4
- updated to SuSE 10.0
* Thu Nov 17 2005 Markus Neteler 6.1.cvs
- upgraded to Mandriva 2006, 6.1.cvs
* Fri Aug 05 2005 Otto Dassau 6.0.1RC2
- changed prefix 
* Fri Aug 05 2005 Markus Neteler 6.0.1RC1
- updated to GRASS 6.0.1
* Fri Mar 11 2005 Markus Neteler 6.0.0-1
- updated to GRASS 6.0.0
* Tue Nov 9 2004 Markus Neteler 5.7.0-2
- GRASS 5.3 no longer required as all code moved into this repository
* Wed Jun 17 2004 Markus Neteler 5.7.0-1
- removed unixODBC, added mysql
- specfile cleanup
* Tue May 24 2004 Markus Neteler 5.7.0-1beta4
- rewritten from 5.3 specs
