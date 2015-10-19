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

#include "UPnPFile.h"
#include "UPnPDirectory.h"
#include "FileFactory.h"
#include "FileItem.h"
#include "URL.h"

using namespace XFILE;

CUPnPFile::CUPnPFile()
{
}

CUPnPFile::~CUPnPFile()
{
}

bool CUPnPFile::Open(const CURL& url)
{
  CFileItem item_new;
  if (CUPnPDirectory::GetResource(url, item_new))
  {
    //CLog::Log(LOGDEBUG,"FileUPnP - file redirect to %s.", item_new.GetPath().c_str());
    IFile *pNewImp = CFileFactory::CreateLoader(item_new.GetPath());    
    CURL *pNewUrl = new CURL(item_new.GetPath());    
    if (pNewImp)
    {
      throw new CRedirectException(pNewImp, pNewUrl);
    }
    delete pNewUrl;    
  }
  return false;
}

int CUPnPFile::Stat(const CURL& url, struct __stat64* buffer)
{
  CFileItem item_new;
  if (CUPnPDirectory::GetResource(url, item_new))
  {
    //CLog::Log(LOGDEBUG,"FileUPnP - file redirect to %s.", item_new.GetPath().c_str());
    IFile *pNewImp = CFileFactory::CreateLoader(item_new.GetPath());
    CURL *pNewUrl = new CURL(item_new.GetPath());
    if (pNewImp)
    {
      throw new CRedirectException(pNewImp, pNewUrl);
    }
    delete pNewUrl;
  }
  return -1;
}

bool CUPnPFile::Exists(const CURL& url)
{
  CFileItem item_new;
  if (CUPnPDirectory::GetResource(url, item_new))
  {
    //CLog::Log(LOGDEBUG,"FileUPnP - file redirect to %s.", item_new.GetPath().c_str());
    IFile *pNewImp = CFileFactory::CreateLoader(item_new.GetPath());
    CURL *pNewUrl = new CURL(item_new.GetPath());
    if (pNewImp)
    {
      throw new CRedirectException(pNewImp, pNewUrl);
    }
    delete pNewUrl;
  }
  return false;
}
