all:
	@echo "Specify a target:\nmake armv6l\nmake armv7\nmake i386\nmake amd64\n"

armv6l:
	sudo bash build.sh "armv6l"

armv7:
	sudo bash build.sh "armv7"

i386:
	sudo setarch i686 bash build.sh "i386"

amd64:
	sudo bash build.sh "amd64"

aarch64:
	sudo bash build.sh "aarch64"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/etc/dbus-1 >/dev/null 2>&1
	sudo rm -rf files/usr/lib >/dev/null 2>&1
	sudo rm -rf files/usr/bin >/dev/null 2>&1
	sudo rm -f files/usr/share/alsa/alsa.conf.d/20-bluealsa.conf >/dev/null 2>&1
