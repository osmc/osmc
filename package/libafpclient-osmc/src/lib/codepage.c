
/*
 *  codepage.c
 *
 *  Copyright (C) 2007 Alex deVries
 *
 *  These routines handle code page conversions.
 *
 *  Currenly, only UTF8 is supported, but the structure should allow
 *  for classic code pages to be added.
 *
 */


#include <string.h>
#include <stdlib.h>
#include "afp_protocol.h"
#include "utils.h"
#include "unicode.h"

int convert_utf8dec_to_utf8pre(const char *src, int src_len,
	char * dest, int dest_len);
int convert_utf8pre_to_utf8dec(const char * src, int src_len, 
	char * dest, int dest_len);

/* 
 * convert_path_to_unix()
 *
 * This converts an AFP-generated path to Unix's UTF8.  This function
 * does the appropriate encoding lookup.
 */

int convert_path_to_unix(char encoding, char * dest, 
	char * src, int dest_len)
{

	memset(dest,0,dest_len);

	switch (encoding) {
	case kFPUTF8Name:
		convert_utf8dec_to_utf8pre(src, strlen(src), dest, dest_len);
		break;
	case kFPLongName:
		memcpy(dest,src,dest_len);
		break;
	/* This is where you would put support for other codepages. */
	default:
		return -1;
	}
	return 0;
}

/* 
 * convert_path_to_afp()
 *
 * Given a null terminated source, converts the path to an AFP path
 * given the encoding.
 */

int convert_path_to_afp(char encoding, char * dest, 
	char * src, int dest_len)
{
	unsigned char namelen;

	memset(dest,0,dest_len);

	switch (encoding) {
	case kFPUTF8Name: 
		namelen=convert_utf8pre_to_utf8dec(src, strlen(src),
			dest,dest_len);
		break;
	case kFPLongName:
		memcpy(dest,src,dest_len);
		break;
	/* This is where you would put support for other codepages. */
	default:
		return -1;
	}
	return 0;
}

/* convert_utf8dec_to_utf8pre()
 *
 * Conversion for text from Decomposed UTF8 used in AFP to Precomposed
 * UTF8 used elsewhere.
 *
 */

/* This is for converting *from* UTF-8-MAC */

int convert_utf8dec_to_utf8pre(const char *src, int src_len,
	char * dest, int dest_len)
{
	char16 *path16dec, c, prev, *p16dec, *p16pre;
        char16 path16pre[384];  // max 127 * 3 byte UTF8 characters
        char *pathUTF8pre, *p8pre;
        int comp;

	path16dec = UTF8toUCS2(src);
        p16dec = path16dec;
	p16pre = path16pre;

        prev = 0;
        while(*p16dec > 0) {
		c = *p16dec;
		if(prev > 0) {
			comp = UCS2precompose(prev, c);
			if(comp != -1) {
				prev = (char16)comp;  // Keep and try to combine again on next loop
			}
			else {
				*p16pre = prev;
				prev = c;
				p16pre++;
			}
		}
		else {
			prev = c;
		}
		p16dec++;

		if(*p16dec == 0) {		// End of string?
			*p16pre = prev;		// Add last char
			p16pre++;
		}
	}
        *p16pre = 0; // Terminate string

        pathUTF8pre = UCS2toUTF8(path16pre);
        p8pre = pathUTF8pre;

        while(*p8pre) {		// Copy precomposed UTF8 string to dest
		*dest = *p8pre;
                dest++;
                p8pre++;
	}
	*dest = 0;

        if(path16dec)
		free(path16dec);
        if(pathUTF8pre)
		free(pathUTF8pre);

        return 0;
}

/* convert_utf8pre_to_utf8dec()
 *
 * Conversion for text from Precomposed UTF8 to Decomposed UTF8.
 *
 * This is a sample conversion.  The only translation it does is on one 
 * sequence of characters (0xc3 0xa4 becomes 0x61 0xcc 0x88).
 *
 * Fix this.
 */

int convert_utf8pre_to_utf8dec(const char * src, int src_len, 
	char * dest, int dest_len)
{
	int i, j=0;
	for (i=0;i<src_len; i++) {
		if (((src[i] & 0xff)==0xc3) && ((src[i+1] & 0xff)==0xa4)) {
			dest[j]=0x61;
			j++;
			dest[j]=0xcc;
			j++;
			dest[j]=0x88;
			i++;
		} else 
			dest[j]=src[i];
		j++;

	}
	return j;
}

