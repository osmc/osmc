#ifndef __DID_H_
#define __DID_H_

int free_entire_did_cache(struct afp_volume * volume) ;
int remove_did_entry(struct afp_volume * volume, const char * name) ;
unsigned char is_dir(struct afp_volume * volume,
        unsigned int parentdid, const char * path);
int get_dirid(struct afp_volume * volume, const char * path,
        char * basename, unsigned int * dirid);

#endif
