all:
	@echo "Specify a target:\nmake armv7\nmake armv6l\nmake amd64\nmake i386"

armv6l:
	sudo bash build.sh "armv6l"

armv7:
	sudo bash build.sh "armv7"

amd64:
	sudo bash build.sh "amd64"

i386:
	sudo setarch i686 bash build.sh "i386"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files-dev/usr > /dev/null 2>&1
