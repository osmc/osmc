all:
	@echo "Specify a target:\nmake i386"

i386:
	sudo setarch i686 bash build.sh "i386"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files-dev/usr > /dev/null 2>&1
	sudo rm -rf files/lib/firmware >/dev/null 2>&1
