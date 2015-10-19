#pragma once

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

#include "DVDInputStream.h"
#include "utils/HttpHeader.h"

namespace XFILE
{
  class CCurlFile;
}

class CDVDInputStreamHttp : public CDVDInputStream
{
public:
  CDVDInputStreamHttp();
  virtual ~CDVDInputStreamHttp();
  virtual bool Open(const char* strFile, const std::string& content, bool contentLookup);
  virtual void Close();
  virtual int Read(uint8_t* buf, int buf_size);
  virtual int64_t Seek(int64_t offset, int whence);
  virtual bool IsEOF();
  virtual int64_t GetLength();

  CHttpHeader* GetHttpHeader();

protected:
  XFILE::CCurlFile* m_pFile;
  CHttpHeader m_httpHeader;
  bool m_eof;
};

