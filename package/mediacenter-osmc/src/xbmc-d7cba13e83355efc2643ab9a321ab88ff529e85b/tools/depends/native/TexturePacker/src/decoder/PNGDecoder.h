/*
 *      Copyright (C) 2014 Team Kodi
 *      http://kodi.tv
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

#pragma once
#include "IDecoder.h"

class PNGDecoder : public IDecoder
{
  public:
    virtual ~PNGDecoder(){}
    virtual bool CanDecode(const std::string &filename);
    virtual bool LoadFile(const std::string &filename, DecodedFrames &frames);
    virtual void FreeDecodedFrames(DecodedFrames &frames);
    virtual const char* GetImageFormatName() { return "PNG"; }
    virtual const char* GetDecoderName() { return "libpng"; }
  protected:
    virtual void FillSupportedExtensions();
};