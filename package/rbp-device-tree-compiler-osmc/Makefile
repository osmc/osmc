# We don't need an RBP2 version because we don't deploy this. If it runs on build infrastructure that's good enough. 

all:
	echo "Usage: make rbp1"

rbp1:
	sudo bash build.sh rbp1

clean:
	sudo rm -f *.deb > /dev/null 2>&1
	sudo rm -rf files/usr > /dev/null 2>&1
