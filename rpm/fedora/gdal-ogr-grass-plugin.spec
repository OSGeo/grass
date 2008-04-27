# GDAL/OGR support for GRASS: plugin
# This file is Free Software under GNU GPL v>=2.

%define GRASS_VERSION 6.2.2
%define PACKAGE_NAME gdal-grass
%define PACKAGE_VERSION 1.4.1
%define PACKAGE_RELEASE 1

#Query the RPM database to find which redhat/fedora release we are running.
%if %(rpmquery fedora-release | grep -cv 'not installed$')
    %define FCL 1
    %define VER1 %(rpmquery --qf '%{VERSION}' fedora-release)
%endif
%if %(rpmquery redhat-release | grep -v 'not installed$' | grep -c -e '-[0-9][AEW]S')
    %define ENT 1
    %define VER1 %(rpmquery --qf '%{VERSION}' redhat-release|cut -c1)
%endif
%if %(rpmquery sl-release | grep -v 'not installed$' | grep -c -e '-[0-9]')
    %define SLC 1
    %define VER1 %(rpmquery --qf '%{VERSION}' sl-release|cut -c1-)
%endif

Summary:	GDAL/OGR support for GRASS
Name:		%PACKAGE_NAME
Version:	%PACKAGE_VERSION
Epoch:		%PACKAGE_RELEASE
%{?FCL:Release: %{PACKAGE_RELEASE}.fc%{VER1}}
%{?ENT:Release: %{PACKAGE_RELEASE}.E%{VER1}}
%{?SLC:Release: %{PACKAGE_RELEASE}.SL%{VER1}}
Group:		Sciences/Geosciences
Source0:	gdal-grass-%{version}.tar.bz2
License:	MIT licence; Copyright by Frank Warmerdam
URL:		http://www.gdal.org/dl/
Packager:	Brad Douglas <rez@touchofmadness.com>
BuildRoot:	%{_builddir}/%{name}-root
Prefix:		%{_prefix}
Requires: proj >= 4.5.0
Requires: python >= 2.4
Requires: zlib >= 1.2.3
BuildRequires: proj-devel >= 4.5.0
BuildRequires: python-devel >= 2.4
BuildRequires: zlib-devel >= 1.2.3

%description
GDAL and OGR support for GRASS GIS created from gdal-%PACKAGE_VERSION

%prep
%setup -q

# Building GRASS Plugin
AUTOLOAD=%{buildroot}/%{_prefix}/lib/gdalplugins
mkdir -p ${AUTOLOAD}

%configure \
 --prefix=%{_prefix} \
 --enable-shared     \
 --with-grass=%{_prefix}/grass-%{GRASS_VERSION} \
 --with-gdal=%{_prefix}/bin/gdal-config \
 --with-autoload=${AUTOLOAD}

%build
make prefix=%{buildroot}/%{_prefix} BINDIR=%{buildroot}/%{_bindir} \
PREFIX=%{buildroot}%{_prefix} install

%install
make prefix=%{buildroot}/%{_prefix} BINDIR=%{buildroot}/%{_bindir} \
PREFIX=%{buildroot}%{_prefix} install

# make plugin available on system
install -d %{buildroot}/etc/ld.so.conf.d
echo "%{_prefix}/lib/gdalplugins" > %{buildroot}/etc/ld.so.conf.d/gdal-grass.conf

%clean
rm -rf %{_builddir}/%{PACKAGE_NAME}-%{version}
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_prefix}/lib/gdalplugins/gdal_GRASS.so
%{_prefix}/lib/gdalplugins/ogr_GRASS.so
/etc/ld.so.conf.d/gdal-grass.conf

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%changelog
* Tue Jul 17 2007 Brad Douglas <rez@touchofmadness.com>
- updated to gdal-grass 1.4.1
- updated to GDAL 1.4.2 - GRASS 6.2.2
* Fri Oct 20 2006 Markus Neteler <neteler@itc.it>
- updated to GDAL 1.3.2 - GRASS 6.2.0; fixed /etc/ld.so.conf.d/gdal-grass.conf
* Tue Oct 11 2005 Otto Dassau 1.3.1-1
- adaptions for gdal-grass package
* Tue Aug 24 2005 Otto Dassau 1.3.0-1
- adaptions for python support
* Tue Aug 9 2005 Otto Dassau 1.2.6-1
- adapted to SuSE93
* Fri Jul 8 2005 Otto Dassau 1.2.6-1
- adapted to SuSE93
* Sun Mar 20 2005 Markus Neteler 1.2.6-1
- updated to GDAL 1.2.6, AUTOLOAD
