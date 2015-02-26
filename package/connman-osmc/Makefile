all:
	echo -e "Specify a target:\nmake rbp1\nmake armv7\nmake atv\n"

rbp1:
	sudo bash build.sh "rbp1"

armv7:
	sudo bash build.sh "armv7"

atv:
	sudo bash build.sh "atv"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
