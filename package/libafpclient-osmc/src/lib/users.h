#ifndef __USERS_H_
#define __USERS_H_

int translate_uidgid_to_server(struct afp_volume * volume,
        unsigned int * newuid, unsigned int *newgid);

int translate_uidgid_to_client(struct afp_volume * volume,
        unsigned int * newuid, unsigned int *newgid);

int afp_detect_mapping(struct afp_volume * volume);


#endif
