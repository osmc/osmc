#ifndef _ELILO_CODE_H_
#define _ELILO_CODE_H_

#include "types.h"
#include "linux_code.h"

extern uint32_t high_base_mem;
extern uint32_t high_ext_mem;

extern void		*kernel_start;
extern uint32_t	kernel_size;

extern void		*initrd_start;
extern uint32_t initrd_size;


int		create_boot_params(boot_params_t *bp, char *cmdline);
void	start_kernel(void *kentry, boot_params_t *bp);

#endif
