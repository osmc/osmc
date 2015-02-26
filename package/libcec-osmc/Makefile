all:
	echo -e "Specify a target:\nmake rbp1\nmake rbp2\nmake gen"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

gen:
	sudo bash build.sh "gen"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files-dev/usr > /dev/null 2>&1
