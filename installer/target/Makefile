all:
	@echo "Specify a target:\nmake rbp2\nmake rbp4\nmake vero3\nmake vero5\nmake vero5provision\n"

rbp2:
	sudo bash build.sh "rbp2"

rbp4:
	sudo bash build.sh "rbp4"

vero3:
	sudo bash build.sh "vero3"

vero5:
	sudo bash build.sh "vero5"

vero5provision:

	sudo PROVISION=1 bash build.sh "vero5"

clean:
	sudo rm -rf buildroot*/ > /dev/null 2>&1
	sudo rm -f filesystem.tar.xz >/dev/null 2>&1
	sudo rm -f *.img.gz >/dev/null 2>&1
	sudo rm -f *.md5 >/dev/null 2>&1
