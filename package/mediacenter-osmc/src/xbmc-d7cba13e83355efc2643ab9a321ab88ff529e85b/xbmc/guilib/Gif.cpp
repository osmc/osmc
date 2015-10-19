/*
*      Copyright (C) 2005-2014 Team XBMC
*      http://www.xbmc.org
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
#if defined(HAS_GIFLIB)
#include "Gif.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "guilib/Texture.h"
#include "filesystem/File.h"
#include <algorithm>

#define UNSIGNED_LITTLE_ENDIAN(lo, hi)	((lo) | ((hi) << 8))
#define GIF_MAX_MEMORY 82944000U // about 79 MB, which is equivalent to 10 full hd frames.

class Gifreader
{
public:
  unsigned char* buffer;
  unsigned int buffSize;
  unsigned int readPosition;

  Gifreader() : buffer(nullptr), buffSize(0), readPosition(0) {}
};

int ReadFromMemory(GifFileType* gif, GifByteType* gifbyte, int len)
{
  unsigned int alreadyRead = static_cast<Gifreader*>(gif->UserData)->readPosition;
  int buffSizeLeft = static_cast<Gifreader*>(gif->UserData)->buffSize - alreadyRead;
  int readBytes = len;

  if (len <= 0)
    readBytes = 0;

  if (len > buffSizeLeft)
    readBytes = buffSizeLeft;

  if (readBytes > 0)
  {
    unsigned char* src = static_cast<Gifreader*>(gif->UserData)->buffer + alreadyRead;
    memcpy(gifbyte, src, readBytes);
    static_cast<Gifreader*>(gif->UserData)->readPosition += readBytes;
  }
  return readBytes;
}

int ReadFromVfs(GifFileType* gif, GifByteType* gifbyte, int len)
{
  XFILE::CFile *gifFile = static_cast<XFILE::CFile*>(gif->UserData);
  return gifFile->Read(gifbyte, len);
}


Gif::Gif() :
  m_imageSize(0),
  m_pitch(0),
  m_loops(0),
  m_numFrames(0),
  m_filename(""),
  m_gif(nullptr),
  m_pTemplate(nullptr),
  m_isAnimated(-1)
{
  if (!m_dll.Load())
    CLog::Log(LOGERROR, "Gif::Gif(): Could not load giflib");
  m_gifFile = new XFILE::CFile();
}

Gif::~Gif()
{
  if (m_dll.IsLoaded())
  {
    Close(m_gif);

    m_dll.Unload();
    Release();
  }
  delete m_gifFile;
}

void Gif::Close(GifFileType* gif)
{
  int err = 0;
  int reason = 0;
#if GIFLIB_MAJOR == 5
  err = m_dll.DGifCloseFile(gif, &reason);
#else
  err = m_dll.DGifCloseFile(gif);
  reason = m_dll.GifLastError();
  if (err == GIF_ERROR)
    free(gif);
#endif
  if (err == GIF_ERROR)
    PrettyPrintError(StringUtils::Format("Gif::~Gif(): closing file %s failed", memOrFile().c_str()), reason);
}

void Gif::Release()
{
  delete[] m_pTemplate;
  m_pTemplate = nullptr;
  m_globalPalette.clear();
  m_frames.clear();
}

void Gif::ConvertColorTable(std::vector<GifColor> &dest, ColorMapObject* src, unsigned int size)
{
  for (unsigned int i = 0; i < size; ++i)
  {
    GifColor c;

    c.r = src->Colors[i].Red;
    c.g = src->Colors[i].Green;
    c.b = src->Colors[i].Blue;
    c.a = 0xff;
    dest.push_back(c);
  }
}

bool Gif::LoadGifMetaData(GifFileType* gif)
{
  if (!m_dll.IsLoaded() || !Slurp(gif))
    return false;

  m_height = gif->SHeight;
  m_width = gif->SWidth;
  if (!m_height || !m_width)
  {
    CLog::Log(LOGERROR, "Gif::LoadGif(): Zero sized image. File %s", memOrFile().c_str());
    return false;
  }

  m_numFrames = gif->ImageCount;
  if (m_numFrames > 0)
  {
    ExtensionBlock* extb = gif->SavedImages[0].ExtensionBlocks;
    if (extb && extb->Function == APPLICATION_EXT_FUNC_CODE)
    {
      // Read number of loops
      if (++extb && extb->Function == CONTINUE_EXT_FUNC_CODE)
      {
        m_loops = UNSIGNED_LITTLE_ENDIAN(extb->Bytes[1], extb->Bytes[2]);
      }
    }
  }
  else
  {
    CLog::Log(LOGERROR, "Gif::LoadGif(): No images found in file %s", memOrFile().c_str());
    return false;
  }

  m_pitch = m_width * sizeof(GifColor);
  m_imageSize = m_pitch * m_height;
  unsigned long memoryUsage = m_numFrames * m_imageSize;
  if (memoryUsage > GIF_MAX_MEMORY)
  {
    // at least 1 image
    m_numFrames = std::max(1U, GIF_MAX_MEMORY / m_imageSize);
    CLog::Log(LOGERROR, "Gif::LoadGif(): Memory consumption too high: %lu bytes. Restricting animation to %u. File %s", memoryUsage, m_numFrames, memOrFile().c_str());
  }

  return true;
}

bool Gif::LoadGifMetaData(const char* file)
{
  if (!m_dll.IsLoaded())
    return false;

  m_gifFile->Close();
  if (!m_gifFile->Open(file) || !Open(m_gif, m_gifFile, ReadFromVfs))
    return false;

  return LoadGifMetaData(m_gif);
}

bool Gif::Slurp(GifFileType* gif)
{
  if (m_dll.DGifSlurp(gif) == GIF_ERROR)
  {
    int reason = 0;
#if GIFLIB_MAJOR == 5
    reason = gif->Error;
#else
    reason = m_dll.GifLastError();
#endif
    PrettyPrintError(StringUtils::Format("Gif::LoadGif(): Could not read file %s", memOrFile().c_str()), reason);
    return false;
  }

  return true;
}

bool Gif::LoadGif(const char* file)
{
  m_filename = file;
  if (!LoadGifMetaData(m_filename.c_str()))
    return false;

  try
  {
    InitTemplateAndColormap();

    return ExtractFrames(m_numFrames);
  }
  catch (std::bad_alloc& ba)
  {
    CLog::Log(LOGERROR, "Gif::Load(): Out of memory while reading file %s - %s", memOrFile().c_str(), ba.what());
    Release();
    return false;
  }
}

bool Gif::IsAnimated(const char* file)
{
  if (!m_dll.IsLoaded())
    return false;

  if (m_isAnimated < 0)
  {
    m_filename = file;
    m_isAnimated = 0;

    GifFileType* gif = nullptr;
    XFILE::CFile gifFile;

    if (!gifFile.Open(file) || !Open(gif, &gifFile, ReadFromVfs))
      return false;

    if (gif)
    {
      if (Slurp(gif) && gif->ImageCount > 1)
        m_isAnimated = 1;

      Close(gif);
      gifFile.Close();
    }
  }
  return m_isAnimated > 0;
}

bool Gif::Open(GifFileType*& gif, void *dataPtr, InputFunc readFunc)
{
  int err = 0;
#if GIFLIB_MAJOR == 5
  gif = m_dll.DGifOpen(dataPtr, readFunc, &err);
#else
  gif = m_dll.DGifOpen(dataPtr, readFunc);
  if (!gif)
    err = m_dll.GifLastError();
#endif

  if (!gif)
  {
    PrettyPrintError(StringUtils::Format("Gif::Open(): Could not open file %s", memOrFile().c_str()), err);
    return false;
  }

  return true;
}

void Gif::InitTemplateAndColormap()
{
  m_pTemplate = new unsigned char[m_imageSize];
  memset(m_pTemplate, 0, m_imageSize);

  if (m_gif->SColorMap)
  {
    m_globalPalette.clear();
    ConvertColorTable(m_globalPalette, m_gif->SColorMap, m_gif->SColorMap->ColorCount);
  }
  else
    m_globalPalette.clear();
}

bool Gif::GcbToFrame(GifFrame &frame, unsigned int imgIdx)
{
  int transparent = -1;
  frame.m_delay = 0;
  frame.m_disposal = 0;

  if (m_gif->ImageCount > 0)
  {
#if GIFLIB_MAJOR == 5
    GraphicsControlBlock gcb;
    if (!m_dll.DGifSavedExtensionToGCB(m_gif, imgIdx, &gcb))
    {
      PrettyPrintError(StringUtils::Format("Gif::GcbToFrame(): Could not read GraphicsControlBlock of frame %d in file %s",
        imgIdx, memOrFile().c_str()), m_gif->Error);
      return false;
    }
    // delay in ms
    frame.m_delay = gcb.DelayTime * 10;
    frame.m_disposal = gcb.DisposalMode;
    transparent = gcb.TransparentColor;
#else
    ExtensionBlock* extb = m_gif->SavedImages[imgIdx].ExtensionBlocks;
    while (extb && extb->Function != GRAPHICS_EXT_FUNC_CODE)
      extb++;

    if (!extb || extb->ByteCount != 4)
    {
      CLog::Log(LOGERROR, "Gif::GcbToFrame() : Could not read GraphicsControlBlock of frame %d in file %s",
        imgIdx, memOrFile().c_str());
      return false;
    }
    else
    {
      frame.m_delay = UNSIGNED_LITTLE_ENDIAN(extb->Bytes[1], extb->Bytes[2]) * 10;
      frame.m_disposal = (extb->Bytes[0] >> 2) & 0x07;
      if (extb->Bytes[0] & 0x01)
      {
        transparent = static_cast<int>(extb->Bytes[3]);
        if (transparent < 0)
          transparent += 256;
      }
      else
        transparent = -1;
    }

#endif
  }

  if (transparent >= 0 && (unsigned)transparent < frame.m_palette.size())
    frame.m_palette[transparent].a = 0;
  return true;
}

bool Gif::ExtractFrames(unsigned int count)
{
  if (!m_gif)
    return false;

  if (!m_pTemplate)
  {
    CLog::Log(LOGDEBUG, "Gif::ExtractFrames(): No frame template available");
    return false;
  }

  for (unsigned int i = 0; i < count; i++)
  {
    FramePtr frame(new GifFrame);
    SavedImage savedImage = m_gif->SavedImages[i];
    GifImageDesc imageDesc = m_gif->SavedImages[i].ImageDesc;
    frame->m_height = imageDesc.Height;
    frame->m_width = imageDesc.Width;
    frame->m_top = imageDesc.Top;
    frame->m_left = imageDesc.Left;

    if (frame->m_top + frame->m_height > m_height || frame->m_left + frame->m_width > m_width
      || !frame->m_width || !frame->m_height)
    {
      CLog::Log(LOGDEBUG, "Gif::ExtractFrames(): Illegal frame dimensions: width: %d, height: %d, left: %d, top: %d instead of (%d,%d)",
        frame->m_width, frame->m_height, frame->m_left, frame->m_top, m_width, m_height);
      return false;
    }

    if (imageDesc.ColorMap)
    {
      frame->m_palette.clear();
      ConvertColorTable(frame->m_palette, imageDesc.ColorMap, imageDesc.ColorMap->ColorCount);
      // TODO save a backup of the palette for frames without a table in case there's no global table.
    }
    else if (m_gif->SColorMap)
    {
      frame->m_palette = m_globalPalette;
    }
    else
    {
      CLog::Log(LOGDEBUG, "Gif::ExtractFrames(): No color map found for frame %d", i);
      continue;
    }

    // fill delay, disposal and transparent color into frame
    if (!GcbToFrame(*frame, i))
      return false;

    frame->m_pImage = new unsigned char[m_imageSize];
    frame->m_imageSize = m_imageSize;
    memcpy(frame->m_pImage, m_pTemplate, m_imageSize);

    ConstructFrame(*frame, savedImage.RasterBits);

    if (!PrepareTemplate(*frame))
      return false;

    m_frames.push_back(frame);
  }
  return true;
}

void Gif::ConstructFrame(GifFrame &frame, const unsigned char* src) const
{
  size_t paletteSize = frame.m_palette.size();

  for (unsigned int dest_y = frame.m_top, src_y = 0; src_y < frame.m_height; ++dest_y, ++src_y)
  {
    unsigned char *to = frame.m_pImage + (dest_y * m_pitch) + (frame.m_left * sizeof(GifColor));

    const unsigned char *from = src + (src_y * frame.m_width);
    for (unsigned int src_x = 0; src_x < frame.m_width; ++src_x)
    {
      unsigned char index = *from++;

      if (index >= paletteSize)
      {
        CLog::Log(LOGDEBUG, "Gif::ConstructFrame(): Pixel (%d,%d) has no valid palette entry, skip it", src_x, src_y);
        continue;
      }

      GifColor col = frame.m_palette[index];
      if (col.a != 0)
        memcpy(to, &col, sizeof(GifColor));

      to += 4;
    }
  }
}

bool Gif::PrepareTemplate(const GifFrame &frame)
{
  switch (frame.m_disposal)
  {
    /* No disposal specified. */
  case DISPOSAL_UNSPECIFIED:
    /* Leave image in place */
  case DISPOSE_DO_NOT:
    memcpy(m_pTemplate, frame.m_pImage, m_imageSize);
    break;

    /*
       Clear the frame's area to transparency.
       The disposal names is misleading. Do not restore to the background color because
       this part of the specification is ignored by all browsers/image viewers.
    */
  case DISPOSE_BACKGROUND:
  {
    ClearFrameAreaToTransparency(m_pTemplate, frame);
    break;
  }
  /* Restore to previous content */
  case DISPOSE_PREVIOUS:
  {
    bool valid = false;

    for (int i = m_frames.size() - 1; i >= 0; --i)
    {
      if (m_frames[i]->m_disposal != DISPOSE_PREVIOUS)
      {
        memcpy(m_pTemplate, m_frames[i]->m_pImage, m_imageSize);
        valid = true;
        break;
      }
    }
    if (!valid)
    {
      CLog::Log(LOGDEBUG, "Gif::PrepareTemplate(): Disposal method DISPOSE_PREVIOUS encountered, but could not find a suitable frame.");
      return false;
    }
    break;
  }
  default:
  {
    CLog::Log(LOGDEBUG, "Gif::PrepareTemplate(): Unknown disposal method: %d", frame.m_disposal);
    return false;
  }
  }
  return true;
}

void Gif::ClearFrameAreaToTransparency(unsigned char* dest, const GifFrame &frame)
{
  for (unsigned int dest_y = frame.m_top, src_y = 0; src_y < frame.m_height; ++dest_y, ++src_y)
  {
    unsigned char *to = dest + (dest_y * m_pitch) + (frame.m_left * sizeof(GifColor));
    for (unsigned int src_x = 0; src_x < frame.m_width; ++src_x)
    {
      to += 3;
      *to++ = 0;
    }
  }
}

bool Gif::LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height)
{
  if (!m_dll.IsLoaded())
    return false;

  if (!buffer || !bufSize || !width || !height)
    return false;

  Gifreader reader;
  reader.buffer = buffer;
  reader.buffSize = bufSize;

  if (!Open(m_gif, static_cast<void *>(&reader), ReadFromMemory))
    return false;

  if (!LoadGifMetaData(m_gif))
    return false;

  m_originalWidth = m_width;
  m_originalHeight = m_height;

  try
  {
    InitTemplateAndColormap();

    if (!ExtractFrames(m_numFrames))
      return false;
  }
  catch (std::bad_alloc& ba)
  {
    CLog::Log(LOGERROR, "Gif::LoadImageFromMemory(): Out of memory while extracting gif frames - %s", ba.what());
    Release();
    return false;
  }

  return true;
}

bool Gif::Decode(unsigned char* const pixels, unsigned int pitch, unsigned int format)
{
  if (m_width == 0 || m_height == 0
    || !m_dll.IsLoaded() || !m_gif
    || format != XB_FMT_A8R8G8B8 || !m_numFrames)
    return false;

  const unsigned char *src = m_frames[0]->m_pImage;
  unsigned char* dst = pixels;

  if (pitch == m_pitch)
    memcpy(dst, src, m_imageSize);
  else
  {
    for (unsigned int y = 0; y < m_height; y++)
    {
      memcpy(dst, src, m_pitch);
      src += m_pitch;
      dst += pitch;
    }
  }
  return true;
}

bool Gif::CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const std::string& destFile,
                                     unsigned char* &bufferout, unsigned int &bufferoutSize)
{
  CLog::Log(LOGERROR, "Gif::CreateThumbnailFromSurface(): Not implemented. Something went wrong, we don't store thumbnails as gifs!");
  return false;
}

void Gif::PrettyPrintError(std::string messageTemplate, int reason)
{
  const char* error = m_dll.GifErrorString(reason);
  std::string message;
  if (error)
  {
    message = StringUtils::Format(messageTemplate.append(" - %s").c_str(), error);
  }
  else
  {
    message = messageTemplate.append(" (reason unknown)");
  }
  CLog::Log(LOGERROR, "%s", message.c_str());
}

GifFrame::GifFrame() :
  m_pImage(nullptr),
  m_delay(0),
  m_imageSize(0),
  m_height(0),
  m_width(0),
  m_top(0),
  m_left(0),
  m_disposal(0)
{}


GifFrame::GifFrame(const GifFrame& src) :
  m_pImage(nullptr),
  m_delay(src.m_delay),
  m_imageSize(src.m_imageSize),
  m_height(src.m_height),
  m_width(src.m_width),
  m_top(src.m_top),
  m_left(src.m_left),
  m_disposal(src.m_disposal)
{
  if (src.m_pImage)
  {
    m_pImage = new unsigned char[m_imageSize];
    memcpy(m_pImage, src.m_pImage, m_imageSize);
  }

  if (src.m_palette.size())
  {
    m_palette = src.m_palette;
  }
}

GifFrame::~GifFrame()
{
  delete[] m_pImage;
  m_pImage = nullptr;
}
#endif//HAS_GIFLIB
