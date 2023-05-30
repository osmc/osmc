eall:
	@echo "Specify a target:\nmake rbp2\nmake rbp4\nmake vero3\nmake vero5"

rbp2:
	sudo bash build.sh "rbp2"

rbp4:
	sudo bash build.sh "rbp4"

vero3:
	sudo bash build.sh "vero3"

vero5:
	sudo bash build.sh "vero5"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files-debug/usr > /dev/null 2>&1
