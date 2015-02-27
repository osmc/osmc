all:
	echo -e "Specify a target:\nmake rbp1\nmake armv7\nmake i386"

rbp1:
	sudo bash build.sh "rbp1"

armv7:
	sudo bash build.sh "armv7"

i386:
	sudo bash build.sh "gen"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files-dev/usr > /dev/null 2>&1
