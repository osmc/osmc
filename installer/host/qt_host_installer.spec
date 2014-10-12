#
# spec file for package qt_host_installer
#
# Copyright (c) 2014 email@samnazarko.co.uk
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

%define oscm_tar src

Summary: OSMC Installer for Linux
Name: qt_host_installer
Version: 1
Release: PLACEHOLDER
Source: %{osmc_tar}.tar.gz
URL: https://github.com/samnazarko/osmc
License: GPL-2
Group: Multimedia
%if 0%{?suse_version} == 1110
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
%endif

%if 0%{?mandriva_version} > 2006  
export PATH=/usr/lib/qt4/bin:$PATH  
export QTDIR=%{_prefix}/lib/qt4/  
%endif  

%if 0%{?mandriva_version}
BuildRequires:    libqt4-devel -kernel 
%endif

%if 0%{?suse_version}
BuildRequires: update-desktop-files
%endif

# Common build dependencies
BuildRequires:	libqt4-devel
BuildRequires:	desktop-file-utils
BuildRequires:	gcc-c++

# Common dependencies
Requires:       libjpeg8
Requires:       libmng
Requires:       libtiff5
Requires:       libpng12-0
Requires: 		qt
# Patch
Requires:       patch
# Don't make this noarch although it is just a fetch script otherwise the following wont' work:

# dep_postfix macro is used to append "-32bit" to dependencies for x86_64 on openSUSE
%define dep_postfix %{nil}
%ifarch x86_64
  %if 0%{?suse_version}
    %define dep_postfix -32bit
  %endif
  %if 0%{?fedora}
    %define dep_postfix (x86-32)
  %endif
%endif

%if 0%{?suse_version}
Requires:       libX11-6%{dep_postfix} >= 1.4.99.1
%endif
%if 0%{?fedora}
Requires:       libX11%{dep_postfix} >= 1.4.99.1
%endif


%description
OSMC Installer

%prep
%setup -n %{osmc_tar}

%build
if [ -f Makefile ]; then echo "Cleaning" && make clean; fi
echo Building installer
%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
qmake-qt4
%else
qmake
%endif
make
if [ $? != 0 ]; then echo "Build failed" && exit 1; fi

%check
#make check

%install
rm -rf %{buildroot}

#foldes
install -d %{buildroot}%{_bindir}/osmc
install -d %{buildroot}%{_libdir}/osmc
install -d %{buildroot}/usr/share/applications

#files
install -m 755 %{name} %{buildroot}%{_bindir}/osmc/%{name}
ln -s %{_bindir}/osmc/%{name} %{buildroot}%{_bindir}/%{name}

#lenguage files for qt
#install -m 644 *.qm %{buildroot}%{_bindir}/osmc/
#cp *.qm %{buildroot}%{_bindir}/osmc/ > /dev/null 2>&1

#desktop icon
%if 0%{?suse_version}
%suse_update_desktop_file -i osmcinstaller
%endif

%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
desktop-file-install --dir=${RPM_BUILD_ROOT}%{_datadir}/applications osmcinstaller.desktop
desktop-file-validate %{buildroot}%{_datadir}/applications/osmcinstaller.desktop
%endif

#icon image
install -m 644 icon.png %{buildroot}%{_bindir}/osmc/icon.png

%clean
rm -rf $RPM_BUILD_ROOT

%post
%if 0%{?suse_version} >= 1140
%desktop_database_post
#%icon_theme_cache_post
#%mime_database_post
%endif

%postun
%if 0%{?suse_version} >= 1140
%desktop_database_postun
#%icon_theme_cache_postun
#%mime_database_postun
%endif

%files
%defattr(-,root,root,0755)
%{_bindir}/osmc
%defattr(-,root,root)
%{_bindir}/osmc/%{name}
%{_bindir}/%{name}
#%{_bindir}/osmc/*.qm
/usr/share/applications/osmcinstaller.desktop
%{_bindir}/osmc/icon.png
#%{_libdir}/osmc/*


%changelog
* Mon Oct 12 2014 Sam G. Nazarko
- OBS initial release, thanks to Ivan Gonzalez (malkavi) for help
