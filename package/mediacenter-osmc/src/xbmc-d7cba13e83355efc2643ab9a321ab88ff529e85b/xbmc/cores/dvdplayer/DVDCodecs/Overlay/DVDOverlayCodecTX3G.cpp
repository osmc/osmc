/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include "DVDOverlayCodecTX3G.h"
#include "DVDOverlayText.h"
#include "DVDStreamInfo.h"
#include "DVDCodecs/DVDCodecs.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/auto_buffer.h"

// 3GPP/TX3G (aka MPEG-4 Timed Text) Subtitle support
// 3GPP -> 3rd Generation Partnership Program
// adapted from https://trac.handbrake.fr/browser/trunk/libhb/dectx3gsub.c;

#define LEN_CHECK(x)    do { if((end - pos) < (x)) return OC_ERROR; } while(0)

// NOTE: None of these macros check for buffer overflow
#define READ_U8()       *pos;                                                     pos += 1;
#define READ_U16()      (pos[0] << 8)  |  pos[1];                                 pos += 2;
#define READ_U32()      (pos[0] << 24) | (pos[1] << 16) | (pos[2] << 8) | pos[3]; pos += 4;
#define READ_ARRAY(n)    pos;                                                     pos += n;
#define SKIP_ARRAY(n)    pos += n;

#define FOURCC(str)  ((((uint32_t) str[0]) << 24) | \
                      (((uint32_t) str[1]) << 16) | \
                      (((uint32_t) str[2]) << 8) | \
                      (((uint32_t) str[3]) << 0))
                      
typedef enum {
 BOLD       = 0x1,
 ITALIC     = 0x2,
 UNDERLINE  = 0x4
} FaceStyleFlag;

// NOTE: indices in terms of *character* (not: byte) positions
typedef struct {
  uint16_t  bgnChar;
  uint16_t  endChar;
  uint16_t  fontID;
  uint8_t   faceStyleFlags;  // FaceStyleFlag
  uint8_t   fontSize;
  uint32_t  textColorRGBA;
} StyleRecord;

CDVDOverlayCodecTX3G::CDVDOverlayCodecTX3G() : CDVDOverlayCodec("TX3G Subtitle Decoder")
{
  m_pOverlay = NULL;
  // stupid, this comes from a static global in GUIWindowFullScreen.cpp
  uint32_t colormap[8] = { 0xFFFFFF00, 0xFFFFFFFF, 0xFF0099FF, 0xFF00FF00, 0xFFCCFF00, 0xFF00FFFF, 0xFFE5E5E5, 0xFFC0C0C0 };
  m_textColor = colormap[CSettings::GetInstance().GetInt(CSettings::SETTING_SUBTITLES_COLOR)];
}

CDVDOverlayCodecTX3G::~CDVDOverlayCodecTX3G()
{
  if (m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

bool CDVDOverlayCodecTX3G::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (hints.codec == AV_CODEC_ID_MOV_TEXT)
    return true;
  return false;
}

void CDVDOverlayCodecTX3G::Dispose()
{
  if (m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

int CDVDOverlayCodecTX3G::Decode(DemuxPacket *pPacket)
{
  if (m_pOverlay)
    SAFE_RELEASE(m_pOverlay);

  m_pOverlay = new CDVDOverlayText();
  CDVDOverlayCodec::GetAbsoluteTimes(m_pOverlay->iPTSStartTime, m_pOverlay->iPTSStopTime, pPacket, m_pOverlay->replace);

  // do not move this. READ_XXXX macros modify pos.
  uint8_t  *pos = pPacket->pData;
  uint8_t  *end = pPacket->pData + pPacket->iSize;

  // Parse the packet as a TX3G TextSample.
  // Look for a single StyleBox ('styl') and 
  // read all contained StyleRecords.
  // Ignore all other box types.
  // NOTE: Buffer overflows on read are not checked.
  // ALSO: READ_XXXX/SKIP_XXXX macros will modify pos.
  LEN_CHECK(2);
  uint16_t textLength = READ_U16();
  LEN_CHECK(textLength);
  uint8_t *text = READ_ARRAY(textLength);

  int numStyleRecords = 0;
  // reserve one more style slot for broken encoders

  XUTILS::auto_buffer bgnStyle(textLength+1);
  XUTILS::auto_buffer endStyle(textLength+1);

  memset(bgnStyle.get(), 0, textLength+1);
  memset(endStyle.get(), 0, textLength+1);

  int bgnColorIndex = 0, endColorIndex = 0;
  uint32_t textColorRGBA = m_textColor;
  while (pos < end)
  {
    // Read TextSampleModifierBox
    LEN_CHECK(4);
    uint32_t size = READ_U32();
    if (size == 0)
      size = pos - end;   // extends to end of packet
    if (size == 1)
    {
      CLog::Log(LOGDEBUG, "CDVDOverlayCodecTX3G: TextSampleModifierBox has unsupported large size" );
      break;
    }
    LEN_CHECK(4);
    uint32_t type = READ_U32();
    if (type == FOURCC("uuid"))
    {
      CLog::Log(LOGDEBUG, "CDVDOverlayCodecTX3G: TextSampleModifierBox has unsupported extended type" );
      break;
    }

    if (type == FOURCC("styl"))
    {
      // Found a StyleBox. Parse the contained StyleRecords
      if ( numStyleRecords != 0 )
      {
        CLog::Log(LOGDEBUG, "CDVDOverlayCodecTX3G: found additional StyleBoxes on subtitle; skipping" );
        LEN_CHECK(size);
        SKIP_ARRAY(size);
        continue;
      }

      LEN_CHECK(2);
      numStyleRecords = READ_U16();
      for (int i = 0; i < numStyleRecords; i++)
      {
        StyleRecord curRecord;
        LEN_CHECK(12);
        curRecord.bgnChar         = READ_U16();
        curRecord.endChar         = READ_U16();
        curRecord.fontID          = READ_U16();
        curRecord.faceStyleFlags  = READ_U8();
        curRecord.fontSize        = READ_U8();
        curRecord.textColorRGBA   = READ_U32();
        // clamp bgnChar/bgnChar to textLength,
        // we alloc enough space above and this
        // fixes borken encoders that do not handle
        // endChar correctly.
        if (curRecord.bgnChar > textLength)
          curRecord.bgnChar = textLength;
        if (curRecord.endChar > textLength)
          curRecord.endChar = textLength;

        bgnStyle.get()[curRecord.bgnChar] |= curRecord.faceStyleFlags;
        endStyle.get()[curRecord.endChar] |= curRecord.faceStyleFlags;
        bgnColorIndex = curRecord.bgnChar;
        endColorIndex = curRecord.endChar;
        textColorRGBA = curRecord.textColorRGBA;
      }
    }
    else
    {
      // Found some other kind of TextSampleModifierBox. Skip it.
      LEN_CHECK(size);
      SKIP_ARRAY(size);
    }
  }

  // Copy text to out and add HTML markup for the style records
  int charIndex = 0;
  std::string strUTF8;
  // index over textLength chars to include broken encoders,
  // so we pickup closing styles on broken encoders
  for (pos = text, end = text + textLength; pos <= end; pos++)
  {
    if ((*pos & 0xC0) == 0x80)
    {
      // Is a non-first byte of a multi-byte UTF-8 character
      strUTF8.append((const char*)pos, 1);
      continue;   // ...without incrementing 'charIndex'
    }

    uint8_t bgnStyles = bgnStyle.get()[charIndex];
    uint8_t endStyles = endStyle.get()[charIndex];

    // [B] or [/B] -> toggle bold on and off
    // [I] or [/I] -> toggle italics on and off
    // [COLOR ffab007f] or [/COLOR] -> toggle color on and off
    // [CAPS <option>]  or [/CAPS]  -> toggle capatilization on and off

    if (endStyles & BOLD)
      strUTF8.append("[/B]");
    if (endStyles & ITALIC)
      strUTF8.append("[/I]");
    // we do not support underline
    //if (endStyles & UNDERLINE)
    //  strUTF8.append("[/U]");
    if (endColorIndex == charIndex && textColorRGBA != m_textColor)
      strUTF8.append("[/COLOR]");

    // invert the order from above so we bracket the text correctly.
    if (bgnColorIndex == charIndex && textColorRGBA != m_textColor)
      strUTF8 += StringUtils::Format("[COLOR %8x]", textColorRGBA);
    // we do not support underline
    //if (bgnStyles & UNDERLINE)
    //  strUTF8.append("[U]");
    if (bgnStyles & ITALIC)
      strUTF8.append("[I]");
    if (bgnStyles & BOLD)
      strUTF8.append("[B]");

    // stuff the UTF8 char
    strUTF8.append((const char*)pos, 1);

    // this is a char index, not a byte index.
    charIndex++;
  }
  
  if (strUTF8.empty())
    return OC_BUFFER;

  if (strUTF8[strUTF8.size()-1] == '\n')
    strUTF8.erase(strUTF8.size()-1);

  // add a new text element to our container
  m_pOverlay->AddElement(new CDVDOverlayText::CElementText(strUTF8.c_str()));

  return OC_OVERLAY;
}

void CDVDOverlayCodecTX3G::Reset()
{
  if (m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

void CDVDOverlayCodecTX3G::Flush()
{
  if (m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

CDVDOverlay* CDVDOverlayCodecTX3G::GetOverlay()
{
  if (m_pOverlay)
  {
    CDVDOverlay* overlay = m_pOverlay;
    m_pOverlay = NULL;
    return overlay;
  }
  return NULL;
}
