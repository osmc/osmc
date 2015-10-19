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

#include "ResourceDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/ResourceFile.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CResourceDirectory::CResourceDirectory()
{ }

CResourceDirectory::~CResourceDirectory()
{ }

bool CResourceDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  const std::string pathToUrl(url.Get());
  std::string translatedPath;
  if (!CResourceFile::TranslatePath(url, translatedPath))
    return false;

  if (CDirectory::GetDirectory(translatedPath, items, m_strFileMask, m_flags | DIR_FLAG_GET_HIDDEN))
  { // replace our paths as necessary
    items.SetURL(url);
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      if (StringUtils::StartsWith(item->GetPath(), translatedPath))
        item->SetPath(URIUtils::AddFileToFolder(pathToUrl, item->GetPath().substr(translatedPath.size())));
    }

    return true;
  }

  return false;
}

std::string CResourceDirectory::TranslatePath(const CURL &url)
{
  std::string translatedPath;
  if (!CResourceFile::TranslatePath(url, translatedPath))
    return "";

  return translatedPath;
}
