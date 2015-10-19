/* $Header: /cvsroot/osrs/libtiff/libtiff/tif_predict.c,v 1.2 1999/11/27 21:43:28 warmerda Exp $ */

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
 *
 * Predictor Tag Support (used by multiple codecs).
 */
#include "tiffiop.h"
#include "tif_predict.h"

#include <assert.h>

#define	PredictorState(tif)	((TIFFPredictorState*) (tif)->tif_data)

static	void horAcc8(TIFF*, tidata_t, tsize_t);
static	void horAcc16(TIFF*, tidata_t, tsize_t);
static	void swabHorAcc16(TIFF*, tidata_t, tsize_t);
static	void horDiff8(TIFF*, tidata_t, tsize_t);
static	void horDiff16(TIFF*, tidata_t, tsize_t);
static	int PredictorDecodeRow(TIFF*, tidata_t, tsize_t, tsample_t);
static	int PredictorDecodeTile(TIFF*, tidata_t, tsize_t, tsample_t);
static	int PredictorEncodeRow(TIFF*, tidata_t, tsize_t, tsample_t);
static	int PredictorEncodeTile(TIFF*, tidata_t, tsize_t, tsample_t);

static int
PredictorSetup(TIFF* tif)
{
	TIFFPredictorState* sp = PredictorState(tif);
	TIFFDirectory* td = &tif->tif_dir;

	if (sp->predictor == 1)		/* no differencing */
		return (1);
	if (sp->predictor != 2) {
		TIFFError(tif->tif_name, "\"Predictor\" value %d not supported",
		    sp->predictor);
		return (0);
	}
	if (td->td_bitspersample != 8 && td->td_bitspersample != 16) {
		TIFFError(tif->tif_name,
    "Horizontal differencing \"Predictor\" not supported with %d-bit samples",
		    td->td_bitspersample);
		return (0);
	}
	sp->stride = (td->td_planarconfig == PLANARCONFIG_CONTIG ?
	    td->td_samplesperpixel : 1);
	/*
	 * Calculate the scanline/tile-width size in bytes.
	 */
	if (isTiled(tif))
		sp->rowsize = TIFFTileRowSize(tif);
	else
		sp->rowsize = TIFFScanlineSize(tif);
	return (1);
}

static int
PredictorSetupDecode(TIFF* tif)
{
	TIFFPredictorState* sp = PredictorState(tif);
	TIFFDirectory* td = &tif->tif_dir;

	if (!(*sp->setupdecode)(tif) || !PredictorSetup(tif))
		return (0);
	if (sp->predictor == 2) {
		switch (td->td_bitspersample) {
		case 8:  sp->pfunc = horAcc8; break;
		case 16: sp->pfunc = horAcc16; break;
		}
		/*
		 * Override default decoding method with
		 * one that does the predictor stuff.
		 */
		sp->coderow = tif->tif_decoderow;
		tif->tif_decoderow = PredictorDecodeRow;
		sp->codestrip = tif->tif_decodestrip;
		tif->tif_decodestrip = PredictorDecodeTile;
		sp->codetile = tif->tif_decodetile;
		tif->tif_decodetile = PredictorDecodeTile;
		/*
		 * If the data is horizontally differenced
		 * 16-bit data that requires byte-swapping,
		 * then it must be byte swapped before the
		 * accumulation step.  We do this with a
		 * special-purpose routine and override the
		 * normal post decoding logic that the library
		 * setup when the directory was read.
		 */
		if (tif->tif_flags&TIFF_SWAB) {
			if (sp->pfunc == horAcc16) {
				sp->pfunc = swabHorAcc16;
				tif->tif_postdecode = _TIFFNoPostDecode;
			} /* else handle 32-bit case... */
		}
	}
	return (1);
}

static int
PredictorSetupEncode(TIFF* tif)
{
	TIFFPredictorState* sp = PredictorState(tif);
	TIFFDirectory* td = &tif->tif_dir;

	if (!(*sp->setupencode)(tif) || !PredictorSetup(tif))
		return (0);
	if (sp->predictor == 2) {
		switch (td->td_bitspersample) {
		case 8:  sp->pfunc = horDiff8; break;
		case 16: sp->pfunc = horDiff16; break;
		}
		/*
		 * Override default encoding method with
		 * one that does the predictor stuff.
		 */
		sp->coderow = tif->tif_encoderow;
		tif->tif_encoderow = PredictorEncodeRow;
		sp->codestrip = tif->tif_encodestrip;
		tif->tif_encodestrip = PredictorEncodeTile;
		sp->codetile = tif->tif_encodetile;
		tif->tif_encodetile = PredictorEncodeTile;
	}
	return (1);
}

#define REPEAT4(n, op)		\
    switch (n) {		\
    default: { int i; for (i = n-4; i > 0; i--) { op; } } \
    case 4:  op;		\
    case 3:  op;		\
    case 2:  op;		\
    case 1:  op;		\
    case 0:  ;			\
    }

static void
horAcc8(TIFF* tif, tidata_t cp0, tsize_t cc)
{
	TIFFPredictorState* sp = PredictorState(tif);
	tsize_t stride = sp->stride;

	char* cp = (char*) cp0;
	if (cc > stride) {
		cc -= stride;
		/*
		 * Pipeline the most common cases.
		 */
		if (stride == 3)  {
			u_int cr = cp[0];
			u_int cg = cp[1];
			u_int cb = cp[2];
			do {
				cc -= 3, cp += 3;
				cp[0] = (cr += cp[0]);
				cp[1] = (cg += cp[1]);
				cp[2] = (cb += cp[2]);
			} while ((int32) cc > 0);
		} else if (stride == 4)  {
			u_int cr = cp[0];
			u_int cg = cp[1];
			u_int cb = cp[2];
			u_int ca = cp[3];
			do {
				cc -= 4, cp += 4;
				cp[0] = (cr += cp[0]);
				cp[1] = (cg += cp[1]);
				cp[2] = (cb += cp[2]);
				cp[3] = (ca += cp[3]);
			} while ((int32) cc > 0);
		} else  {
			do {
				REPEAT4(stride, cp[stride] += *cp; cp++)
				cc -= stride;
			} while ((int32) cc > 0);
		}
	}
}

static void
swabHorAcc16(TIFF* tif, tidata_t cp0, tsize_t cc)
{
	TIFFPredictorState* sp = PredictorState(tif);
	tsize_t stride = sp->stride;
	uint16* wp = (uint16*) cp0;
	tsize_t wc = cc / 2;

	if (wc > stride) {
		TIFFSwabArrayOfShort(wp, wc);
		wc -= stride;
		do {
			REPEAT4(stride, wp[stride] += wp[0]; wp++)
			wc -= stride;
		} while ((int32) wc > 0);
	}
}

static void
horAcc16(TIFF* tif, tidata_t cp0, tsize_t cc)
{
	tsize_t stride = PredictorState(tif)->stride;
	uint16* wp = (uint16*) cp0;
	tsize_t wc = cc / 2;

	if (wc > stride) {
		wc -= stride;
		do {
			REPEAT4(stride, wp[stride] += wp[0]; wp++)
			wc -= stride;
		} while ((int32) wc > 0);
	}
}

/*
 * Decode a scanline and apply the predictor routine.
 */
static int
PredictorDecodeRow(TIFF* tif, tidata_t op0, tsize_t occ0, tsample_t s)
{
	TIFFPredictorState *sp = PredictorState(tif);

	assert(sp != NULL);
	assert(sp->coderow != NULL);
	assert(sp->pfunc != NULL);
	if ((*sp->coderow)(tif, op0, occ0, s)) {
		(*sp->pfunc)(tif, op0, occ0);
		return (1);
	} else
		return (0);
}

/*
 * Decode a tile/strip and apply the predictor routine.
 * Note that horizontal differencing must be done on a
 * row-by-row basis.  The width of a "row" has already
 * been calculated at pre-decode time according to the
 * strip/tile dimensions.
 */
static int
PredictorDecodeTile(TIFF* tif, tidata_t op0, tsize_t occ0, tsample_t s)
{
	TIFFPredictorState *sp = PredictorState(tif);

	assert(sp != NULL);
	assert(sp->codetile != NULL);
	if ((*sp->codetile)(tif, op0, occ0, s)) {
		tsize_t rowsize = sp->rowsize;
		assert(rowsize > 0);
		assert(sp->pfunc != NULL);
		while ((long)occ0 > 0) {
			(*sp->pfunc)(tif, op0, (tsize_t) rowsize);
			occ0 -= rowsize;
			op0 += rowsize;
		}
		return (1);
	} else
		return (0);
}

static void
horDiff8(TIFF* tif, tidata_t cp0, tsize_t cc)
{
	TIFFPredictorState* sp = PredictorState(tif);
	tsize_t stride = sp->stride;
	char* cp = (char*) cp0;

	if (cc > stride) {
		cc -= stride;
		/*
		 * Pipeline the most common cases.
		 */
		if (stride == 3) {
			int r1, g1, b1;
			int r2 = cp[0];
			int g2 = cp[1];
			int b2 = cp[2];
			do {
				r1 = cp[3]; cp[3] = r1-r2; r2 = r1;
				g1 = cp[4]; cp[4] = g1-g2; g2 = g1;
				b1 = cp[5]; cp[5] = b1-b2; b2 = b1;
				cp += 3;
			} while ((int32)(cc -= 3) > 0);
		} else if (stride == 4) {
			int r1, g1, b1, a1;
			int r2 = cp[0];
			int g2 = cp[1];
			int b2 = cp[2];
			int a2 = cp[3];
			do {
				r1 = cp[4]; cp[4] = r1-r2; r2 = r1;
				g1 = cp[5]; cp[5] = g1-g2; g2 = g1;
				b1 = cp[6]; cp[6] = b1-b2; b2 = b1;
				a1 = cp[7]; cp[7] = a1-a2; a2 = a1;
				cp += 4;
			} while ((int32)(cc -= 4) > 0);
		} else {
			cp += cc - 1;
			do {
				REPEAT4(stride, cp[stride] -= cp[0]; cp--)
			} while ((int32)(cc -= stride) > 0);
		}
	}
}

static void
horDiff16(TIFF* tif, tidata_t cp0, tsize_t cc)
{
	TIFFPredictorState* sp = PredictorState(tif);
	tsize_t stride = sp->stride;
	int16 *wp = (int16*) cp0;
	tsize_t wc = cc/2;

	if (wc > stride) {
		wc -= stride;
		wp += wc - 1;
		do {
			REPEAT4(stride, wp[stride] -= wp[0]; wp--)
			wc -= stride;
		} while ((int32) wc > 0);
	}
}

static int
PredictorEncodeRow(TIFF* tif, tidata_t bp, tsize_t cc, tsample_t s)
{
	TIFFPredictorState *sp = PredictorState(tif);

	assert(sp != NULL);
	assert(sp->pfunc != NULL);
	assert(sp->coderow != NULL);
/* XXX horizontal differencing alters user's data XXX */
	(*sp->pfunc)(tif, bp, cc);
	return ((*sp->coderow)(tif, bp, cc, s));
}

static int
PredictorEncodeTile(TIFF* tif, tidata_t bp0, tsize_t cc0, tsample_t s)
{
	TIFFPredictorState *sp = PredictorState(tif);
	tsize_t cc = cc0, rowsize;
	u_char* bp = bp0;

	assert(sp != NULL);
	assert(sp->pfunc != NULL);
	assert(sp->codetile != NULL);
	rowsize = sp->rowsize;
	assert(rowsize > 0);
	while ((long)cc > 0) {
		(*sp->pfunc)(tif, bp, (tsize_t) rowsize);
		cc -= rowsize;
		bp += rowsize;
	}
	return ((*sp->codetile)(tif, bp0, cc0, s));
}

#define	FIELD_PREDICTOR	(FIELD_CODEC+0)		/* XXX */

static const TIFFFieldInfo predictFieldInfo[] = {
    { TIFFTAG_PREDICTOR,	 1, 1, TIFF_SHORT,	FIELD_PREDICTOR,
      FALSE,	FALSE,	"Predictor" },
};
#define	N(a)	(sizeof (a) / sizeof (a[0]))

static int
PredictorVSetField(TIFF* tif, ttag_t tag, va_list ap)
{
	TIFFPredictorState *sp = PredictorState(tif);

	switch (tag) {
	case TIFFTAG_PREDICTOR:
		sp->predictor = (uint16) va_arg(ap, int);
		TIFFSetFieldBit(tif, FIELD_PREDICTOR);
		break;
	default:
		return (*sp->vsetparent)(tif, tag, ap);
	}
	tif->tif_flags |= TIFF_DIRTYDIRECT;
	return (1);
}

static int
PredictorVGetField(TIFF* tif, ttag_t tag, va_list ap)
{
	TIFFPredictorState *sp = PredictorState(tif);

	switch (tag) {
	case TIFFTAG_PREDICTOR:
		*va_arg(ap, uint16*) = sp->predictor;
		break;
	default:
		return (*sp->vgetparent)(tif, tag, ap);
	}
	return (1);
}

static void
PredictorPrintDir(TIFF* tif, FILE* fd, long flags)
{
	TIFFPredictorState* sp = PredictorState(tif);

	(void) flags;
	if (TIFFFieldSet(tif,FIELD_PREDICTOR)) {
		fprintf(fd, "  Predictor: ");
		switch (sp->predictor) {
		case 1: fprintf(fd, "none "); break;
		case 2: fprintf(fd, "horizontal differencing "); break;
		}
		fprintf(fd, "%u (0x%x)\n", sp->predictor, sp->predictor);
	}
	if (sp->printdir)
		(*sp->printdir)(tif, fd, flags);
}

int
TIFFPredictorInit(TIFF* tif)
{
	TIFFPredictorState* sp = PredictorState(tif);

	/*
	 * Merge codec-specific tag information and
	 * override parent get/set field methods.
	 */
	_TIFFMergeFieldInfo(tif, predictFieldInfo, N(predictFieldInfo));
	sp->vgetparent = tif->tif_vgetfield;
	tif->tif_vgetfield = PredictorVGetField;/* hook for predictor tag */
	sp->vsetparent = tif->tif_vsetfield;
	tif->tif_vsetfield = PredictorVSetField;/* hook for predictor tag */
	sp->printdir = tif->tif_printdir;
	tif->tif_printdir = PredictorPrintDir;	/* hook for predictor tag */

	sp->setupdecode = tif->tif_setupdecode;
	tif->tif_setupdecode = PredictorSetupDecode;
	sp->setupencode = tif->tif_setupencode;
	tif->tif_setupencode = PredictorSetupEncode;

	sp->predictor = 1;			/* default value */
	sp->pfunc = NULL;			/* no predictor routine */
	return (1);
}
