#ifndef _LINUX_CODE_H_
#define _LINUX_CODE_H_

#include "types.h"
#include "utils.h"

#define hi32(a) ((UINT32)(a >> 32))
#define lo32(a) ((UINT32)(a))
	
// page alignment macros (include/asm-i386/page.h)
// Align the pointer to the (next) page boundary
// PAGE_SHIFT determines the page size
#define PAGE_SHIFT				12
#define PAGE_SIZE				(1UL << PAGE_SHIFT)
#define PAGE_MASK				(~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr)        (((addr)+PAGE_SIZE-1)&PAGE_MASK)
 
// Descriptor table pointer format.
#pragma pack(1)
typedef struct {
	uint16_t	limit;
	uint32_t	base;
} dt_addr_t;
#pragma pack()

typedef struct {
	void		*kstart;
	void		*kend;
	void		*kentry;
} kdesc_t;

typedef struct {
	void		*start_addr; 
	uint32_t	pgcnt;
	uint32_t	size;
} memdesc_t;

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
//acpi memory search
//000E0000h to 000FFFFFh
//
/* RSDP location */
#define ACPI_BIOS_ROM_BASE	0xE0000L
#define ACPI_BIOS_ROM_END	0x100000L
#define ACPI_RSDP1_SIG 0x20445352		/* 'RSD ' */
#define ACPI_RSDP2_SIG 0x20525450		/* 'PTR ' */
//
/* Root System Descriptor Pointer */
// ACPI 1.0 Root System Description Table (RSDT)
typedef struct acpi_rsdp_rev1
{
	uint8_t		signature [4];          /* ACPI signature (4 ASCII characters) */\
	uint32_t	length;                 /* Length of table, in bytes, including header */\
	uint8_t		revision;               /* ACPI Specification minor version # */\
	uint8_t		checksum;               /* To make sum of entire table == 0 */\
	uint8_t		oem_id [6];             /* OEM identification */\
	uint8_t		oem_table_id [8];       /* OEM table identification */\
	uint32_t	oem_revision;           /* OEM revision number */\
	uint8_t		asl_compiler_id [4];    /* ASL compiler vendor ID */\
	uint32_t	asl_compiler_revision;  /* ASL compiler revision number */
	uint32_t	table_offset_entry [2]; /* Array of pointers to other ACPI tables */
} acpi_rsdp_rev1_t;

/* ACPI 2.0 table RSDP */
typedef struct acpi_rsdp {
	char  signature[8];     /* RSDP signature "RSD PTR" */
	u8    checksum;         /* checksum of the first 20 bytes */
	char  oem_id[6];        /* OEM ID, "LXBIOS" */
	u8    revision;         /* 0 for APCI 1.0, 2 for ACPI 2.0 */
	u32   rsdt_address;     /* physical address of RSDT */
	u32   length;           /* total length of RSDP (including extended part) */
	u64   xsdt_address;     /* physical address of XSDT */
	u8    ext_checksum;     /* checksum of whole table */
	u8    reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
//smbios memory search
//000F0000h to 000FFFFFh
//
// SMBIOS 
// linux scans 0xF0000 to 0xF0000 + 0x10000 searching for 
//	the SMBIOS (DMI) signature "_DMI_" and correct dmi checksum
// SMBIOS Structure Table Entry Point.  See DSP0134 2.1.1 for more information.
//  The structure table entry point is located by searching for the anchor.
 
typedef struct smbios_entry {
	char		smbe_eanchor[4];		/* anchor tag (SMB_ENTRY_EANCHOR) "_SM_" */
	uint8_t		smbe_ecksum;			/* checksum of entry point structure */
	uint8_t		smbe_elen;				/* length in bytes of entry point */
	uint8_t		smbe_major;				/* major version of the SMBIOS spec */
	uint8_t		smbe_minor;				/* minor version of the SMBIOS spec */
	uint16_t	smbe_maxssize;			/* maximum size in bytes of a struct */
	uint8_t		smbe_revision;			/* entry point structure revision */
	uint8_t		smbe_format[5];			/* entry point revision-specific data */
	char		smbe_ianchor[5];		/* intermed. tag (SMB_ENTRY_IANCHOR) "_DMI_" */
	uint8_t		smbe_icksum;			/* intermed. checksum */
	uint16_t	smbe_stlen;				/* length in bytes of structure table */
	uint32_t	smbe_staddr;			/* physical addr of structure table */
	uint16_t	smbe_stnum;				/* number of structure table entries */
	uint8_t		smbe_bcdrev;			/* BCD value representing DMI version */
} __attribute__((packed)) smbios_entry_t;

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
// Definitions for converting EFI memory map to E820 map for Linux
// These definitions are from include/linux/asm-x86/e820.h
// The structure ia32_boot_params below is updated to accommodate E820 map
// EFI memory map is converted to E820 map in this structure and passed
// to Linux. This way the OS does not need to do the conversion.
//
// (linux/include/linux/efi.h)
typedef struct _efi_memory_desc_t {
	u32 type;
	u32 pad;
	u64 phys_addr;
	u64 virt_addr;
	u64 num_pages;
	u64 attribute;
} efi_memory_desc_t;

// EFI Memory types
#define EFI_RESERVED_TYPE                0
#define EFI_LOADER_CODE                  1
#define EFI_LOADER_DATA                  2
#define EFI_BOOT_SERVICES_CODE           3
#define EFI_BOOT_SERVICES_DATA           4
#define EFI_RUNTIME_SERVICES_CODE        5
#define EFI_RUNTIME_SERVICES_DATA        6
#define EFI_CONVENTIONAL_MEMORY          7
#define EFI_UNUSABLE_MEMORY              8
#define EFI_ACPI_RECLAIM_MEMORY          9
#define EFI_ACPI_MEMORY_NVS             10
#define EFI_MEMORY_MAPPED_IO            11
#define EFI_MEMORY_MAPPED_IO_PORT_SPACE 12
#define EFI_PAL_CODE                    13
#define EFI_MAX_MEMORY_TYPE             14

/* Attribute values: */
#define EFI_MEMORY_UC           ((u64)0x0000000000000001ULL)    /* uncached */
#define EFI_MEMORY_WC           ((u64)0x0000000000000002ULL)    /* write-coalescing */
#define EFI_MEMORY_WT           ((u64)0x0000000000000004ULL)    /* write-through */
#define EFI_MEMORY_WB           ((u64)0x0000000000000008ULL)    /* write-back */
#define EFI_MEMORY_WP           ((u64)0x0000000000001000ULL)    /* write-protect */
#define EFI_MEMORY_RP           ((u64)0x0000000000002000ULL)    /* read-protect */
#define EFI_MEMORY_XP           ((u64)0x0000000000004000ULL)    /* execute-protect */
#define EFI_MEMORY_RUNTIME      ((u64)0x8000000000000000ULL)    /* range requires runtime mapping */
#define EFI_MEMORY_DESCRIPTOR_VERSION   1

#define EFI_PAGE_SHIFT          12

#define NextEFIMemoryDescriptor(Ptr,Size)  ( (efi_memory_desc_t*) ( ((UINT8*)Ptr) + Size) )

typedef struct {
	u64 signature;
	u32 revision;
	u32 headersize;
	u32 crc32;
	u32 reserved;
} efi_table_hdr_t;
//
typedef struct {
	efi_table_hdr_t hdr;
	unsigned long	get_time;
	unsigned long	set_time;
	unsigned long	get_wakeup_time;
	unsigned long	set_wakeup_time;
	unsigned long	set_virtual_address_map;
	unsigned long	convert_pointer;
	unsigned long	get_variable;
	unsigned long	get_next_variable;
	unsigned long	set_variable;
	unsigned long	get_next_high_mono_count;
	unsigned long	reset_system;
} efi_runtime_services_t;
//
typedef struct {
	efi_table_hdr_t	hdr;
	unsigned long	fw_vendor;        // physical addr of CHAR16 vendor string
	u32				fw_revision;
	unsigned long	con_in_handle;
	unsigned long	con_in;
	unsigned long	con_out_handle;
	unsigned long	con_out;
	unsigned long	stderr_handle;
	unsigned long	stderr;
	efi_runtime_services_t *runtime;
	unsigned long	boottime;
	unsigned long	nr_tables;
	unsigned long	tables;
} efi_system_table_t;

typedef struct {
	u8	b[16];
} efi_guid_t;

#define EFI_GUID(a,b,c,d0,d1,d2,d3,d4,d5,d6,d7) \
	((efi_guid_t) \
		{{ (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff, \
			(b) & 0xff, ((b) >> 8) & 0xff, \
			(c) & 0xff, ((c) >> 8) & 0xff, \
			(d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) }})

static inline int
efi_guidcmp (efi_guid_t left, efi_guid_t right)
{
	return( memcmp(&left, &right, sizeof(efi_guid_t) ) );
}
/*
*  EFI Configuration Table and GUID definitions
*/
#define NULL_GUID \
	EFI_GUID(  0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 )

#define MPS_TABLE_GUID    \
	EFI_GUID(  0xeb9d2d2f, 0x2d88, 0x11d3, 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d )

#define ACPI_TABLE_GUID    \
	EFI_GUID(  0xeb9d2d30, 0x2d88, 0x11d3, 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d )

#define ACPI_20_TABLE_GUID    \
	EFI_GUID(  0x8868e871, 0xe4f1, 0x11d3, 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 )

#define SMBIOS_TABLE_GUID    \
	EFI_GUID(  0xeb9d2d31, 0x2d88, 0x11d3, 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d )

#define SAL_SYSTEM_TABLE_GUID    \
	EFI_GUID(  0xeb9d2d32, 0x2d88, 0x11d3, 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d )

#define HCDP_TABLE_GUID \
	EFI_GUID(  0xf951938d, 0x620b, 0x42ef, 0x82, 0x79, 0xa8, 0x4b, 0x79, 0x61, 0x78, 0x98 )

#define UGA_IO_PROTOCOL_GUID \
	EFI_GUID(  0x61a4d49e, 0x6f68, 0x4f1b, 0xb9, 0x22, 0xa8, 0x6e, 0xed, 0xb, 0x7, 0xa2 )

#define EFI_GLOBAL_VARIABLE_GUID \
	EFI_GUID(  0x8be4df61, 0x93ca, 0x11d2, 0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c )

//
typedef struct {
	efi_guid_t		guid;
	unsigned long	table;
} efi_config_table_t;
//
typedef struct _efi_linux_table {
	efi_system_table_t	*systab;				   /* EFI system table */
	void				*mps;                      /* MPS table */
	void				*acpi;                     /* ACPI table  (IA64 ext 0.71) */
	void				*acpi20;                   /* ACPI table  (ACPI 2.0) */
	void				*smbios;                   /* SM BIOS table */
	void				*sal_systab;               /* SAL system table */
	void				*boot_info;                /* boot info table */
	void				*hcdp;                     /* HCDP table */
	void				*uga;                      /* UGA table */
	void				*get_time;
	void				*set_time;
	void				*get_wakeup_time;
	void				*set_wakeup_time;
	void				*get_variable;
	void				*get_next_variable;
	void				*set_variable;
	void				*get_next_high_mono_count;
	void				*reset_system;
	void				*set_virtual_address_map;
	/*
	efi_get_time_t					*get_time;
	efi_set_time_t					*set_time;
	efi_get_wakeup_time_t			*get_wakeup_time;
	efi_set_wakeup_time_t			*set_wakeup_time;
	efi_get_variable_t				*get_variable;
	efi_get_next_variable_t			*get_next_variable;
	efi_set_variable_t				*set_variable;
	efi_get_next_high_mono_count_t	*get_next_high_mono_count;
	efi_reset_system_t				*reset_system;
	efi_set_virtual_address_map_t	*set_virtual_address_map;
	*/
} efi_linux_table;


/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
// (linux/include/screen_info.h)
typedef struct _screen_info {
	uint8_t  orig_x;           /* 0x00 */
	uint8_t  orig_y;           /* 0x01 */
	uint16_t ext_mem_k;        /* 0x02 */
	uint16_t orig_video_page;  /* 0x04 */
	uint8_t  orig_video_mode;  /* 0x06 */
	// monochrome display		0x07
	// 320x200/4				0x0D
	// 640x200/4				0x0E
	// 640x350/4				0x10
	// 640x480/4				0x12
	// 800x600/4				0x6A

	uint8_t  orig_video_cols;  /* 0x07 */
	uint16_t unused2;          /* 0x08 */
	uint16_t orig_video_ega_bx;/* 0x0a */
	uint16_t unused3;          /* 0x0c */
	uint8_t  orig_video_lines; /* 0x0e */
	uint8_t  orig_video_isVGA; /* 0x0f */
	uint16_t orig_video_points;/* 0x10 */
  
	/* VESA graphic mode -- linear frame buffer */
	uint16_t lfb_width;        /* 0x12 */
	uint16_t lfb_height;       /* 0x14 */
	uint16_t lfb_depth;        /* 0x16 */
	uint32_t lfb_base;         /* 0x18 */
	uint32_t lfb_size;         /* 0x1c */
	uint16_t cl_magic, cl_offset; /* 0x20 */
	uint16_t lfb_linelength;   /* 0x24 */
	uint8_t  red_size;         /* 0x26 */
	uint8_t  red_pos;          /* 0x27 */
	uint8_t  green_size;       /* 0x28 */
	uint8_t  green_pos;        /* 0x29 */
	uint8_t  blue_size;        /* 0x2a */
	uint8_t  blue_pos;         /* 0x2b */
	uint8_t  rsvd_size;        /* 0x2c */
	uint8_t  rsvd_pos;         /* 0x2d */
	uint16_t vesapm_seg;       /* 0x2e */
	uint16_t vesapm_off;       /* 0x30 */
	uint16_t pages;            /* 0x32 */
	uint16_t vesa_attributes;  /* 0x34 */
	uint32_t capabilities;     /* 0x36 */
	uint8_t  _reserved[6];     /* 0x3a */
 } __attribute__((packed)) screen_info;
//
// E820 Memory types: (from linux/include/asm-i386/e820.h)
#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3
#define E820_NVS        4
#define E820MAX			128
//
struct e820entry {
	uint64_t	addr;			// start of memory segment
	uint64_t	size;			// size  of memory segment
	uint32_t	type;			// type  of memory segment
} __attribute__((packed));
//
struct e820map {
	uint32_t	nr_map;
	struct e820entry map[E820MAX];
};
//
// (linux/include/linux/edd.h)
#define EDD_MBR_SIG_MAX 16		/* max number of signatures to store */
#define EDDMAXNR 6				/* number of edd_info structs starting at EDDBUF  *///
// (linux/include/video/edid.h)
struct edid_info {
	unsigned char dummy[128];
};
//
// (linux/include/asm-x86/ist.h)
struct ist_info {
	uint32_t	signature;
	uint32_t	command;
	uint32_t	event;
	uint32_t	perf_level;
};
//
// (linux/include/linux/apm_bios.h)
typedef struct _apm_bios_info {
	uint16_t	version;
	uint16_t	cseg;
	uint32_t	offset;
	uint16_t	cseg_16;
	uint16_t	dseg;
	uint16_t	flags;
	uint16_t	cseg_len;
	uint16_t	cseg_16_len;
	uint16_t	dseg_len;
} apm_bios_info;
//
// (include/asm-x86/bootparam.h)
typedef struct _setup_header {
	uint8_t		setup_sects;
	uint16_t	root_flags;
	uint32_t	syssize;
	uint16_t	ram_size;
	#define	RAMDISK_IMAGE_START_MASK        0x07FF
	#define RAMDISK_PROMPT_FLAG             0x8000
	#define RAMDISK_LOAD_FLAG               0x4000
	uint16_t	vid_mode;
	uint16_t	root_dev;
	uint16_t	boot_flag;
	uint16_t	jump;
	uint32_t	header;
	uint16_t	version;
	uint32_t	realmode_swtch;
	uint16_t	start_sys;
	uint16_t	kernel_version;
	uint8_t		type_of_loader;
	uint8_t		loadflags;
	#define LOADED_HIGH     (1<<0)
	#define KEEP_SEGMENTS   (1<<6)
	#define CAN_USE_HEAP    (1<<7)
	uint16_t	setup_move_size;
	uint32_t	code32_start;
	uint32_t	ramdisk_image;
	uint32_t	ramdisk_size;
	uint32_t	bootsect_kludge;
	uint16_t	heap_end_ptr;
	uint16_t	_pad1;
	uint32_t	cmd_line_ptr;
	uint32_t	initrd_addr_max;
	uint32_t	kernel_alignment;
	uint8_t		relocatable_kernel;
	uint8_t		_pad2[3];
	uint32_t	cmdline_size;
	uint32_t	hardware_subarch;
	uint64_t	hardware_subarch_data;
} __attribute__((packed)) setup_header;
//
typedef struct _sys_desc_table {
	uint16_t	length;
	uint8_t		table[14];
} sys_desc_table;
//
struct efi_info {
	uint32_t	_pad1;
	uint32_t	efi_systab;
	uint32_t	efi_memdesc_size;
	uint32_t	efi_memdesc_version;
	uint32_t	efi_memmap;
	uint32_t	efi_memmap_size;
	uint32_t _pad2[2];
};
// The so-called "zeropage"
typedef struct _boot_params {
	screen_info		screen_info;							/* 0x000 */
	apm_bios_info	apm_bios_info;							/* 0x040 */
	uint8_t			_pad2[12];								/* 0x054 */
	struct ist_info ist_info;								/* 0x060 */
	uint8_t			_pad3[16];								/* 0x070 */
	uint8_t			hd0_info[16];	/* obsolete! */			/* 0x080 */
	uint8_t			hd1_info[16];	/* obsolete! */			/* 0x090 */
	sys_desc_table	sys_desc_table;							/* 0x0a0 */
	uint8_t			_pad4[144];								/* 0x0b0 */
	struct edid_info edid_info;								/* 0x140 */
	struct efi_info	efi_info;								/* 0x1c0 */
	uint32_t		alt_mem_k;								/* 0x1e0 */
	uint32_t		scratch;		/* Scratch field! */    /* 0x1e4 */
	uint8_t			e820_entries;							/* 0x1e8 */
	uint8_t			eddbuf_entries;							/* 0x1e9 */
	uint8_t			edd_mbr_sig_buf_entries;				/* 0x1ea */
	uint8_t			_pad6[6];								/* 0x1eb */
	setup_header	hdr;			/* setup header */		/* 0x1f1 */
	uint8_t			_pad7[ 0x290 - 0x1f1 - sizeof(setup_header) ];
	uint32_t		edd_mbr_sig_buffer[EDD_MBR_SIG_MAX];	/* 0x290 */
	struct e820entry e820_map[E820MAX];						/* 0x2d0 */
	uint8_t			_pad8[48];								/* 0xcd0 */
	//struct edd_info eddbuf[EDDMAXNR];						/* 0xd00 */
	//uint8_t		_pad9[276];								/* 0xeec */
} __attribute__((packed)) boot_params;

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
#pragma pack(1)
/* The header of Linux/i386 kernel */
typedef struct linux_header {
    uint8_t  reserved1[0x1f1];								/* 0x000 */
    uint8_t  setup_sects;									/* 0x1f1 */
    uint16_t root_flags;									/* 0x1f2 */
    uint8_t  reserved2[6];									/* 0x1f4 */
    uint16_t vid_mode;										/* 0x1fa */
    uint16_t root_dev;										/* 0x1fc */
    uint16_t boot_sector_magic;								/* 0x1fe */
    /* 2.00+ */
    uint8_t  reserved3[2];									/* 0x200 */
    uint8_t  header_magic[4];								/* 0x202 */
    uint16_t protocol_version;								/* 0x206 */
    uint32_t realmode_swtch;								/* 0x208 */
    uint16_t start_sys;										/* 0x20c */
    uint16_t kver_addr;										/* 0x20e */
    uint8_t  type_of_loader;								/* 0x210 */
    uint8_t  loadflags;										/* 0x211 */
    uint16_t setup_move_size;								/* 0x212 */
    uint32_t code32_start;									/* 0x214 */
    uint32_t ramdisk_image;									/* 0x218 */
    uint32_t ramdisk_size;									/* 0x21c */
    uint8_t  reserved4[4];									/* 0x220 */
    /* 2.01+ */
    uint16_t heap_end_ptr;									/* 0x224 */
    uint8_t  reserved5[2];									/* 0x226 */
    /* 2.02+ */
    uint32_t cmd_line_ptr;									/* 0x228 */
    /* 2.03+ */
    uint32_t initrd_addr_max;								/* 0x22c */
} linux_header_t;

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
typedef union ia32_boot_params {
	uint8_t raw[0x2000];
	struct {
/* Cursor position before passing control to kernel. */
/* 0x00 */	uint8_t orig_x;			/* LDR */
/* 0x01 */	uint8_t orig_y;			/* LDR */

/* Available contiguous extended memory in KB. */
/* 0x02 */	uint16_t ext_mem_k;		/* LDR */

/* Video page, mode and screen width before passing control to kernel. */
/* 0x04 */	uint16_t orig_video_page;		/* LDR */
/* 0x06 */	uint8_t orig_video_mode;		/* LDR */
/* 0x07 */	uint8_t orig_video_cols;		/* LDR */

/* 0x08 */	uint16_t unused_1;		/* unused */

/* %%TBD */
/* 0x0A */	uint16_t orig_ega_bx;		/* LDR */

/* 0x0C */	uint16_t unused_2;		/* unused */

/* Screen height before passing control to kernel. */
/* 0x0E */	uint8_t orig_video_rows;		/* LDR */

/* %%TBD */
/* 0x0F */	uint8_t is_vga;			/* LDR */
#define VIDEO_TYPE_VLFB		0x23    /* VESA VGA in graphic mode     */
#define VIDEO_TYPE_EFI		0x70	/* EFI graphic mode		*/
/* 0x10 */	uint16_t orig_video_points;	/* LDR */

/* %%TBD */
/* 0x12 */	uint16_t lfb_width;		/* LDR */
/* 0x14 */	uint16_t lfb_height;		/* LDR */
/* 0x16 */	uint16_t lfb_depth;		/* LDR */
/* 0x18 */	uint32_t lfb_base;		/* LDR */
/* 0x1C */	uint32_t lfb_size;		/* LDR */

/* Offset of command line (from start of ia32_boot_param struct). */
/* The command line magik number must be set for the kernel setup */
/* code to use the command line offset. */
/* 0x20 */	uint16_t cmdline_magik;		/* LDR */
#define CMDLINE_MAGIK		0xA33F
/* 0x22 */	uint16_t cmdline_offset;		/* LDR */

/* %%TBD */
/* 0x24 */	uint16_t lfb_line_len;		/* LDR */

/* %%TBD */
/* 0x26 */	uint8_t lfb_red_size;		/* LDR */
/* 0x27 */	uint8_t lfb_red_pos;		/* LDR */
/* 0x28 */	uint8_t lfb_green_size;		/* LDR */
/* 0x29 */	uint8_t lfb_green_pos;		/* LDR */
/* 0x2A */	uint8_t lfb_blue_size;		/* LDR */
/* 0x2B */	uint8_t lfb_blue_pos;		/* LDR */
/* 0x2C */	uint8_t lfb_rsvd_size;		/* LDR */
/* 0x2D */	uint8_t lfb_rsvd_pos;		/* LDR */

/* %%TBD */
/* 0x2E */	uint16_t vesa_seg;		/* LDR */
/* 0x30 */	uint16_t vesa_off;		/* LDR */

/* %%TBD */
/* 0x32 */	uint16_t lfb_pages;		/* LDR */
/* 0x34 */	uint8_t lfb_reserved[0x0C];	/* reserved */

/* %%TBD */
/* 0x40 */	uint16_t apm_bios_ver;		/* LDR */
#define NO_APM_BIOS		0x0000

/* %%TBD */
/* 0x42 */	uint16_t bios_code_seg;		/* LDR */
/* 0x44 */	uint32_t bios_entry_point;	/* LDR */
/* 0x48 */	uint16_t bios_code_seg16;		/* LDR */
/* 0x4A */	uint16_t bios_data_seg;		/* LDR */

/* %%TBD */
/* 0x4C */	uint16_t apm_bios_flags;		/* LDR */
#define NO_32BIT_APM_MASK	0xFFFD

/* %%TBD */
/* 0x4E */	uint32_t bios_code_len;		/* LDR */
/* 0x52 */	uint16_t bios_data_len;		/* LDR */

/* 0x54 */	uint8_t unused_3[0x2C];		/* unused */

/* %%TBD */
/* 0x80 */	uint8_t hd0_info[0x10];		/* LDR */
/* 0x90 */	uint8_t hd1_info[0x10];		/* LDR */

/* %%TBD */
/* 0xA0 */	uint16_t mca_info_len;		/* LDR */
/* 0xA2 */	uint8_t mca_info_buf[0x10];	/* LDR */

/* 0xB2 */	uint8_t unused_4[0x10E];		/* unused */

/* EFI boot loader signature. */
/* 0x1C0 */	uint8_t efi_loader_sig[4];	/* LDR */
#define EFI_LOADER_SIG		"EFIL"
#define EFI_LOADER_SIG_IA32	"EL32"

/* Address of the EFI system table. */
/* 0x1C4 */	uint32_t efi_sys_tbl;		/* LDR */

/* EFI memory descriptor size. */
/* 0x1C8 */	uint32_t efi_mem_desc_size;	/* LDR */

/* EFI memory descriptor version. */
/* 0x1CC */	uint32_t efi_mem_desc_ver;	/* LDR */

/* Address & size of EFI memory map. */
/* 0x1D0 */	uint32_t efi_mem_map;		/* LDR */
/* 0x1D4 */	uint32_t efi_mem_map_size;	/* LDR */

/* Address & size of loader. */
/* 0x1D8 */	uint32_t loader_start;		/* LDR */
/* 0x1DC */	uint32_t loader_size;		/* LDR */

/* Available contiguous extended memory in KB. */
/* 0x1E0 */	uint32_t alt_mem_k;		/* LDR */

/* 0x1E4 */	uint32_t unused_51;		/* unused */
/* 0x1E8 */	uint8_t  e820_nrmap;
/* 0x1E9 */	uint32_t unused_52[2];		/* unused */

/* Size of setup code in sectors (1 sector == 512 bytes). */
/* 0x1F1 */	uint8_t setup_sectors;		/* BLD */

/* %%TBD */
/* 0x1F2 */	uint16_t mount_root_rdonly;	/* BLD */

/* %%TBD */
/* 0x1F4 */	uint16_t sys_size;		/* BLD */

/* %%TBD */
/* 0x1F6 */	uint16_t swap_dev;		/* BLD */

/* %%TBD */
/* 0x1F8 */	uint16_t ramdisk_flags;		/* BLD */
#define RAMDISK_PROMPT		0x8000
#define RAMDISK_LOAD		0x4000

/* %%TBD */
/* 0x1FA */	uint16_t video_mode_flag;		/* BLD */

/* %%TBD */
/* 0x1FC */	uint16_t orig_root_dev;		/* BLD */

/* 0x1FE */	uint8_t unused_6;			/* unused */

/* %%TBD */
/* 0x1FF */	uint8_t aux_dev_info;		/* LDR */
#define NO_MOUSE		0x00
#define FOUND_MOUSE		0xAA

/* Jump past setup data (not used in EFI). */
/* 0x200 */	uint16_t jump;			/* BLD */

/* Setup data signature. */
/* 0x202 */	uint8_t setup_sig[4];		/* BLD */
#define SETUP_SIG		"HdrS"

/* %%TBD */
/* 0x206 */	uint8_t hdr_minor;		/* BLD */
/* 0x207 */	uint8_t hdr_major;		/* BLD */

/* %%TBD */
/* 0x208 */	uint32_t rm_switch;		/* LDD */

/* %%TBD */
/* 0x20C */	uint16_t start_sys_seg;		/* BLD */

/* %%TBD */
/* 0x20E */	uint16_t kernel_verstr_offset;	/* BLD */

/* Loader type & version. */
/* 0x210 */	uint8_t loader_type;		/* LDR */
#define LDRTYPE_ELILO			0x50	/* 5?h == elilo */
						/* ?0h == revision */

/* 0x211 */	uint8_t loader_flags;		/* BLD and LDR */
#define LDRFLAG_LOADED_HIGH			0x01
#define LDRFLAG_CAN_USE_HEAP		0x80
#define LDRFLAG_BOOT_PARAM_RELOC	0x40

/* %%TBD */
/* 0x212 */	uint16_t setup_move_size;		/* BLD */

/* %%TBD */
/* 0x214 */	uint32_t kernel_start;		/* LDR */

/* %%TBD */
/* 0x218 */	uint32_t initrd_start;		/* LDR */
/* 0x21C */	uint32_t initrd_size;		/* LDR */

/* %%TBD */
/* 0x220 */	uint32_t bootsect_helper;		/* BLD */

/* %%TBD */
/* 2.01+ */
/* 0x224 */	uint16_t heap_end_ptr;		/* LDR */

/* %%TBD */
/* 0x226 */	uint16_t unused_7;		/* LDR */

/* 2.02+ */
/* 0x228 */	uint32_t cmdline_addr; 		/* LDR */
/* 2.03+ */
/* 0x22c *uint32_t initrd_addr_max;	sdd*/	
/* 0x22C */	uint32_t unused_8[41];
/* 0x2D0 */	uint8_t  e820_map[2560];
	} s;
} boot_params_t;
#pragma pack()

/* 
If the Linux kernel boot protocol is 0x205 or later, and the flag 
at offset 0x234 in the kernel header is 1, then the guest 
kernel was built with CONFIG_RELOCATABLE=y

In this scenario we merely need to tell the kernel what address 
it has been relocated to by writing 0x200000 into the kernel 
header at offset 0x214.

This should work for 2.6.20 or later on i386.
*/

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
extern dt_addr_t gdt_addr;
extern dt_addr_t idt_addr;

extern uint16_t init_gdt[];
extern uint32_t init_gdt_size;

#endif
