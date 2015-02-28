all:
	echo -e "Specify a target:\nmake rbp1\nmake armv6l\nmake i386"

armv6l:
	sudo bash build.sh "armv6l"

armv7:
	sudo bash build.sh "armv7"

i386:
	sudo bash build.sh "gen"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -f files/etc/osmc/apps.d/*tvheadend-app-osmc >/dev/null 2>&1
	sudo rm -rf src/ >/dev/null 2>&1
