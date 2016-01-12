all:
	@echo "Specify a target:\nmake armv7\nmake gen"

armv7:
	sudo bash build.sh "armv7"

gen:
	sudo bash build.sh "gen"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
