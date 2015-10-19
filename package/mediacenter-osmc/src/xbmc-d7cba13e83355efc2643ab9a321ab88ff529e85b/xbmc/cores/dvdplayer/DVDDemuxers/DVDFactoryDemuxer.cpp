/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "DVDFactoryDemuxer.h"

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamHttp.h"
#include "DVDInputStreams/DVDInputStreamPVRManager.h"

#include "DVDDemuxFFmpeg.h"
#include "DVDDemuxShoutcast.h"
#include "DVDDemuxBXA.h"
#include "DVDDemuxCDDA.h"
#include "DVDDemuxPVRClient.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

using namespace PVR;

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream, bool fileinfo)
{
  if (!pInputStream)
    return NULL;

  // Try to open the AirTunes demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) && pInputStream->GetContent().compare("audio/x-xbmc-pcm") == 0 )
  {
    // audio/x-xbmc-pcm this is the used codec for AirTunes
    // (apples audio only streaming)
    std::unique_ptr<CDVDDemuxBXA> demuxer(new CDVDDemuxBXA());
    if(demuxer->Open(pInputStream))
      return demuxer.release();
    else
      return NULL;
  }
  
  // Try to open CDDA demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) && pInputStream->GetContent().compare("application/octet-stream") == 0)
  {
    std::string filename = pInputStream->GetFileName();
    if (filename.substr(0, 7) == "cdda://")
    {
      CLog::Log(LOGDEBUG, "DVDFactoryDemuxer: Stream is probably CD audio. Creating CDDA demuxer.");

      std::unique_ptr<CDVDDemuxCDDA> demuxer(new CDVDDemuxCDDA());
      if (demuxer->Open(pInputStream))
      {
        return demuxer.release();
      }
    }
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_HTTP))
  {
    CDVDInputStreamHttp* pHttpStream = (CDVDInputStreamHttp*)pInputStream;
    CHttpHeader *header = pHttpStream->GetHttpHeader();

    /* check so we got the meta information as requested in our http header */
    if( header->GetValue("icy-metaint").length() > 0 )
    {
      std::unique_ptr<CDVDDemuxShoutcast> demuxer(new CDVDDemuxShoutcast());
      if(demuxer->Open(pInputStream))
        return demuxer.release();
      else
        return NULL;
    }
  }

  bool streaminfo = true; /* Look for streams before playback */
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    CDVDInputStreamPVRManager* pInputStreamPVR = (CDVDInputStreamPVRManager*)pInputStream;
    CDVDInputStream* pOtherStream = pInputStreamPVR->GetOtherStream();

    /* Don't parse the streaminfo for some cases of streams to reduce the channel switch time */
    bool useFastswitch = URIUtils::IsUsingFastSwitch(pInputStream->GetFileName());
    streaminfo = !useFastswitch;

    if(pOtherStream)
    {
      /* Used for MediaPortal PVR addon (uses PVR otherstream for playback of rtsp streams) */
      if (pOtherStream->IsStreamType(DVDSTREAM_TYPE_FFMPEG))
      {
        std::unique_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
        if(demuxer->Open(pOtherStream, streaminfo))
          return demuxer.release();
        else
          return NULL;
      }
    }

    /* Use PVR demuxer only for live streams */
    if (URIUtils::IsPVRChannel(pInputStream->GetFileName()))
    {
      std::shared_ptr<CPVRClient> client;
      if (g_PVRClients->GetPlayingClient(client) &&
          client->HandlesDemuxing())
      {
        std::unique_ptr<CDVDDemuxPVRClient> demuxer(new CDVDDemuxPVRClient());
        if(demuxer->Open(pInputStream))
          return demuxer.release();
        else
          return NULL;
      }
    }
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FFMPEG))
  {
    bool useFastswitch = URIUtils::IsUsingFastSwitch(pInputStream->GetFileName());
    streaminfo = !useFastswitch;
  }

  std::unique_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
  if(demuxer->Open(pInputStream, streaminfo, fileinfo))
    return demuxer.release();
  else
    return NULL;
}

