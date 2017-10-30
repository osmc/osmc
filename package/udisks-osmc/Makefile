all:
	@echo "Specify a target:\nmake armv6l\nmake armv7\nmake aarch64\n"

armv6l:
	sudo bash build.sh "armv6l"

armv7:
	sudo bash build.sh "armv7"

amd64:
	sudo bash build.sh "amd64"

aarch64:
	sudo bash build.sh "aarch64"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files/lib > /dev/null 2>&1
	sudo rm -rf files/etc > /dev/null 2>&1
	sudo rm -rf files/sbin >/dev/null 2>&1
