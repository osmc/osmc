Summary: Apple Filing Protocol client
Distribution: Fedora 8
Name: afpfs-ng
Version: 0.8.1
Release: 1
URL: http://sourceforge.net/projects/afpfs-ng/
Source0: %{name}-%{version}.tar.bz2
License: GPL
Group: System Environment/Base
BuildRoot: %{_tmppath}/%{name}-root
Packager: Alex deVries <alexthepuffin@gmail.com>
BuildRequires: fuse-devel libgcrypt-devel gmp-devel readline-devel
Requires: libgcrypt gmp readline

%description
afpfs-ng is an Apple Filing Protocol client that will allow Linux and BSD systems to see files exported from a Mac OS system or netatalk with AFP over TCP.

%package devel
Summary: Headers for AFP clients
Group: Development/Libraries

%description devel
afpfs-ng development files for new clients

%prep
%setup -q

%build
automake --add-missing --include-deps --foreign
autoconf
%configure
make

%install
%makeinstall
mkdir -p %{RPM_BUILD_ROOT}/usr/share/man/man1
mkdir -p %{RPM_BUILD_ROOT}/usr/include/afpfs-ng
mkdir -p $RPM_BUILD_ROOT/usr/include/afpfs-ng
cp include/afp.h $RPM_BUILD_ROOT/usr/include/afpfs-ng

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/%{name}-%{version}/

%files
%defattr(-,root,root)
/usr/bin/*
/usr/lib/*
/usr/share/man/*

%doc COPYING AUTHORS ChangeLog docs/README docs/performance docs/FEATURES.txt docs/REPORTING-BUGS.txt

%files devel
/usr/include/afpfs-ng/afp.h
/usr/lib/libafpclient.a
/usr/lib/libafpclient.la


%changelog
* Fri Feb 15 2008 Alex deVries <alexthepuffin@gmail.com>.
- Updated to 0.8

* Sat Mar 31 2007 Alex deVries <alexthepuffin@gmail.com>.
- Updated to 0.4.1

* Sun Feb 11 2007 Alex deVries <alexthepuffin@gmail.com>
- Updated to 0.4

* Tue Nov 28 2006 Houritsuchu <houritsuchu@hotmail.com>
- Initial build.
