lib:
	gcc -c -fpic libosmc.c
	gcc -shared -o libosmc.so libosmc.o
	strip libosmc.o

test:   lib
	cp -ar libosmc.so /usr/lib
	gcc libosmc-test.c -o libosmc-test -losmc

clean:
	rm -f *.o > /dev/null 2>&1
	rm -f *.so >/dev/null 2>&1
	rm -f libosmc-test >/dev/null 2>&1
