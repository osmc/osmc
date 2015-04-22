
ply-image: ply-image.c ply-frame-buffer.c checkmodifier.c Makefile
	gcc ply-image.c ply-frame-buffer.c -o ply-image -lpng12 -lm -lz
	strip ply-image
	gcc checkmodifier.c -o checkmodifier
	strip checkmodifier
	
clean:
	rm -f ply-image
	rm -f checkmodifier
