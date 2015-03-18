all:
	echo -e "Specify a target:\nmake rbp\nmake rbp2\nmake vero1"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

vero1:
	sudo bash build.sh "vero1"

clean:
	sudo rm -rf buildroot*/ > /dev/null 2>&1
	sudo rm -f *.img.gz >/dev/null 2>&1
	sudo rm -f *.md5 >/dev/null 2>&1
