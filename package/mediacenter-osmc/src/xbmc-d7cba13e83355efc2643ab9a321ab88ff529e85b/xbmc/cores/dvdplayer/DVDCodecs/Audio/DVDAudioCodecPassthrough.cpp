/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"

#include <algorithm>

#include "cores/AudioEngine/AEFactory.h"

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(void) :
  m_buffer    (NULL),
  m_bufferSize(0)
{
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  bool bSupportsAC3Out    = CAEFactory::SupportsRaw(AE_FMT_AC3, hints.samplerate);
  bool bSupportsEAC3Out   = CAEFactory::SupportsRaw(AE_FMT_EAC3, 192000);
  bool bSupportsDTSOut    = CAEFactory::SupportsRaw(AE_FMT_DTS, hints.samplerate);
  bool bSupportsTrueHDOut = CAEFactory::SupportsRaw(AE_FMT_TRUEHD, 192000);
  bool bSupportsDTSHDOut  = CAEFactory::SupportsRaw(AE_FMT_DTSHD, 192000);

  /* only get the dts core from the parser if we don't support dtsHD */
  m_info.SetCoreOnly(!bSupportsDTSHDOut);
  m_bufferSize = 0;

  /* 32kHz E-AC-3 passthrough requires 128kHz IEC 60958 stream
   * which HDMI does not support, and IEC 61937 does not mention
   * reduced sample rate support, so support only 44.1 and 48 */
  if ((hints.codec == AV_CODEC_ID_AC3 && bSupportsAC3Out) ||
      (hints.codec == AV_CODEC_ID_EAC3 && bSupportsEAC3Out && (hints.samplerate == 44100 || hints.samplerate == 48000)) ||
      (hints.codec == AV_CODEC_ID_DTS && bSupportsDTSOut) ||
      (hints.codec == AV_CODEC_ID_TRUEHD && bSupportsTrueHDOut))
  {
    return true;
  }

  return false;
}

int CDVDAudioCodecPassthrough::GetSampleRate()
{
  return m_info.GetOutputRate();
}

int CDVDAudioCodecPassthrough::GetEncodedSampleRate()
{
  return m_info.GetSampleRate();
}

enum AEDataFormat CDVDAudioCodecPassthrough::GetDataFormat()
{
  switch(m_info.GetDataType())
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      return AE_FMT_AC3;

    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      return AE_FMT_DTS;

    case CAEStreamInfo::STREAM_TYPE_EAC3:
      return AE_FMT_EAC3;

    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      return AE_FMT_TRUEHD;

    case CAEStreamInfo::STREAM_TYPE_DTSHD:
      return AE_FMT_DTSHD;

    default:
      return AE_FMT_INVALID; //Unknown stream type
  }
}

int CDVDAudioCodecPassthrough::GetChannels()
{
  return m_info.GetOutputChannels();
}

int CDVDAudioCodecPassthrough::GetEncodedChannels()
{
  return m_info.GetChannels();
}

CAEChannelInfo CDVDAudioCodecPassthrough::GetChannelMap()
{
  return m_info.GetChannelMap();
}

void CDVDAudioCodecPassthrough::Dispose()
{
  if (m_buffer)
  {
    delete[] m_buffer;
    m_buffer = NULL;
  }

  m_bufferSize = 0;
}

int CDVDAudioCodecPassthrough::Decode(uint8_t* pData, int iSize)
{
  if (iSize <= 0) return 0;

  unsigned int size = m_bufferSize;
  unsigned int used = m_info.AddData(pData, iSize, &m_buffer, &size);
  m_bufferSize = std::max(m_bufferSize, size);

  /* if we have a frame */
  if (size)
    m_packer.Pack(m_info, m_buffer, size);

  return used;
}

int CDVDAudioCodecPassthrough::GetData(uint8_t** dst)
{
  int size = m_packer.GetSize();
  *dst     = m_packer.GetBuffer();
  return size;
}

void CDVDAudioCodecPassthrough::Reset()
{
}

int CDVDAudioCodecPassthrough::GetBufferSize()
{
  return (int)m_info.GetBufferSize();
}
