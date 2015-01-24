#include	"types.h"
#include	"linux_code.h"
#include	"darwin_code.h"


// Descriptor table base addresses & limits for Linux startup.
dt_addr_t gdt_addr = { 0x800, 0x94000 };
dt_addr_t idt_addr = { 0, 0 }; 

// Initial GDT layout for Linux startup.
uint16_t init_gdt[] = {
	/* gdt[0]: (0x00) dummy */
	0, 0, 0, 0, 
	
	/* gdt[1]: (0x08) unused */
	0, 0, 0, 0,

	/* Documented linux kernel segments */
	/* gdt[2]: (0x10) flat code segment */
	0xFFFF,		/* 4Gb - (0x100000*0x1000 = 4Gb) */
	0x0000,		/* base address=0 */
	0x9A00,		/* code read/exec */
	0x00CF,		/* granularity=4096, 386 (+5th nibble of limit) */
	/* gdt[3]: (0x18) flat data segment */
	0xFFFF,		/* 4Gb - (0x100000*0x1000 = 4Gb) */
	0x0000,		/* base address=0 */
	0x9200,		/* data read/write */
	0x00CF,		/* granularity=4096, 386 (+5th nibble of limit) */

	/* gdt[4]: (0x20) unused */
	0, 0, 0, 0,
	/* gdt[5]: (0x28) unused */
	0, 0, 0, 0,

	/* gdt[6]: (0x30) unused */
	0, 0, 0, 0,
	/* gdt[7]: (0x38) unused */
	0, 0, 0, 0,

	/* gdt[8]: (0x40) unused */
	0, 0, 0, 0,
	/* gdt[9]: (0x48) unused */
	0, 0, 0, 0,

	/* gdt[10]:(0x50) unused */
	0, 0, 0, 0,
	/* gdt[11]:(0x58) unused */
	0, 0, 0, 0,

	/* Segments used by the 2.5.x kernel */
	/* gdt[12]:(0x60) flat code segment */
	0xFFFF,		/* 4Gb - (0x100000*0x1000 = 4Gb) */
	0x0000,		/* base address=0 */
	0x9A00,		/* code read/exec */
	0x00CF,		/* granularity=4096, 386 (+5th nibble of limit) */
	/* gdt[13]:(0x68) flat data segment */
	0xFFFF,		/* 4Gb - (0x100000*0x1000 = 4Gb) */
	0x0000,		/* base address=0 */
	0x9200,		/* data read/write */
	0x00CF,		/* granularity=4096, 386 (+5th nibble of limit) */
};
 uint32_t init_gdt_size = sizeof(init_gdt);
 
 

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
/* Convert EFI memory map to E820 map for the operating system
 * This code is based on a Linux kernel patch submitted by Edgar Hucek
 */
/* Add a memory region to the e820 map */
static void add_memory_region(struct e820entry *e820_map,
	int *e820_nr_map,
	UINT64 start,
	UINT64 size,
	UINT32 type)
{
	int x = *e820_nr_map;

	if (x == E820MAX) {
		printk(L"ATV: Too many entries in the memory map!\n");
		return;
	}

	if ((x > 0) && e820_map[x-1].addr + e820_map[x-1].size == start
	    && e820_map[x-1].type == type)
		e820_map[x-1].size += size;
	else {
		e820_map[x].addr = start;
		e820_map[x].size = size;
		e820_map[x].type = type;
		(*e820_nr_map)++;
	}
}

/*------------------------------------------------------------------------------*/
void fill_e820map(boot_params_t *bp)
{
	int						nr_map, e820_nr_map = 0, i;
	UINT64					start, end, size;
	efi_memory_desc_t		*md, *p;
	struct e820entry		*e820_map;

	nr_map = bp->s.efi_mem_map_size/bp->s.efi_mem_desc_size;
	e820_map = (struct e820entry *)bp->s.e820_map;

	for (i = 0, p = (efi_memory_desc_t*)bp->s.efi_mem_map; i < nr_map; i++) {
		md = p;
		switch (md->type) {
			// ACPI tables -- to be preserved by loader/OS until ACPI is enable
			// once enabled, can be treated as conventional memory
			case EFI_ACPI_RECLAIM_MEMORY:
				add_memory_region(e820_map, &e820_nr_map,
						  md->phys_addr,
						  md->num_pages << EFI_PAGE_SHIFT,
						  E820_ACPI);
			break;
			// must be preserved by loader/OS in working an ACPI S1-S3 states
			case EFI_RUNTIME_SERVICES_CODE:
			case EFI_RUNTIME_SERVICES_DATA:
			case EFI_RESERVED_TYPE:
			case EFI_MEMORY_MAPPED_IO:
			case EFI_MEMORY_MAPPED_IO_PORT_SPACE:
			case EFI_UNUSABLE_MEMORY:
			case EFI_PAL_CODE:
				add_memory_region(e820_map, &e820_nr_map,
						  md->phys_addr,
						  md->num_pages << EFI_PAGE_SHIFT,
						  E820_RESERVED);
			break;
			// can be treaded as conventional memory by loader/OS
			case EFI_LOADER_CODE:
			case EFI_LOADER_DATA:
			case EFI_BOOT_SERVICES_CODE:
			case EFI_BOOT_SERVICES_DATA:
			case EFI_CONVENTIONAL_MEMORY:
				start = md->phys_addr;
				size  = md->num_pages << EFI_PAGE_SHIFT;
				end   = start + size;
				/* Fix up for BIOS that claims RAM in 640K-1MB region */
				if (start < 0x100000ULL && end > 0xA0000ULL) {
					if (start < 0xA0000ULL) {
						/* start < 640K
						 * set memory map from start to 640K
						 */
						add_memory_region(e820_map,
								  &e820_nr_map,
								  start,
								  0xA0000ULL-start,
								  E820_RAM);
					}
					if (end <= 0x100000ULL) {
						continue;
					}
					// end > 1MB, set memory map avoiding 640K to 1MB hole
					start = 0x100000ULL;
					size = end - start;
				}
				add_memory_region(e820_map, &e820_nr_map,
						  start, size, E820_RAM);
			break;
			// ACPI working memory --- should be preserved by loader/OS in the working
			//  and ACPI S1-S3 states
			case EFI_ACPI_MEMORY_NVS:
				add_memory_region(e820_map, &e820_nr_map,
						  md->phys_addr,
						  md->num_pages << EFI_PAGE_SHIFT,
						  E820_NVS);
			break;
			default:
				/* We should not hit this case */
				printk("ATV: default add_memory_region, should not see this\n");
				add_memory_region(e820_map, &e820_nr_map,
						  md->phys_addr,
						  md->num_pages << EFI_PAGE_SHIFT,
						  E820_RESERVED);
			break;
		}
		p = (efi_memory_desc_t*)NextEFIMemoryDescriptor(p, bp->s.efi_mem_desc_size);
	}
	bp->s.e820_nrmap = e820_nr_map;
}

/*------------------------------------------------------------------------------*/
void print_e820_memory_map(boot_params_t *bp)
{
	int					i;
	struct e820entry	*e820_map;
	
	e820_map = (struct e820entry*)bp->s.e820_map;

	for (i = 0; i < bp->s.e820_nrmap; i++) {
		printk("ATV: %s: 0x%08X%08X - 0x%08X%08X ", "E820 Map",
			hi32( e820_map[i].addr ),
			lo32( e820_map[i].addr ),
			hi32( e820_map[i].addr + e820_map[i].size),
			lo32( e820_map[i].addr + e820_map[i].size) );
		switch (e820_map[i].type) {
			case E820_RAM:
				printk("(usable)\n");
			break;
			case E820_RESERVED:
				printk("(reserved)\n");
			break;
			case E820_ACPI:
				printk("(ACPI data)\n");
			break;
			case E820_NVS:
				printk("(ACPI NVS)\n");
			break;
			default:
				printk("type %u\n", e820_map[i].type);
			break;
		}
	}
}

/*------------------------------------------------------------------------------*/
void quirk_fixup_efi_memmap(boot_params_t *bp)
{
	/* November 26, 2007 -- Scott Davilla (davilla@4pi.com)
	  The appletv efi firmware has a bug that effects linux kernel when
	booting from efi. Three EFI RunTime Services Code/Data segments overlap
	a declared free ememory segment. This can cause code/data overwrites
	and result in unknown crashes/hangs when running linux.
	*/
	int                     num_maps, i;
	UINT64                  bgn, end, bgn_match, end_match;
	efi_memory_desc_t       *md, *p;

	bgn_match = end_match = -1;
	
	num_maps = bp->s.efi_mem_map_size/bp->s.efi_mem_desc_size;

	// gather up the offending memory ranges
	// these are the two EFI_RUNTIME_SERVICES_CODE and one EFI_RUNTIME_SERVICES_DATA
	// memmap sections. This routine assumes that the sections will appear in order
	// which they seem to always do for the appleTV
	for (i = 0, p = (efi_memory_desc_t*)bp->s.efi_mem_map; i < num_maps; i++) {
		md = p;

		bgn = md->phys_addr;
		end = md->phys_addr + (md->num_pages << EFI_PAGE_SHIFT);
		//
		switch (md->type) {
			case EFI_RUNTIME_SERVICES_CODE:
			case EFI_RUNTIME_SERVICES_DATA:
				if (bgn_match == -1) {
					bgn_match = md->phys_addr;
					end_match = md->phys_addr + (md->num_pages << EFI_PAGE_SHIFT);
				} else {
					if (end_match == md->phys_addr) {
						end_match = md->phys_addr + (md->num_pages << EFI_PAGE_SHIFT);
					}
				}
				/*
				printk("mem%02d: type=%d, ", i, md->Type );
				printk("attr=0x%08X%08X\n", hi32(md->Attribute), lo32(md->Attribute) );
				printk("   range=[0x%08X%08X-", hi32(bgn), lo32(bgn) );
				printk("0x%08X%08X], ", hi32(end), lo32(end) );
				printk("%dMB\n", lo32(md->NumberOfPages >> (20 - EFI_PAGE_SHIFT)) );
				*/
			break;
		}
		p = NextEFIMemoryDescriptor(p, bp->s.efi_mem_desc_size);
	}

	for (i = 0, p = (efi_memory_desc_t*)bp->s.efi_mem_map; i < num_maps; i++) {
		md = p;

		bgn = md->phys_addr;
		end = md->phys_addr + (md->num_pages << EFI_PAGE_SHIFT);
		
		/*
		printk("mem%02d: type=%d, ", i, md->Type );
		printk("attr=0x%08X%08X\n", hi32(md->Attribute), lo32(md->Attribute) );
		printk("   range=[0x%08X%08X-", hi32(bgn), lo32(bgn) );
		printk("0x%08X%08X], ", hi32(end), lo32(end) );
		printk("%dMB\n", lo32(md->NumberOfPages >> (20 - EFI_PAGE_SHIFT)) );
		*/
		
		// find problem free memory segment */
		if ( (bgn == bgn_match) & (end >= end_match) ) {
			UINT64		new_bgn, new_end, new_pages;

			//printk("   found memory overlap\n");                                                       
			//printk("   memory range=[0x%08X%08X-", hi32(bgn), lo32(bgn) );
			//printk("0x%08X%08X]\n", hi32(end), lo32(end) );

			new_bgn = end_match;
			new_pages = (end - new_bgn) / (1 << EFI_PAGE_SHIFT);

			new_end = new_bgn + (new_pages << EFI_PAGE_SHIFT);
			printk("ATV:   fixing memory overlap\n");
			printk("ATV:   memory range=[0x%08X%08X-", hi32(new_bgn), lo32(new_bgn) );
			printk("ATV:     0x%08X%08X]\n", hi32(new_end), lo32(new_end) );

			md->phys_addr = new_bgn;
			md->num_pages = new_pages;
		}

		p = NextEFIMemoryDescriptor(p, bp->s.efi_mem_desc_size);
	}

	for (i = 0, p = (efi_memory_desc_t*)bp->s.efi_mem_map; i < num_maps; i++) {
		UINT64   target;

		target = 0x025AE000;
		md = p;

		bgn = md->phys_addr;
		end = md->phys_addr + (md->num_pages << EFI_PAGE_SHIFT);

		if ( (bgn < target) & (end > target) ) {
			UINT64          new_bgn, new_end, new_pages;


			new_bgn = bgn;
			new_pages = (target - new_bgn) / (1 << EFI_PAGE_SHIFT);

			new_end = new_bgn + (new_pages << EFI_PAGE_SHIFT);
			printk("ATV:   fixing memory target\n");

			md->phys_addr = new_bgn;
			md->num_pages = new_pages;

			md->num_pages = new_pages;
		}

		p = NextEFIMemoryDescriptor(p, bp->s.efi_mem_desc_size);
	}
}


