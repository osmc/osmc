#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include <map>
#include <string>

#include "ViewState.h"
#include "events/IEvent.h"
#include "guilib/GraphicContext.h"
#include "settings/lib/ISubSettings.h"
#include "settings/lib/Setting.h"
#include "threads/CriticalSection.h"

class TiXmlNode;

class CViewStateSettings : public ISubSettings
{
public:
  static CViewStateSettings& GetInstance();

  virtual bool Load(const TiXmlNode *settings) override;
  virtual bool Save(TiXmlNode *settings) const override;
  virtual void Clear() override;

  const CViewState* Get(const std::string &viewState) const;
  CViewState* Get(const std::string &viewState);

  SettingLevel GetSettingLevel() const { return m_settingLevel; }
  void SetSettingLevel(SettingLevel settingLevel);
  void CycleSettingLevel();
  SettingLevel GetNextSettingLevel() const;

  EventLevel GetEventLevel() const { return m_eventLevel; }
  void SetEventLevel(EventLevel eventLevel);
  void CycleEventLevel();
  EventLevel GetNextEventLevel() const;
  bool ShowHigherEventLevels() const { return m_eventShowHigherLevels; }
  void SetShowHigherEventLevels(bool showHigherEventLevels) { m_eventShowHigherLevels = showHigherEventLevels; }
  void ToggleShowHigherEventLevels() { m_eventShowHigherLevels = !m_eventShowHigherLevels; }

protected:
  CViewStateSettings();
  CViewStateSettings(const CViewStateSettings&);
  CViewStateSettings const& operator=(CViewStateSettings const&);
  virtual ~CViewStateSettings();

private:
  std::map<std::string, CViewState*> m_viewStates;
  SettingLevel m_settingLevel;
  EventLevel m_eventLevel;
  bool m_eventShowHigherLevels;
  CCriticalSection m_critical;

  void AddViewState(const std::string& strTagName, int defaultView = DEFAULT_VIEW_LIST, SortBy defaultSort = SortByLabel);
};
