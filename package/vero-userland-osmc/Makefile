all:
	echo "Usage: make vero\n"

vero:
	sudo bash build.sh vero

clean:
	sudo rm -f *.deb >/dev/null 2>&1
	sudo rm -rf files/usr/lib >/dev/null 2>&1
	sudo rm -rf files/usr/include >/dev/null 2>&1 # Before the move
	sudo rm -rf files-dev/usr/include >/dev/null 2>&1
	sudo rm -rf files/lib/ >/dev/null 2>&1
