/*  HTTP-RTMP Stream Gateway
 *  Copyright (C) 2009 Andrej Stepanchuk
 *  Copyright (C) 2009-2010 Howard Chu
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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <signal.h>
#include <getopt.h>

#include <assert.h>

#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"

#include "thread.h"

#define RD_SUCCESS		0
#define RD_FAILED		1
#define RD_INCOMPLETE		2

#define PACKET_SIZE 1024*1024

#ifdef WIN32
#define InitSockets()	{\
        WORD version;			\
        WSADATA wsaData;		\
					\
        version = MAKEWORD(1,1);	\
        WSAStartup(version, &wsaData);	}

#define	CleanupSockets()	WSACleanup()
#else
#define InitSockets()
#define	CleanupSockets()
#endif

enum
{
  STREAMING_ACCEPTING,
  STREAMING_IN_PROGRESS,
  STREAMING_STOPPING,
  STREAMING_STOPPED
};

typedef struct
{
  int socket;
  int state;

} STREAMING_SERVER;

STREAMING_SERVER *httpServer = 0;	// server structure pointer

STREAMING_SERVER *startStreaming(const char *address, int port);
void stopStreaming(STREAMING_SERVER * server);

typedef struct
{
  AVal hostname;
  int rtmpport;
  int protocol;
  int bLiveStream;		// is it a live stream? then we can't seek/resume

  long int timeout;		// timeout connection after 120 seconds
  uint32_t bufferTime;

  char *rtmpurl;
  AVal fullUrl;
  AVal playpath;
  AVal swfUrl;
  AVal tcUrl;
  AVal pageUrl;
  AVal app;
  AVal auth;
  AVal swfHash;
  AVal flashVer;
  AVal token;
  AVal subscribepath;
  AVal usherToken; //Justin.tv auth token
  AVal sockshost;
  AMFObject extras;
  int edepth;
  uint32_t swfSize;
  int swfAge;
  int swfVfy;

  uint32_t dStartOffset;
  uint32_t dStopOffset;

#ifdef CRYPTO
  unsigned char hash[RTMP_SWF_HASHLEN];
#endif
} RTMP_REQUEST;

#define STR2AVAL(av,str)	av.av_val = str; av.av_len = strlen(av.av_val)

int
parseAMF(AMFObject *obj, const char *arg, int *depth)
{
  AMFObjectProperty prop = {{0,0}};
  int i;
  char *p;

  if (arg[1] == ':')
    {
      p = (char *)arg+2;
      switch(arg[0])
        {
        case 'B':
          prop.p_type = AMF_BOOLEAN;
          prop.p_vu.p_number = atoi(p);
          break;
        case 'S':
          prop.p_type = AMF_STRING;
          STR2AVAL(prop.p_vu.p_aval,p);
          break;
        case 'N':
          prop.p_type = AMF_NUMBER;
          prop.p_vu.p_number = strtod(p, NULL);
          break;
        case 'Z':
          prop.p_type = AMF_NULL;
          break;
        case 'O':
          i = atoi(p);
          if (i)
            {
              prop.p_type = AMF_OBJECT;
            }
          else
            {
              (*depth)--;
              return 0;
            }
          break;
        default:
          return -1;
        }
    }
  else if (arg[2] == ':' && arg[0] == 'N')
    {
      p = strchr(arg+3, ':');
      if (!p || !*depth)
        return -1;
      prop.p_name.av_val = (char *)arg+3;
      prop.p_name.av_len = p - (arg+3);

      p++;
      switch(arg[1])
        {
        case 'B':
          prop.p_type = AMF_BOOLEAN;
          prop.p_vu.p_number = atoi(p);
          break;
        case 'S':
          prop.p_type = AMF_STRING;
          STR2AVAL(prop.p_vu.p_aval,p);
          break;
        case 'N':
          prop.p_type = AMF_NUMBER;
          prop.p_vu.p_number = strtod(p, NULL);
          break;
        case 'O':
          prop.p_type = AMF_OBJECT;
          break;
        default:
          return -1;
        }
    }
  else
    return -1;

  if (*depth)
    {
      AMFObject *o2;
      for (i=0; i<*depth; i++)
        {
          o2 = &obj->o_props[obj->o_num-1].p_vu.p_object;
          obj = o2;
        }
    }
  AMF_AddProp(obj, &prop);
  if (prop.p_type == AMF_OBJECT)
    (*depth)++;
  return 0;
}

/* this request is formed from the parameters and used to initialize a new request,
 * thus it is a default settings list. All settings can be overriden by specifying the
 * parameters in the GET request. */
RTMP_REQUEST defaultRTMPRequest;

int ParseOption(char opt, char *arg, RTMP_REQUEST * req);

#ifdef _DEBUG
uint32_t debugTS = 0;

int pnum = 0;

FILE *netstackdump = NULL;
FILE *netstackdump_read = NULL;
#endif

/* inplace http unescape. This is possible .. strlen(unescaped_string)  <= strlen(esacped_string) */
void
http_unescape(char *data)
{
  char hex[3];
  char *stp;
  int src_x = 0;
  int dst_x = 0;

  int length = (int) strlen(data);
  hex[2] = 0;

  while (src_x < length)
    {
      if (strncmp(data + src_x, "%", 1) == 0 && src_x + 2 < length)
	{
	  //
	  // Since we encountered a '%' we know this is an escaped character
	  //
	  hex[0] = data[src_x + 1];
	  hex[1] = data[src_x + 2];
	  data[dst_x] = (char) strtol(hex, &stp, 16);
	  dst_x += 1;
	  src_x += 3;
	}
      else if (src_x != dst_x)
	{
	  //
	  // This doesn't need to be unescaped. If we didn't unescape anything previously
	  // there is no need to copy the string either
	  //
	  data[dst_x] = data[src_x];
	  src_x += 1;
	  dst_x += 1;
	}
      else
	{
	  //
	  // This doesn't need to be unescaped, however we need to copy the string
	  //
	  src_x += 1;
	  dst_x += 1;
	}
    }
  data[dst_x] = '\0';
}

TFTYPE
controlServerThread(void *unused)
{
  char ich;
  while (1)
    {
      ich = getchar();
      switch (ich)
	{
	case 'q':
	  RTMP_LogPrintf("Exiting\n");
	  stopStreaming(httpServer);
	  exit(0);
	  break;
	default:
	  RTMP_LogPrintf("Unknown command \'%c\', ignoring\n", ich);
	}
    }
  TFRET();
}

/*
ssize_t readHTTPLine(int sockfd, char *buffer, size_t length)
{
	size_t i=0;

	while(i < length-1) {
		char c;
		int n = read(sockfd, &c, 1);

		if(n == 0)
			break;

		buffer[i] = c;
		i++;

		if(c == '\n')
			break;
	}
	buffer[i]='\0';
	i++;

	return i;
}

int isHTTPRequestEOF(char *line, size_t length)
{
	if(length < 2)
		return TRUE;

	if(line[0]=='\r' && line[1]=='\n')
		return TRUE;

	return FALSE;
}
*/

void processTCPrequest(STREAMING_SERVER * server,	// server socket and state (our listening socket)
		       int sockfd	// client connection socket
  )
{
  char buf[512] = { 0 };	// answer buffer
  char header[2048] = { 0 };	// request header
  char *filename = NULL;	// GET request: file name //512 not enuf
  char *buffer = NULL;		// stream buffer
  char *ptr = NULL;		// header pointer
  int len;

  size_t nRead = 0;

  char srvhead[] = "\r\nServer: HTTP-RTMP Stream Server " RTMPDUMP_VERSION "\r\n";

  char *status = "404 Not Found";

  server->state = STREAMING_IN_PROGRESS;

  RTMP rtmp = { 0 };
  uint32_t dSeek = 0;		// can be used to start from a later point in the stream

  // reset RTMP options to defaults specified upon invokation of streams
  RTMP_REQUEST req;
  memcpy(&req, &defaultRTMPRequest, sizeof(RTMP_REQUEST));

  // timeout for http requests
  fd_set fds;
  struct timeval tv;

  memset(&tv, 0, sizeof(struct timeval));
  tv.tv_sec = 5;

  // go through request lines
  //do {
  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);

  if (select(sockfd + 1, &fds, NULL, NULL, &tv) <= 0)
    {
      RTMP_Log(RTMP_LOGERROR, "Request timeout/select failed, ignoring request");
      goto quit;
    }
  else
    {
      nRead = recv(sockfd, header, 2047, 0);
      header[2047] = '\0';

      RTMP_Log(RTMP_LOGDEBUG, "%s: header: %s", __FUNCTION__, header);

      if (strstr(header, "Range: bytes=") != 0)
	{
	  // TODO check range starts from 0 and asking till the end.
	  RTMP_LogPrintf("%s, Range request not supported\n", __FUNCTION__);
	  len = sprintf(buf, "HTTP/1.0 416 Requested Range Not Satisfiable%s\r\n",
		  srvhead);
	  send(sockfd, buf, len, 0);
	  goto quit;
	}

      if (strncmp(header, "GET", 3) == 0 && nRead > 4)
	{
	  filename = header + 4;

	  // filter " HTTP/..." from end of request
	  char *p = filename;
	  while (*p != '\0')
	    {
	      if (*p == ' ')
		{
		  *p = '\0';
		  break;
		}
	      p++;
	    }
	}
    }
  //} while(!isHTTPRequestEOF(header, nRead));

  // if we got a filename from the GET method
  if (filename != NULL)
    {
      RTMP_Log(RTMP_LOGDEBUG, "%s: Request header: %s", __FUNCTION__, filename);
      if (filename[0] == '/')
	{			// if its not empty, is it /?
	  ptr = filename + 1;

	  // parse parameters
	  if (*ptr == '?')
	    {
	      ptr++;
	      int len = strlen(ptr);

	      while (len >= 2)
		{
		  char ich = *ptr;
		  ptr++;
		  if (*ptr != '=')
		    goto filenotfound;	// long parameters not (yet) supported

		  ptr++;
		  len -= 2;

		  // get position of the next '&'
		  char *temp;

		  unsigned int nArgLen = len;
		  if ((temp = strstr(ptr, "&")) != 0)
		    {
		      nArgLen = temp - ptr;
		    }

		  char *arg = (char *) malloc((nArgLen + 1) * sizeof(char));
		  memcpy(arg, ptr, nArgLen * sizeof(char));
		  arg[nArgLen] = '\0';

		  //RTMP_Log(RTMP_LOGDEBUG, "%s: unescaping parameter: %s", __FUNCTION__, arg);
		  http_unescape(arg);

		  RTMP_Log(RTMP_LOGDEBUG, "%s: parameter: %c, arg: %s", __FUNCTION__,
		      ich, arg);

		  ptr += nArgLen + 1;
		  len -= nArgLen + 1;

		  if (!ParseOption(ich, arg, &req))
		    {
		      status = "400 unknown option";
		      goto filenotfound;
		    }
		}
	    }
	}
      else
	{
	  goto filenotfound;
	}
    }
  else
    {
      RTMP_LogPrintf("%s: No request header received/unsupported method\n",
		__FUNCTION__);
    }

  // do necessary checks right here to make sure the combined request of default values and GET parameters is correct
  if (!req.hostname.av_len && !req.fullUrl.av_len)
    {
      RTMP_Log(RTMP_LOGERROR,
	  "You must specify a hostname (--host) or url (-r \"rtmp://host[:port]/playpath\") containing a hostname");
      status = "400 Missing Hostname";
      goto filenotfound;
    }
  if (req.playpath.av_len == 0 && !req.fullUrl.av_len)
    {
      RTMP_Log(RTMP_LOGERROR,
	  "You must specify a playpath (--playpath) or url (-r \"rtmp://host[:port]/playpath\") containing a playpath");
      status = "400 Missing Playpath";
      goto filenotfound;;
    }

  if (req.protocol == RTMP_PROTOCOL_UNDEFINED && !req.fullUrl.av_len)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "You haven't specified a protocol (--protocol) or rtmp url (-r), using default protocol RTMP");
      req.protocol = RTMP_PROTOCOL_RTMP;
    }
  if (req.rtmpport == -1 && !req.fullUrl.av_len)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "You haven't specified a port (--port) or rtmp url (-r), using default port");
      req.rtmpport = 0;
    }
  if (req.rtmpport == 0 && !req.fullUrl.av_len)
    {
      if (req.protocol & RTMP_FEATURE_SSL)
	req.rtmpport = 443;
      else if (req.protocol & RTMP_FEATURE_HTTP)
	req.rtmpport = 80;
      else
	req.rtmpport = 1935;
    }

  if (req.tcUrl.av_len == 0)
    {
      char str[512] = { 0 };
      req.tcUrl.av_len = snprintf(str, 511, "%s://%.*s:%d/%.*s",
	RTMPProtocolStringsLower[req.protocol], req.hostname.av_len,
	req.hostname.av_val, req.rtmpport, req.app.av_len, req.app.av_val);
      req.tcUrl.av_val = (char *) malloc(req.tcUrl.av_len + 1);
      strcpy(req.tcUrl.av_val, str);
    }

  if (req.swfVfy)
    {
#ifdef CRYPTO
        if (RTMP_HashSWF(req.swfUrl.av_val, &req.swfSize, req.hash, req.swfAge) == 0)
          {
            req.swfHash.av_val = (char *)req.hash;
            req.swfHash.av_len = RTMP_SWF_HASHLEN;
          }
#endif
    }

  // after validation of the http request send response header
  len = sprintf(buf, "HTTP/1.0 200 OK%sContent-Type: video/flv\r\n\r\n", srvhead);
  send(sockfd, buf, len, 0);

  // send the packets
  buffer = (char *) calloc(PACKET_SIZE, 1);

  // User defined seek offset
  if (req.dStartOffset > 0)
    {
      if (req.bLiveStream)
	RTMP_Log(RTMP_LOGWARNING,
	    "Can't seek in a live stream, ignoring --seek option");
      else
	dSeek += req.dStartOffset;
    }

  if (dSeek != 0)
    {
      RTMP_LogPrintf("Starting at TS: %d ms\n", dSeek);
    }

  RTMP_Log(RTMP_LOGDEBUG, "Setting buffer time to: %dms", req.bufferTime);
  RTMP_Init(&rtmp);
  RTMP_SetBufferMS(&rtmp, req.bufferTime);
  if (!req.fullUrl.av_len)
    {
      RTMP_SetupStream(&rtmp, req.protocol, &req.hostname, req.rtmpport, &req.sockshost,
		       &req.playpath, &req.tcUrl, &req.swfUrl, &req.pageUrl, &req.app, &req.auth, &req.swfHash, req.swfSize, &req.flashVer, &req.subscribepath, &req.usherToken, dSeek, req.dStopOffset,
		       req.bLiveStream, req.timeout);
    }
  else
    {
      if (RTMP_SetupURL(&rtmp, req.fullUrl.av_val) == FALSE)
        {
          RTMP_Log(RTMP_LOGERROR, "Couldn't parse URL: %s", req.fullUrl.av_val);
          return;
        }
    }
  /* backward compatibility, we always sent this as true before */
  if (req.auth.av_len)
    rtmp.Link.lFlags |= RTMP_LF_AUTH;

  rtmp.Link.extras = req.extras;
  rtmp.Link.token = req.token;
  rtmp.m_read.timestamp = dSeek;

  RTMP_LogPrintf("Connecting ... port: %d, app: %s\n", req.rtmpport, req.app.av_val);
  if (!RTMP_Connect(&rtmp, NULL))
    {
      RTMP_LogPrintf("%s, failed to connect!\n", __FUNCTION__);
    }
  else
    {
      unsigned long size = 0;
      double percent = 0;
      double duration = 0.0;

      int nWritten = 0;
      int nRead = 0;

      do
	{
	  nRead = RTMP_Read(&rtmp, buffer, PACKET_SIZE);

	  if (nRead > 0)
	    {
	      if ((nWritten = send(sockfd, buffer, nRead, 0)) < 0)
		{
		  RTMP_Log(RTMP_LOGERROR, "%s, sending failed, error: %d", __FUNCTION__,
		      GetSockError());
		  goto cleanup;	// we are in STREAMING_IN_PROGRESS, so we'll go to STREAMING_ACCEPTING
		}

	      size += nRead;

	      //RTMP_LogPrintf("write %dbytes (%.1f KB)\n", nRead, nRead/1024.0);
	      if (duration <= 0)	// if duration unknown try to get it from the stream (onMetaData)
		duration = RTMP_GetDuration(&rtmp);

	      if (duration > 0)
		{
		  percent =
		    ((double) (dSeek + rtmp.m_read.timestamp)) / (duration *
							   1000.0) * 100.0;
		  percent = ((double) (int) (percent * 10.0)) / 10.0;
		  RTMP_LogStatus("\r%.3f KB / %.2f sec (%.1f%%)",
			    (double) size / 1024.0,
			    (double) (rtmp.m_read.timestamp) / 1000.0, percent);
		}
	      else
		{
		  RTMP_LogStatus("\r%.3f KB / %.2f sec", (double) size / 1024.0,
			    (double) (rtmp.m_read.timestamp) / 1000.0);
		}
	    }
#ifdef _DEBUG
	  else
	    {
	      RTMP_Log(RTMP_LOGDEBUG, "zero read!");
	    }
#endif
	}
      while (server->state == STREAMING_IN_PROGRESS && nRead > -1
	     && RTMP_IsConnected(&rtmp) && nWritten >= 0);
    }
cleanup:
  RTMP_LogPrintf("Closing connection... ");
  RTMP_Close(&rtmp);
  RTMP_LogPrintf("done!\n\n");

quit:
  if (buffer)
    {
      free(buffer);
      buffer = NULL;
    }

  if (sockfd)
    closesocket(sockfd);

  if (server->state == STREAMING_IN_PROGRESS)
    server->state = STREAMING_ACCEPTING;

  return;

filenotfound:
  RTMP_LogPrintf("%s, %s, %s\n", __FUNCTION__, status, filename);
  len = sprintf(buf, "HTTP/1.0 %s%s\r\n", status, srvhead);
  send(sockfd, buf, len, 0);
  goto quit;
}

TFTYPE
serverThread(void *arg)
{
  STREAMING_SERVER *server = arg;
  server->state = STREAMING_ACCEPTING;

  while (server->state == STREAMING_ACCEPTING)
    {
      struct sockaddr_in addr;
      socklen_t addrlen = sizeof(struct sockaddr_in);
      int sockfd =
	accept(server->socket, (struct sockaddr *) &addr, &addrlen);

      if (sockfd > 0)
	{
	  // Create a new process and transfer the control to that
	  RTMP_Log(RTMP_LOGDEBUG, "%s: accepted connection from %s\n", __FUNCTION__,
	      inet_ntoa(addr.sin_addr));
	  processTCPrequest(server, sockfd);
	  RTMP_Log(RTMP_LOGDEBUG, "%s: processed request\n", __FUNCTION__);
	}
      else
	{
	  RTMP_Log(RTMP_LOGERROR, "%s: accept failed", __FUNCTION__);
	}
    }
  server->state = STREAMING_STOPPED;
  TFRET();
}

STREAMING_SERVER *
startStreaming(const char *address, int port)
{
  struct sockaddr_in addr;
  int sockfd;
  STREAMING_SERVER *server;

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd == -1)
    {
      RTMP_Log(RTMP_LOGERROR, "%s, couldn't create socket", __FUNCTION__);
      return 0;
    }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(address);	//htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) ==
      -1)
    {
      RTMP_Log(RTMP_LOGERROR, "%s, TCP bind failed for port number: %d", __FUNCTION__,
	  port);
      return 0;
    }

  if (listen(sockfd, 10) == -1)
    {
      RTMP_Log(RTMP_LOGERROR, "%s, listen failed", __FUNCTION__);
      closesocket(sockfd);
      return 0;
    }

  server = (STREAMING_SERVER *) calloc(1, sizeof(STREAMING_SERVER));
  server->socket = sockfd;

  ThreadCreate(serverThread, server);

  return server;
}

void
stopStreaming(STREAMING_SERVER * server)
{
  assert(server);

  if (server->state != STREAMING_STOPPED)
    {
      if (server->state == STREAMING_IN_PROGRESS)
	{
	  server->state = STREAMING_STOPPING;

	  // wait for streaming threads to exit
	  while (server->state != STREAMING_STOPPED)
	    msleep(1);
	}

      if (closesocket(server->socket))
	RTMP_Log(RTMP_LOGERROR, "%s: Failed to close listening socket, error %d",
	    __FUNCTION__, GetSockError());

      server->state = STREAMING_STOPPED;
    }
}


void
sigIntHandler(int sig)
{
  RTMP_ctrlC = TRUE;
  RTMP_LogPrintf("Caught signal: %d, cleaning up, just a second...\n", sig);
  if (httpServer)
    stopStreaming(httpServer);
  signal(SIGINT, SIG_DFL);
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

// this will parse RTMP related options as needed
// excludes the following options: h, d, g

// Return values: true (option parsing ok)
//                false (option not parsed/invalid)
int
ParseOption(char opt, char *arg, RTMP_REQUEST * req)
{
  switch (opt)
    {
#ifdef CRYPTO
    case 'w':
      {
	int res = hex2bin(arg, &req->swfHash.av_val);
	if (!res || res != RTMP_SWF_HASHLEN)
	  {
	    req->swfHash.av_val = NULL;
	    RTMP_Log(RTMP_LOGWARNING,
		"Couldn't parse swf hash hex string, not hexstring or not %d bytes, ignoring!", RTMP_SWF_HASHLEN);
	  }
	req->swfHash.av_len = RTMP_SWF_HASHLEN;
	break;
      }
    case 'x':
      {
	int size = atoi(arg);
	if (size <= 0)
	  {
	    RTMP_Log(RTMP_LOGERROR, "SWF Size must be at least 1, ignoring\n");
	  }
	else
	  {
	    req->swfSize = size;
	  }
	break;
      }
    case 'W':
      {
        STR2AVAL(req->swfUrl, arg);
        req->swfVfy = 1;
      }
      break;
    case 'X':
      {
	int num = atoi(arg);
	if (num < 0)
	  {
	    RTMP_Log(RTMP_LOGERROR, "SWF Age must be non-negative, ignoring\n");
	  }
	else
	  {
	    req->swfAge = num;
	  }
	break;
      }
#endif
    case 'b':
      {
	int32_t bt = atol(arg);
	if (bt < 0)
	  {
	    RTMP_Log(RTMP_LOGERROR,
		"Buffer time must be greater than zero, ignoring the specified value %d!",
		bt);
	  }
	else
	  {
	    req->bufferTime = bt;
	  }
	break;
      }
    case 'v':
      req->bLiveStream = TRUE;	// no seeking or resuming possible!
      break;
    case 'd':
      STR2AVAL(req->subscribepath, arg);
      break;
    case 'n':
      STR2AVAL(req->hostname, arg);
      break;
    case 'c':
      req->rtmpport = atoi(arg);
      break;
    case 'l':
      {
	int protocol = atoi(arg);
	if (protocol < RTMP_PROTOCOL_RTMP || protocol > RTMP_PROTOCOL_RTMPTS)
	  {
	    RTMP_Log(RTMP_LOGERROR, "Unknown protocol specified: %d, using default",
		protocol);
	    return FALSE;
	  }
	else
	  {
	    req->protocol = protocol;
	  }
	break;
      }
    case 'y':
      STR2AVAL(req->playpath, arg);
      break;
    case 'r':
      {
	req->rtmpurl = arg;

	AVal parsedHost, parsedPlaypath, parsedApp;
	unsigned int parsedPort = 0;
	int parsedProtocol = RTMP_PROTOCOL_UNDEFINED;

	if (!RTMP_ParseURL
	    (req->rtmpurl, &parsedProtocol, &parsedHost, &parsedPort,
	     &parsedPlaypath, &parsedApp))
	  {
	    RTMP_Log(RTMP_LOGWARNING, "Couldn't parse the specified url (%s)!", arg);
	  }
	else
	  {
	    if (!req->hostname.av_len)
	      req->hostname = parsedHost;
	    if (req->rtmpport == -1)
	      req->rtmpport = parsedPort;
	    if (req->playpath.av_len == 0 && parsedPlaypath.av_len)
	      {
		    req->playpath = parsedPlaypath;
	      }
	    if (req->protocol == RTMP_PROTOCOL_UNDEFINED)
	      req->protocol = parsedProtocol;
	    if (req->app.av_len == 0 && parsedApp.av_len)
	      {
		    req->app = parsedApp;
	      }
	  }
	break;
      }
    case 'i':
      STR2AVAL(req->fullUrl, arg);
      break;
    case 's':
      STR2AVAL(req->swfUrl, arg);
      break;
    case 't':
      STR2AVAL(req->tcUrl, arg);
      break;
    case 'p':
      STR2AVAL(req->pageUrl, arg);
      break;
    case 'a':
      STR2AVAL(req->app, arg);
      break;
    case 'f':
      STR2AVAL(req->flashVer, arg);
      break;
    case 'u':
      STR2AVAL(req->auth, arg);
      break;
    case 'C':
      parseAMF(&req->extras, arg, &req->edepth);
      break;
    case 'm':
      req->timeout = atoi(arg);
      break;
    case 'A':
      req->dStartOffset = (int)(atof(arg) * 1000.0);
      //printf("dStartOffset = %d\n", dStartOffset);
      break;
    case 'B':
      req->dStopOffset = (int)(atof(arg) * 1000.0);
      //printf("dStartOffset = %d\n", dStartOffset);
      break;
    case 'T':
      STR2AVAL(req->token, arg);
      break;
    case 'S':
      STR2AVAL(req->sockshost, arg);
    case 'q':
      RTMP_debuglevel = RTMP_LOGCRIT;
      break;
    case 'V':
      RTMP_debuglevel = RTMP_LOGDEBUG;
      break;
    case 'z':
      RTMP_debuglevel = RTMP_LOGALL;
      break;
    case 'j':
      STR2AVAL(req->usherToken, arg);
      break;
    default:
      RTMP_LogPrintf("unknown option: %c, arg: %s\n", opt, arg);
      return FALSE;
    }
  return TRUE;
}

int
main(int argc, char **argv)
{
  int nStatus = RD_SUCCESS;

  // http streaming server
  char DEFAULT_HTTP_STREAMING_DEVICE[] = "0.0.0.0";	// 0.0.0.0 is any device

  char *httpStreamingDevice = DEFAULT_HTTP_STREAMING_DEVICE;	// streaming device, default 0.0.0.0
  int nHttpStreamingPort = 80;	// port

  RTMP_LogPrintf("HTTP-RTMP Stream Gateway %s\n", RTMPDUMP_VERSION);
  RTMP_LogPrintf("(c) 2010 Andrej Stepanchuk, Howard Chu; license: GPL\n\n");

  // init request
  memset(&defaultRTMPRequest, 0, sizeof(RTMP_REQUEST));

  defaultRTMPRequest.rtmpport = -1;
  defaultRTMPRequest.protocol = RTMP_PROTOCOL_UNDEFINED;
  defaultRTMPRequest.bLiveStream = FALSE;	// is it a live stream? then we can't seek/resume

  defaultRTMPRequest.timeout = 120;	// timeout connection after 120 seconds
  defaultRTMPRequest.bufferTime = 20 * 1000;

  defaultRTMPRequest.swfAge = 30;

  int opt;
  struct option longopts[] = {
    {"help", 0, NULL, 'h'},
    {"url", 1, NULL, 'i'},
    {"host", 1, NULL, 'n'},
    {"port", 1, NULL, 'c'},
    {"socks", 1, NULL, 'S'},
    {"protocol", 1, NULL, 'l'},
    {"playpath", 1, NULL, 'y'},
    {"rtmp", 1, NULL, 'r'},
    {"swfUrl", 1, NULL, 's'},
    {"tcUrl", 1, NULL, 't'},
    {"pageUrl", 1, NULL, 'p'},
    {"app", 1, NULL, 'a'},
#ifdef CRYPTO
    {"swfhash", 1, NULL, 'w'},
    {"swfsize", 1, NULL, 'x'},
    {"swfVfy", 1, NULL, 'W'},
    {"swfAge", 1, NULL, 'X'},
#endif
    {"auth", 1, NULL, 'u'},
    {"conn", 1, NULL, 'C'},
    {"flashVer", 1, NULL, 'f'},
    {"live", 0, NULL, 'v'},
    //{"flv",     1, NULL, 'o'},
    //{"resume",  0, NULL, 'e'},
    {"timeout", 1, NULL, 'm'},
    {"buffer", 1, NULL, 'b'},
    //{"skip",    1, NULL, 'k'},
    {"device", 1, NULL, 'D'},
    {"sport", 1, NULL, 'g'},
    {"subscribe", 1, NULL, 'd'},
    {"start", 1, NULL, 'A'},
    {"stop", 1, NULL, 'B'},
    {"token", 1, NULL, 'T'},
    {"debug", 0, NULL, 'z'},
    {"quiet", 0, NULL, 'q'},
    {"verbose", 0, NULL, 'V'},
    {"jtv", 1, NULL, 'j'},
    {0, 0, 0, 0}
  };

  signal(SIGINT, sigIntHandler);
#ifndef WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  InitSockets();

  while ((opt =
	  getopt_long(argc, argv,
		      "hvqVzr:s:t:i:p:a:f:u:n:c:l:y:m:d:D:A:B:T:g:w:x:W:X:S:j:", longopts,
		      NULL)) != -1)
    {
      switch (opt)
	{
	case 'h':
	  RTMP_LogPrintf
	    ("\nThis program serves media content streamed from RTMP onto HTTP.\n\n");
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
	    ("--protocol|-l           Overrides the protocol in the rtmp url (0 - RTMP, 2 - RTMPE)\n");
	  RTMP_LogPrintf
	    ("--playpath|-y           Overrides the playpath parsed from rtmp url\n");
	  RTMP_LogPrintf("--swfUrl|-s url         URL to player swf file\n");
	  RTMP_LogPrintf
	    ("--tcUrl|-t url          URL to played stream (default: \"rtmp://host[:port]/app\")\n");
	  RTMP_LogPrintf("--pageUrl|-p url        Web URL of played programme\n");
	  RTMP_LogPrintf("--app|-a app            Name of target app in server\n");
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
	    ("--live|-v               Get a live stream, no --resume (seeking) of live streams possible\n");
	  RTMP_LogPrintf
	    ("--subscribe|-d string   Stream name to subscribe to (otherwise defaults to playpath if live is specified)\n");
	  RTMP_LogPrintf
	    ("--timeout|-m num        Timeout connection num seconds (default: %lu)\n",
	     defaultRTMPRequest.timeout);
	  RTMP_LogPrintf
	    ("--start|-A num          Start at num seconds into stream (not valid when using --live)\n");
	  RTMP_LogPrintf
	    ("--stop|-B num           Stop at num seconds into stream\n");
	  RTMP_LogPrintf
	    ("--token|-T key          Key for SecureToken response\n");
	  RTMP_LogPrintf
	    ("--jtv|-j JSON           Authentication token for Justin.tv legacy servers\n");
	  RTMP_LogPrintf
	    ("--buffer|-b             Buffer time in milliseconds (default: %u)\n\n",
	     defaultRTMPRequest.bufferTime);

	  RTMP_LogPrintf
	    ("--device|-D             Streaming device ip address (default: %s)\n",
	     DEFAULT_HTTP_STREAMING_DEVICE);
	  RTMP_LogPrintf
	    ("--sport|-g              Streaming port (default: %d)\n\n",
	     nHttpStreamingPort);
	  RTMP_LogPrintf
	    ("--quiet|-q              Suppresses all command output.\n");
	  RTMP_LogPrintf("--verbose|-V            Verbose command output.\n");
	  RTMP_LogPrintf("--debug|-z              Debug level command output.\n");
	  RTMP_LogPrintf
	    ("If you don't pass parameters for swfUrl, pageUrl, or auth these properties will not be included in the connect ");
	  RTMP_LogPrintf("packet.\n\n");
	  return RD_SUCCESS;
	  break;
	  // streaming server specific options
	case 'D':
	  if (inet_addr(optarg) == INADDR_NONE)
	    {
	      RTMP_Log(RTMP_LOGERROR,
		  "Invalid binding address (requested address %s), ignoring",
		  optarg);
	    }
	  else
	    {
	      httpStreamingDevice = optarg;
	    }
	  break;
	case 'g':
	  {
	    int port = atoi(optarg);
	    if (port < 0 || port > 65535)
	      {
		RTMP_Log(RTMP_LOGERROR,
		    "Streaming port out of range (requested port %d), ignoring\n",
		    port);
	      }
	    else
	      {
		nHttpStreamingPort = port;
	      }
	    break;
	  }
	default:
	  //RTMP_LogPrintf("unknown option: %c\n", opt);
	  if (!ParseOption(opt, optarg, &defaultRTMPRequest))
	    return RD_FAILED;
	  break;
	}
    }

#ifdef _DEBUG
  netstackdump = fopen("netstackdump", "wb");
  netstackdump_read = fopen("netstackdump_read", "wb");
#endif

  // start text UI
  ThreadCreate(controlServerThread, 0);

  // start http streaming
  if ((httpServer =
       startStreaming(httpStreamingDevice, nHttpStreamingPort)) == 0)
    {
      RTMP_Log(RTMP_LOGERROR, "Failed to start HTTP server, exiting!");
      return RD_FAILED;
    }
  RTMP_LogPrintf("Streaming on http://%s:%d\n", httpStreamingDevice,
	    nHttpStreamingPort);

  while (httpServer->state != STREAMING_STOPPED)
    {
      sleep(1);
    }
  RTMP_Log(RTMP_LOGDEBUG, "Done, exiting...");

  CleanupSockets();

#ifdef _DEBUG
  if (netstackdump != 0)
    fclose(netstackdump);
  if (netstackdump_read != 0)
    fclose(netstackdump_read);
#endif
  return nStatus;
}
