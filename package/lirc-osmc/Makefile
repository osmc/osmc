all:
	echo -e "Specify a target:\nmake rbp\nmake gen"

rbp:
	sudo bash build.sh "rbp"

gen:
	sudo bash build.sh "gen"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
