#pragma once
/*
*      Copyright (C) 2014 Team XBMC
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

#include "filesystem/IDirectory.h"

namespace XFILE
{
  class CWin32SMBFile; // forward declaration

  class CWin32SMBDirectory : public IDirectory
  {
    friend class CWin32SMBFile;
  public:
    CWin32SMBDirectory(void);
    virtual ~CWin32SMBDirectory(void);
    virtual bool GetDirectory(const CURL& url, CFileItemList& items);
    virtual bool Create(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual bool Remove(const CURL& url);
  protected:
    bool RealCreate(const CURL& url, bool tryToConnect);
    bool RealExists(const CURL& url, bool tryToConnect);
    static bool GetNetworkResources(const CURL& basePath, CFileItemList& items);
    bool ConnectAndAuthenticate(CURL& url, bool allowPromptForCredential = false);
  };
}
