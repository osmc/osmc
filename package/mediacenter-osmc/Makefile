all:
	@echo "Specify a target:\nmake rbp1\nmake rbp2\n\make atv\nmake vero\n"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

atv:
	sudo bash build.sh "atv"

vero:
	sudo bash build.sh "vero"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
