all:
	echo -e "Specify a target:\nmake rbp\nmake imx6"

rbp:
	sudo bash build.sh "rbp"

imx6:
	sudo bash build.sh "imx6"

clean:
	sudo rm -rf buildroot*/ > /dev/null 2>&1
	sudo rm -f *.img.gz >/dev/null 2>&1
	sudo rm -f *.md5 >/dev/null 2>&1
