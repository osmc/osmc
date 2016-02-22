all:
	@echo "Specify a target:\nmake rbp1\nmake rbp2"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf files/lib > /dev/null 2>&1
	sudo rm -f files/etc/osmc/apps.d/*vnc-app-osmc >/dev/null 2>&1
	sudo rm -rf src/ >/dev/null 2>&1
	sudo rm -f *vnc-app-osmc.json >/dev/null 2>&1
