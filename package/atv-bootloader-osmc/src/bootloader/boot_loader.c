
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
// original loader_init.c from mach_boot_linux (Edgar (gimli) Hucek)

//ACPI=0xfefd000 <6> ACPI 2.0=0xfefd014 <6> SMBIOS=0xfebe000 
//[ 0.000000] mem00: type=EFI_CONVENTIONAL_MEMORY,  attr=0xf, range=[0x0000000000000000-0x000000000008f000) (0MB)
//[ 0.000000] mem01: type=EFI_ACPI_MEMORY_NVS,		attr=0xf, range=[0x000000000008f000-0x0000000000090000) (0MB)
//[ 0.000000] mem02: type=EFI_CONVENTIONAL_MEMORY,  attr=0xf, range=[0x0000000000090000-0x00000000000a0000) (0MB)
//[ 0.000000] mem36: type=EFI_RESERVED_TYPE,		attr=0x0, range=[0x00000000000a0000-0x00000000000c0000) (0MB)
//[ 0.000000] mem03: type=EFI_CONVENTIONAL_MEMORY,  attr=0xf, range=[0x00000000000c0000-0x000000000b000000) (175MB)
//[ 0.000000] mem04: type=EFI_LOADER_DATA,			attr=0xf, range=[0x000000000b000000-0x000000000b66e000) (6MB)
//[ 0.000000] mem05: type=EFI_CONVENTIONAL_MEMORY,	attr=0xf, range=[0x000000000b66e000-0x000000000d9c4000) (35MB)
//[ 0.000000] mem06: type=EFI_BOOT_SERVICES_DATA,	attr=0xf, range=[0x000000000d9c4000-0x000000000da13000) (0MB)
//[ 0.000000] mem07: type=EFI_CONVENTIONAL_MEMORY,	attr=0xf, range=[0x000000000da13000-0x000000000da24000) (0MB)
//[ 0.000000] mem08: type=EFI_LOADER_CODE,			attr=0xf, range=[0x000000000da24000-0x000000000da4e000) (0MB)
//[ 0.000000] mem09: type=EFI_CONVENTIONAL_MEMORY,	attr=0xf, range=[0x000000000da4e000-0x000000000e6ed000) (12MB)
//

// PCI: BIOS Bug: MCFG area at f0000000 is not E820-reserved
// pnp: 00:01: iomem range 0xf0000000-0xf3ffffff could not be reserved (64MB)
// pnp: 00:01: iomem range 0xfed1c000-0xfed1ffff could not be reserved
// PCI: Failed to allocate mem resource #6:20000@20000000 for 0000:01:00.0

// RAM top					0x10000000
// mach_kernel  loads at	0x02000000
// linux kernel loads at	0x00100000
// linux kernel reserve to	0x00400000
// linux initrd loads at	0x00F43800


#include "types.h"
#include "utils.h"
#include "linux_code.h"
#include "elilo_code.h"
#include "darwin_code.h"

extern int	sprintf(char * buf, const char *fmt, ...);

//
mach_boot_parms		*mach_bp;

//

//
/**********************************************************************/

#define KERNEL_RESERVE_SIZE		0x00400000
#define BOOT_PARAM_MEMSIZE      0x00004000	// 16384
/**********************************************************************/
void
load_linux(unsigned int args)
{
	int                     i;
	char                    *ptr;
	kdesc_t                 kd;
	unsigned char           szBootSect[BOOT_PARAM_MEMSIZE];
	boot_params_t           *bp = (boot_params_t*)szBootSect;
	char                    *cmdline = (char*)&szBootSect[BOOT_PARAM_MEMSIZE - 2048];
	unsigned long           kernel_len = 0;
	unsigned char           *kernel_ptr = NULL;
	unsigned long           initrd_len = 0;
	unsigned char           *initrd_ptr = NULL;
	
	mach_bp = (mach_boot_parms*)args;

	vmode.width  = mach_bp->video.rowb / 4;
	vmode.height = mach_bp->video.height;
	vmode.xmargin = 0;
	// clear the screen
	//sleep(10);
	memset((char*)mach_bp->video.addr, 0x00, vmode.width * vmode.height * 4);
	//
	VIDEO_CURSOR_POSX = 0;
	VIDEO_CURSOR_POSY = 0;
	VIDEO_ATTR = 0xffc8c8c8;
	//
	//printk("mach_bp->devtree_len=0x%08X, mach_bp->devtree_ptr=0x%08X", 
	//	mach_bp->devtree_len, mach_bp->devtree_ptr);
	//sleep(10);

	// find the kernel and load it in the proper location
	kernel_ptr = (unsigned char*)getsectdatafromheader(&_mh_execute_header, "__TEXT", "__vmlinuz", &kernel_len);
	//printk("ATV: kernel_ptr = 0x%08X, kernel_len = 0x%08X\n", kernel_ptr, kernel_len);
	// kernel integrity check
	if (kernel_ptr[0x1FE] != 0x55 || kernel_ptr[0x1FF] != 0xAA) {
		printk("ATV: kernel_ptr[0x1FE] = 0x%X, kernel_ptr[0x1FF] = 0x%X\n", kernel_ptr[0x1FE], kernel_ptr[0x1FF]);
		printk("ATV: Kernel is not a vmlinuz or bzImage kernel image.\n");
		while(1);
	}
	// load kernel into it in the proper location (1M)
	kernel_start = (VOID*)0x00100000;
	// zero kernel destination in ram memory
	memset(kernel_start, 0x00, KERNEL_RESERVE_SIZE);
	// compute kernel size from actual kernel using zero-page setup_sectors (512 bytes per sector)
	kernel_size = kernel_len - ((kernel_ptr[0x1F1] + 1) * 512);
	// copy loaded kernel to base memory location
	memcpy(kernel_start, &kernel_ptr[ (kernel_ptr[0x1F1] + 1) * 512 ], kernel_size);

	// find possible initrd, start_kernel will handle loading into a proper location)
	initrd_ptr   = (unsigned char*)getsectdatafromheader(&_mh_execute_header, "__TEXT", "__initrd", &initrd_len);
	//printk("ATV: initrd_ptr = 0x%08X, initrd_len = 0x%08X\n", initrd_ptr, initrd_len);
	initrd_start = initrd_ptr;
	initrd_size  = initrd_len;

	// setup the kernel boot parameters
	// zero boot param structure
	memset(bp, 0x00, BOOT_PARAM_MEMSIZE);
	// copy the "zero page" from the kernel that was loaded (8192 bytes)
	memcpy(bp, kernel_ptr, 0x2000);

	// create linux kernel command line params from mach-o passed command line params
	// this comes from com.apple.Boot.plist under <key>Kernel Flags</key>
	sprintf(cmdline, "%s", mach_bp->cmdline);
	/*
	sprintf(cmdline, "%s video=imacfb:appletv,width:%d,height:%d,linelength:%d,base:%d", 
		mach_bp->cmdline,
		mach_bp->video.width,
		mach_bp->video.height,
		mach_bp->video.rowb,
		mach_bp->video.addr);
	*/

	// now format the linux kernel boot params
	create_boot_params(bp, cmdline);
	//sleep(10);

	// kernel_start = 0x100000 defined in system.c
	kd.kstart = kd.kentry = kernel_start;
	kd.kend   = ((UINT8*)kd.kstart) + KERNEL_RESERVE_SIZE;
	start_kernel(kd.kentry, bp);
	printk("An error occurred");

	while(1);
}
