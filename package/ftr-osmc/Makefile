all:
        @echo "Specify a target:\nmake rbp1\nmake rbp2\nmake atv\nmake vero1\nmake vero2\n"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

atv:
	sudo setarch i686 bash build.sh "atv"

vero1:
	sudo bash build.sh "vero1"

vero2:
	sudo bash build.sh "vero2"

pc:
	sudo bash build.sh "pc"

clean:
	sudo rm -f *.deb >/dev/null 2>&1
	sudo rm -f files/usr/bin/ftr >/dev/null 2>&1
