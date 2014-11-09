all:
	echo -e "Specify a target:\nmake rbp\make atv\n"

rbp:
	sudo bash build.sh "rbp"

atv:
	sudo bash build.sh "atv"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf Kodi > /dev/null 2>&1
	sudo rm -rf Kodi-PVR > /dev/null 2>&1
