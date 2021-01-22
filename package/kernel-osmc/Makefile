all:
	@echo "Specify a target:\nmake rbp2\nmake vero364"

rbp2:
	sudo bash build.sh "rbp2"

rbp464:
	sudo bash build.sh "rbp464"

vero364:
	sudo bash build.sh "vero364"

pc:
	sudo bash build.sh "pc"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files-image >/dev/null 2>&1
	sudo rm -rf files-headers >/dev/null 2>&1
