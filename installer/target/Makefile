all:
	@echo "Specify a target:\nmake rbp1\nmake rbp2\nmake vero1\nmake appletv"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

vero1:
	sudo bash build.sh "vero1"

appletv:
	sudo bash build.sh "appletv"

clean:
	sudo rm -rf buildroot*/ > /dev/null 2>&1
	sudo rm -f *.img.gz >/dev/null 2>&1
	sudo rm -f *.md5 >/dev/null 2>&1
