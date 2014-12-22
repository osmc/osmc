all:
	echo -e "Specify a target:\nmake rbp\nmake armv7\nmake gen"

rbp:
	sudo bash build.sh "rbp"

armv7:
	sudo bash build.sh "armv7"

gen:
	sudo bash build.sh "gen"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files-dev/usr > /dev/null 2>&1
