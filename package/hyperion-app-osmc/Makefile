all:
	@echo "Specify a target:\nmake rbp1\nmake rbp2"

rbp1:
	sudo bash build.sh "rbp1"

rbp2:
	sudo bash build.sh "rbp2"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -f files/etc/osmc/apps.d/*hyperion-app-osmc >/dev/null 2>&1
	sudo rm -rf src/ >/dev/null 2>&1
