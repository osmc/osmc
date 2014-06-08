#ifndef __CODE_PAGE_H_
#define __CODE_PAGE_H_
int convert_utf8dec_to_utf8pre(const char *src, int src_len,
	char * dest, int dest_len);
int convert_utf8pre_to_utf8dec(const char * src, int src_len, 
	char * dest, int dest_len);
int convert_path_to_unix(char encoding, char * dest, 
	char * src, int dest_len);
int convert_path_to_afp(char encoding, char * dest, 
	char * src, int dest_len);
#endif
