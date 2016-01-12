all:
	@echo "Specify a target:\nmake rbp1\nmake rbp2\nmake i386\nmake vero"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

i386:
	sudo setarch i686 bash build.sh "i386"

vero:
	sudo bash build.sh "vero"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files-dev/usr > /dev/null 2>&1
