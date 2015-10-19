#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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

#include "events/UniqueEvent.h"
#include "media/MediaType.h"

class CMediaLibraryEvent : public CUniqueEvent
{
public:
  CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, EventLevel level = EventLevelInformation);
  CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, EventLevel level = EventLevelInformation);
  CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, EventLevel level = EventLevelInformation);
  CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, const CVariant& executionLabel, EventLevel level = EventLevelInformation);
  virtual ~CMediaLibraryEvent() { }

  virtual const char* GetType() const { return "MediaLibraryEvent"; }
  virtual std::string GetExecutionLabel() const;

  virtual bool CanExecute() const { return !m_mediaType.empty(); }
  virtual bool Execute() const;

protected:
  MediaType m_mediaType;
  std::string m_mediaPath;
};
