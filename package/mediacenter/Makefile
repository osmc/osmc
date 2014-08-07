all:
	echo -e "Specify a target:\nmake rbp\make atv\n"

rbp:
	sudo bash build.sh "rbp"

atv:
	sudo bash build.sh "atv"

clean:
	sudo rm *.deb
	sudo rm -rf files/usr > /dev/null 2>&1
	sudo rm -rf src-a > /dev/null 2>&1
	sudo rm -rf src-b > /dev/null 2>&1
