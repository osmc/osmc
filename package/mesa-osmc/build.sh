# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

MESA_REV="ea9f8c26bc952c6e0e24e086e364d16aa7841bd9"
pull_source "https://github.com/mesa3d/mesa/archive/${MESA_REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "mesa-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building mesa-osmc"
	out=$(pwd)/files
	if [ -d files/usr ]; then rm -rf files/usr; fi
	if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-mesa-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "libdrm-dev"
	handle_dep "python3-pip"
	handle_dep "python3-mako"
	handle_dep "flex"
	handle_dep "pkg-config"
	handle_dep "bison" # meson dep
	handle_dep "git" # meson dep
	handle_dep "cmake" # meson dep
	handle_dep "zlib1g-dev"
	handle_dep "libexpat1-dev"
	handle_dep "meson"
	echo "Package: ${1}-mesa-osmc" >> files/DEBIAN/control && echo "Package: ${1}-mesa-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/mesa-*
	install_patch "../../patches" "{$1}"
	rm -rf build
	mkdir -p build
	pushd build
	FLAGS=-Wl,-rpath=/usr/osmc/lib
	export CFLAGS=$FLAGS
	export CXXFLAGS=$FLAGS
	export CPPFLAGS=$FLAGS
	if [ "$1" == "rbp2" ]; then
		meson setup \
		--prefix=/usr/osmc \
		--libdir=/usr/osmc/lib \
		-Dgallium-drivers=vc4,v3d,kmsro \
		-Dgallium-extra-hud=false \
		-Dgallium-omx=disabled \
		-Dgallium-nine=false \
		-Dgallium-opencl=disabled \
		-Dshader-cache=enabled \
		-Dshared-glapi=enabled \
		-Dopengl=true \
		-Dgbm=enabled \
		-Degl=enabled \
		-Dvalgrind=disabled \
		-Dlibunwind=disabled \
		-Dlmsensors=disabled \
		-Dbuild-tests=false \
		-Ddraw-use-llvm=false \
		-Dselinux=false \
		-Dosmesa=false \
		-Dplatforms="" \
		-Ddri3=disabled \
		-Dglx=disabled \
		-Dglvnd=false \
		-Dllvm=disabled \
		-Dgallium-vdpau=disabled \
		-Dgallium-va=disabled \
		-Dgallium-xa=disabled \
		-Dgles1=disabled \
		-Dgles2=enabled \
		-Dvulkan-drivers=""
	fi
	if [ $? != 0 ]; then echo -e "MESA configuration failed" && exit 1; fi
	ninja
	if [ $? != 0 ]; then echo -e "MESA build failed" && exit 1; fi
	DESTDIR=$out ninja install
	if [ $? != 0 ]; then echo -e "MESA installation failed" && exit 1; fi
	popd
	popd
	mkdir -p files-dev/usr/osmc
	mv files/usr/osmc/include  files-dev/usr/osmc
        strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-mesa-osmc.deb
	dpkg_build files-dev ${1}-mesa-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
