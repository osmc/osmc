/* $Header: /cvsroot/osrs/libtiff/libtiff/tif_open.c,v 1.6 2001/07/20 02:22:46 warmerda Exp $ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library.
 */
#include "tiffiop.h"

void _TIFFSetDefaultCompressionState(TIFF* tif);

static const long typemask[13] = {
	0L,		/* TIFF_NOTYPE */
	0x000000ffL,	/* TIFF_BYTE */
	0xffffffffL,	/* TIFF_ASCII */
	0x0000ffffL,	/* TIFF_SHORT */
	0xffffffffL,	/* TIFF_LONG */
	0xffffffffL,	/* TIFF_RATIONAL */
	0x000000ffL,	/* TIFF_SBYTE */
	0x000000ffL,	/* TIFF_UNDEFINED */
	0x0000ffffL,	/* TIFF_SSHORT */
	0xffffffffL,	/* TIFF_SLONG */
	0xffffffffL,	/* TIFF_SRATIONAL */
	0xffffffffL,	/* TIFF_FLOAT */
	0xffffffffL,	/* TIFF_DOUBLE */
};
static const int bigTypeshift[13] = {
	0,		/* TIFF_NOTYPE */
	24,		/* TIFF_BYTE */
	0,		/* TIFF_ASCII */
	16,		/* TIFF_SHORT */
	0,		/* TIFF_LONG */
	0,		/* TIFF_RATIONAL */
	24,		/* TIFF_SBYTE */
	24,		/* TIFF_UNDEFINED */
	16,		/* TIFF_SSHORT */
	0,		/* TIFF_SLONG */
	0,		/* TIFF_SRATIONAL */
	0,		/* TIFF_FLOAT */
	0,		/* TIFF_DOUBLE */
};
static const int litTypeshift[13] = {
	0,		/* TIFF_NOTYPE */
	0,		/* TIFF_BYTE */
	0,		/* TIFF_ASCII */
	0,		/* TIFF_SHORT */
	0,		/* TIFF_LONG */
	0,		/* TIFF_RATIONAL */
	0,		/* TIFF_SBYTE */
	0,		/* TIFF_UNDEFINED */
	0,		/* TIFF_SSHORT */
	0,		/* TIFF_SLONG */
	0,		/* TIFF_SRATIONAL */
	0,		/* TIFF_FLOAT */
	0,		/* TIFF_DOUBLE */
};

/*
 * Initialize the shift & mask tables, and the
 * byte swapping state according to the file
 * contents and the machine architecture.
 */
static void
TIFFInitOrder(TIFF* tif, int magic, int bigendian)
{
	tif->tif_typemask = typemask;
	if (magic == TIFF_BIGENDIAN) {
		tif->tif_typeshift = bigTypeshift;
		if (!bigendian)
			tif->tif_flags |= TIFF_SWAB;
	} else {
		tif->tif_typeshift = litTypeshift;
		if (bigendian)
			tif->tif_flags |= TIFF_SWAB;
	}
}

int
_TIFFgetMode(const char* mode, const char* module)
{
	int m = -1;

	switch (mode[0]) {
	case 'r':
		m = O_RDONLY;
		if (mode[1] == '+')
			m = O_RDWR;
		break;
	case 'w':
	case 'a':
		m = O_RDWR|O_CREAT;
		if (mode[0] == 'w')
			m |= O_TRUNC;
		break;
	default:
		TIFFError(module, "\"%s\": Bad mode", mode);
		break;
	}
	return (m);
}

TIFF*
TIFFClientOpen(
	const char* name, const char* mode,
	thandle_t clientdata,
	TIFFReadWriteProc readproc,
	TIFFReadWriteProc writeproc,
	TIFFSeekProc seekproc,
	TIFFCloseProc closeproc,
	TIFFSizeProc sizeproc,
	TIFFMapFileProc mapproc,
	TIFFUnmapFileProc unmapproc
)
{
	static const char module[] = "TIFFClientOpen";
	TIFF *tif;
	int m, bigendian;
	const char* cp;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		goto bad2;
	tif = (TIFF *)_TIFFmalloc(sizeof (TIFF) + strlen(name) + 1);
	if (tif == NULL) {
		TIFFError(module, "%s: Out of memory (TIFF structure)", name);
		goto bad2;
	}
	_TIFFmemset(tif, 0, sizeof (*tif));
	tif->tif_name = (char *)tif + sizeof (TIFF);
	strcpy(tif->tif_name, name);
	tif->tif_mode = m &~ (O_CREAT|O_TRUNC);
	tif->tif_curdir = (tdir_t) -1;		/* non-existent directory */
	tif->tif_curoff = 0;
	tif->tif_curstrip = (tstrip_t) -1;	/* invalid strip */
	tif->tif_row = (uint32) -1;		/* read/write pre-increment */
	tif->tif_clientdata = clientdata;
	tif->tif_readproc = readproc;
	tif->tif_writeproc = writeproc;
	tif->tif_seekproc = seekproc;
	tif->tif_closeproc = closeproc;
	tif->tif_sizeproc = sizeproc;
	tif->tif_mapproc = mapproc;
	tif->tif_unmapproc = unmapproc;
	_TIFFSetDefaultCompressionState(tif);	/* setup default state */
	/*
	 * Default is to return data MSB2LSB and enable the
	 * use of memory-mapped files and strip chopping when
	 * a file is opened read-only.
	 */
	tif->tif_flags = FILLORDER_MSB2LSB;
	if (m == O_RDONLY )
            tif->tif_flags |= TIFF_MAPPED;

#ifdef STRIPCHOP_DEFAULT
	if (m == O_RDONLY || m == O_RDWR)
		tif->tif_flags |= STRIPCHOP_DEFAULT;
#endif

	{ union { int32 i; char c[4]; } u; u.i = 1; bigendian = u.c[0] == 0; }
	/*
	 * Process library-specific flags in the open mode string.
	 * The following flags may be used to control intrinsic library
	 * behaviour that may or may not be desirable (usually for
	 * compatibility with some application that claims to support
	 * TIFF but only supports some braindead idea of what the
	 * vendor thinks TIFF is):
	 *
	 * 'l'		use little-endian byte order for creating a file
	 * 'b'		use big-endian byte order for creating a file
	 * 'L'		read/write information using LSB2MSB bit order
	 * 'B'		read/write information using MSB2LSB bit order
	 * 'H'		read/write information using host bit order
	 * 'M'		enable use of memory-mapped files when supported
	 * 'm'		disable use of memory-mapped files
	 * 'C'		enable strip chopping support when reading
	 * 'c'		disable strip chopping support
	 *
	 * The use of the 'l' and 'b' flags is strongly discouraged.
	 * These flags are provided solely because numerous vendors,
	 * typically on the PC, do not correctly support TIFF; they
	 * only support the Intel little-endian byte order.  This
	 * support is not configured by default because it supports
	 * the violation of the TIFF spec that says that readers *MUST*
	 * support both byte orders.  It is strongly recommended that
	 * you not use this feature except to deal with busted apps
	 * that write invalid TIFF.  And even in those cases you should
	 * bang on the vendors to fix their software.
	 *
	 * The 'L', 'B', and 'H' flags are intended for applications
	 * that can optimize operations on data by using a particular
	 * bit order.  By default the library returns data in MSB2LSB
	 * bit order for compatibiltiy with older versions of this
	 * library.  Returning data in the bit order of the native cpu
	 * makes the most sense but also requires applications to check
	 * the value of the FillOrder tag; something they probabyl do
	 * not do right now.
	 *
	 * The 'M' and 'm' flags are provided because some virtual memory
	 * systems exhibit poor behaviour when large images are mapped.
	 * These options permit clients to control the use of memory-mapped
	 * files on a per-file basis.
	 *
	 * The 'C' and 'c' flags are provided because the library support
	 * for chopping up large strips into multiple smaller strips is not
	 * application-transparent and as such can cause problems.  The 'c'
	 * option permits applications that only want to look at the tags,
	 * for example, to get the unadulterated TIFF tag information.
	 */
	for (cp = mode; *cp; cp++)
		switch (*cp) {
		case 'b':
			if ((m&O_CREAT) && !bigendian)
				tif->tif_flags |= TIFF_SWAB;
			break;
		case 'l':
			if ((m&O_CREAT) && bigendian)
				tif->tif_flags |= TIFF_SWAB;
			break;
		case 'B':
			tif->tif_flags = (tif->tif_flags &~ TIFF_FILLORDER) |
			    FILLORDER_MSB2LSB;
			break;
		case 'L':
			tif->tif_flags = (tif->tif_flags &~ TIFF_FILLORDER) |
			    FILLORDER_LSB2MSB;
			break;
		case 'H':
			tif->tif_flags = (tif->tif_flags &~ TIFF_FILLORDER) |
			    HOST_FILLORDER;
			break;
		case 'M':
			if (m == O_RDONLY)
				tif->tif_flags |= TIFF_MAPPED;
			break;
		case 'm':
			if (m == O_RDONLY)
				tif->tif_flags &= ~TIFF_MAPPED;
			break;
		case 'C':
			if (m == O_RDONLY)
				tif->tif_flags |= TIFF_STRIPCHOP;
			break;
		case 'c':
			if (m == O_RDONLY)
				tif->tif_flags &= ~TIFF_STRIPCHOP;
			break;
		}
	/*
	 * Read in TIFF header.
	 */
	if (!ReadOK(tif, &tif->tif_header, sizeof (TIFFHeader))) {
		if (tif->tif_mode == O_RDONLY) {
			TIFFError(name, "Cannot read TIFF header");
			goto bad;
		}
		/*
		 * Setup header and write.
		 */
		tif->tif_header.tiff_magic = tif->tif_flags & TIFF_SWAB
		    ? (bigendian ? TIFF_LITTLEENDIAN : TIFF_BIGENDIAN)
		    : (bigendian ? TIFF_BIGENDIAN : TIFF_LITTLEENDIAN);
		tif->tif_header.tiff_version = TIFF_VERSION;
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&tif->tif_header.tiff_version);
		tif->tif_header.tiff_diroff = 0;	/* filled in later */

                /*
                 * This seek shouldn't be necessary, but I have had some
                 * crazy problems with a failed fseek() on Solaris leaving
                 * the current file pointer out of whack when an fwrite()
                 * is done. 
                 */
                TIFFSeekFile( tif, 0, SEEK_SET );

		if (!WriteOK(tif, &tif->tif_header, sizeof (TIFFHeader))) {
			TIFFError(name, "Error writing TIFF header");
			goto bad;
		}
		/*
		 * Setup the byte order handling.
		 */
		TIFFInitOrder(tif, tif->tif_header.tiff_magic, bigendian);
		/*
		 * Setup default directory.
		 */
		if (!TIFFDefaultDirectory(tif))
			goto bad;
		tif->tif_diroff = 0;
		return (tif);
	}
	/*
	 * Setup the byte order handling.
	 */
	if (tif->tif_header.tiff_magic != TIFF_BIGENDIAN &&
	    tif->tif_header.tiff_magic != TIFF_LITTLEENDIAN) {
		TIFFError(name,  "Not a TIFF file, bad magic number %d (0x%x)",
		    tif->tif_header.tiff_magic,
		    tif->tif_header.tiff_magic);
		goto bad;
	}
	TIFFInitOrder(tif, tif->tif_header.tiff_magic, bigendian);
	/*
	 * Swap header if required.
	 */
	if (tif->tif_flags & TIFF_SWAB) {
		TIFFSwabShort(&tif->tif_header.tiff_version);
		TIFFSwabLong(&tif->tif_header.tiff_diroff);
	}
	/*
	 * Now check version (if needed, it's been byte-swapped).
	 * Note that this isn't actually a version number, it's a
	 * magic number that doesn't change (stupid).
	 */
	if (tif->tif_header.tiff_version != TIFF_VERSION) {
		TIFFError(name,
		    "Not a TIFF file, bad version number %d (0x%x)",
		    tif->tif_header.tiff_version,
		    tif->tif_header.tiff_version); 
		goto bad;
	}
	tif->tif_flags |= TIFF_MYBUFFER;
	tif->tif_rawcp = tif->tif_rawdata = 0;
	tif->tif_rawdatasize = 0;
	/*
	 * Setup initial directory.
	 */
	switch (mode[0]) {
	case 'r':
		tif->tif_nextdiroff = tif->tif_header.tiff_diroff;
		/*
		 * Try to use a memory-mapped file if the client
		 * has not explicitly suppressed usage with the
		 * 'm' flag in the open mode (see above).
		 */
		if ((tif->tif_flags & TIFF_MAPPED) &&
	!TIFFMapFileContents(tif, (tdata_t*) &tif->tif_base, &tif->tif_size))
			tif->tif_flags &= ~TIFF_MAPPED;
		if (TIFFReadDirectory(tif)) {
                        if( m != O_RDONLY 
                          && tif->tif_dir.td_compression != COMPRESSION_NONE )
                        {
                            TIFFError( name, 
                                       "Can't open a compressed TIFF file"
                                       " with compression for update." );
                            goto bad;
                        }
			tif->tif_rawcc = -1;
			tif->tif_flags |= TIFF_BUFFERSETUP;
			return (tif);
		}
		break;
	case 'a':
		/*
		 * New directories are automatically append
		 * to the end of the directory chain when they
		 * are written out (see TIFFWriteDirectory).
		 */
		if (!TIFFDefaultDirectory(tif))
			goto bad;
		return (tif);
	}
bad:
	tif->tif_mode = O_RDONLY;	/* XXX avoid flush */
	TIFFClose(tif);
	return ((TIFF*)0);
bad2:
	(void) (*closeproc)(clientdata);
	return ((TIFF*)0);
}

/*
 * Query functions to access private data.
 */

/*
 * Return open file's name.
 */
const char *
TIFFFileName(TIFF* tif)
{
	return (tif->tif_name);
}

/*
 * Return open file's I/O descriptor.
 */
int
TIFFFileno(TIFF* tif)
{
	return (int)(tif->tif_fd);
}

/*
 * Return read/write mode.
 */
int
TIFFGetMode(TIFF* tif)
{
	return (tif->tif_mode);
}

/*
 * Return nonzero if file is organized in
 * tiles; zero if organized as strips.
 */
int
TIFFIsTiled(TIFF* tif)
{
	return (isTiled(tif));
}

/*
 * Return current row being read/written.
 */
uint32
TIFFCurrentRow(TIFF* tif)
{
	return (tif->tif_row);
}

/*
 * Return index of the current directory.
 */
tdir_t
TIFFCurrentDirectory(TIFF* tif)
{
	return (tif->tif_curdir);
}

/*
 * Return current strip.
 */
tstrip_t
TIFFCurrentStrip(TIFF* tif)
{
	return (tif->tif_curstrip);
}

/*
 * Return current tile.
 */
ttile_t
TIFFCurrentTile(TIFF* tif)
{
	return (tif->tif_curtile);
}

/*
 * Return nonzero if the file has byte-swapped data.
 */
int
TIFFIsByteSwapped(TIFF* tif)
{
	return ((tif->tif_flags & TIFF_SWAB) != 0);
}

/*
 * Return nonzero if the data is returned up-sampled.
 */
int
TIFFIsUpSampled(TIFF* tif)
{
	return (isUpSampled(tif));
}

/*
 * Return nonzero if the data is returned in MSB-to-LSB bit order.
 */
int
TIFFIsMSB2LSB(TIFF* tif)
{
	return (isFillOrder(tif, FILLORDER_MSB2LSB));
}
