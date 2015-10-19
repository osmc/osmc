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

#include "system.h"

#include "utils/auto_buffer.h"

#include <string>
#include <sstream>

class CDVDInputStream;

// buffered class for subtitle reading

class CDVDSubtitleStream
{
public:
  CDVDSubtitleStream();
  virtual ~CDVDSubtitleStream();

  bool Open(const std::string& strFile);

  /** \brief Checks if the subtitle associated with the pInputStream
   *         is known to be incompatible, e.g., vob sub files.
   *  \param[in] pInputStream The input stream for the subtitle to check.
   */
  bool IsIncompatible(CDVDInputStream* pInputStream, XUTILS::auto_buffer& buf, size_t* bytesRead);

  int Read(char* buf, int buf_size);
  long Seek(long offset, int whence);

  char* ReadLine(char* pBuffer, int iLen);
  //wchar* ReadLineW(wchar* pBuffer, int iLen) { return NULL; };

  std::stringstream m_stringstream;
};

