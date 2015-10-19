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

#include <string>
#include <cstdlib>
#include "threads/SystemClock.h"
#include "DVDFileInfo.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "pictures/Picture.h"
#include "video/VideoInfoTag.h"
#include "filesystem/StackDirectory.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include "DVDStreamInfo.h"
#include "DVDInputStreams/DVDInputStream.h"
#ifdef HAVE_LIBBLURAY
#include "DVDInputStreams/DVDInputStreamBluray.h"
#endif
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDDemuxers/DVDDemuxFFmpeg.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "DVDDemuxers/DVDDemuxVobsub.h"

#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "filesystem/File.h"
#include "cores/FFmpeg.h"
#include "TextureCache.h"
#include "Util.h"
#include "utils/LangCodeExpander.h"


bool CDVDFileInfo::GetFileDuration(const std::string &path, int& duration)
{
  std::unique_ptr<CDVDInputStream> input;
  std::unique_ptr<CDVDDemux> demux;

  input.reset(CDVDFactoryInputStream::CreateInputStream(NULL, path, ""));
  if (!input.get())
    return false;

  if (!input->Open(path.c_str(), "", true))
    return false;

  demux.reset(CDVDFactoryDemuxer::CreateDemuxer(input.get(), true));
  if (!demux.get())
    return false;

  duration = demux->GetStreamLength();
  if (duration > 0)
    return true;
  else
    return false;
}

int DegreeToOrientation(int degrees)
{
  switch(degrees)
  {
    case 90:
      return 5;
    case 180:
      return 2;
    case 270:
      return 7;
    default:
      return 0;
  }
}

bool CDVDFileInfo::ExtractThumb(const std::string &strPath,
                                CTextureDetails &details,
                                CStreamDetails *pStreamDetails, int pos)
{
  std::string redactPath = CURL::GetRedacted(strPath);
  unsigned int nTime = XbmcThreads::SystemClockMillis();
  CDVDInputStream *pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, strPath, "");
  if (!pInputStream)
  {
    CLog::Log(LOGERROR, "InputStream: Error creating stream for %s", redactPath.c_str());
    return false;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)
   || pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    CLog::Log(LOGDEBUG, "%s: disc streams not supported for thumb extraction, file: %s", __FUNCTION__, redactPath.c_str());
    delete pInputStream;
    return false;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    delete pInputStream;
    return false;
  }

  if (!pInputStream->Open(strPath.c_str(), "", true))
  {
    CLog::Log(LOGERROR, "InputStream: Error opening, %s", redactPath.c_str());
    if (pInputStream)
      delete pInputStream;
    return false;
  }

  CDVDDemux *pDemuxer = NULL;

  try
  {
    pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(pInputStream, true);
    if(!pDemuxer)
    {
      delete pInputStream;
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return false;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opening demuxer", __FUNCTION__);
    if (pDemuxer)
      delete pDemuxer;
    delete pInputStream;
    return false;
  }

  if (pStreamDetails)
  {
    DemuxerToStreamDetails(pInputStream, pDemuxer, *pStreamDetails, strPath);

    //extern subtitles
    std::vector<std::string> filenames;
    std::string video_path;
    if (strPath.empty())
      video_path = pInputStream->GetFileName();
    else
      video_path = strPath;

    CUtil::ScanForExternalSubtitles(video_path, filenames);

    for(unsigned int i=0;i<filenames.size();i++)
    {
      // if vobsub subtitle:
      if (URIUtils::GetExtension(filenames[i]) == ".idx")
      {
        std::string strSubFile;
        if ( CUtil::FindVobSubPair(filenames, filenames[i], strSubFile) )
          AddExternalSubtitleToDetails(video_path, *pStreamDetails, filenames[i], strSubFile);
      }
      else
      {
        if ( !CUtil::IsVobSub(filenames, filenames[i]) )
        {
          AddExternalSubtitleToDetails(video_path, *pStreamDetails, filenames[i]);
        }
      }
    }
  }

  int nVideoStream = -1;
  for (int i = 0; i < pDemuxer->GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = pDemuxer->GetStream(i);
    if (pStream)
    {
      // ignore if it's a picture attachment (e.g. jpeg artwork)
      if(pStream->type == STREAM_VIDEO && !(pStream->flags & AV_DISPOSITION_ATTACHED_PIC))
        nVideoStream = i;
      else
        pStream->SetDiscard(AVDISCARD_ALL);
    }
  }

  bool bOk = false;
  int packetsTried = 0;

  if (nVideoStream != -1)
  {
    CDVDVideoCodec *pVideoCodec;

    CDVDStreamInfo hint(*pDemuxer->GetStream(nVideoStream), true);
    hint.software = true;

    if (hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_MPEG1VIDEO)
    {
      // libmpeg2 is not thread safe so use ffmepg for mpeg2/mpeg1 thumb extraction
      CDVDCodecOptions dvdOptions;
      pVideoCodec = CDVDFactoryCodec::OpenCodec(new CDVDVideoCodecFFmpeg(), hint, dvdOptions);
    }
    else
    {
      pVideoCodec = CDVDFactoryCodec::CreateVideoCodec( hint );
    }

    if (pVideoCodec)
    {
      int nTotalLen = pDemuxer->GetStreamLength();
      int nSeekTo = (pos==-1?nTotalLen / 3:pos);

      CLog::Log(LOGDEBUG,"%s - seeking to pos %dms (total: %dms) in %s", __FUNCTION__, nSeekTo, nTotalLen, redactPath.c_str());
      if (pDemuxer->SeekTime(nSeekTo, true))
      {
        int iDecoderState = VC_ERROR;
        DVDVideoPicture picture;

        memset(&picture, 0, sizeof(picture));

        // num streams * 160 frames, should get a valid frame, if not abort.
        int abort_index = pDemuxer->GetNrOfStreams() * 160;
        do
        {
          DemuxPacket* pPacket = pDemuxer->Read();
          packetsTried++;

          if (!pPacket)
            break;

          if (pPacket->iStreamId != nVideoStream)
          {
            CDVDDemuxUtils::FreeDemuxPacket(pPacket);
            continue;
          }

          iDecoderState = pVideoCodec->Decode(pPacket->pData, pPacket->iSize, pPacket->dts, pPacket->pts);
          CDVDDemuxUtils::FreeDemuxPacket(pPacket);

          if (iDecoderState & VC_ERROR)
            break;

          if (iDecoderState & VC_PICTURE)
          {
            memset(&picture, 0, sizeof(DVDVideoPicture));
            if (pVideoCodec->GetPicture(&picture))
            {
              if(!(picture.iFlags & DVP_FLAG_DROPPED))
                break;
            }
          }

        } while (abort_index--);

        if (iDecoderState & VC_PICTURE && !(picture.iFlags & DVP_FLAG_DROPPED))
        {
          {
            unsigned int nWidth = g_advancedSettings.GetThumbSize();
            double aspect = (double)picture.iDisplayWidth / (double)picture.iDisplayHeight;
            if(hint.forced_aspect && hint.aspect != 0)
              aspect = hint.aspect;
            unsigned int nHeight = (unsigned int)((double)g_advancedSettings.GetThumbSize() / aspect);

            uint8_t *pOutBuf = new uint8_t[nWidth * nHeight * 4];
            struct SwsContext *context = sws_getContext(picture.iWidth, picture.iHeight,
                  PIX_FMT_YUV420P, nWidth, nHeight, PIX_FMT_BGRA, SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);

            if (context)
            {
              uint8_t *src[] = { picture.data[0], picture.data[1], picture.data[2], 0 };
              int     srcStride[] = { picture.iLineSize[0], picture.iLineSize[1], picture.iLineSize[2], 0 };
              uint8_t *dst[] = { pOutBuf, 0, 0, 0 };
              int     dstStride[] = { (int)nWidth*4, 0, 0, 0 };
              int orientation = DegreeToOrientation(hint.orientation);
              sws_scale(context, src, srcStride, 0, picture.iHeight, dst, dstStride);
              sws_freeContext(context);

              details.width = nWidth;
              details.height = nHeight;
              CPicture::CacheTexture(pOutBuf, nWidth, nHeight, nWidth * 4, orientation, nWidth, nHeight, CTextureCache::GetCachedPath(details.file));
              bOk = true;
            }

            delete [] pOutBuf;
          }
        }
        else
        {
          CLog::Log(LOGDEBUG,"%s - decode failed in %s after %d packets.", __FUNCTION__, redactPath.c_str(), packetsTried);
        }
      }
      delete pVideoCodec;
    }
  }

  if (pDemuxer)
    delete pDemuxer;

  delete pInputStream;

  if(!bOk)
  {
    XFILE::CFile file;
    if(file.OpenForWrite(CTextureCache::GetCachedPath(details.file)))
      file.Close();
  }

  unsigned int nTotalTime = XbmcThreads::SystemClockMillis() - nTime;
  CLog::Log(LOGDEBUG,"%s - measured %u ms to extract thumb from file <%s> in %d packets. ", __FUNCTION__, nTotalTime, redactPath.c_str(), packetsTried);
  return bOk;
}

/**
 * \brief Open the item pointed to by pItem and extact streamdetails
 * \return true if the stream details have changed
 */
bool CDVDFileInfo::GetFileStreamDetails(CFileItem *pItem)
{
  if (!pItem)
    return false;

  std::string strFileNameAndPath;
  if (pItem->HasVideoInfoTag())
    strFileNameAndPath = pItem->GetVideoInfoTag()->m_strFileNameAndPath;

  if (strFileNameAndPath.empty())
    strFileNameAndPath = pItem->GetPath();

  std::string playablePath = strFileNameAndPath;
  if (URIUtils::IsStack(playablePath))
    playablePath = XFILE::CStackDirectory::GetFirstStackedFile(playablePath);

  CDVDInputStream *pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, playablePath, "");
  if (!pInputStream)
    return false;

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    delete pInputStream;
    return false;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || !pInputStream->Open(playablePath.c_str(), "", true))
  {
    delete pInputStream;
    return false;
  }

  CDVDDemux *pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(pInputStream, true);
  if (pDemuxer)
  {
    bool retVal = DemuxerToStreamDetails(pInputStream, pDemuxer, pItem->GetVideoInfoTag()->m_streamDetails, strFileNameAndPath);
    delete pDemuxer;
    delete pInputStream;
    return retVal;
  }
  else
  {
    delete pInputStream;
    return false;
  }
}

bool CDVDFileInfo::DemuxerToStreamDetails(CDVDInputStream *pInputStream, CDVDDemux *pDemuxer, const std::vector<CStreamDetailSubtitle> &subs, CStreamDetails &details)
{
  bool result = DemuxerToStreamDetails(pInputStream, pDemuxer, details);
  for (unsigned int i = 0; i < subs.size(); i++)
  {
    CStreamDetailSubtitle* sub = new CStreamDetailSubtitle();
    sub->m_strLanguage = subs[i].m_strLanguage;
    details.AddStream(sub);
    result = true;
  }
  return result;
}

/* returns true if details have been added */
bool CDVDFileInfo::DemuxerToStreamDetails(CDVDInputStream *pInputStream, CDVDDemux *pDemux, CStreamDetails &details, const std::string &path)
{
  bool retVal = false;
  details.Reset();

  const CURL pathToUrl(path);
  for (int iStream=0; iStream<pDemux->GetNrOfStreams(); iStream++)
  {
    CDemuxStream *stream = pDemux->GetStream(iStream);
    if (stream->type == STREAM_VIDEO && !(stream->flags & AV_DISPOSITION_ATTACHED_PIC))
    {
      CStreamDetailVideo *p = new CStreamDetailVideo();
      p->m_iWidth = ((CDemuxStreamVideo *)stream)->iWidth;
      p->m_iHeight = ((CDemuxStreamVideo *)stream)->iHeight;
      p->m_fAspect = ((CDemuxStreamVideo *)stream)->fAspect;
      if (p->m_fAspect == 0.0f)
        p->m_fAspect = (float)p->m_iWidth / p->m_iHeight;
      pDemux->GetStreamCodecName(iStream, p->m_strCodec);
      p->m_iDuration = pDemux->GetStreamLength();
      p->m_strStereoMode = ((CDemuxStreamVideo *)stream)->stereo_mode;

      // stack handling
      if (URIUtils::IsStack(path))
      {
        CFileItemList files;
        XFILE::CStackDirectory stack;
        stack.GetDirectory(pathToUrl, files);

        // skip first path as we already know the duration
        for (int i = 1; i < files.Size(); i++)
        {
           int duration = 0;
           if (CDVDFileInfo::GetFileDuration(files[i]->GetPath(), duration))
             p->m_iDuration = p->m_iDuration + duration;
        }
      }

      // finally, calculate seconds
      if (p->m_iDuration > 0)
        p->m_iDuration = p->m_iDuration / 1000;

      details.AddStream(p);
      retVal = true;
    }

    else if (stream->type == STREAM_AUDIO)
    {
      CStreamDetailAudio *p = new CStreamDetailAudio();
      p->m_iChannels = ((CDemuxStreamAudio *)stream)->iChannels;
      p->m_strLanguage = stream->language;
      pDemux->GetStreamCodecName(iStream, p->m_strCodec);
      details.AddStream(p);
      retVal = true;
    }

    else if (stream->type == STREAM_SUBTITLE)
    {
      CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
      p->m_strLanguage = stream->language;
      details.AddStream(p);
      retVal = true;
    }
  }  /* for iStream */

  details.DetermineBestStreams();
#ifdef HAVE_LIBBLURAY
  // correct bluray runtime. we need the duration from the input stream, not the demuxer.
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    if(((CDVDInputStreamBluray*)pInputStream)->GetTotalTime() > 0)
    {
      CStreamDetailVideo* detailVideo = (CStreamDetailVideo*)details.GetNthStream(CStreamDetail::VIDEO, 0);
      if (detailVideo)
        detailVideo->m_iDuration = ((CDVDInputStreamBluray*)pInputStream)->GetTotalTime() / 1000;
    }
  }
#endif
  return retVal;
}

bool CDVDFileInfo::AddExternalSubtitleToDetails(const std::string &path, CStreamDetails &details, const std::string& filename, const std::string& subfilename)
{
  std::string ext = URIUtils::GetExtension(filename);
  std::string vobsubfile = subfilename;
  if(ext == ".idx")
  {
    if (vobsubfile.empty())
      vobsubfile = URIUtils::ReplaceExtension(filename, ".sub");

    CDVDDemuxVobsub v;
    if (!v.Open(filename, STREAM_SOURCE_NONE, vobsubfile))
      return false;

    int count = v.GetNrOfStreams();

    for(int i = 0; i < count; i++)
    {
      CStreamDetailSubtitle *dsub = new CStreamDetailSubtitle();
      CDemuxStream* stream = v.GetStream(i);
      std::string lang = stream->language;
      dsub->m_strLanguage = g_LangCodeExpander.ConvertToISO6392T(lang);
      details.AddStream(dsub);
    }
    return true;
  }
  if(ext == ".sub")
  {
    std::string strReplace(URIUtils::ReplaceExtension(filename,".idx"));
    if (XFILE::CFile::Exists(strReplace))
      return false;
  }

  CStreamDetailSubtitle *dsub = new CStreamDetailSubtitle();
  ExternalStreamInfo info;
  CUtil::GetExternalStreamDetailsFromFilename(path, filename, info);
  dsub->m_strLanguage = g_LangCodeExpander.ConvertToISO6392T(info.language);
  details.AddStream(dsub);

  return true;
}

