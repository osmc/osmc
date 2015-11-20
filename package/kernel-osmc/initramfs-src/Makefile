all:
	@echo "available targets: \nmake kernel\nmake cpio"

kernel:
	bash build.sh "kernel"
	
cpio:
	bash build.sh "cpio"

clean:
	rm -f initramfs.gz >/dev/null 2>&1
	rm -rf target >/dev/null 2>&1
