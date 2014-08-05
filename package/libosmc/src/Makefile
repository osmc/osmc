lib:
	gcc -c -fpic libosmc.c
	gcc -shared -o libosmc.so libosmc.o
	strip libosmc.o

clean:
	rm -f *.o > /dev/null 2>&1
	rm -f *.so >/dev/null 2>&1
	rm -f libosmc-test >/dev/null 2>&1
