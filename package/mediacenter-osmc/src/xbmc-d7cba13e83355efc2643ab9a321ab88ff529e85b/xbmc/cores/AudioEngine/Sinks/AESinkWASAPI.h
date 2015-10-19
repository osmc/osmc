#pragma once
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

#include <stdint.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"


class CAESinkWASAPI : public IAESink
{
public:
    virtual const char *GetName() { return "WASAPI"; }

    CAESinkWASAPI();
    virtual ~CAESinkWASAPI();

    virtual bool Initialize  (AEAudioFormat &format, std::string &device);
    virtual void Deinitialize();

    virtual void         GetDelay(AEDelayStatus& status);
    virtual double       GetCacheTotal               ();
    virtual unsigned int AddPackets                  (uint8_t **data, unsigned int frames, unsigned int offset);
    virtual void         Drain                       ();
    static  void         EnumerateDevicesEx          (AEDeviceInfoList &deviceInfoList, bool force = false);
private:
    bool         InitializeExclusive(AEAudioFormat &format);
    void         AEChannelsFromSpeakerMask(DWORD speakers);
    static DWORD        SpeakerMaskFromAEChannels(const CAEChannelInfo &channels);
    static void         BuildWaveFormatExtensible(AEAudioFormat &format, WAVEFORMATEXTENSIBLE &wfxex);
    static void         BuildWaveFormatExtensibleIEC61397(AEAudioFormat &format, WAVEFORMATEXTENSIBLE_IEC61937 &wfxex);
    bool IsUSBDevice();

    static const char  *WASAPIErrToStr(HRESULT err);

    HANDLE              m_needDataEvent;
    IMMDevice          *m_pDevice;
    IAudioClient       *m_pAudioClient;
    IAudioRenderClient *m_pRenderClient;
    IAudioClock        *m_pAudioClock;

    AEAudioFormat       m_format;
    enum AEDataFormat   m_encodedFormat;
    unsigned int        m_encodedChannels;
    unsigned int        m_encodedSampleRate;
    CAEChannelInfo      m_channelLayout;
    std::string         m_device;

    enum AEDataFormat   sinkReqFormat;
    enum AEDataFormat   sinkRetFormat;

    bool                m_running;
    bool                m_initialized;
    bool                m_isSuspended;    /* sink is in a suspended state - release audio device */
    bool                m_isDirty;        /* sink output failed - needs re-init or new device */

    unsigned int        m_uiBufferLen;    /* wasapi endpoint buffer size, in frames */
    double              m_avgTimeWaiting; /* time between next buffer of data from SoftAE and driver call for data */
    double              m_sinkLatency;    /* time in seconds of total duration of the two WASAPI buffers */
    uint64_t            m_sinkFrames;
    uint64_t            m_clockFreq;

    uint8_t            *m_pBuffer;
    int                 m_bufferPtr;
};
