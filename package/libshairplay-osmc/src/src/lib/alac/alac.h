#ifndef __ALAC__DECOMP_H
#define __ALAC__DECOMP_H

typedef struct alac_file alac_file;

alac_file *create_alac(int samplesize, int numchannels);
void decode_frame(alac_file *alac,
                  unsigned char *inbuffer,
                  void *outbuffer, int *outputsize);
void alac_set_info(alac_file *alac, char *inputbuffer);
void destroy_alac(alac_file *alac);

#endif /* __ALAC__DECOMP_H */

