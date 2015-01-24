#ifndef _DARWIN_CODE_H_
#define _DARWIN_CODE_H_

#include "types.h"

#define DARWIN_IMAGE_BASE	0x002000000
// xnu pexpert/pexpert/i386/boot.h
/* Values for v_display */
#define VGA_TEXT_MODE		0
#define GRAPHICS_MODE		1
#define FB_TEXT_MODE		2
//
typedef struct _Boot_Video {
	uint32_t	v_baseAddr;	// Base address of video memory
	uint32_t	v_display;	// Display Code (0, 1, 2)
	uint32_t	v_rowBytes;	// Number of bytes per pixel row
	uint32_t	v_width;	// Width
	uint32_t	v_height;	// Height
	uint32_t	v_depth;	// Pixel Depth
} __attribute__((aligned(4))) Boot_Video;
//
typedef struct _mach_video_parms {
	uint32_t	addr;
	uint32_t	disp;
	uint32_t	rowb;
	uint32_t	width;
	uint32_t	height;
	uint32_t	depth;
} __attribute__((aligned(4))) mach_video_parms;
//
typedef struct _mach_boot_parms {
    uint16_t	rev;
    uint16_t	ver;
    char	cmdline[CMDLINE];
    uint32_t    efi_mem_map;
    uint32_t    efi_mem_map_size;
    uint32_t    efi_mem_desc_size;
    uint32_t    efi_mem_desc_ver;
    mach_video_parms video;
    uint32_t    devtree_ptr;			// Base of flattened device tree
    uint32_t	devtree_len;			// Length of flattened tree
    uint32_t    kaddr;
    uint32_t    ksize;
    uint32_t    efi_runtime_page;
    uint32_t    efi_runtime_page_count;
    uint32_t    efi_sys_tbl;
    uint8_t     efi_mode;
    uint8_t     __reserved1[3];
    uint32_t    __reserved2[7];

} __attribute__((aligned(4))) mach_boot_parms;
//
extern mach_boot_parms *mach_bp;

//
/**********************************************************************/
// defined by mach-o ld linker
extern struct mach_header _mh_execute_header;
//
typedef int	cpu_type_t;
typedef int	cpu_subtype_t;
typedef int vm_prot_t;
//
// snatched from /usr/include/mach-o/loader.h
/*
 * The 32-bit mach header appears at the very beginning of the object file for
 * 32-bit architectures.
 */
struct mach_header {
	uint32_t		magic;		/* mach magic number identifier */
	cpu_type_t		cputype;	/* cpu specifier */
	cpu_subtype_t	cpusubtype;	/* machine specifier */
	uint32_t		filetype;	/* type of file */
	uint32_t		ncmds;		/* number of load commands */
	uint32_t		sizeofcmds;	/* the size of all the load commands */
	uint32_t		flags;		/* flags */
};
/* Constant for the magic field of the mach_header (32-bit architectures) */
#define	MH_MAGIC	0xfeedface	/* the mach magic number */
#define MH_CIGAM	0xcefaedfe	/* NXSwapInt(MH_MAGIC) */
#define	MH_OBJECT	0x1		/* relocatable object file */
//
//
#define	LC_SEGMENT	0x1	/* segment of this file to be mapped */
struct segment_command { /* for 32-bit architectures */
	uint32_t	cmd;		/* LC_SEGMENT */
	uint32_t	cmdsize;	/* includes sizeof section structs */
	char		segname[16];	/* segment name */
	uint32_t	vmaddr;		/* memory address of this segment */
	uint32_t	vmsize;		/* memory size of this segment */
	uint32_t	fileoff;	/* file offset of this segment */
	uint32_t	filesize;	/* amount to map from the file */
	vm_prot_t	maxprot;	/* maximum VM protection */
	vm_prot_t	initprot;	/* initial VM protection */
	uint32_t	nsects;		/* number of sections in segment */
	uint32_t	flags;		/* flags */
};
//
struct section { /* for 32-bit architectures */
	char		sectname[16];	/* name of this section */
	char		segname[16];	/* segment this section goes in */
	uint32_t	addr;		/* memory address of this section */
	uint32_t	size;		/* size in bytes of this section */
	uint32_t	offset;		/* file offset of this section */
	uint32_t	align;		/* section alignment (power of 2) */
	uint32_t	reloff;		/* file offset of relocation entries */
	uint32_t	nreloc;		/* number of relocation entries */
	uint32_t	flags;		/* flags (section type and attributes)*/
	uint32_t	reserved1;	/* reserved (for offset or index) */
	uint32_t	reserved2;	/* reserved (for count or sizeof) */
};
//


const struct section*
getsectbynamefromheader(struct mach_header *mhp, const char *segname, const char *sectname);
char*
getsectdatafromheader(struct mach_header *mhp, const char *segname, const char *sectname, unsigned long *size);

#endif
