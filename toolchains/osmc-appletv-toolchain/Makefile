all:
		sudo bash build.sh

local:
		touch /tmp/disable-lb
		sudo bash build.sh

clean:
		sudo rm -f *.deb > /dev/null 2>&1
		sudo rm -rf output > /dev/null 2>&1
		sudo rm -f /tmp/disable-lb > /dev/null 2>&1
