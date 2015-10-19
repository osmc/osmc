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

#include "utils/UrlOptions.h"
#include <string>

class CFileItemList;

namespace XFILE
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CQueryParams;

    typedef enum _NODE_TYPE
    {
      NODE_TYPE_NONE=0,
      NODE_TYPE_MOVIES_OVERVIEW,
      NODE_TYPE_TVSHOWS_OVERVIEW,
      NODE_TYPE_GENRE,
      NODE_TYPE_ACTOR,
      NODE_TYPE_ROOT,
      NODE_TYPE_OVERVIEW,
      NODE_TYPE_TITLE_MOVIES,
      NODE_TYPE_YEAR,
      NODE_TYPE_DIRECTOR,
      NODE_TYPE_TITLE_TVSHOWS,
      NODE_TYPE_SEASONS,
      NODE_TYPE_EPISODES,
      NODE_TYPE_RECENTLY_ADDED_MOVIES,
      NODE_TYPE_RECENTLY_ADDED_EPISODES,
      NODE_TYPE_STUDIO,
      NODE_TYPE_MUSICVIDEOS_OVERVIEW,
      NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS,
      NODE_TYPE_TITLE_MUSICVIDEOS,
      NODE_TYPE_MUSICVIDEOS_ALBUM,
      NODE_TYPE_SETS,
      NODE_TYPE_COUNTRY,
      NODE_TYPE_TAGS
    } NODE_TYPE;

    typedef struct {
      NODE_TYPE   node;
      std::string id;
      int         label;
    } Node;
    
    class CDirectoryNode
    {
    public:
      static CDirectoryNode* ParseURL(const std::string& strPath);
      static void GetDatabaseInfo(const std::string& strPath, CQueryParams& params);
      virtual ~CDirectoryNode();

      NODE_TYPE GetType() const;

      bool GetChilds(CFileItemList& items);
      virtual NODE_TYPE GetChildType() const;
      virtual std::string GetLocalizedName() const;

      CDirectoryNode* GetParent() const;

      bool CanCache() const;
    protected:
      CDirectoryNode(NODE_TYPE Type, const std::string& strName, CDirectoryNode* pParent);
      static CDirectoryNode* CreateNode(NODE_TYPE Type, const std::string& strName, CDirectoryNode* pParent);

      void AddOptions(const std::string &options);
      void CollectQueryParams(CQueryParams& params) const;

      const std::string& GetName() const;
      int GetID() const;
      void RemoveParent();

      virtual bool GetContent(CFileItemList& items) const;

      std::string BuildPath() const;

    private:
      void AddQueuingFolder(CFileItemList& items) const;

    private:
      NODE_TYPE m_Type;
      std::string m_strName;
      CDirectoryNode* m_pParent;
      CUrlOptions m_options;
    };
  }
}



