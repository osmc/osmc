all:
	@echo "Specify a target:\nmake rbp2\nmake vero3\nmake vero5\n"

rbp2:
	sudo bash build.sh "rbp2"

vero3:
	sudo bash build.sh "vero3"

vero5:
	sudo bash build.sh "vero5"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files-dev/usr > /dev/null 2>&1
