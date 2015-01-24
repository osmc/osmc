.PHONY: inject_kernel
# handy tools to remember under darwin tools
# otool -l mach_kernel 
# size -m mach_kernel
#
#
# get OS type from shell
OSTYPE	= $(shell uname)
#
ARCH	= i386

# if Linxu, use the darwin-cross tools to compile/link
ifeq ($(OSTYPE),Linux)
  CC  := /opt/darwin-cross/bin/i386-apple-darwin8-gcc-4.0
  LD  := /opt/darwin-cross/bin/i386-apple-darwin8-ld
else
  CC  := /usr/bin/gcc-4.0
  LD  := /usr/bin/ld
	LDFLAGS = -classic_linker
endif

# start.o must be 1st in the link order (ld below)
OBJ	= start.o vsprintf.o console.o utils.o elilo_code.o darwin_code.o linux_code.o boot_loader.o

KERN_OBJ:= vmlinuz.obj initrd.obj

mach_kernel: $(KERN_OBJ) $(OBJ)
	$(LD) $(LDFLAGS) -arch $(ARCH) -o mach_kernel $(OBJ) $(KERN_OBJ) \
	-static \
	-force_cpusubtype_ALL \
	-e __start \
	-segalign 0x1000 \
	-segaddr __TEXT 0x2000000 \
	-sectalign __TEXT __text 0x1000 \
	-sectalign __DATA __common 0x1000 \
	-sectalign __DATA __bss 0x1000 \
	-sectcreate __PRELINK __text /dev/null \
	-sectcreate __PRELINK __symtab /dev/null \
	-sectcreate __PRELINK __info /dev/null

%.o:	%.c
	$(CC) -c -arch $(ARCH) -static -nostdlib -o $@ -c $<

%.o:	%.s
	$(CC) -c -arch $(ARCH) -static -nostdlib -DASSEMBLER -o $@ -c $<

vmlinuz.obj: vmlinuz
	$(LD) $(LDFLAGS) -arch $(ARCH) -w -r -o vmlinuz.obj -sectcreate __TEXT __vmlinuz vmlinuz

initrd.obj: initrd.gz
	$(LD) $(LDFLAGS) -arch $(ARCH) -w -r -o initrd.obj -sectcreate __TEXT __initrd initrd.gz

clean:
	rm -f *.o $(KERN_OBJ) mach_kernel

#xxd mach_kernel | sed -e "s/ffff ffff 1000/0100 0000 1000/" | xxd -r - mach_kernel
