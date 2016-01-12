all:
	@echo "Specify a target:\nmake rbp1\nmake rbp2\nmake vero\nmake atv"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

vero:
	sudo bash build.sh "vero"

vero2:
	sudo bash build.sh "vero2"

atv:
	sudo setarch i686 bash build.sh "atv"

pc:
	sudo bash build.sh "pc"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files-image >/dev/null 2>&1
	sudo rm -rf files-headers >/dev/null 2>&1
