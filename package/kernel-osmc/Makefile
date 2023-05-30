all:
	@echo "Specify a target:\nmake rbp2\nmake rbp464\nmake vero364\nmakevero564"

rbp2:
	sudo bash build.sh "rbp2"

rbp464:
	sudo bash build.sh "rbp464"

vero364:
	sudo bash build.sh "vero364"

vero564:
	sudo bash build.sh "vero564"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files-image/lib >/dev/null 2>&1
	sudo rm -rf files-image/boot >/dev/null 2>&1
	sudo rm -rf files-headers/usr/ >/dev/null 2>&1
	sudo rm -rf files-headers-sanitised/usr >/dev/null 2>&1
	sudo rm -rf files-source/usr >/dev/null 2>&1
	sudo rm -rf files-debug/var >/dev/null 2>&1
