#include	"utils.h"
#include	"darwin_code.h"

// darwin/src/modules/cctools/libmacho/getsecbyname.c
/**********************************************************************/
// This routine returns the section structure for the named section in the
// named segment for the mach_header pointer passed to it if it exist.
// Otherwise it returns zero.
const struct section*
getsectbynamefromheader(struct mach_header *mhp, const char *segname, const char *sectname)
{
	struct segment_command	*sgp;
	struct section			*sp;
	long					i, j;
        
	//printk("getsectbynamefromheader\n");
	sgp = (struct segment_command*)((char*)mhp + sizeof(struct mach_header));
	for(i = 0; i < mhp->ncmds; i++){
	    if (sgp->cmd == LC_SEGMENT) {
			//printk("sgp->segname = %s, matching to %s.\n", sgp->segname, segname);

			//if (strncmp(sgp->segname, segname, sizeof(sgp->segname)) == 0 || mhp->filetype == MH_OBJECT) {
			if (strncmp(sgp->segname, segname, strlen(sgp->segname)) == 0 || mhp->filetype == MH_OBJECT) {
				sp = (struct section*)((char*)sgp + sizeof(struct segment_command));
				for(j = 0; j < sgp->nsects; j++) {
					//if (strncmp(sp->sectname, sectname, sizeof(sp->sectname)) == 0 &&
					//	strncmp(sp->segname,  segname,  sizeof(sp->segname)) == 0) {
					if (strncmp(sp->sectname, sectname, strlen(sp->sectname)) == 0 &&
						strncmp(sp->segname,  segname,  strlen(sp->segname)) == 0) {
							printk("ATV: found - section %s and segment %s\n", sectname, segname);
							return(sp);
					}
					//
					sp = (struct section *)((char *)sp + sizeof(struct section));
				}
			}
		}
	    sgp = (struct segment_command *)((char *)sgp + sgp->cmdsize);
	}
	printk("ATV: not found - section %s and segment %s\n", sectname, segname);
	//
	return((struct section *)0);
}
/**********************************************************************/
// This routine returns the a pointer to the data for the named section in the
// named segment if it exist in the mach header passed to it.  Also it returns
// the size of the section data indirectly through the pointer size.  Otherwise
//  it returns zero for the pointer and the size.
char*
getsectdatafromheader(struct mach_header *mhp, const char *segname, const char *sectname, unsigned long *size)
{
	const struct section	*sp;

	//printk("getsectdatafromheader\n");
	sp = getsectbynamefromheader(mhp, segname, sectname);
	if(sp == (struct section*)0) {
		*size = 0;
		return((char*)0);
	}
	*size = sp->size;
	//
	return( (char*)(sp->addr) );
}
//
