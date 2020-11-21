# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

MESA_REV="663d464366675bf6d44c5d4d00e04cbdfa3f6057"
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
	handle_dep "bison"
	# Mount procfs so we can get correct core count for meson
	# Need meson build 0.52 or later
	if [ ! -f /usr/local/bin/meson ]
	then
		pip3 install meson
		if [ $? != 0 ]; then echo "Could not install meson build system" && exit 1; fi
		ln -s /usr/local/bin/meson /usr/bin/meson
	fi
	echo "Package: ${1}-mesa-osmc" >> files/DEBIAN/control && echo "Package: ${1}-mesa-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/mesa-*
	rm -rf build
	mkdir -p build
	pushd build
	if [ "$1" == "rbp2" ]; then
		meson --prefix=/usr/osmc \
		--libdir=/usr/osmc/lib \
		--includedir=/usr/osmc/include \
		-Ddri-drivers="" \
		-Dgallium-drivers=vc4,v3d,kmsro \
		-Dgallium-extra-hud=false \
		-Dgallium-xvmc=false \
		-Dgallium-omx=disabled \
		-Dgallium-nine=false \
		-Dgallium-opencl=disabled \
		-Dvulkan-drivers= \
		-Dshader-cache=true \
		-Dshared-glapi=true \
		-Dopengl=true \
		-Dgbm=true \
		-Degl=true \
		-Dvalgrind=false \
		-Dlibunwind=false \
		-Dlmsensors=false \
		-Dbuild-tests=false \
		-Dselinux=false \
		-Dosmesa=none \
		-Dplatforms=drm -Ddri3=false -Dglx=disabled -Dglvnd=false \
		-Dllvm=false \
		-Dgallium-vdpau=false \
		-Dgallium-va=false \
		-Dgallium-xa=false \
		-Dgles1=false \
		-Dgles2=true
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
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-mesa-osmc.deb
	dpkg_build files-dev ${1}-mesa-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
