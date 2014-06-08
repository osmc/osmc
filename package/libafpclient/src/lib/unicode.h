/**********************************************************************
 *
 * unicode.h: Functions to handle UTF8/UCS2 coded strings.
 *
 * Most of these functions have been adopted from Roland Krause's
 * UTF8.c, which is part of the XawPlus package. See
 * http://freenet-homepage.de/kra/ for details.
 *
 * int str16len()	A strlen() on a char16 string
 * char16 *str16chr()	A strchr() on a char16 string
 * void str16cpy()	A strcpy() on a char16 string
 * void str16ncpy()	A strncpy() on a char16 string
 * void str16cat()	A strcat() on a char16 string
 *
 * int mbCharLen()	Calc number of byte of an UTF8 character
 * int mbStrLen()	Calc # of characters in an UTF8 string
 * char16 *UTF8toUCS2() Convert UTF8 string to UCS2/UNICODE
 * char *UCS2toUTF8()   Convert UCS2/UNICODE string to UTF8
 *
 * int UCS2precompose() Canonically combine two UCS2 characters
 *
 * Copyright (c) Roland Krause 2002, roland_krause@freenet.de
 * Copyright (c) Michael Ulbrich 2007, mul@rentapacs.de
 *
 * This module is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 **********************************************************************/

#ifndef unicode_h
#define unicode_h

#include <ctype.h>

/* The data type used for 16 bit character strings.
 * The format is handled compatible to *XChar2b* used by Xlib.
 */
typedef unsigned short char16;


/*	Function Name:	str16len
 *	Description: 	Determine the string length of a char16 string
 *			independent of the locale settings.
 *	Arguments:	str16	- A terminated string of char16's
 *	Returns:	Length in char16's
 */
extern int str16len(
#if NeedFunctionPrototypes
	char16 *	/* str16 */
#endif
);

/*	Function Name:	str16chr
 *	Description: 	Search an 8 bit character in a char16 string.
 *			The upper byte of *ch* is assumed as '0'!
 *	Arguments:	str16	- A terminated string of char16's
 *			ch	- An 8 bit character
 *	Returns:	Position of the leftmost occurance of *ch*
 *			in str16 or NULL.
 */
extern char16 *str16chr(
#if NeedFunctionPrototypes
	char16 *,	/* str16 */
	char		/* ch */
#endif
);

/*	Function Name:	str16cpy
 *	Description: 	Copy a string of char16's from *src* to *dest*
 *	Arguments:	dest	- Destination string
 *			src	- Source string
 *	Returns:	None
 */
extern void str16cpy(
#if NeedFunctionPrototypes
	char16 *,	/* dest */
	char16 *	/* src */
#endif
);

/*	Function Name:	str16ncpy
 *	Description: 	Copy *n* char16's from *src* to *dest* and
 *			terminate *dest*.
 *	Arguments:	dest	- Destination string
 *			src	- Source string
 *			n	- # of characters to copy
 *	Returns:	None	
 */
extern void str16ncpy(
#if NeedFunctionPrototypes
	char16 *,	/* dest */
	char16 *,	/* src */
	size_t		/* n */
#endif
);

/*	Function Name:	str16cat
 *	Description: 	Concatenate the string of char16's in *src* with *dest*.
 *	Arguments:	dest	- Destination string
 *			src	- Source string
 *	Returns:	None
 */
extern void str16cat(
#if NeedFunctionPrototypes
	char16 *,	/* dest */
	char16 *	/* src */
#endif
);

/*	Function Name:	mbCharLen
 *	Description: 	Determine the length in byte of an UTF8 coded
 *			character.
 *	Arguments:	str	- Pointer into an UTF8 coded string
 *	Returns:	Number of byte of the next character in the string
 *			or 0 in case of an error.
 */
extern int mbCharLen(
#if NeedFunctionPrototypes
	char *		/* str */
#endif
);

/*	Function Name:	mbStrLen
 *	Description: 	Determine the string length of an UTF8 coded string
 *			in characters (not in byte!).
 *	Arguments:	str	- The UTF8 coded string
 *	Returns:	The length in characters, illegal coded bytes
 *			are counted as one character per byte.
 *			See UTF8toUCS2() for the reason!
 */
extern int mbStrLen(
#if NeedFunctionPrototypes
	char *		/* str */
#endif
);

/*	Function Name:	UTF8toUCS2
 *	Description: 	Conversion of an UTF8 coded string into UCS2/UNICODE.
 *			If the encoding of the character is not representable
 *			in two bytes, the tilde sign ~ is written into the
 *			result string at this position.
 *			For an illegal UTF8 code an asterix * is stored in
 *			the result string.
 *	Arguments:	str	- The UTF8 coded string
 *	Returns:	The UCS2 coded result string. The allocated memory
 *			for this string has to be freed by the caller!
 *			The result string is stored independent of the
 *			architecture in the high byte/low byte order and is
 *			compatible to the XChar2b format! Type casting is valid.
 *			char16 is used to increase the performance.
 */
extern char16 *UTF8toUCS2(
#if NeedFunctionPrototypes
	char *		/* str */
#endif
);

/*      Function Name:  UCS2toUTF8
 *      Description:    Conversion of an UCS2 coded string into UTF8.
 *      Arguments:      str16     - The UCS2 coded string
 *      Returns:        The UTF8 coded result string. The allocated memory
 *                      for this string has to be freed by the caller!
 */
extern char *UCS2toUTF8(
#if NeedFunctionPrototypes
        char16 *          /* str */
#endif
);

/*      Function Name:  UCS2precompose
 *      Description:    Canonically combine two UCS2 characters, if matching
 *                      pattern is found in table. Uniform binary search
 *                      algorithm from D. Knuth TAOCP Vol.3 p.414.
 *      Arguments:      first   - the first UCS2 character
 *                      second  - the second UCS2 character
 *      Returns:        Canonical composition of first and second or
 *                      -1 if no such composition exists in table.
 */
extern int UCS2precompose(
#if NeedFunctionPrototypes
	char16,           /* first */
	char16            /* second */
#endif
);

#endif

