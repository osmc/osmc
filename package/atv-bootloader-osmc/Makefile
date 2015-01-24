all:
	echo "Usage: make atv"

atv:
	sudo setarch i686 bash build.sh atv

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf src/initrd > /dev/null 2>&1
	sudo rm -f src/bootloader/vmlinuz > /dev/null 2>&1
	sudo rm -f src/bootloader/initrd.img.gz > /dev/null 2>&1
