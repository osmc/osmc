/*
 *  Copyright (C) 2001-2003 Hewlett-Packard Co.
 *	Contributed by Stephane Eranian <eranian@hpl.hp.com>
 *	Contributed by Mike Johnston <johnston@intel.com>
 *	Contributed by Chris Ahna <christopher.j.ahna@intel.com>
 *
 * This file is part of the ELILO, the EFI Linux boot loader.
 *
 *  ELILO is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  ELILO is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ELILO; see the file COPYING.  If not, write to the Free
 *  Software Foundation, 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * Please check out the elilo.txt for complete documentation on how
 * to use this program.
 */

#include "types.h"
#include "linux_code.h"
#include "darwin_code.h"

#define DEFAULT_FB_MEM  1024*1024*16
/*
 * Highest available base memory address.
 *
 * For traditional kernels and loaders this is always at 0x90000.
 * For updated kernels and loaders this is computed by taking the
 * highest available base memory address and rounding down to the
 * nearest 64 kB boundary and then subtracting 64 kB.
 *
 * A non-compressed kernel is automatically assumed to be an updated
 * kernel.  A compressed kernel that has bit 6 (0x40) set in the
 * loader_flags field is also assumed to be an updated kernel.
 */
uint32_t high_base_mem = 0x90000;
/*
 * Highest available extended memory address.
 *
 * This is computed by taking the highest available extended memory
 * address and rounding down to the nearest EFI_PAGE_SIZE (usually
 * 4 kB) boundary.  
 * This is only used for backward compatibility.
 */
uint32_t high_ext_mem = 63 * 1024 * 1024;

/* This starting address will hold true for all of the loader types for now */
void *kernel_start   = (VOID *)0x00100000;	/* 1M */
uint32_t kernel_size = 0;

void *initrd_start   = NULL;
uint32_t initrd_size = 0;

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
int
create_boot_params(boot_params_t *bp, char *cmdline)
{
	uint16_t		hdr_version;
	linux_header_t	org_linux_header;

	// save a copy of the original linux header
	org_linux_header = *(linux_header_t*)bp;
	//printk(" initrd_addr_max=0x%lx ", org_linux_header.initrd_addr_max);
	//sleep(20);
	
	// Save off our header revision information.
	hdr_version = (bp->s.hdr_major << 8) | bp->s.hdr_minor;

	//Clear out unused memory in boot sector image.
	bp->s.unused_1 = 0;
	bp->s.unused_2 = 0;
	memset(bp->s.unused_3, 0x00, sizeof bp->s.unused_3);
	memset(bp->s.unused_4, 0x00, sizeof bp->s.unused_4);
	memset(&bp->s.unused_51, 0x00, sizeof bp->s.unused_51);
	memset(bp->s.unused_52, 0x00, sizeof bp->s.unused_52);
	bp->s.unused_6 = 0;
	bp->s.unused_7 = 0;
	memset(bp->s.unused_8, 0x00, sizeof bp->s.unused_8);

	// Tell kernel this was loaded by an advanced loader type.
	// If this field is zero, the initrd_start and initrd_size
	// fields are ignored by the kernel.
	bp->s.loader_type = 0xFF; /* Unregistered Linux loader */
	/* 0x210	char		LOADER_TYPE
		0xTV: T=0 for LILO
		1 for Loadlin
		2 for bootsect-loader
		3 for SYSLINUX
		4 for ETHERBOOT
		5 for ELILO
		7 for GRuB
		8 for U-BOOT
		9 for Xen
		V = version
	 // bit0 = 1: kernel is loaded high (bzImage)
	*/
	bp->s.loader_flags = LDRFLAG_LOADED_HIGH;
	//kernel_setup->heap_end_ptr = 0xffff;	/* 64K heap */
	//kernel_setup->flags = 0x81;		/* loaded high, heap existant */

	// Setup command line information.
	bp->s.cmdline_magik = CMDLINE_MAGIK;
	bp->s.cmdline_offset = (UINT8*)cmdline - (UINT8*)bp;

	// Clear out the cmdline_addr field so the kernel can find 
	// the cmdline.
	bp->s.cmdline_addr = 0x0;

	// Setup hard drive parameters.
	// %%TBD - It should be okay to zero fill the hard drive
	// info buffers.  The kernel should do its own detection.
	memset(bp->s.hd0_info, 0x00, sizeof bp->s.hd0_info);
	memset(bp->s.hd1_info, 0x00, sizeof bp->s.hd1_info);

	// Max Ram Memory info (Size of memory above 1MB in KB)
	bp->s.alt_mem_k = high_ext_mem / 1024;
	if (bp->s.alt_mem_k <= 65535) {
		bp->s.ext_mem_k = (uint16_t)bp->s.alt_mem_k;
	} else {
		bp->s.ext_mem_k = 65535;
	}

	// Initial RAMdisk and root device stuff.
	// These RAMdisk flags are not needed, just zero them.
	bp->s.ramdisk_flags = 0;
	bp->s.initrd_start  = 0;
	bp->s.initrd_size   = 0;
	if (initrd_start && initrd_size) {
		bp->s.initrd_start = (uint32_t)initrd_start;
		bp->s.initrd_size  = initrd_size;
		// This is the RAMdisk root device for RedHat 2.2.x
		// kernels (major 0x01, minor 0x00).
		bp->s.orig_root_dev= 0x0100;
	}
	//kernel_setup->initrd_addr_max = RAMSIZE_USE;
	
	/*
	// Find out the kernel's restriction on how high the initrd can be placed
	(this is from etherboot -> linux_load.c -> load_initrd
	if (hdr->protocol_version >= 0x203) {
		max = hdr->initrd_addr_max;
	} else {
		max = 0x38000000; // Hardcoded value for older kernels 
	}
	end = max;
	start = end - size;
	start &= ~0xfff; // page align
	end = start + size;

	*/

	// APM BIOS info.
	bp->s.apm_bios_ver = NO_APM_BIOS;
	bp->s.bios_code_seg = 0;
	bp->s.bios_entry_point = 0;
	bp->s.bios_code_seg16 = 0;
	bp->s.bios_data_seg = 0;
	bp->s.apm_bios_flags = 0;
	bp->s.bios_code_len = 0;
	bp->s.bios_data_len = 0;

	// MCA BIOS info (misnomer).
	bp->s.mca_info_len = 0;
	memset(bp->s.mca_info_buf, 0x00, sizeof bp->s.mca_info_buf);

	// Pointing device presence.  The kernel will detect this.
	bp->s.aux_dev_info = NO_MOUSE;

	// EFI loader signature 
	//memcpy(bp->s.efi_loader_sig, EFI_LOADER_SIG, 4);
	memset(bp->s.efi_loader_sig, 0, 4);

	// Kernel entry point.
	bp->s.kernel_start = (UINT32)kernel_start;


	// video info
	bp->s.orig_x			= 0;
	bp->s.orig_y			= 24;
	bp->s.orig_video_cols	= 80;
	bp->s.orig_video_rows	= 25;
	bp->s.orig_video_points = 16; 
	bp->s.orig_video_page	= 0;

	// check mach_kernel boot params for framebuffer type
	if ( strstr(mach_bp->cmdline, "video=efifb") ) {
		// EFI linear frame buffer
		bp->s.is_vga = VIDEO_TYPE_EFI;
		bp->s.orig_video_mode = 0;
		//
	} else if ( strstr(mach_bp->cmdline, "video=imac_fb") ) {
		// iMac EFI linear frame buffer
		// this only works if imac_fb has appletv patches or else
		// imac_fb will fail to load because of the DMI name check.
		bp->s.is_vga = 0;	
		bp->s.orig_video_mode = 3; // what is mode = 3 ???
		//
	} else if ( strstr(mach_bp->cmdline, "video=vesafb") ) {
		// VESA linear frame buffer
		// vesafb seems to work but might fail if any video bios 
		// calls are done -- no pc bios/video bios present
		bp->s.is_vga = VIDEO_TYPE_VLFB;	
		bp->s.orig_video_mode = 0;
		//
	} else {
		// default to vesafb
		bp->s.is_vga = VIDEO_TYPE_VLFB;	
		bp->s.orig_video_mode = 0;
	}
	//bp->s.is_vga			= 1; // VGA in standard 80x25 text mode
	//bp->s.is_vga			= 0; // seems to imply VIDEO_TYPE_EGAC if zero (drivers/char/vga.c)
	//bp->s.orig_video_mode	= 0;
	//bp->s.orig_video_mode	= 3; // what is mode = 3 ???
	bp->s.orig_ega_bx		= 0;

	bp->s.lfb_width			= mach_bp->video.width;
	bp->s.lfb_height		= mach_bp->video.height;
	bp->s.lfb_depth			= mach_bp->video.depth;
	bp->s.lfb_base			= mach_bp->video.addr;
	bp->s.lfb_size			= DEFAULT_FB_MEM / 65536; // 256	// size in 64k units
	bp->s.lfb_line_len		= mach_bp->video.rowb;
	bp->s.lfb_pages			= (DEFAULT_FB_MEM + 4095) / 4096;	// size in page units
	//bp->s.lfb_pages			= 1;
	bp->s.lfb_red_size		= 8;
	bp->s.lfb_red_pos		= 16;
	bp->s.lfb_green_size		= 8;
	bp->s.lfb_green_pos		= 8;
	bp->s.lfb_blue_size		= 8;
	bp->s.lfb_blue_pos		= 0;
	bp->s.lfb_rsvd_size		= 8;
	bp->s.lfb_rsvd_pos		= 24;
	bp->s.vesa_seg			= 0;
	bp->s.vesa_off			= 0;

	bp->s.efi_mem_map		= mach_bp->efi_mem_map;
	bp->s.efi_mem_map_size	= mach_bp->efi_mem_map_size;
	bp->s.efi_mem_desc_size = mach_bp->efi_mem_desc_size;
	bp->s.efi_mem_desc_ver	= mach_bp->efi_mem_desc_ver;
	bp->s.efi_sys_tbl		= mach_bp->efi_sys_tbl;
	
	printk("ATV: fixup efi memmap\n");
	quirk_fixup_efi_memmap(bp);
	//
	// Now that we have EFI memory map, convert it to E820 map
	//	and update the bootparam accordingly
	printk("ATV: converting EFI memmap -> e820 memmap\n");
	fill_e820map(bp);
	//
	//print_e820_memory_map(bp);
	//sleep(60);
	//
	//
	
	efi_system_table_t	*system_table;
	efi_config_table_t	*config_tables;
	int					i, num_config_tables;
	efi_linux_table		efi;
	
	system_table		= (efi_system_table_t*)mach_bp->efi_sys_tbl;
	num_config_tables	= system_table->nr_tables;
	config_tables		= (efi_config_table_t*)system_table->tables;

	// Let's see what config tables the efi firmware passed to us.
	for (i = 0; i < num_config_tables; i++) {
		if (efi_guidcmp(config_tables[i].guid, MPS_TABLE_GUID) == 0) {
			efi.mps = (void*)config_tables[i].table;
			//printk(" MPS=0x%lx ", config_tables[i].table);
			//
		} else if (efi_guidcmp(config_tables[i].guid, ACPI_20_TABLE_GUID) == 0) {
			efi.acpi20 = (void*)config_tables[i].table;
			//printk(" ACPI 2.0=0x%lx ", config_tables[i].table);
			//
		} else if (efi_guidcmp(config_tables[i].guid, ACPI_TABLE_GUID) == 0) {
			efi.acpi = (void*)config_tables[i].table;
			//printk(" ACPI=0x%lx ", config_tables[i].table);
			//
		} else if (efi_guidcmp(config_tables[i].guid, SMBIOS_TABLE_GUID) == 0) {
			efi.smbios = (void*) config_tables[i].table;
			//printk(" SMBIOS=0x%lx ", config_tables[i].table);
			//
		} else if (efi_guidcmp(config_tables[i].guid, HCDP_TABLE_GUID) == 0) {
			efi.hcdp = (void*)config_tables[i].table;
			//printk(" HCDP=0x%lx ", config_tables[i].table);
			//
		} else if (efi_guidcmp(config_tables[i].guid, UGA_IO_PROTOCOL_GUID) == 0) {
			efi.uga = (void*)config_tables[i].table;
			//printk(" UGA=0x%lx ", config_tables[i].table);
		}
	}
	//printk("\n");

	// rsdp_low_mem is unsigned long so alignment below works
	unsigned long		rsdp_low_mem   = 0xF8000;
	unsigned long		smbios_low_mem = 0xF8100;
	//
	printk("ATV: clone ACPI entry to %lx...\n", rsdp_low_mem);
	// We need at copy the RSDP down low so linux can find it
	// copy RSDP table entry from efi location to low mem location
	memcpy((void*)rsdp_low_mem, efi.acpi20, sizeof(acpi_rsdp_t) );

	printk("ATV: clone SMBIOS entry to %lx...\n", smbios_low_mem);
	// We need at copy the SMBIOS Table Entry Point down low so linux can find it
	// copy SMBIOS Table Entry Point from efi location to low mem location
	memcpy((void*)smbios_low_mem, efi.smbios, sizeof(smbios_entry_t) );
	return(0);
}


/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
// How to jump to kernel code
void
start_kernel(void *kentry, boot_params_t *bp)
{
	// Disable interrupts.
	asm volatile ( "cli" : : );

	// Relocate initrd, if present.
	if (bp->s.initrd_start) {
		unsigned long	initrd_base;
		
		initrd_base = DARWIN_IMAGE_BASE - PAGE_ALIGN( bp->s.initrd_size );
		memcpy((void*)initrd_base, (void*)bp->s.initrd_start, bp->s.initrd_size);
		bp->s.initrd_start = initrd_base;
		//memcpy((void*)(15 * 1024 * 1024), (void*)bp->s.initrd_start, bp->s.initrd_size);
		//bp->s.initrd_start = 15 * 1024 * 1024;
	}
	
	// Copy boot sector, setup data and command line
	// to final resting place.  We need to copy
	// BOOT_PARAM_MEMSIZE bytes.
	memcpy((void*)high_base_mem, bp, 0x4000);
	// update cmdline_addr
	bp = (boot_params_t*)high_base_mem;
	bp->s.cmdline_addr = high_base_mem + bp->s.cmdline_offset;

	// Initialize Linux GDT.
	memset((void*)gdt_addr.base, 0x00, gdt_addr.limit);
	memcpy((void*)gdt_addr.base, init_gdt, init_gdt_size );

	// Load descriptor table pointers.
	asm volatile ( "lidt %0" : : "m" (idt_addr) );
	asm volatile ( "lgdt %0" : : "m" (gdt_addr) );

	// ebx := 0  (%%TBD - do not know why, yet)
	// ecx := kernel entry point
	// esi := address of boot sector and setup data
	asm volatile ( "movl %0, %%esi" : : "m" (high_base_mem) );
	asm volatile ( "movl %0, %%ecx" : : "m" (kentry) );
	asm volatile ( "xorl %%ebx, %%ebx" : : );

	// Jump to kernel entry point.
	asm volatile ( "jmp *%%ecx" : : );
}


