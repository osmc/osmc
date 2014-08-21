lib:
	gcc -c -fpic services.c identity.c
	gcc -shared -o libosmc.so libosmc.o
	strip libosmc.so

clean:
	rm -f *.o > /dev/null 2>&1
	rm -f *.so >/dev/null 2>&1
