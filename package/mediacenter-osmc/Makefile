all:
	@echo "Specify a target:\nmake rbp1\nmake rbp2\nmake atv\nmake vero\nmake vero2\n"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

atv:
	sudo setarch i686 bash build.sh "atv"

vero:
	sudo bash build.sh "vero"

vero2:
	sudo bash build.sh "vero2"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files-debug/usr > /dev/null 2>&1
