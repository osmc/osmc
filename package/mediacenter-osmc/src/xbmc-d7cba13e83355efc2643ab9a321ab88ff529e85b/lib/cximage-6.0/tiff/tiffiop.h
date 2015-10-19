/* $Header: /cvsroot/osrs/libtiff/libtiff/tiffiop.h,v 1.3 2000/01/28 20:56:59 warmerda Exp $ */

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

#ifndef _TIFFIOP_
#define	_TIFFIOP_
/*
 * ``Library-private'' definitions.
 */
/*
 * UNIX systems should run the configure script to generate
 * a port.h file that reflects the system capabilities.
 * Doing this obviates all the dreck done in tiffcomp.h.
 */
#if defined(unix) || defined(__unix)
//#include "port.h"
#include "tiffconf.h"
#include "tiffcomp.h"
#else
#include "tiffconf.h"
#include "tiffcomp.h"
#endif
#include "tiffio.h"
#include "tif_dir.h"

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

/*
 * Typedefs for ``method pointers'' used internally.
 */
typedef	unsigned char tidataval_t;	/* internal image data value type */
typedef	tidataval_t* tidata_t;		/* reference to internal image data */

typedef	void (*TIFFVoidMethod)(TIFF*);
typedef	int (*TIFFBoolMethod)(TIFF*);
typedef	int (*TIFFPreMethod)(TIFF*, tsample_t);
typedef	int (*TIFFCodeMethod)(TIFF*, tidata_t, tsize_t, tsample_t);
typedef	int (*TIFFSeekMethod)(TIFF*, uint32);
typedef	void (*TIFFPostMethod)(TIFF*, tidata_t, tsize_t);
typedef	int (*TIFFVSetMethod)(TIFF*, ttag_t, va_list);
typedef	int (*TIFFVGetMethod)(TIFF*, ttag_t, va_list);
typedef	void (*TIFFPrintMethod)(TIFF*, FILE*, long);
typedef	uint32 (*TIFFStripMethod)(TIFF*, uint32);
typedef	void (*TIFFTileMethod)(TIFF*, uint32*, uint32*);

struct tiff {
	char*	tif_name;	/* name of open file */
	void*	tif_fd;		/* open file descriptor */
	int		tif_mode;	/* open mode (O_*) */
	uint32	tif_flags;
#define	TIFF_FILLORDER		0x0003	/* natural bit fill order for machine */
#define	TIFF_DIRTYHEADER	0x0004	/* header must be written on close */
#define	TIFF_DIRTYDIRECT	0x0008	/* current directory must be written */
#define	TIFF_BUFFERSETUP	0x0010	/* data buffers setup */
#define	TIFF_CODERSETUP		0x0020	/* encoder/decoder setup done */
#define	TIFF_BEENWRITING	0x0040	/* written 1+ scanlines to file */
#define	TIFF_SWAB		0x0080	/* byte swap file information */
#define	TIFF_NOBITREV		0x0100	/* inhibit bit reversal logic */
#define	TIFF_MYBUFFER		0x0200	/* my raw data buffer; free on close */
#define	TIFF_ISTILED		0x0400	/* file is tile, not strip- based */
#define	TIFF_MAPPED		0x0800	/* file is mapped into memory */
#define	TIFF_POSTENCODE		0x1000	/* need call to postencode routine */
#define	TIFF_INSUBIFD		0x2000	/* currently writing a subifd */
#define	TIFF_UPSAMPLED		0x4000	/* library is doing data up-sampling */ 
#define	TIFF_STRIPCHOP		0x8000	/* enable strip chopping support */
	toff_t		tif_diroff;	/* file offset of current directory */
	toff_t		tif_nextdiroff;	/* file offset of following directory */
	TIFFDirectory	tif_dir;	/* internal rep of current directory */
	TIFFHeader	tif_header;	/* file's header block */
        tidata_t        tif_clientdir;  /* client TIFF directory */
	const int*	tif_typeshift;	/* data type shift counts */
	const long*	tif_typemask;	/* data type masks */
	uint32		tif_row;	/* current scanline */
	tdir_t		tif_curdir;	/* current directory (index) */
	tstrip_t	tif_curstrip;	/* current strip for read/write */
	toff_t		tif_curoff;	/* current offset for read/write */
	toff_t		tif_dataoff;	/* current offset for writing dir */
#if SUBIFD_SUPPORT
	uint16		tif_nsubifd;	/* remaining subifds to write */
	toff_t		tif_subifdoff;	/* offset for patching SubIFD link */
#endif
/* tiling support */
	uint32 		tif_col;	/* current column (offset by row too) */
	ttile_t		tif_curtile;	/* current tile for read/write */
	tsize_t		tif_tilesize;	/* # of bytes in a tile */
/* compression scheme hooks */
	TIFFBoolMethod	tif_setupdecode;/* called once before predecode */
	TIFFPreMethod	tif_predecode;	/* pre- row/strip/tile decoding */
	TIFFBoolMethod	tif_setupencode;/* called once before preencode */
	TIFFPreMethod	tif_preencode;	/* pre- row/strip/tile encoding */
	TIFFBoolMethod	tif_postencode;	/* post- row/strip/tile encoding */
	TIFFCodeMethod	tif_decoderow;	/* scanline decoding routine */
	TIFFCodeMethod	tif_encoderow;	/* scanline encoding routine */
	TIFFCodeMethod	tif_decodestrip;/* strip decoding routine */
	TIFFCodeMethod	tif_encodestrip;/* strip encoding routine */
	TIFFCodeMethod	tif_decodetile;	/* tile decoding routine */
	TIFFCodeMethod	tif_encodetile;	/* tile encoding routine */
	TIFFVoidMethod	tif_close;	/* cleanup-on-close routine */
	TIFFSeekMethod	tif_seek;	/* position within a strip routine */
	TIFFVoidMethod	tif_cleanup;	/* cleanup state routine */
	TIFFStripMethod	tif_defstripsize;/* calculate/constrain strip size */
	TIFFTileMethod	tif_deftilesize;/* calculate/constrain tile size */
	tidata_t	tif_data;	/* compression scheme private data */
/* input/output buffering */
	tsize_t		tif_scanlinesize;/* # of bytes in a scanline */
	tsize_t		tif_scanlineskew;/* scanline skew for reading strips */
	tidata_t	tif_rawdata;	/* raw data buffer */
	tsize_t		tif_rawdatasize;/* # of bytes in raw data buffer */
	tidata_t	tif_rawcp;	/* current spot in raw buffer */
	tsize_t		tif_rawcc;	/* bytes unread from raw buffer */
/* memory-mapped file support */
	tidata_t	tif_base;	/* base of mapped file */
	toff_t		tif_size;	/* size of mapped file region (bytes) */
	TIFFMapFileProc	tif_mapproc;	/* map file method */
	TIFFUnmapFileProc tif_unmapproc;/* unmap file method */
/* input/output callback methods */
	thandle_t	tif_clientdata;	/* callback parameter */
	TIFFReadWriteProc tif_readproc;	/* read method */
	TIFFReadWriteProc tif_writeproc;/* write method */
	TIFFSeekProc	tif_seekproc;	/* lseek method */
	TIFFCloseProc	tif_closeproc;	/* close method */
	TIFFSizeProc	tif_sizeproc;	/* filesize method */
/* post-decoding support */
	TIFFPostMethod	tif_postdecode;	/* post decoding routine */
/* tag support */
	TIFFFieldInfo**	tif_fieldinfo;	/* sorted table of registered tags */
	int		tif_nfields;	/* # entries in registered tag table */
	TIFFVSetMethod	tif_vsetfield;	/* tag set routine */
	TIFFVGetMethod	tif_vgetfield;	/* tag get routine */
	TIFFPrintMethod	tif_printdir;	/* directory print routine */
};

#define	isPseudoTag(t)	(t > 0xffff)	/* is tag value normal or pseudo */

#define	isTiled(tif)	(((tif)->tif_flags & TIFF_ISTILED) != 0)
#define	isMapped(tif)	(((tif)->tif_flags & TIFF_MAPPED) != 0)
#define	isFillOrder(tif, o)	(((tif)->tif_flags & (o)) != 0)
#define	isUpSampled(tif)	(((tif)->tif_flags & TIFF_UPSAMPLED) != 0)
#define	TIFFReadFile(tif, buf, size) \
	((*(tif)->tif_readproc)((tif)->tif_clientdata,buf,size))
#define	TIFFWriteFile(tif, buf, size) \
	((*(tif)->tif_writeproc)((tif)->tif_clientdata,buf,size))
#define	TIFFSeekFile(tif, off, whence) \
	((*(tif)->tif_seekproc)((tif)->tif_clientdata,(toff_t)(off),whence))
#define	TIFFCloseFile(tif) \
	((*(tif)->tif_closeproc)((tif)->tif_clientdata))
#define	TIFFGetFileSize(tif) \
	((*(tif)->tif_sizeproc)((tif)->tif_clientdata))
#define	TIFFMapFileContents(tif, paddr, psize) \
	((*(tif)->tif_mapproc)((tif)->tif_clientdata,paddr,psize))
#define	TIFFUnmapFileContents(tif, addr, size) \
	((*(tif)->tif_unmapproc)((tif)->tif_clientdata,addr,size))

/*
 * Default Read/Seek/Write definitions.
 */
#ifndef ReadOK
#define	ReadOK(tif, buf, size) \
	(TIFFReadFile(tif, (tdata_t) buf, (tsize_t)(size)) == (tsize_t)(size))
#endif
#ifndef SeekOK
#define	SeekOK(tif, off) \
	(TIFFSeekFile(tif, (toff_t) off, SEEK_SET) == (toff_t) off)
#endif
#ifndef WriteOK
#define	WriteOK(tif, buf, size) \
	(TIFFWriteFile(tif, (tdata_t) buf, (tsize_t) size) == (tsize_t) size)
#endif

/* NB: the uint32 casts are to silence certain ANSI-C compilers */
#define	TIFFhowmany(x, y) ((((uint32)(x))+(((uint32)(y))-1))/((uint32)(y)))
#define	TIFFroundup(x, y) (TIFFhowmany(x,y)*((uint32)(y)))

#if defined(__cplusplus)
extern "C" {
#endif
extern	int _TIFFgetMode(const char*, const char*);
extern	int _TIFFNoRowEncode(TIFF*, tidata_t, tsize_t, tsample_t);
extern	int _TIFFNoStripEncode(TIFF*, tidata_t, tsize_t, tsample_t);
extern	int _TIFFNoTileEncode(TIFF*, tidata_t, tsize_t, tsample_t);
extern	int _TIFFNoRowDecode(TIFF*, tidata_t, tsize_t, tsample_t);
extern	int _TIFFNoStripDecode(TIFF*, tidata_t, tsize_t, tsample_t);
extern	int _TIFFNoTileDecode(TIFF*, tidata_t, tsize_t, tsample_t);
extern	void _TIFFNoPostDecode(TIFF*, tidata_t, tsize_t);
extern  int  _TIFFNoPreCode (TIFF*, tsample_t); 
extern	int _TIFFNoSeek(TIFF*, uint32);
extern	void _TIFFSwab16BitData(TIFF*, tidata_t, tsize_t);
extern	void _TIFFSwab32BitData(TIFF*, tidata_t, tsize_t);
extern	void _TIFFSwab64BitData(TIFF*, tidata_t, tsize_t);
extern	int TIFFFlushData1(TIFF*);
extern	void TIFFFreeDirectory(TIFF*);
extern	int TIFFDefaultDirectory(TIFF*);
extern	int TIFFSetCompressionScheme(TIFF*, int);
extern	int TIFFSetDefaultCompressionState(TIFF*);
extern	uint32 _TIFFDefaultStripSize(TIFF*, uint32);
extern	void _TIFFDefaultTileSize(TIFF*, uint32*, uint32*);

extern	void _TIFFsetByteArray(void**, void*, long);
extern	void _TIFFsetString(char**, char*);
extern	void _TIFFsetShortArray(uint16**, uint16*, long);
extern	void _TIFFsetLongArray(uint32**, uint32*, long);
extern	void _TIFFsetFloatArray(float**, float*, long);
extern	void _TIFFsetDoubleArray(double**, double*, long);

extern	void _TIFFprintAscii(FILE*, const char*);
extern	void _TIFFprintAsciiTag(FILE*, const char*, const char*);

GLOBALDATA(TIFFErrorHandler,_TIFFwarningHandler);
GLOBALDATA(TIFFErrorHandler,_TIFFerrorHandler);

extern	int TIFFInitDumpMode(TIFF*, int);
#ifdef PACKBITS_SUPPORT
extern	int TIFFInitPackBits(TIFF*, int);
#endif
#ifdef CCITT_SUPPORT
extern	int TIFFInitCCITTRLE(TIFF*, int), TIFFInitCCITTRLEW(TIFF*, int);
extern	int TIFFInitCCITTFax3(TIFF*, int), TIFFInitCCITTFax4(TIFF*, int);
#endif
#ifdef THUNDER_SUPPORT
extern	int TIFFInitThunderScan(TIFF*, int);
#endif
#ifdef NEXT_SUPPORT
extern	int TIFFInitNeXT(TIFF*, int);
#endif
#ifdef LZW_SUPPORT
extern	int TIFFInitLZW(TIFF*, int);
#endif
#ifdef OJPEG_SUPPORT
extern	int TIFFInitOJPEG(TIFF*, int);
#endif
#ifdef JPEG_SUPPORT
extern	int TIFFInitJPEG(TIFF*, int);
#endif
#ifdef JBIG_SUPPORT
extern	int TIFFInitJBIG(TIFF*, int);
#endif
#ifdef ZIP_SUPPORT
extern	int TIFFInitZIP(TIFF*, int);
#endif
#ifdef PIXARLOG_SUPPORT
extern	int TIFFInitPixarLog(TIFF*, int);
#endif
#ifdef LOGLUV_SUPPORT
extern	int TIFFInitSGILog(TIFF*, int);
#endif
#ifdef VMS
extern	const TIFFCodec _TIFFBuiltinCODECS[];
#else
extern	TIFFCodec _TIFFBuiltinCODECS[];
#endif

#if defined(__cplusplus)
}
#endif
#endif /* _TIFFIOP_ */
