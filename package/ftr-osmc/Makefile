all:
        echo -e "Specify a target:\nmake rbp1\nmake rbp2\nmake atv\n"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

atv:
	sudo bash build.sh "atv"

clean:
	sudo rm -f *.deb >/dev/null 2>&1
	sudo rm -f files/usr/bin/ftr >/dev/null 2>&1
