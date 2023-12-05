all:
	@echo "Specify a target:\nmake armv7\nmake aarch64\n"

armv7:
	sudo bash build.sh "armv7"

aarch64:
	sudo bash build.sh "aarch64"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -f files/etc/osmc/apps.d/*aml-vnc-app-osmc >/dev/null 2>&1
	sudo rm -rf src/ >/dev/null 2>&1
