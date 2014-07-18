/*  RTMPDump
 *  Copyright (C) 2009 Andrej Stepanchuk
 *  Copyright (C) 2009 Howard Chu
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTMPDump; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#define _FILE_OFFSET_BITS	64

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include <signal.h>		// to catch Ctrl-C
#include <getopt.h>

#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"

#ifdef WIN32
#define fseeko fseeko64
#define ftello ftello64
#include <io.h>
#include <fcntl.h>
#define	SET_BINMODE(f)	setmode(fileno(f), O_BINARY)
#else
#define	SET_BINMODE(f)
#endif

#define RD_SUCCESS		0
#define RD_FAILED		1
#define RD_INCOMPLETE		2
#define RD_NO_CONNECT		3

#define DEF_TIMEOUT	30	/* seconds */
#define DEF_BUFTIME	(10 * 60 * 60 * 1000)	/* 10 hours default */
#define DEF_SKIPFRM	0

// starts sockets
int
InitSockets()
{
#ifdef WIN32
  WORD version;
  WSADATA wsaData;

  version = MAKEWORD(1, 1);
  return (WSAStartup(version, &wsaData) == 0);
#else
  return TRUE;
#endif
}

inline void
CleanupSockets()
{
#ifdef WIN32
  WSACleanup();
#endif
}

#ifdef _DEBUG
uint32_t debugTS = 0;
int pnum = 0;

FILE *netstackdump = 0;
FILE *netstackdump_read = 0;
#endif

uint32_t nIgnoredFlvFrameCounter = 0;
uint32_t nIgnoredFrameCounter = 0;
#define MAX_IGNORED_FRAMES	50

FILE *file = 0;

void
sigIntHandler(int sig)
{
  RTMP_ctrlC = TRUE;
  RTMP_LogPrintf("Caught signal: %d, cleaning up, just a second...\n", sig);
  // ignore all these signals now and let the connection close
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
#ifndef WIN32
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
#endif
}

#define HEX2BIN(a)      (((a)&0x40)?((a)&0xf)+9:((a)&0xf))
int hex2bin(char *str, char **hex)
{
  char *ptr;
  int i, l = strlen(str);

  if (l & 1)
  	return 0;

  *hex = malloc(l/2);
  ptr = *hex;
  if (!ptr)
    return 0;

  for (i=0; i<l; i+=2)
    *ptr++ = (HEX2BIN(str[i]) << 4) | HEX2BIN(str[i+1]);
  return l/2;
}

static const AVal av_onMetaData = AVC("onMetaData");
static const AVal av_duration = AVC("duration");
static const AVal av_conn = AVC("conn");
static const AVal av_token = AVC("token");
static const AVal av_playlist = AVC("playlist");
static const AVal av_true = AVC("true");

int
OpenResumeFile(const char *flvFile,	// file name [in]
	       FILE ** file,	// opened file [out]
	       off_t * size,	// size of the file [out]
	       char **metaHeader,	// meta data read from the file [out]
	       uint32_t * nMetaHeaderSize,	// length of metaHeader [out]
	       double *duration)	// duration of the stream in ms [out]
{
  size_t bufferSize = 0;
  char hbuf[16], *buffer = NULL;

  *nMetaHeaderSize = 0;
  *size = 0;

  *file = fopen(flvFile, "r+b");
  if (!*file)
    return RD_SUCCESS;		// RD_SUCCESS, because we go to fresh file mode instead of quiting

  fseek(*file, 0, SEEK_END);
  *size = ftello(*file);
  fseek(*file, 0, SEEK_SET);

  if (*size > 0)
    {
      // verify FLV format and read header
      uint32_t prevTagSize = 0;

      // check we've got a valid FLV file to continue!
      if (fread(hbuf, 1, 13, *file) != 13)
	{
	  RTMP_Log(RTMP_LOGERROR, "Couldn't read FLV file header!");
	  return RD_FAILED;
	}
      if (hbuf[0] != 'F' || hbuf[1] != 'L' || hbuf[2] != 'V'
	  || hbuf[3] != 0x01)
	{
	  RTMP_Log(RTMP_LOGERROR, "Invalid FLV file!");
	  return RD_FAILED;
	}

      if ((hbuf[4] & 0x05) == 0)
	{
	  RTMP_Log(RTMP_LOGERROR,
	      "FLV file contains neither video nor audio, aborting!");
	  return RD_FAILED;
	}

      uint32_t dataOffset = AMF_DecodeInt32(hbuf + 5);
      fseek(*file, dataOffset, SEEK_SET);

      if (fread(hbuf, 1, 4, *file) != 4)
	{
	  RTMP_Log(RTMP_LOGERROR, "Invalid FLV file: missing first prevTagSize!");
	  return RD_FAILED;
	}
      prevTagSize = AMF_DecodeInt32(hbuf);
      if (prevTagSize != 0)
	{
	  RTMP_Log(RTMP_LOGWARNING,
	      "First prevTagSize is not zero: prevTagSize = 0x%08X",
	      prevTagSize);
	}

      // go through the file to find the meta data!
      off_t pos = dataOffset + 4;
      int bFoundMetaHeader = FALSE;

      while (pos < *size - 4 && !bFoundMetaHeader)
	{
	  fseeko(*file, pos, SEEK_SET);
	  if (fread(hbuf, 1, 4, *file) != 4)
	    break;

	  uint32_t dataSize = AMF_DecodeInt24(hbuf + 1);

	  if (hbuf[0] == 0x12)
	    {
	      if (dataSize > bufferSize)
		{
                  /* round up to next page boundary */
                  bufferSize = dataSize + 4095;
		  bufferSize ^= (bufferSize & 4095);
		  free(buffer);
                  buffer = malloc(bufferSize);
                  if (!buffer)
		    return RD_FAILED;
		}

	      fseeko(*file, pos + 11, SEEK_SET);
	      if (fread(buffer, 1, dataSize, *file) != dataSize)
		break;

	      AMFObject metaObj;
	      int nRes = AMF_Decode(&metaObj, buffer, dataSize, FALSE);
	      if (nRes < 0)
		{
		  RTMP_Log(RTMP_LOGERROR, "%s, error decoding meta data packet",
		      __FUNCTION__);
		  break;
		}

	      AVal metastring;
	      AMFProp_GetString(AMF_GetProp(&metaObj, NULL, 0), &metastring);

	      if (AVMATCH(&metastring, &av_onMetaData))
		{
		  AMF_Dump(&metaObj);

		  *nMetaHeaderSize = dataSize;
		  if (*metaHeader)
		    free(*metaHeader);
		  *metaHeader = (char *) malloc(*nMetaHeaderSize);
		  memcpy(*metaHeader, buffer, *nMetaHeaderSize);

		  // get duration
		  AMFObjectProperty prop;
		  if (RTMP_FindFirstMatchingProperty
		      (&metaObj, &av_duration, &prop))
		    {
		      *duration = AMFProp_GetNumber(&prop);
		      RTMP_Log(RTMP_LOGDEBUG, "File has duration: %f", *duration);
		    }

		  bFoundMetaHeader = TRUE;
		  break;
		}
	      //metaObj.Reset();
	      //delete obj;
	    }
	  pos += (dataSize + 11 + 4);
	}

      free(buffer);
      if (!bFoundMetaHeader)
	RTMP_Log(RTMP_LOGWARNING, "Couldn't locate meta data!");
    }

  return RD_SUCCESS;
}

int
GetLastKeyframe(FILE * file,	// output file [in]
		int nSkipKeyFrames,	// max number of frames to skip when searching for key frame [in]
		uint32_t * dSeek,	// offset of the last key frame [out]
		char **initialFrame,	// content of the last keyframe [out]
		int *initialFrameType,	// initial frame type (audio/video) [out]
		uint32_t * nInitialFrameSize)	// length of initialFrame [out]
{
  const size_t bufferSize = 16;
  char buffer[bufferSize];
  uint8_t dataType;
  int bAudioOnly;
  off_t size;

  fseek(file, 0, SEEK_END);
  size = ftello(file);

  fseek(file, 4, SEEK_SET);
  if (fread(&dataType, sizeof(uint8_t), 1, file) != 1)
    return RD_FAILED;

  bAudioOnly = (dataType & 0x4) && !(dataType & 0x1);

  RTMP_Log(RTMP_LOGDEBUG, "bAudioOnly: %d, size: %llu", bAudioOnly,
      (unsigned long long) size);

  // ok, we have to get the timestamp of the last keyframe (only keyframes are seekable) / last audio frame (audio only streams)

  //if(!bAudioOnly) // we have to handle video/video+audio different since we have non-seekable frames
  //{
  // find the last seekable frame
  off_t tsize = 0;
  uint32_t prevTagSize = 0;

  // go through the file and find the last video keyframe
  do
    {
      int xread;
    skipkeyframe:
      if (size - tsize < 13)
	{
	  RTMP_Log(RTMP_LOGERROR,
	      "Unexpected start of file, error in tag sizes, couldn't arrive at prevTagSize=0");
	  return RD_FAILED;
	}
      fseeko(file, size - tsize - 4, SEEK_SET);
      xread = fread(buffer, 1, 4, file);
      if (xread != 4)
	{
	  RTMP_Log(RTMP_LOGERROR, "Couldn't read prevTagSize from file!");
	  return RD_FAILED;
	}

      prevTagSize = AMF_DecodeInt32(buffer);
      //RTMP_Log(RTMP_LOGDEBUG, "Last packet: prevTagSize: %d", prevTagSize);

      if (prevTagSize == 0)
	{
	  RTMP_Log(RTMP_LOGERROR, "Couldn't find keyframe to resume from!");
	  return RD_FAILED;
	}

      if (prevTagSize < 0 || prevTagSize > size - 4 - 13)
	{
	  RTMP_Log(RTMP_LOGERROR,
	      "Last tag size must be greater/equal zero (prevTagSize=%d) and smaller then filesize, corrupt file!",
	      prevTagSize);
	  return RD_FAILED;
	}
      tsize += prevTagSize + 4;

      // read header
      fseeko(file, size - tsize, SEEK_SET);
      if (fread(buffer, 1, 12, file) != 12)
	{
	  RTMP_Log(RTMP_LOGERROR, "Couldn't read header!");
	  return RD_FAILED;
	}
      //*
#ifdef _DEBUG
      uint32_t ts = AMF_DecodeInt24(buffer + 4);
      ts |= (buffer[7] << 24);
      RTMP_Log(RTMP_LOGDEBUG, "%02X: TS: %d ms", buffer[0], ts);
#endif //*/

      // this just continues the loop whenever the number of skipped frames is > 0,
      // so we look for the next keyframe to continue with
      //
      // this helps if resuming from the last keyframe fails and one doesn't want to start
      // the download from the beginning
      //
      if (nSkipKeyFrames > 0
	  && !(!bAudioOnly
	       && (buffer[0] != 0x09 || (buffer[11] & 0xf0) != 0x10)))
	{
#ifdef _DEBUG
	  RTMP_Log(RTMP_LOGDEBUG,
	      "xxxxxxxxxxxxxxxxxxxxxxxx Well, lets go one more back!");
#endif
	  nSkipKeyFrames--;
	  goto skipkeyframe;
	}

    }
  while ((bAudioOnly && buffer[0] != 0x08) || (!bAudioOnly && (buffer[0] != 0x09 || (buffer[11] & 0xf0) != 0x10)));	// as long as we don't have a keyframe / last audio frame

  // save keyframe to compare/find position in stream
  *initialFrameType = buffer[0];
  *nInitialFrameSize = prevTagSize - 11;
  *initialFrame = (char *) malloc(*nInitialFrameSize);

  fseeko(file, size - tsize + 11, SEEK_SET);
  if (fread(*initialFrame, 1, *nInitialFrameSize, file) != *nInitialFrameSize)
    {
      RTMP_Log(RTMP_LOGERROR, "Couldn't read last keyframe, aborting!");
      return RD_FAILED;
    }

  *dSeek = AMF_DecodeInt24(buffer + 4);	// set seek position to keyframe tmestamp
  *dSeek |= (buffer[7] << 24);
  //}
  //else // handle audio only, we can seek anywhere we'd like
  //{
  //}

  if (*dSeek < 0)
    {
      RTMP_Log(RTMP_LOGERROR,
	  "Last keyframe timestamp is negative, aborting, your file is corrupt!");
      return RD_FAILED;
    }
  RTMP_Log(RTMP_LOGDEBUG, "Last keyframe found at: %d ms, size: %d, type: %02X", *dSeek,
      *nInitialFrameSize, *initialFrameType);

  /*
     // now read the timestamp of the frame before the seekable keyframe:
     fseeko(file, size-tsize-4, SEEK_SET);
     if(fread(buffer, 1, 4, file) != 4) {
     RTMP_Log(RTMP_LOGERROR, "Couldn't read prevTagSize from file!");
     goto start;
     }
     uint32_t prevTagSize = RTMP_LIB::AMF_DecodeInt32(buffer);
     fseeko(file, size-tsize-4-prevTagSize+4, SEEK_SET);
     if(fread(buffer, 1, 4, file) != 4) {
     RTMP_Log(RTMP_LOGERROR, "Couldn't read previous timestamp!");
     goto start;
     }
     uint32_t timestamp = RTMP_LIB::AMF_DecodeInt24(buffer);
     timestamp |= (buffer[3]<<24);

     RTMP_Log(RTMP_LOGDEBUG, "Previous timestamp: %d ms", timestamp);
   */

  if (*dSeek != 0)
    {
      // seek to position after keyframe in our file (we will ignore the keyframes resent by the server
      // since they are sent a couple of times and handling this would be a mess)
      fseeko(file, size - tsize + prevTagSize + 4, SEEK_SET);

      // make sure the WriteStream doesn't write headers and ignores all the 0ms TS packets
      // (including several meta data headers and the keyframe we seeked to)
      //bNoHeader = TRUE; if bResume==true this is true anyway
    }

  //}

  return RD_SUCCESS;
}

int
Download(RTMP * rtmp,		// connected RTMP object
	 FILE * file, uint32_t dSeek, uint32_t dStopOffset, double duration, int bResume, char *metaHeader, uint32_t nMetaHeaderSize, char *initialFrame, int initialFrameType, uint32_t nInitialFrameSize, int nSkipKeyFrames, int bStdoutMode, int bLiveStream, int bRealtimeStream, int bHashes, int bOverrideBufferTime, uint32_t bufferTime, double *percent)	// percentage downloaded [out]
{
  int32_t now, lastUpdate;
  int bufferSize = 64 * 1024;
  char *buffer;
  int nRead = 0;
  off_t size = ftello(file);
  unsigned long lastPercent = 0;

  rtmp->m_read.timestamp = dSeek;

  *percent = 0.0;

  if (rtmp->m_read.timestamp)
    {
      RTMP_Log(RTMP_LOGDEBUG, "Continuing at TS: %d ms\n", rtmp->m_read.timestamp);
    }

  if (bLiveStream)
    {
      RTMP_LogPrintf("Starting Live Stream\n");
    }
  else
    {
      // print initial status
      // Workaround to exit with 0 if the file is fully (> 99.9%) downloaded
      if (duration > 0)
	{
	  if ((double) rtmp->m_read.timestamp >= (double) duration * 999.0)
	    {
	      RTMP_LogPrintf("Already Completed at: %.3f sec Duration=%.3f sec\n",
			(double) rtmp->m_read.timestamp / 1000.0,
			(double) duration / 1000.0);
	      return RD_SUCCESS;
	    }
	  else
	    {
	      *percent = ((double) rtmp->m_read.timestamp) / (duration * 1000.0) * 100.0;
	      *percent = ((double) (int) (*percent * 10.0)) / 10.0;
	      RTMP_LogPrintf("%s download at: %.3f kB / %.3f sec (%.1f%%)\n",
			bResume ? "Resuming" : "Starting",
			(double) size / 1024.0, (double) rtmp->m_read.timestamp / 1000.0,
			*percent);
	    }
	}
      else
	{
	  RTMP_LogPrintf("%s download at: %.3f kB\n",
		    bResume ? "Resuming" : "Starting",
		    (double) size / 1024.0);
	}
      if (bRealtimeStream)
	RTMP_LogPrintf("  in approximately realtime (disabled BUFX speedup hack)\n");
    }

  if (dStopOffset > 0)
    RTMP_LogPrintf("For duration: %.3f sec\n", (double) (dStopOffset - dSeek) / 1000.0);

  if (bResume && nInitialFrameSize > 0)
    rtmp->m_read.flags |= RTMP_READ_RESUME;
  rtmp->m_read.initialFrameType = initialFrameType;
  rtmp->m_read.nResumeTS = dSeek;
  rtmp->m_read.metaHeader = metaHeader;
  rtmp->m_read.initialFrame = initialFrame;
  rtmp->m_read.nMetaHeaderSize = nMetaHeaderSize;
  rtmp->m_read.nInitialFrameSize = nInitialFrameSize;

  buffer = (char *) malloc(bufferSize);

  now = RTMP_GetTime();
  lastUpdate = now - 1000;
  do
    {
      nRead = RTMP_Read(rtmp, buffer, bufferSize);
      //RTMP_LogPrintf("nRead: %d\n", nRead);
      if (nRead > 0)
	{
	  if (fwrite(buffer, sizeof(unsigned char), nRead, file) !=
	      (size_t) nRead)
	    {
	      RTMP_Log(RTMP_LOGERROR, "%s: Failed writing, exiting!", __FUNCTION__);
	      free(buffer);
	      return RD_FAILED;
	    }
	  size += nRead;

	  //RTMP_LogPrintf("write %dbytes (%.1f kB)\n", nRead, nRead/1024.0);
	  if (duration <= 0)	// if duration unknown try to get it from the stream (onMetaData)
	    duration = RTMP_GetDuration(rtmp);

	  if (duration > 0)
	    {
	      // make sure we claim to have enough buffer time!
	      if (!bOverrideBufferTime && bufferTime < (duration * 1000.0))
		{
		  bufferTime = (uint32_t) (duration * 1000.0) + 5000;	// extra 5sec to make sure we've got enough

		  RTMP_Log(RTMP_LOGDEBUG,
		      "Detected that buffer time is less than duration, resetting to: %dms",
		      bufferTime);
		  RTMP_SetBufferMS(rtmp, bufferTime);
		  RTMP_UpdateBufferMS(rtmp);
		}
	      *percent = ((double) rtmp->m_read.timestamp) / (duration * 1000.0) * 100.0;
	      *percent = ((double) (int) (*percent * 10.0)) / 10.0;
	      if (bHashes)
		{
		  if (lastPercent + 1 <= *percent)
		    {
		      RTMP_LogStatus("#");
		      lastPercent = (unsigned long) *percent;
		    }
		}
	      else
		{
		  now = RTMP_GetTime();
		  if (abs(now - lastUpdate) > 200)
		    {
		      RTMP_LogStatus("\r%.3f kB / %.2f sec (%.1f%%)",
				(double) size / 1024.0,
				(double) (rtmp->m_read.timestamp) / 1000.0, *percent);
		      lastUpdate = now;
		    }
		}
	    }
	  else
	    {
	      now = RTMP_GetTime();
	      if (abs(now - lastUpdate) > 200)
		{
		  if (bHashes)
		    RTMP_LogStatus("#");
		  else
		    RTMP_LogStatus("\r%.3f kB / %.2f sec", (double) size / 1024.0,
			      (double) (rtmp->m_read.timestamp) / 1000.0);
		  lastUpdate = now;
		}
	    }
	}
      else
	{
#ifdef _DEBUG
	  RTMP_Log(RTMP_LOGDEBUG, "zero read!");
#endif
	  if (rtmp->m_read.status == RTMP_READ_EOF)
	    break;
	}

    }
  while (!RTMP_ctrlC && nRead > -1 && RTMP_IsConnected(rtmp) && !RTMP_IsTimedout(rtmp));
  free(buffer);
  if (nRead < 0)
    nRead = rtmp->m_read.status;

  /* Final status update */
  if (!bHashes)
    {
      if (duration > 0)
	{
	  *percent = ((double) rtmp->m_read.timestamp) / (duration * 1000.0) * 100.0;
	  *percent = ((double) (int) (*percent * 10.0)) / 10.0;
	  RTMP_LogStatus("\r%.3f kB / %.2f sec (%.1f%%)",
	    (double) size / 1024.0,
	    (double) (rtmp->m_read.timestamp) / 1000.0, *percent);
	}
      else
	{
	  RTMP_LogStatus("\r%.3f kB / %.2f sec", (double) size / 1024.0,
	    (double) (rtmp->m_read.timestamp) / 1000.0);
	}
    }

  RTMP_Log(RTMP_LOGDEBUG, "RTMP_Read returned: %d", nRead);

  if (bResume && nRead == -2)
    {
      RTMP_LogPrintf("Couldn't resume FLV file, try --skip %d\n\n",
		nSkipKeyFrames + 1);
      return RD_FAILED;
    }

  if (nRead == -3)
    return RD_SUCCESS;

  if ((duration > 0 && *percent < 99.9) || RTMP_ctrlC || nRead < 0
      || RTMP_IsTimedout(rtmp))
    {
      return RD_INCOMPLETE;
    }

  return RD_SUCCESS;
}

#define STR2AVAL(av,str)	av.av_val = str; av.av_len = strlen(av.av_val)

void usage(char *prog)
{
	  RTMP_LogPrintf
	    ("\n%s: This program dumps the media content streamed over RTMP.\n\n", prog);
	  RTMP_LogPrintf("--help|-h               Prints this help screen.\n");
	  RTMP_LogPrintf
	    ("--url|-i url            URL with options included (e.g. rtmp://host[:port]/path swfUrl=url tcUrl=url)\n");
	  RTMP_LogPrintf
	    ("--rtmp|-r url           URL (e.g. rtmp://host[:port]/path)\n");
	  RTMP_LogPrintf
	    ("--host|-n hostname      Overrides the hostname in the rtmp url\n");
	  RTMP_LogPrintf
	    ("--port|-c port          Overrides the port in the rtmp url\n");
	  RTMP_LogPrintf
	    ("--socks|-S host:port    Use the specified SOCKS proxy\n");
	  RTMP_LogPrintf
	    ("--protocol|-l num       Overrides the protocol in the rtmp url (0 - RTMP, 2 - RTMPE)\n");
	  RTMP_LogPrintf
	    ("--playpath|-y path      Overrides the playpath parsed from rtmp url\n");
	  RTMP_LogPrintf
	    ("--playlist|-Y           Set playlist before playing\n");
	  RTMP_LogPrintf("--swfUrl|-s url         URL to player swf file\n");
	  RTMP_LogPrintf
	    ("--tcUrl|-t url          URL to played stream (default: \"rtmp://host[:port]/app\")\n");
	  RTMP_LogPrintf("--pageUrl|-p url        Web URL of played programme\n");
	  RTMP_LogPrintf("--app|-a app            Name of target app on server\n");
#ifdef CRYPTO
	  RTMP_LogPrintf
	    ("--swfhash|-w hexstring  SHA256 hash of the decompressed SWF file (32 bytes)\n");
	  RTMP_LogPrintf
	    ("--swfsize|-x num        Size of the decompressed SWF file, required for SWFVerification\n");
	  RTMP_LogPrintf
	    ("--swfVfy|-W url         URL to player swf file, compute hash/size automatically\n");
	  RTMP_LogPrintf
	    ("--swfAge|-X days        Number of days to use cached SWF hash before refreshing\n");
#endif
	  RTMP_LogPrintf
	    ("--auth|-u string        Authentication string to be appended to the connect string\n");
	  RTMP_LogPrintf
	    ("--conn|-C type:data     Arbitrary AMF data to be appended to the connect string\n");
	  RTMP_LogPrintf
	    ("                        B:boolean(0|1), S:string, N:number, O:object-flag(0|1),\n");
	  RTMP_LogPrintf
	    ("                        Z:(null), NB:name:boolean, NS:name:string, NN:name:number\n");
	  RTMP_LogPrintf
	    ("--flashVer|-f string    Flash version string (default: \"%s\")\n",
	     RTMP_DefaultFlashVer.av_val);
	  RTMP_LogPrintf
	    ("--live|-v               Save a live stream, no --resume (seeking) of live streams possible\n");
	  RTMP_LogPrintf
	    ("--subscribe|-d string   Stream name to subscribe to (otherwise defaults to playpath if live is specifed)\n");
	  RTMP_LogPrintf
	    ("--realtime|-R           Don't attempt to speed up download via the Pause/Unpause BUFX hack\n");
	  RTMP_LogPrintf
	    ("--flv|-o string         FLV output file name, if the file name is - print stream to stdout\n");
	  RTMP_LogPrintf
	    ("--resume|-e             Resume a partial RTMP download\n");
	  RTMP_LogPrintf
	    ("--timeout|-m num        Timeout connection num seconds (default: %u)\n",
	     DEF_TIMEOUT);
	  RTMP_LogPrintf
	    ("--start|-A num          Start at num seconds into stream (not valid when using --live)\n");
	  RTMP_LogPrintf
	    ("--stop|-B num           Stop at num seconds into stream\n");
	  RTMP_LogPrintf
	    ("--token|-T key          Key for SecureToken response\n");
	  RTMP_LogPrintf
	    ("--jtv|-j JSON           Authentication token for Justin.tv legacy servers\n");
	  RTMP_LogPrintf
	    ("--hashes|-#             Display progress with hashes, not with the byte counter\n");
	  RTMP_LogPrintf
	    ("--buffer|-b             Buffer time in milliseconds (default: %u)\n",
	     DEF_BUFTIME);
	  RTMP_LogPrintf
	    ("--skip|-k num           Skip num keyframes when looking for last keyframe to resume from. Useful if resume fails (default: %d)\n\n",
	     DEF_SKIPFRM);
	  RTMP_LogPrintf
	    ("--quiet|-q              Suppresses all command output.\n");
	  RTMP_LogPrintf("--verbose|-V            Verbose command output.\n");
	  RTMP_LogPrintf("--debug|-z              Debug level command output.\n");
	  RTMP_LogPrintf
	    ("If you don't pass parameters for swfUrl, pageUrl, or auth these properties will not be included in the connect ");
	  RTMP_LogPrintf("packet.\n\n");
}

int
main(int argc, char **argv)
{
  extern char *optarg;

  int nStatus = RD_SUCCESS;
  double percent = 0;
  double duration = 0.0;

  int nSkipKeyFrames = DEF_SKIPFRM;	// skip this number of keyframes when resuming

  int bOverrideBufferTime = FALSE;	// if the user specifies a buffer time override this is true
  int bStdoutMode = TRUE;	// if true print the stream directly to stdout, messages go to stderr
  int bResume = FALSE;		// true in resume mode
  uint32_t dSeek = 0;		// seek position in resume mode, 0 otherwise
  uint32_t bufferTime = DEF_BUFTIME;

  // meta header and initial frame for the resume mode (they are read from the file and compared with
  // the stream we are trying to continue
  char *metaHeader = 0;
  uint32_t nMetaHeaderSize = 0;

  // video keyframe for matching
  char *initialFrame = 0;
  uint32_t nInitialFrameSize = 0;
  int initialFrameType = 0;	// tye: audio or video

  AVal hostname = { 0, 0 };
  AVal playpath = { 0, 0 };
  AVal subscribepath = { 0, 0 };
  AVal usherToken = { 0, 0 }; //Justin.tv auth token
  int port = -1;
  int protocol = RTMP_PROTOCOL_UNDEFINED;
  int retries = 0;
  int bLiveStream = FALSE;	// is it a live stream? then we can't seek/resume
  int bRealtimeStream = FALSE;  // If true, disable the BUFX hack (be patient)
  int bHashes = FALSE;		// display byte counters not hashes by default

  long int timeout = DEF_TIMEOUT;	// timeout connection after 120 seconds
  uint32_t dStartOffset = 0;	// seek position in non-live mode
  uint32_t dStopOffset = 0;
  RTMP rtmp = { 0 };

  AVal fullUrl = { 0, 0 };
  AVal swfUrl = { 0, 0 };
  AVal tcUrl = { 0, 0 };
  AVal pageUrl = { 0, 0 };
  AVal app = { 0, 0 };
  AVal auth = { 0, 0 };
  AVal swfHash = { 0, 0 };
  uint32_t swfSize = 0;
  AVal flashVer = { 0, 0 };
  AVal sockshost = { 0, 0 };

#ifdef CRYPTO
  int swfAge = 30;	/* 30 days for SWF cache by default */
  int swfVfy = 0;
  unsigned char hash[RTMP_SWF_HASHLEN];
#endif

  char *flvFile = 0;

  signal(SIGINT, sigIntHandler);
  signal(SIGTERM, sigIntHandler);
#ifndef WIN32
  signal(SIGHUP, sigIntHandler);
  signal(SIGPIPE, sigIntHandler);
  signal(SIGQUIT, sigIntHandler);
#endif

  RTMP_debuglevel = RTMP_LOGINFO;

  // Check for --quiet option before printing any output
  int index = 0;
  while (index < argc)
    {
      if (strcmp(argv[index], "--quiet") == 0
	  || strcmp(argv[index], "-q") == 0)
	RTMP_debuglevel = RTMP_LOGCRIT;
      index++;
    }

  RTMP_LogPrintf("RTMPDump %s\n", RTMPDUMP_VERSION);
  RTMP_LogPrintf
    ("(c) 2010 Andrej Stepanchuk, Howard Chu, The Flvstreamer Team; license: GPL\n");

  if (!InitSockets())
    {
      RTMP_Log(RTMP_LOGERROR,
	  "Couldn't load sockets support on your platform, exiting!");
      return RD_FAILED;
    }

  /* sleep(30); */

  RTMP_Init(&rtmp);

  int opt;
  struct option longopts[] = {
    {"help", 0, NULL, 'h'},
    {"host", 1, NULL, 'n'},
    {"port", 1, NULL, 'c'},
    {"socks", 1, NULL, 'S'},
    {"protocol", 1, NULL, 'l'},
    {"playpath", 1, NULL, 'y'},
    {"playlist", 0, NULL, 'Y'},
    {"url", 1, NULL, 'i'},
    {"rtmp", 1, NULL, 'r'},
    {"swfUrl", 1, NULL, 's'},
    {"tcUrl", 1, NULL, 't'},
    {"pageUrl", 1, NULL, 'p'},
    {"app", 1, NULL, 'a'},
    {"auth", 1, NULL, 'u'},
    {"conn", 1, NULL, 'C'},
#ifdef CRYPTO
    {"swfhash", 1, NULL, 'w'},
    {"swfsize", 1, NULL, 'x'},
    {"swfVfy", 1, NULL, 'W'},
    {"swfAge", 1, NULL, 'X'},
#endif
    {"flashVer", 1, NULL, 'f'},
    {"live", 0, NULL, 'v'},
    {"realtime", 0, NULL, 'R'},
    {"flv", 1, NULL, 'o'},
    {"resume", 0, NULL, 'e'},
    {"timeout", 1, NULL, 'm'},
    {"buffer", 1, NULL, 'b'},
    {"skip", 1, NULL, 'k'},
    {"subscribe", 1, NULL, 'd'},
    {"start", 1, NULL, 'A'},
    {"stop", 1, NULL, 'B'},
    {"token", 1, NULL, 'T'},
    {"hashes", 0, NULL, '#'},
    {"debug", 0, NULL, 'z'},
    {"quiet", 0, NULL, 'q'},
    {"verbose", 0, NULL, 'V'},
    {"jtv", 1, NULL, 'j'},
    {0, 0, 0, 0}
  };

  while ((opt =
	  getopt_long(argc, argv,
		      "hVveqzRr:s:t:i:p:a:b:f:o:u:C:n:c:l:y:Ym:k:d:A:B:T:w:x:W:X:S:#j:",
		      longopts, NULL)) != -1)
    {
      switch (opt)
	{
	case 'h':
	  usage(argv[0]);
	  return RD_SUCCESS;
#ifdef CRYPTO
	case 'w':
	  {
	    int res = hex2bin(optarg, &swfHash.av_val);
	    if (res != RTMP_SWF_HASHLEN)
	      {
		swfHash.av_val = NULL;
		RTMP_Log(RTMP_LOGWARNING,
		    "Couldn't parse swf hash hex string, not hexstring or not %d bytes, ignoring!", RTMP_SWF_HASHLEN);
	      }
	    swfHash.av_len = RTMP_SWF_HASHLEN;
	    break;
	  }
	case 'x':
	  {
	    int size = atoi(optarg);
	    if (size <= 0)
	      {
		RTMP_Log(RTMP_LOGERROR, "SWF Size must be at least 1, ignoring\n");
	      }
	    else
	      {
		swfSize = size;
	      }
	    break;
	  }
        case 'W':
	  STR2AVAL(swfUrl, optarg);
	  swfVfy = 1;
          break;
        case 'X':
	  {
	    int num = atoi(optarg);
	    if (num < 0)
	      {
		RTMP_Log(RTMP_LOGERROR, "SWF Age must be non-negative, ignoring\n");
	      }
	    else
	      {
		swfAge = num;
	      }
	  }
          break;
#endif
	case 'k':
	  nSkipKeyFrames = atoi(optarg);
	  if (nSkipKeyFrames < 0)
	    {
	      RTMP_Log(RTMP_LOGERROR,
		  "Number of keyframes skipped must be greater or equal zero, using zero!");
	      nSkipKeyFrames = 0;
	    }
	  else
	    {
	      RTMP_Log(RTMP_LOGDEBUG, "Number of skipped key frames for resume: %d",
		  nSkipKeyFrames);
	    }
	  break;
	case 'b':
	  {
	    int32_t bt = atol(optarg);
	    if (bt < 0)
	      {
		RTMP_Log(RTMP_LOGERROR,
		    "Buffer time must be greater than zero, ignoring the specified value %d!",
		    bt);
	      }
	    else
	      {
		bufferTime = bt;
		bOverrideBufferTime = TRUE;
	      }
	    break;
	  }
	case 'v':
	  bLiveStream = TRUE;	// no seeking or resuming possible!
	  break;
	case 'R':
	  bRealtimeStream = TRUE; // seeking and resuming is still possible
	  break;
	case 'd':
	  STR2AVAL(subscribepath, optarg);
	  break;
	case 'n':
	  STR2AVAL(hostname, optarg);
	  break;
	case 'c':
	  port = atoi(optarg);
	  break;
	case 'l':
	  protocol = atoi(optarg);
	  if (protocol < RTMP_PROTOCOL_RTMP || protocol > RTMP_PROTOCOL_RTMPTS)
	    {
	      RTMP_Log(RTMP_LOGERROR, "Unknown protocol specified: %d", protocol);
	      return RD_FAILED;
	    }
	  break;
	case 'y':
	  STR2AVAL(playpath, optarg);
	  break;
	case 'Y':
	  RTMP_SetOpt(&rtmp, &av_playlist, (AVal *)&av_true);
	  break;
	case 'r':
	  {
	    AVal parsedHost, parsedApp, parsedPlaypath;
	    unsigned int parsedPort = 0;
	    int parsedProtocol = RTMP_PROTOCOL_UNDEFINED;

	    if (!RTMP_ParseURL
		(optarg, &parsedProtocol, &parsedHost, &parsedPort,
		 &parsedPlaypath, &parsedApp))
	      {
		RTMP_Log(RTMP_LOGWARNING, "Couldn't parse the specified url (%s)!",
		    optarg);
	      }
	    else
	      {
		if (!hostname.av_len)
		  hostname = parsedHost;
		if (port == -1)
		  port = parsedPort;
		if (playpath.av_len == 0 && parsedPlaypath.av_len)
		  {
		    playpath = parsedPlaypath;
		  }
		if (protocol == RTMP_PROTOCOL_UNDEFINED)
		  protocol = parsedProtocol;
		if (app.av_len == 0 && parsedApp.av_len)
		  {
		    app = parsedApp;
		  }
	      }
	    break;
	  }
	case 'i':
	  STR2AVAL(fullUrl, optarg);
          break;
	case 's':
	  STR2AVAL(swfUrl, optarg);
	  break;
	case 't':
	  STR2AVAL(tcUrl, optarg);
	  break;
	case 'p':
	  STR2AVAL(pageUrl, optarg);
	  break;
	case 'a':
	  STR2AVAL(app, optarg);
	  break;
	case 'f':
	  STR2AVAL(flashVer, optarg);
	  break;
	case 'o':
	  flvFile = optarg;
	  if (strcmp(flvFile, "-"))
	    bStdoutMode = FALSE;

	  break;
	case 'e':
	  bResume = TRUE;
	  break;
	case 'u':
	  STR2AVAL(auth, optarg);
	  break;
	case 'C': {
	  AVal av;
	  STR2AVAL(av, optarg);
	  if (!RTMP_SetOpt(&rtmp, &av_conn, &av))
	    {
	      RTMP_Log(RTMP_LOGERROR, "Invalid AMF parameter: %s", optarg);
	      return RD_FAILED;
	    }
	  }
	  break;
	case 'm':
	  timeout = atoi(optarg);
	  break;
	case 'A':
	  dStartOffset = (int) (atof(optarg) * 1000.0);
	  break;
	case 'B':
	  dStopOffset = (int) (atof(optarg) * 1000.0);
	  break;
	case 'T': {
	  AVal token;
	  STR2AVAL(token, optarg);
	  RTMP_SetOpt(&rtmp, &av_token, &token);
	  }
	  break;
	case '#':
	  bHashes = TRUE;
	  break;
	case 'q':
	  RTMP_debuglevel = RTMP_LOGCRIT;
	  break;
	case 'V':
	  RTMP_debuglevel = RTMP_LOGDEBUG;
	  break;
	case 'z':
	  RTMP_debuglevel = RTMP_LOGALL;
	  break;
	case 'S':
	  STR2AVAL(sockshost, optarg);
	  break;
	case 'j':
	  STR2AVAL(usherToken, optarg);
	  break;
	default:
	  RTMP_LogPrintf("unknown option: %c\n", opt);
	  usage(argv[0]);
	  return RD_FAILED;
	  break;
	}
    }

  if (!hostname.av_len && !fullUrl.av_len)
    {
      RTMP_Log(RTMP_LOGERROR,
	  "You must specify a hostname (--host) or url (-r \"rtmp://host[:port]/playpath\") containing a hostname");
      return RD_FAILED;
    }
  if (playpath.av_len == 0 && !fullUrl.av_len)
    {
      RTMP_Log(RTMP_LOGERROR,
	  "You must specify a playpath (--playpath) or url (-r \"rtmp://host[:port]/playpath\") containing a playpath");
      return RD_FAILED;
    }

  if (protocol == RTMP_PROTOCOL_UNDEFINED && !fullUrl.av_len)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "You haven't specified a protocol (--protocol) or rtmp url (-r), using default protocol RTMP");
      protocol = RTMP_PROTOCOL_RTMP;
    }
  if (port == -1 && !fullUrl.av_len)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "You haven't specified a port (--port) or rtmp url (-r), using default port 1935");
      port = 0;
    }
  if (port == 0 && !fullUrl.av_len)
    {
      if (protocol & RTMP_FEATURE_SSL)
	port = 443;
      else if (protocol & RTMP_FEATURE_HTTP)
	port = 80;
      else
	port = 1935;
    }

  if (flvFile == 0)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "You haven't specified an output file (-o filename), using stdout");
      bStdoutMode = TRUE;
    }

  if (bStdoutMode && bResume)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "Can't resume in stdout mode, ignoring --resume option");
      bResume = FALSE;
    }

  if (bLiveStream && bResume)
    {
      RTMP_Log(RTMP_LOGWARNING, "Can't resume live stream, ignoring --resume option");
      bResume = FALSE;
    }

#ifdef CRYPTO
  if (swfVfy)
    {
      if (RTMP_HashSWF(swfUrl.av_val, &swfSize, hash, swfAge) == 0)
        {
          swfHash.av_val = (char *)hash;
          swfHash.av_len = RTMP_SWF_HASHLEN;
        }
    }

  if (swfHash.av_len == 0 && swfSize > 0)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "Ignoring SWF size, supply also the hash with --swfhash");
      swfSize = 0;
    }

  if (swfHash.av_len != 0 && swfSize == 0)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "Ignoring SWF hash, supply also the swf size  with --swfsize");
      swfHash.av_len = 0;
      swfHash.av_val = NULL;
    }
#endif

  if (tcUrl.av_len == 0)
    {
	  tcUrl.av_len = strlen(RTMPProtocolStringsLower[protocol]) +
	  	hostname.av_len + app.av_len + sizeof("://:65535/");
      tcUrl.av_val = (char *) malloc(tcUrl.av_len);
	  if (!tcUrl.av_val)
	    return RD_FAILED;
      tcUrl.av_len = snprintf(tcUrl.av_val, tcUrl.av_len, "%s://%.*s:%d/%.*s",
	  	   RTMPProtocolStringsLower[protocol], hostname.av_len,
		   hostname.av_val, port, app.av_len, app.av_val);
    }

  int first = 1;

  // User defined seek offset
  if (dStartOffset > 0)
    {
      // Live stream
      if (bLiveStream)
	{
	  RTMP_Log(RTMP_LOGWARNING,
	      "Can't seek in a live stream, ignoring --start option");
	  dStartOffset = 0;
	}
    }

  if (!fullUrl.av_len)
    {
      RTMP_SetupStream(&rtmp, protocol, &hostname, port, &sockshost, &playpath,
		       &tcUrl, &swfUrl, &pageUrl, &app, &auth, &swfHash, swfSize,
		       &flashVer, &subscribepath, &usherToken, dSeek, dStopOffset, bLiveStream, timeout);
    }
  else
    {
      if (RTMP_SetupURL(&rtmp, fullUrl.av_val) == FALSE)
        {
          RTMP_Log(RTMP_LOGERROR, "Couldn't parse URL: %s", fullUrl.av_val);
          return RD_FAILED;
	}
    }

  /* Try to keep the stream moving if it pauses on us */
  if (!bLiveStream && !bRealtimeStream && !(protocol & RTMP_FEATURE_HTTP))
    rtmp.Link.lFlags |= RTMP_LF_BUFX;

  off_t size = 0;

  // ok, we have to get the timestamp of the last keyframe (only keyframes are seekable) / last audio frame (audio only streams)
  if (bResume)
    {
      nStatus =
	OpenResumeFile(flvFile, &file, &size, &metaHeader, &nMetaHeaderSize,
		       &duration);
      if (nStatus == RD_FAILED)
	goto clean;

      if (!file)
	{
	  // file does not exist, so go back into normal mode
	  bResume = FALSE;	// we are back in fresh file mode (otherwise finalizing file won't be done)
	}
      else
	{
	  nStatus = GetLastKeyframe(file, nSkipKeyFrames,
				    &dSeek, &initialFrame,
				    &initialFrameType, &nInitialFrameSize);
	  if (nStatus == RD_FAILED)
	    {
	      RTMP_Log(RTMP_LOGDEBUG, "Failed to get last keyframe.");
	      goto clean;
	    }

	  if (dSeek == 0)
	    {
	      RTMP_Log(RTMP_LOGDEBUG,
		  "Last keyframe is first frame in stream, switching from resume to normal mode!");
	      bResume = FALSE;
	    }
	}
    }

  if (!file)
    {
      if (bStdoutMode)
	{
	  file = stdout;
	  SET_BINMODE(file);
	}
      else
	{
	  file = fopen(flvFile, "w+b");
	  if (file == 0)
	    {
	      RTMP_LogPrintf("Failed to open file! %s\n", flvFile);
	      return RD_FAILED;
	    }
	}
    }

#ifdef _DEBUG
  netstackdump = fopen("netstackdump", "wb");
  netstackdump_read = fopen("netstackdump_read", "wb");
#endif

  while (!RTMP_ctrlC)
    {
      RTMP_Log(RTMP_LOGDEBUG, "Setting buffer time to: %dms", bufferTime);
      RTMP_SetBufferMS(&rtmp, bufferTime);

      if (first)
	{
	  first = 0;
	  RTMP_LogPrintf("Connecting ...\n");

	  if (!RTMP_Connect(&rtmp, NULL))
	    {
	      nStatus = RD_NO_CONNECT;
	      break;
	    }

	  RTMP_Log(RTMP_LOGINFO, "Connected...");

	  // User defined seek offset
	  if (dStartOffset > 0)
	    {
	      // Don't need the start offset if resuming an existing file
	      if (bResume)
		{
		  RTMP_Log(RTMP_LOGWARNING,
		      "Can't seek a resumed stream, ignoring --start option");
		  dStartOffset = 0;
		}
	      else
		{
		  dSeek = dStartOffset;
		}
	    }

	  // Calculate the length of the stream to still play
	  if (dStopOffset > 0)
	    {
	      // Quit if start seek is past required stop offset
	      if (dStopOffset <= dSeek)
		{
		  RTMP_LogPrintf("Already Completed\n");
		  nStatus = RD_SUCCESS;
		  break;
		}
	    }

	  if (!RTMP_ConnectStream(&rtmp, dSeek))
	    {
	      nStatus = RD_FAILED;
	      break;
	    }
	}
      else
	{
	  nInitialFrameSize = 0;

          if (retries)
            {
	      RTMP_Log(RTMP_LOGERROR, "Failed to resume the stream\n\n");
	      if (!RTMP_IsTimedout(&rtmp))
	        nStatus = RD_FAILED;
	      else
	        nStatus = RD_INCOMPLETE;
	      break;
            }
	  RTMP_Log(RTMP_LOGINFO, "Connection timed out, trying to resume.\n\n");
          /* Did we already try pausing, and it still didn't work? */
          if (rtmp.m_pausing == 3)
            {
              /* Only one try at reconnecting... */
              retries = 1;
              dSeek = rtmp.m_pauseStamp;
              if (dStopOffset > 0)
                {
                  if (dStopOffset <= dSeek)
                    {
                      RTMP_LogPrintf("Already Completed\n");
		      nStatus = RD_SUCCESS;
		      break;
                    }
                }
              if (!RTMP_ReconnectStream(&rtmp, dSeek))
                {
	          RTMP_Log(RTMP_LOGERROR, "Failed to resume the stream\n\n");
	          if (!RTMP_IsTimedout(&rtmp))
		    nStatus = RD_FAILED;
	          else
		    nStatus = RD_INCOMPLETE;
	          break;
                }
            }
	  else if (!RTMP_ToggleStream(&rtmp))
	    {
	      RTMP_Log(RTMP_LOGERROR, "Failed to resume the stream\n\n");
	      if (!RTMP_IsTimedout(&rtmp))
		nStatus = RD_FAILED;
	      else
		nStatus = RD_INCOMPLETE;
	      break;
	    }
	  bResume = TRUE;
	}

      nStatus = Download(&rtmp, file, dSeek, dStopOffset, duration, bResume,
			 metaHeader, nMetaHeaderSize, initialFrame,
			 initialFrameType, nInitialFrameSize, nSkipKeyFrames,
			 bStdoutMode, bLiveStream, bRealtimeStream, bHashes,
			 bOverrideBufferTime, bufferTime, &percent);
      free(initialFrame);
      initialFrame = NULL;

      /* If we succeeded, we're done.
       */
      if (nStatus != RD_INCOMPLETE || !RTMP_IsTimedout(&rtmp) || bLiveStream)
	break;
    }

  if (nStatus == RD_SUCCESS)
    {
      RTMP_LogPrintf("Download complete\n");
    }
  else if (nStatus == RD_INCOMPLETE)
    {
      RTMP_LogPrintf
	("Download may be incomplete (downloaded about %.2f%%), try resuming\n",
	 percent);
    }

clean:
  RTMP_Log(RTMP_LOGDEBUG, "Closing connection.\n");
  RTMP_Close(&rtmp);

  if (file != 0)
    fclose(file);

  CleanupSockets();

#ifdef _DEBUG
  if (netstackdump != 0)
    fclose(netstackdump);
  if (netstackdump_read != 0)
    fclose(netstackdump_read);
#endif
  return nStatus;
}
