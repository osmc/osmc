all:
	@echo "Specify a target:\nmake trans\nmake armv6l\nmake armv7\nmake aarch64"

trans:
	sudo bash build.sh "trans"

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
