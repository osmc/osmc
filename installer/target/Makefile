all:
	echo -e "Specify a target:\nmake rbp\nmake rbp2\nmake imx6"

rbp:
	sudo bash build.sh "rbp"

rbp2:
	sudo bash build.sh "rbp2"

imx6:
	sudo bash build.sh "imx6"

clean:
	sudo rm -rf buildroot*/ > /dev/null 2>&1
	sudo rm -f *.img.gz >/dev/null 2>&1
	sudo rm -f *.md5 >/dev/null 2>&1
