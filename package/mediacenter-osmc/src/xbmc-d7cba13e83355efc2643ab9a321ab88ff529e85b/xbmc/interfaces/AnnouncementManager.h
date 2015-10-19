#pragma once
/*
 *      Copyright (C) 2005-2014 Team XBMC
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
#include <vector>

#include "IAnnouncer.h"
#include "FileItem.h"
#include "threads/CriticalSection.h"
#include "utils/GlobalsHandling.h"

class CVariant;

namespace ANNOUNCEMENT
{
  class CAnnouncementManager
  {
  public:
    virtual ~CAnnouncementManager();

    static CAnnouncementManager& GetInstance();

    void Deinitialize();

    void AddAnnouncer(IAnnouncer *listener);
    void RemoveAnnouncer(IAnnouncer *listener);

    void Announce(AnnouncementFlag flag, const char *sender, const char *message);
    void Announce(AnnouncementFlag flag, const char *sender, const char *message, CVariant &data);
    void Announce(AnnouncementFlag flag, const char *sender, const char *message, CFileItemPtr item);
    void Announce(AnnouncementFlag flag, const char *sender, const char *message, CFileItemPtr item, CVariant &data);
  private:
    CAnnouncementManager();
    CAnnouncementManager(const CAnnouncementManager&);
    CAnnouncementManager const& operator=(CAnnouncementManager const&);

    CCriticalSection m_critSection;
    std::vector<IAnnouncer *> m_announcers;
  };
}
