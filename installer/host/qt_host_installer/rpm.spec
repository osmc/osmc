Summary: OSMC Installer
Name: qt_host_installer
Version: 1
Release: 1
Source: %{name}.tar.gz
URL: https://github.com/samnazarko/osmc
License: GPLv2
Group: Multimedia

BuildRequires:  update-desktop-files

Requires:       libjpeg8
Requires:       libmng
Requires:       libtiff5
Requires:       libpng12-0
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
%setup -n %{name}

%build
if [ -f Makefile ]; then echo "Cleaning" && make clean; fi
echo Building installer
qmake
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
#install -m 755 osmcinstaller.desktop %{buildroot}/usr/share/applications/osmcinstaller.desktop
%suse_update_desktop_file -i osmcinstaller
#With this we can create a new desktop file for the rpm
##suse_update_desktop_file -c osmcinstaller_rpm "OSMC Installer" "OSMC Installer" "/usr/bin/xdg-su -c /usr/bin/osmc/%{name}" "/usr/bin/osmc/icon.png" "AudioVideo;Video;Player;TV"

#icon image
install -m 644 icon.png %{buildroot}%{_bindir}/osmc/icon.png

VERSION=$(cat %{name}.pro | grep VERSION | tail -n 1 | awk {'print $3'})
echo "Version: ${VERSION}"

%clean
rm -rf $RPM_BUILD_ROOT

%post
%desktop_database_post
%icon_theme_cache_post
#%mime_database_post

%postun
%desktop_database_postun
%icon_theme_cache_postun
#%mime_database_postun

%files
%defattr(-,root,root)
%{_bindir}/osmc/%{name}
%{_bindir}/%{name}
#%{_bindir}/osmc/*.qm
/usr/share/applications/osmcinstaller.desktop
%{_bindir}/osmc/icon.png
#%{_libdir}/osmc/*


%changelog
* Thu Sep 25 2014 Ivan Gonzalez <ig@example.com> 0.8.18.1-0.1
- Initial RPM release
