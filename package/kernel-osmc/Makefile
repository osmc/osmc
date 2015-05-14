all:
	echo -e "Specify a target:\nmake rbp1"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

vero:
	sudo bash build.sh "vero"

atv:
	sudo setarch i686 bash build.sh "atv"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files-image >/dev/null 2>&1
