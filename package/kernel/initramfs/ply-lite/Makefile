
ply-image: ply-image.c ply-frame-buffer.c Makefile
	gcc -static ply-image.c ply-frame-buffer.c -o ply-image -lpng12 -lm -lz
	strip ply-image
	
clean:
	rm -f ply-image *~ gmon.out
	
	
	
