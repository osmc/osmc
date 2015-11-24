all:
	@echo "available targets: DEVICE=rbp1/rbp2/vero \nmake kernel\nmake cpio"

kernel:
	bash build.sh "kernel" "${DEVICE}"
	
cpio:
	bash build.sh "cpio" "${DEVICE}"

clean:
	rm -f initramfs.gz >/dev/null 2>&1
	rm -rf target >/dev/null 2>&1
