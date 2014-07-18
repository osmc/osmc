#ifndef __FORKLIST_H_
#define __FORKLIST_H_
void add_opened_fork(struct afp_volume * volume, struct afp_file_info * fp);
void remove_opened_fork(struct afp_volume * volume, struct afp_file_info * fp);
void remove_fork_list(struct afp_volume * volume); 
#endif
