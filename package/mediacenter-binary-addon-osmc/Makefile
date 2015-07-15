ADDONS := $(shell echo "$(ADDONS)")

all:
	echo -e "Specify a target:\nmake armv6l\nmake armv7\nmake i386\nADDONS=\"all\" or addon name"

armv6l:
	sudo bash build.sh "armv6l" "$(ADDONS)"

armv7:
	sudo bash build.sh "armv7" "$(ADDONS)"

i386:
	sudo setarch i686 bash build.sh "i386" "$(ADDONS)"

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
