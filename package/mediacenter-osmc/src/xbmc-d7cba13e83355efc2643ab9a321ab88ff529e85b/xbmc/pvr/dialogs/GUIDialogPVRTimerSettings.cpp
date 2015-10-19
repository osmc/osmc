/*
 *      Copyright (C) 2012-2014 Team XBMC
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

#include "GUIDialogPVRTimerSettings.h"

#include "addons/include/xbmc_pvr_types.h"
#include "dialogs/GUIDialogNumeric.h"
#include "FileItem.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimerType.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "settings/SettingUtils.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace PVR;

#define SETTING_TMR_TYPE          "timer.type"
#define SETTING_TMR_ACTIVE        "timer.active"
#define SETTING_TMR_NAME          "timer.name"
#define SETTING_TMR_EPGSEARCH     "timer.epgsearch"
#define SETTING_TMR_FULLTEXT      "timer.fulltext"
#define SETTING_TMR_CHANNEL       "timer.channel"
#define SETTING_TMR_START_ANYTIME "timer.startanytime"
#define SETTING_TMR_END_ANYTIME   "timer.endanytime"
#define SETTING_TMR_START_DAY     "timer.startday"
#define SETTING_TMR_END_DAY       "timer.endday"
#define SETTING_TMR_BEGIN         "timer.begin"
#define SETTING_TMR_END           "timer.end"
#define SETTING_TMR_WEEKDAYS      "timer.weekdays"
#define SETTING_TMR_FIRST_DAY     "timer.firstday"
#define SETTING_TMR_NEW_EPISODES  "timer.newepisodes"
#define SETTING_TMR_BEGIN_PRE     "timer.startmargin"
#define SETTING_TMR_END_POST      "timer.endmargin"
#define SETTING_TMR_PRIORITY      "timer.priority"
#define SETTING_TMR_LIFETIME      "timer.lifetime"
#define SETTING_TMR_MAX_REC       "timer.maxrecordings"
#define SETTING_TMR_DIR           "timer.directory"
#define SETTING_TMR_REC_GROUP     "timer.recgroup"

#define TYPE_DEP_VISIBI_COND_ID_POSTFIX     "visibi.typedep"
#define TYPE_DEP_ENABLE_COND_ID_POSTFIX     "enable.typedep"
#define CHANNEL_DEP_VISIBI_COND_ID_POSTFIX  "visibi.channeldep"
#define START_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX  "visibi.startanytimedep"
#define END_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX    "visibi.endanytimedep"

#define ENTRY_ANY_CHANNEL (-1)

CGUIDialogPVRTimerSettings::CGUIDialogPVRTimerSettings() :
  CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_TIMER_SETTING, "DialogPVRTimerSettings.xml"),
  m_bIsRadio(false),
  m_bIsNewTimer(true),
  m_bTimerActive(false),
  m_bFullTextEpgSearch(true),
  m_bStartAnyTime(false),
  m_bEndAnyTime(false),
  m_iWeekdays(PVR_WEEKDAY_NONE),
  m_iPreventDupEpisodes(0),
  m_iMarginStart(0),
  m_iMarginEnd(0),
  m_iPriority(0),
  m_iLifetime(0),
  m_iMaxRecordings(0),
  m_iRecordingGroup(0)
{
  m_loadType = LOAD_EVERY_TIME;
}

CGUIDialogPVRTimerSettings::~CGUIDialogPVRTimerSettings()
{
}

void CGUIDialogPVRTimerSettings::SetTimer(CFileItem *item)
{
  if (item == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::SetTimer - No item");
    return;
  }

  m_timerInfoTag = item->GetPVRTimerInfoTag();

  if (!m_timerInfoTag)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::SetTimer - No timer info tag");
    return;
  }

  // Copy data we need from tag. Do not modify the tag itself until Save()!
  m_timerType     = m_timerInfoTag->GetTimerType();
  m_bIsRadio      = m_timerInfoTag->m_bIsRadio;
  m_bIsNewTimer   = m_timerInfoTag->m_iClientIndex == PVR_TIMER_NO_CLIENT_INDEX;
  m_bTimerActive  = m_bIsNewTimer || m_timerInfoTag->IsActive();
  m_bStartAnyTime = m_bIsNewTimer || m_timerInfoTag->m_bStartAnyTime;
  m_bEndAnyTime   = m_bIsNewTimer || m_timerInfoTag->m_bEndAnyTime;
  m_strTitle      = m_timerInfoTag->m_strTitle;

  m_startLocalTime    = m_timerInfoTag->StartAsLocalTime();
  m_endLocalTime      = m_timerInfoTag->EndAsLocalTime();
  m_timerStartTimeStr = m_startLocalTime.GetAsLocalizedTime("", false);
  m_timerEndTimeStr   = m_endLocalTime.GetAsLocalizedTime("", false);
  m_firstDayLocalTime = m_timerInfoTag->FirstDayAsLocalTime();

  m_strEpgSearchString = m_timerInfoTag->m_strEpgSearchString;

  if (m_bIsNewTimer && m_strEpgSearchString.empty() && (m_strTitle != g_localizeStrings.Get(19056)))
    m_strEpgSearchString = m_strTitle;

  m_bFullTextEpgSearch  = m_timerInfoTag->m_bFullTextEpgSearch;
  m_iWeekdays           = m_timerInfoTag->m_iWeekdays;

  if (m_bIsNewTimer && (m_iWeekdays == PVR_WEEKDAY_NONE))
    m_iWeekdays = PVR_WEEKDAY_ALLDAYS;

  m_iPreventDupEpisodes = m_timerInfoTag->m_iPreventDupEpisodes;
  m_iMarginStart        = m_timerInfoTag->m_iMarginStart;
  m_iMarginEnd          = m_timerInfoTag->m_iMarginEnd;
  m_iPriority           = m_timerInfoTag->m_iPriority;
  m_iLifetime           = m_timerInfoTag->m_iLifetime;
  m_iMaxRecordings      = m_timerInfoTag->m_iMaxRecordings;
  m_strDirectory        = m_timerInfoTag->m_strDirectory;
  m_iRecordingGroup     = m_timerInfoTag->m_iRecordingGroup;

  InitializeChannelsList();
  InitializeTypesList();

  // Channel
  m_channel = ChannelDescriptor();

  if (m_timerInfoTag->m_iClientChannelUid == PVR_INVALID_CHANNEL_UID)
  {
    bool bChannelSet(false);
    if (m_timerType && m_timerType->IsRepeatingEpgBased())
    {
      // Select "Any channel"
      const auto it = m_channelEntries.find(ENTRY_ANY_CHANNEL);
      if (it != m_channelEntries.end())
      {
        m_channel = it->second;
        bChannelSet = true;
      }
    }
    else if (m_bIsNewTimer)
    {
      // Select first real (not "Any channel") entry.
      for (const auto &channel : m_channelEntries)
      {
        if (channel.second.channelUid != PVR_INVALID_CHANNEL_UID)
        {
          m_channel = channel.second;
          bChannelSet = true;
          break;
        }
      }
    }

    if (!bChannelSet)
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::SetTimer - Unable to map PVR_INVALID_CHANNEL_UID to channel entry!");
  }
  else
  {
    // Find matching channel entry
    bool bChannelSet(false);
    for (const auto &channel : m_channelEntries)
    {
      if ((channel.second.channelUid == m_timerInfoTag->m_iClientChannelUid) &&
          (channel.second.clientId == m_timerInfoTag->m_iClientId))
      {
        m_channel = channel.second;
        bChannelSet = true;
        break;
      }
    }

    if (!bChannelSet)
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::SetTimer - Unable to map channel uid to channel entry!");
  }


  if (!m_timerType)
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::SetTimer - No timer type!");
}

void CGUIDialogPVRTimerSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetButtonLabels();
}

void CGUIDialogPVRTimerSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("pvrtimersettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::InitializeSettings - Unable to add settings category");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::InitializeSettings - Unable to add settings group");
    return;
  }

  CSetting *setting = NULL;

  // Timer type
  setting = AddList(group, SETTING_TMR_TYPE, 803, 0, 0, TypesFiller, 803);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_TYPE);

  // Timer enabled/disabled
  setting = AddToggle(group, SETTING_TMR_ACTIVE, 19074, 0, m_bTimerActive);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_ACTIVE);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_ACTIVE);

  // Name
  setting = AddEdit(group, SETTING_TMR_NAME, 19075, 0, m_strTitle, true, false, 19097);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_NAME);

  // epg search string (only for epg-based repeating timers)
  setting = AddEdit(group, SETTING_TMR_EPGSEARCH, 804, 0, m_strEpgSearchString, true, false, 805);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_EPGSEARCH);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_EPGSEARCH);

  // epg fulltext search (only for epg-based repeating timers)
  setting = AddToggle(group, SETTING_TMR_FULLTEXT, 806, 0, m_bFullTextEpgSearch);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_FULLTEXT);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_FULLTEXT);

  // Channel
  setting = AddList(group, SETTING_TMR_CHANNEL, 19078, 0, 0, ChannelsFiller, 19078);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_CHANNEL);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_CHANNEL);

  // Days of week (only for repeating timers)
  std::vector<int> weekdaysPreselect;
  if (m_iWeekdays & PVR_WEEKDAY_MONDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_MONDAY);
  if (m_iWeekdays & PVR_WEEKDAY_TUESDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_TUESDAY);
  if (m_iWeekdays & PVR_WEEKDAY_WEDNESDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_WEDNESDAY);
  if (m_iWeekdays & PVR_WEEKDAY_THURSDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_THURSDAY);
  if (m_iWeekdays & PVR_WEEKDAY_FRIDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_FRIDAY);
  if (m_iWeekdays & PVR_WEEKDAY_SATURDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_SATURDAY);
  if (m_iWeekdays & PVR_WEEKDAY_SUNDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_SUNDAY);

  setting = AddList(group, SETTING_TMR_WEEKDAYS, 19079, 0, weekdaysPreselect, WeekdaysFiller, 19079, 1, -1, true, -1, WeekdaysValueFormatter);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_WEEKDAYS);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_WEEKDAYS);

  // "Start any time" (only for repeating timers)
  setting = AddToggle(group, SETTING_TMR_START_ANYTIME, 810, 0, m_bStartAnyTime);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_START_ANYTIME);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_START_ANYTIME);

  // Start day (day + month + year only, no hours, minutes)
  setting = AddSpinner(group, SETTING_TMR_START_DAY, 19128, 0, GetDateAsIndex(m_startLocalTime), DaysFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_START_DAY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_START_DAY);
  AddStartAnytimeDependentVisibilityCondition(setting, SETTING_TMR_START_DAY);

  // Start time (hours + minutes only, no day, month, year)
  setting = AddButton(group, SETTING_TMR_BEGIN, 19126, 0);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_BEGIN);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_BEGIN);
  AddStartAnytimeDependentVisibilityCondition(setting, SETTING_TMR_BEGIN);

  // "End any time" (only for repeating timers)
  setting = AddToggle(group, SETTING_TMR_END_ANYTIME, 817, 0, m_bEndAnyTime);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END_ANYTIME);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END_ANYTIME);

  // End day (day + month + year only, no hours, minutes)
  setting = AddSpinner(group, SETTING_TMR_END_DAY, 19129, 0, GetDateAsIndex(m_endLocalTime), DaysFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END_DAY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END_DAY);
  AddEndAnytimeDependentVisibilityCondition(setting, SETTING_TMR_END_DAY);

  // End time (hours + minutes only, no day, month, year)
  setting = AddButton(group, SETTING_TMR_END, 19127, 0);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END);
  AddEndAnytimeDependentVisibilityCondition(setting, SETTING_TMR_END);

  // First day (only for repeating timers)
  setting = AddSpinner(group, SETTING_TMR_FIRST_DAY, 19084, 0, GetDateAsIndex(m_firstDayLocalTime), DaysFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_FIRST_DAY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_FIRST_DAY);

  // "Prevent duplicate episodes" (only for repeating timers)
  setting = AddList(group, SETTING_TMR_NEW_EPISODES, 812, 0, m_iPreventDupEpisodes, DupEpisodesFiller, 812);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_NEW_EPISODES);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_NEW_EPISODES);

  // Pre and post record time
  setting = AddSpinner(group, SETTING_TMR_BEGIN_PRE, 813, 0, m_iMarginStart, 0, 1, 60, 14044);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_BEGIN_PRE);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_BEGIN_PRE);

  setting = AddSpinner(group, SETTING_TMR_END_POST,  814, 0, m_iMarginEnd,   0, 1, 60, 14044);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END_POST);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END_POST);

  // Priority
  setting = AddList(group, SETTING_TMR_PRIORITY, 19082, 0, m_iPriority, PrioritiesFiller, 19082);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_PRIORITY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_PRIORITY);

  // Lifetime
  setting = AddList(group, SETTING_TMR_LIFETIME, 19083, 0, m_iLifetime, LifetimesFiller, 19083);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_LIFETIME);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_LIFETIME);

  // MaxRecordings
  setting = AddList(group, SETTING_TMR_MAX_REC, 818, 0, m_iMaxRecordings, MaxRecordingsFiller, 818);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_MAX_REC);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_MAX_REC);

  // Recording folder
  setting = AddEdit(group, SETTING_TMR_DIR, 19076, 0, m_strDirectory, true, false, 19104);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_DIR);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_DIR);

  // Recording group
  setting = AddList(group, SETTING_TMR_REC_GROUP, 811, 0, m_iRecordingGroup, RecordingGroupFiller, 811);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_REC_GROUP);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_REC_GROUP);
}

int CGUIDialogPVRTimerSettings::GetWeekdaysFromSetting(const CSetting *setting)
{
  const CSettingList *settingList = static_cast<const CSettingList*>(setting);
  if (settingList->GetElementType() != SettingTypeInteger)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::GetWeekdaysFromSetting - wrong weekdays element type");
    return 0;
  }
  int weekdays = 0;
  std::vector<CVariant> list = CSettingUtils::GetList(settingList);
  for (const auto &value : list)
  {
    if (!value.isInteger())
    {
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::GetWeekdaysFromSetting - wrong weekdays value type");
      return 0;
    }
    weekdays += static_cast<int>(value.asInteger());
  }

  return weekdays;
}

void CGUIDialogPVRTimerSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingChanged - No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == SETTING_TMR_TYPE)
  {
    int idx = static_cast<const CSettingInt*>(setting)->GetValue();
    const auto it = m_typeEntries.find(idx);
    if (it != m_typeEntries.end())
    {
      m_timerType = it->second;

      if (m_timerType->IsRepeating() && (m_iWeekdays == PVR_WEEKDAY_ALLDAYS))
        SetButtonLabels(); // update "Any day" vs. "Every day"
    }
    else
    {
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingChanged - Unable to get 'type' value");
    }
  }
  else if (settingId == SETTING_TMR_ACTIVE)
  {
    m_bTimerActive = static_cast<const CSettingBool*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_NAME)
  {
    m_strTitle = static_cast<const CSettingString*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_EPGSEARCH)
  {
    m_strEpgSearchString = static_cast<const CSettingString*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_FULLTEXT)
  {
    m_bFullTextEpgSearch = static_cast<const CSettingBool*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_CHANNEL)
  {
    int idx = static_cast<const CSettingInt*>(setting)->GetValue();
    const auto it = m_channelEntries.find(idx);
    if (it != m_channelEntries.end())
    {
      m_channel = it->second;
    }
    else
    {
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingChanged - Unable to get 'type' value");
    }
  }
  else if (settingId == SETTING_TMR_WEEKDAYS)
  {
    m_iWeekdays = GetWeekdaysFromSetting(setting);
  }
  else if (settingId == SETTING_TMR_START_ANYTIME)
  {
    m_bStartAnyTime = static_cast<const CSettingBool*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_END_ANYTIME)
  {
    m_bEndAnyTime = static_cast<const CSettingBool*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_START_DAY)
  {
    SetDateFromIndex(m_startLocalTime, static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SETTING_TMR_END_DAY)
  {
    SetDateFromIndex(m_endLocalTime, static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SETTING_TMR_FIRST_DAY)
  {
    SetDateFromIndex(m_firstDayLocalTime, static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SETTING_TMR_NEW_EPISODES)
  {
    m_iPreventDupEpisodes = static_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_BEGIN_PRE)
  {
    m_iMarginStart = static_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_END_POST)
  {
    m_iMarginEnd = static_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_PRIORITY)
  {
    m_iPriority = static_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_LIFETIME)
  {
    m_iLifetime = static_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_MAX_REC)
  {
    m_iMaxRecordings = static_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_DIR)
  {
    m_strDirectory = static_cast<const CSettingString*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_REC_GROUP)
  {
    m_iRecordingGroup = static_cast<const CSettingInt*>(setting)->GetValue();
  }
}

void CGUIDialogPVRTimerSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingAction - No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_TMR_BEGIN)
  {
    SYSTEMTIME timerStartTime;
    m_startLocalTime.GetAsSystemTime(timerStartTime);
    if (CGUIDialogNumeric::ShowAndGetTime(timerStartTime, g_localizeStrings.Get(14066)))
    {
      SetTimeFromSystemTime(m_startLocalTime, timerStartTime);
      m_timerStartTimeStr = m_startLocalTime.GetAsLocalizedTime("", false);
      SetButtonLabels();
    }
  }
  else if (settingId == SETTING_TMR_END)
  {
    SYSTEMTIME timerEndTime;
    m_endLocalTime.GetAsSystemTime(timerEndTime);
    if (CGUIDialogNumeric::ShowAndGetTime(timerEndTime, g_localizeStrings.Get(14066)))
    {
      SetTimeFromSystemTime(m_endLocalTime, timerEndTime);
      m_timerEndTimeStr = m_endLocalTime.GetAsLocalizedTime("", false);
      SetButtonLabels();
    }
  }
}

void CGUIDialogPVRTimerSettings::Save()
{
  // Timer type
  m_timerInfoTag->SetTimerType(m_timerType);

  // Timer active/inactive
  m_timerInfoTag->m_state = m_bTimerActive ? PVR_TIMER_STATE_SCHEDULED : PVR_TIMER_STATE_DISABLED;

  // Name
  m_timerInfoTag->m_strTitle = m_strTitle;

  // epg search string (only for epg-based repeating timers)
  m_timerInfoTag->m_strEpgSearchString = m_strEpgSearchString;

  // epg fulltext search, instead of just title match. (only for epg-based repeating timers)
  m_timerInfoTag->m_bFullTextEpgSearch = m_bFullTextEpgSearch;

  // Channel
  CPVRChannelPtr channel(g_PVRChannelGroups->GetByUniqueID(m_channel.channelUid, m_channel.clientId));
  if (channel)
  {
    m_timerInfoTag->m_iClientChannelUid = channel->UniqueID();
    m_timerInfoTag->m_iClientId         = channel->ClientID();
    m_timerInfoTag->m_bIsRadio          = channel->IsRadio();
    m_timerInfoTag->m_iChannelNumber    = channel->ChannelNumber();

    m_timerInfoTag->UpdateChannel();
  }
  else
  {
    if (m_timerType->IsOnetime() || m_timerType->IsManual())
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::Save - No channel");

    m_timerInfoTag->m_iClientChannelUid = m_channel.channelUid;
    m_timerInfoTag->m_iClientId         = m_timerType->GetClientId();
  }

  if (m_timerType->SupportsStartAnyTime() && m_timerType->IsEpgBased()) // Start anytime toggle is displayed
    m_timerInfoTag->m_bStartAnyTime = m_bStartAnyTime;
  else
    m_bStartAnyTime = false; // Assume start time change needs checking for
  if (m_timerType->SupportsEndAnyTime() && m_timerType->IsEpgBased()) // End anytime toggle is displayed
    m_timerInfoTag->m_bEndAnyTime = m_bEndAnyTime;
  else
    m_bEndAnyTime = false; // Assume end time change needs checking for
  // Begin and end time
  const CDateTime now(CDateTime::GetCurrentDateTime());
  if (!m_bStartAnyTime && !m_bEndAnyTime)
  {
    if (m_timerType->SupportsStartTime() &&    // has start clock entry
        m_timerType->SupportsEndTime() &&      // and end clock entry
        m_timerType->IsRepeating())            // but no associated start/end day spinners
    {
      if (m_endLocalTime < m_startLocalTime)   // And the end clock is earlier than the start clock
      {
        CLog::Log(LOGDEBUG, "CGUIDialogPVRTimerSettings::Save - End before start, adding a day.");
        m_endLocalTime += CDateTimeSpan(1, 0, 0, 0);
        if (m_endLocalTime < m_startLocalTime)
        {
          CLog::Log(LOGWARNING, "CGUIDialogPVRTimerSettings::Save - End before start. Setting end time to start time.");
          m_endLocalTime = m_startLocalTime;
        }
      }
      else if (m_endLocalTime > (m_startLocalTime + CDateTimeSpan(1, 0, 0, 0))) // Or the duration is more than a day
      {
        CLog::Log(LOGDEBUG, "CGUIDialogPVRTimerSettings::Save - End > 1 day after start, removing a day.");
        m_endLocalTime -= CDateTimeSpan(1, 0, 0, 0);
        if (m_endLocalTime > (m_startLocalTime + CDateTimeSpan(1, 0, 0, 0)))
        {
          CLog::Log(LOGWARNING, "CGUIDialogPVRTimerSettings::Save - End > 1 day after start. Setting end time to start time.");
          m_endLocalTime = m_startLocalTime;
        }
      }
    }
    else if (m_endLocalTime < m_startLocalTime) // Assume the user knows what they are doing, but log a warning just in case
    {
      CLog::Log(LOGWARNING, "CGUIDialogPVRTimerSettings::Save - Specified recording end time < start time: expect errors!");
    }
    m_timerInfoTag->SetStartFromLocalTime(m_startLocalTime);
    m_timerInfoTag->SetEndFromLocalTime(m_endLocalTime);
  }
  else if (!m_bStartAnyTime)
    m_timerInfoTag->SetStartFromLocalTime(m_startLocalTime);
  else if (!m_bEndAnyTime)
    m_timerInfoTag->SetEndFromLocalTime(m_endLocalTime);

  // Days of week (only for repeating timers)
  if (m_timerType->IsRepeating())
    m_timerInfoTag->m_iWeekdays = m_iWeekdays;
  else
    m_timerInfoTag->m_iWeekdays = PVR_WEEKDAY_NONE;

  // First day (only for repeating timers)
  m_timerInfoTag->SetFirstDayFromLocalTime(m_firstDayLocalTime);

  // "New episodes only" (only for repeating timers)
  m_timerInfoTag->m_iPreventDupEpisodes = m_iPreventDupEpisodes;

  // Pre and post record time
  m_timerInfoTag->m_iMarginStart = m_iMarginStart;
  m_timerInfoTag->m_iMarginEnd   = m_iMarginEnd;

  // Priority
  m_timerInfoTag->m_iPriority = m_iPriority;

  // Lifetime
  m_timerInfoTag->m_iLifetime = m_iLifetime;

  // MaxRecordings
  m_timerInfoTag->m_iMaxRecordings = m_iMaxRecordings;

  // Recording folder
  m_timerInfoTag->m_strDirectory = m_strDirectory;

  // Recording group
  m_timerInfoTag->m_iRecordingGroup = m_iRecordingGroup;

  // Set the timer's title to the channel name if it's empty or 'New Timer'
  if (channel && (m_strTitle.empty() || m_strTitle == g_localizeStrings.Get(19056)))
    m_timerInfoTag->m_strTitle = channel->ChannelName();

  // Update summary
  m_timerInfoTag->UpdateSummary();
}

void CGUIDialogPVRTimerSettings::SetButtonLabels()
{
  // timer start time
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_TMR_BEGIN);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    SET_CONTROL_LABEL2(settingControl->GetID(), m_timerStartTimeStr);
  }

  // timer end time
  settingControl = GetSettingControl(SETTING_TMR_END);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    SET_CONTROL_LABEL2(settingControl->GetID(), m_timerEndTimeStr);
  }
}

void CGUIDialogPVRTimerSettings::AddCondition(
  CSetting *setting, const std::string &identifier, SettingConditionCheck condition,
  SettingDependencyType depType, const std::string &settingId)
{
  m_settingsManager->AddCondition(identifier, condition, this);
  CSettingDependency dep(depType, m_settingsManager);
  dep.And()->Add(
    CSettingDependencyConditionPtr(
      new CSettingDependencyCondition(identifier, "true", settingId, false, m_settingsManager)));
  SettingDependencies deps(setting->GetDependencies());
  deps.push_back(dep);
  setting->SetDependencies(deps);
}

int CGUIDialogPVRTimerSettings::GetDateAsIndex(const CDateTime &datetime)
{
  const CDateTime date(datetime.GetYear(), datetime.GetMonth(), datetime.GetDay(), 0, 0, 0);
  time_t t(0);
  date.GetAsTime(t);
  return static_cast<int>(t);
}

void CGUIDialogPVRTimerSettings::SetDateFromIndex(CDateTime &datetime, int date)
{
  const CDateTime newDate(static_cast<time_t>(date));
  datetime.SetDateTime(
    newDate.GetYear(), newDate.GetMonth(), newDate.GetDay(),
    datetime.GetHour(), datetime.GetMinute(), datetime.GetSecond());
}

void CGUIDialogPVRTimerSettings::SetTimeFromSystemTime(CDateTime &datetime, const SYSTEMTIME &time)
{
  const CDateTime newTime(time);
  datetime.SetDateTime(
    datetime.GetYear(), datetime.GetMonth(), datetime.GetDay(),
    newTime.GetHour(), newTime.GetMinute(), newTime.GetSecond());
}

void CGUIDialogPVRTimerSettings::InitializeTypesList()
{
  m_typeEntries.clear();

  int idx = 0;
  const std::vector<CPVRTimerTypePtr> types(CPVRTimerType::GetAllTypes());
  for (const auto &type : types)
  {
    if (m_bIsNewTimer)
    {
      // Type definition prohibits created of new instances.
      // But the dialog can act as a viewer for these types. (m_bIsNewTimer is false in this case)
      if (type->ForbidsNewInstances())
        continue;

      // Read-only timers cannot be created using this dialog.
      // But the dialog can act as a viewer for read-only types (m_bIsNewTimer is false in this case)
      if (type->IsReadOnly())
        continue;

      // Drop TimerTypes that require EPGInfo, if none is populated
      if (type->RequiresEpgTagOnCreate() && !m_timerInfoTag->HasEpgInfoTag())
        continue;

      // Drop TimerTypes without 'Series' EPG attributes if none are set
      if (type->RequiresEpgSeriesOnCreate() && !m_timerInfoTag->HasSeriesEpgInfoTag())
        continue;

      // Drop TimerTypes that forbid EPGInfo, if it is populated
      if (type->ForbidsEpgTagOnCreate() && m_timerInfoTag->HasEpgInfoTag())
        continue;
    }

    m_typeEntries.insert(std::make_pair(idx++, type));
  }
}

void CGUIDialogPVRTimerSettings::InitializeChannelsList()
{
  m_channelEntries.clear();

  CFileItemList channelsList;
  g_PVRChannelGroups->GetGroupAll(m_bIsRadio)->GetMembers(channelsList);

  for (int i = 0; i < channelsList.Size(); ++i)
  {
    const CPVRChannelPtr channel(channelsList[i]->GetPVRChannelInfoTag());
    std::string channelDescription(
      StringUtils::Format("%i %s", channel->ChannelNumber(), channel->ChannelName().c_str()));
    m_channelEntries.insert(
      std::make_pair(i, ChannelDescriptor(channel->UniqueID(), channel->ClientID(), channelDescription)));
  }

  // Add special "any channel" entry (used for epg-based repeating timers).
  m_channelEntries.insert(
    std::make_pair(
      ENTRY_ANY_CHANNEL, ChannelDescriptor(PVR_INVALID_CHANNEL_UID, 0, g_localizeStrings.Get(809))));
}

void CGUIDialogPVRTimerSettings::TypesFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    current = 0;

    bool foundCurrent(false);
    for (const auto &typeEntry : pThis->m_typeEntries)
    {
      list.push_back(std::make_pair(typeEntry.second->GetDescription(), typeEntry.first));

      if (!foundCurrent && (*(pThis->m_timerType) == *(typeEntry.second)))
      {
        current = typeEntry.first;
        foundCurrent = true;
      }
    }
  }
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypesFiller - No dialog");
}

void CGUIDialogPVRTimerSettings::ChannelsFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    current = 0;

    bool foundCurrent(false);
    for (const auto &channelEntry : pThis->m_channelEntries)
    {
      if (channelEntry.first == ENTRY_ANY_CHANNEL)
      {
        // For repeating epg-based timers only, add an "any channel" entry.
        if (pThis->m_timerType->IsRepeatingEpgBased())
          list.push_back(std::make_pair(channelEntry.second.description, channelEntry.first));
        else
          continue;
      }
      else
      {
        // Only include channels supplied by the currently active PVR client.
        if (channelEntry.second.clientId == pThis->m_timerType->GetClientId())
          list.push_back(std::make_pair(channelEntry.second.description, channelEntry.first));
      }

      if (!foundCurrent && (pThis->m_channel == channelEntry.second))
      {
        current = channelEntry.first;
        foundCurrent = true;
      }
    }
  }
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::ChannelsFiller - No dialog");
}

void CGUIDialogPVRTimerSettings::DaysFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    current = 0;

    // Data range: "today" until "yesterday next year"
    const CDateTime now(CDateTime::GetCurrentDateTime());
    CDateTime time(now.GetYear(), now.GetMonth(), now.GetDay(), 0, 0, 0);
    const CDateTime yesterdayPlusOneYear(
      time.GetYear() + 1, time.GetMonth(), time.GetDay() - 1, time.GetHour(), time.GetMinute(), time.GetSecond());

    CDateTime oldCDateTime;
    if (setting->GetId() == SETTING_TMR_FIRST_DAY)
      oldCDateTime = pThis->m_timerInfoTag->FirstDayAsLocalTime();
    else if (setting->GetId() == SETTING_TMR_START_DAY)
      oldCDateTime = pThis->m_timerInfoTag->StartAsLocalTime();
    else
      oldCDateTime = pThis->m_timerInfoTag->EndAsLocalTime();
    const CDateTime oldCDate(oldCDateTime.GetYear(), oldCDateTime.GetMonth(), oldCDateTime.GetDay(), 0, 0, 0);

    if ((oldCDate < time) || (oldCDate > yesterdayPlusOneYear))
      list.push_back(std::make_pair(oldCDate.GetAsLocalizedDate(true /*long date*/), GetDateAsIndex(oldCDate)));

    while (time <= yesterdayPlusOneYear)
    {
      list.push_back(std::make_pair(time.GetAsLocalizedDate(true /*long date*/), GetDateAsIndex(time)));
      time += CDateTimeSpan(1, 0, 0, 0);
    }

    if (setting->GetId() == SETTING_TMR_FIRST_DAY)
      current = GetDateAsIndex(pThis->m_firstDayLocalTime);
    else if (setting->GetId() == SETTING_TMR_START_DAY)
      current = GetDateAsIndex(pThis->m_startLocalTime);
    else
      current = GetDateAsIndex(pThis->m_endLocalTime);
  }
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::DaysFiller - No dialog");
}

void CGUIDialogPVRTimerSettings::DupEpisodesFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    pThis->m_timerType->GetPreventDuplicateEpisodesValues(list);
    current = pThis->m_iPreventDupEpisodes;
  }
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::DupEpisodesFiller - No dialog");
}

void CGUIDialogPVRTimerSettings::WeekdaysFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    list.push_back(std::make_pair(g_localizeStrings.Get(831), PVR_WEEKDAY_MONDAY));    // "Mondays"
    list.push_back(std::make_pair(g_localizeStrings.Get(832), PVR_WEEKDAY_TUESDAY));   // "Tuesdays"
    list.push_back(std::make_pair(g_localizeStrings.Get(833), PVR_WEEKDAY_WEDNESDAY)); // "Wednesdays"
    list.push_back(std::make_pair(g_localizeStrings.Get(834), PVR_WEEKDAY_THURSDAY));  // "Thursdays"
    list.push_back(std::make_pair(g_localizeStrings.Get(835), PVR_WEEKDAY_FRIDAY));    // "Fridays"
    list.push_back(std::make_pair(g_localizeStrings.Get(836), PVR_WEEKDAY_SATURDAY));  // "Saturdays"
    list.push_back(std::make_pair(g_localizeStrings.Get(837), PVR_WEEKDAY_SUNDAY));    // "Sundays"

    current = pThis->m_iWeekdays;
  }
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::WeekdaysFiller - No dialog");
}

void CGUIDialogPVRTimerSettings::PrioritiesFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    pThis->m_timerType->GetPriorityValues(list);
    current = pThis->m_iPriority;
  }
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::PrioritiesFiller - No dialog");
}

void CGUIDialogPVRTimerSettings::LifetimesFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    pThis->m_timerType->GetLifetimeValues(list);
    current = pThis->m_iLifetime;
  }
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::LifetimesFiller - No dialog");
}

void CGUIDialogPVRTimerSettings::MaxRecordingsFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    pThis->m_timerType->GetMaxRecordingsValues(list);
    current = pThis->m_iMaxRecordings;
  }
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::MaxRecordingsFiller - No dialog");
}

void CGUIDialogPVRTimerSettings::RecordingGroupFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    pThis->m_timerType->GetRecordingGroupValues(list);
    current = pThis->m_iRecordingGroup;
  }
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::RecordingGroupFiller - No dialog");
}

std::string CGUIDialogPVRTimerSettings::WeekdaysValueFormatter(const CSetting *setting)
{
  return CPVRTimerInfoTag::GetWeekdaysString(GetWeekdaysFromSetting(setting), true, true);
}

void CGUIDialogPVRTimerSettings::AddTypeDependentEnableCondition(CSetting *setting, const std::string &identifier)
{
  // Enable setting depending on read-only attribute of the selected timer type
  std::string id(identifier);
  id.append(TYPE_DEP_ENABLE_COND_ID_POSTFIX);
  AddCondition(setting, id, TypeReadOnlyCondition, SettingDependencyTypeEnable, SETTING_TMR_TYPE);
}

bool CGUIDialogPVRTimerSettings::TypeReadOnlyCondition(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeReadOnlyCondition - No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  std::string cond(condition);
  cond.erase(cond.find(TYPE_DEP_ENABLE_COND_ID_POSTFIX));

  // For existing timers, disable type selector (view/edit of existing timer).
  if (!pThis->m_bIsNewTimer)
  {
    if (cond == SETTING_TMR_TYPE)
      return false;
  }

  // If only one type is available, disable type selector.
  if (pThis->m_typeEntries.size() == 1)
  {
    if (cond == SETTING_TMR_TYPE)
      return false;
  }

  // For existing one time epg-based timers, disable editing of epg-filled data.
  if (!pThis->m_bIsNewTimer && pThis->m_timerType->IsOnetimeEpgBased())
  {
    if ((cond == SETTING_TMR_NAME)      ||
        (cond == SETTING_TMR_CHANNEL)   ||
        (cond == SETTING_TMR_START_DAY) ||
        (cond == SETTING_TMR_END_DAY)   ||
        (cond == SETTING_TMR_BEGIN)     ||
        (cond == SETTING_TMR_END))
      return false;
  }

  /* Always enable enable/disable, if supported by the timer type. */
  if (pThis->m_timerType->SupportsEnableDisable())
  {
    if (cond == SETTING_TMR_ACTIVE)
      return true;
  }

  // Let the PVR client decide...
  int idx = static_cast<const CSettingInt*>(setting)->GetValue();
  const auto entry = pThis->m_typeEntries.find(idx);
  if (entry != pThis->m_typeEntries.end())
    return !entry->second->IsReadOnly();
  else
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeReadOnlyCondition - No type entry");

  return false;
}

void CGUIDialogPVRTimerSettings::AddTypeDependentVisibilityCondition(CSetting *setting, const std::string &identifier)
{
  // Show or hide setting depending on attributes of the selected timer type
  std::string id(identifier);
  id.append(TYPE_DEP_VISIBI_COND_ID_POSTFIX);
  AddCondition(setting, id, TypeSupportsCondition, SettingDependencyTypeVisible, SETTING_TMR_TYPE);
}

bool CGUIDialogPVRTimerSettings::TypeSupportsCondition(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeSupportsCondition - No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  int idx = static_cast<const CSettingInt*>(setting)->GetValue();
  const auto entry = pThis->m_typeEntries.find(idx);
  if (entry != pThis->m_typeEntries.end())
  {
    std::string cond(condition);
    cond.erase(cond.find(TYPE_DEP_VISIBI_COND_ID_POSTFIX));

    if (cond == SETTING_TMR_EPGSEARCH)
      return entry->second->SupportsEpgTitleMatch() || entry->second->SupportsEpgFulltextMatch();
    else if (cond == SETTING_TMR_FULLTEXT)
      return entry->second->SupportsEpgFulltextMatch();
    else if (cond == SETTING_TMR_ACTIVE)
      return entry->second->SupportsEnableDisable();
    else if (cond == SETTING_TMR_CHANNEL)
      return entry->second->SupportsChannels();
    else if (cond == SETTING_TMR_START_ANYTIME)
      return entry->second->SupportsStartAnyTime() && entry->second->IsEpgBased();
    else if (cond == SETTING_TMR_END_ANYTIME)
      return entry->second->SupportsEndAnyTime() && entry->second->IsEpgBased();
    else if (cond == SETTING_TMR_START_DAY)
      return entry->second->SupportsStartTime() && entry->second->IsOnetime();
    else if (cond == SETTING_TMR_END_DAY)
      return entry->second->SupportsEndTime() && entry->second->IsOnetime();
    else if (cond == SETTING_TMR_BEGIN)
      return entry->second->SupportsStartTime();
    else if (cond == SETTING_TMR_END)
      return entry->second->SupportsEndTime();
    else if (cond == SETTING_TMR_WEEKDAYS)
      return entry->second->SupportsWeekdays();
    else if (cond == SETTING_TMR_FIRST_DAY)
      return entry->second->SupportsFirstDay();
    else if (cond == SETTING_TMR_NEW_EPISODES)
      return entry->second->SupportsRecordOnlyNewEpisodes();
    else if ((cond == SETTING_TMR_BEGIN_PRE) ||
             (cond == SETTING_TMR_END_POST))
      return entry->second->SupportsStartEndMargin();
    else if (cond == SETTING_TMR_PRIORITY)
      return entry->second->SupportsPriority();
    else if (cond == SETTING_TMR_LIFETIME)
      return entry->second->SupportsLifetime();
    else if (cond == SETTING_TMR_MAX_REC)
      return entry->second->SupportsMaxRecordings();
    else if (cond == SETTING_TMR_DIR)
      return entry->second->SupportsRecordingFolders();
    else if (cond == SETTING_TMR_REC_GROUP)
      return entry->second->SupportsRecordingGroup();
    else
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeSupportsCondition - Unknown condition");
  }
  else
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeSupportsCondition - No type entry");
  }
  return false;
}

void CGUIDialogPVRTimerSettings::AddStartAnytimeDependentVisibilityCondition(CSetting *setting, const std::string &identifier)
{
  // Show or hide setting depending on value of setting "any time"
  std::string id(identifier);
  id.append(START_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX);
  AddCondition(setting, id, StartAnytimeSetCondition, SettingDependencyTypeVisible, SETTING_TMR_START_ANYTIME);
}

bool CGUIDialogPVRTimerSettings::StartAnytimeSetCondition(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::AnytimeSetCondition - No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  // "any time" setting is only relevant for epg-based timers.
  if (!pThis->m_timerType->IsEpgBased())
    return true;

  // If 'Start anytime' option isn't supported, don't hide start time
  if (!pThis->m_timerType->SupportsStartAnyTime())
    return true;

  std::string cond(condition);
  cond.erase(cond.find(START_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX));

  if ((cond == SETTING_TMR_START_DAY) ||
      (cond == SETTING_TMR_BEGIN))
  {
    bool bAnytime = static_cast<const CSettingBool*>(setting)->GetValue();
    return !bAnytime;
  }
  return false;
}

void CGUIDialogPVRTimerSettings::AddEndAnytimeDependentVisibilityCondition(CSetting *setting, const std::string &identifier)
{
  // Show or hide setting depending on value of setting "any time"
  std::string id(identifier);
  id.append(END_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX);
  AddCondition(setting, id, EndAnytimeSetCondition, SettingDependencyTypeVisible, SETTING_TMR_END_ANYTIME);
}

bool CGUIDialogPVRTimerSettings::EndAnytimeSetCondition(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::AnytimeSetCondition - No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  // "any time" setting is only relevant for epg-based timers.
  if (!pThis->m_timerType->IsEpgBased())
    return true;

  // If 'End anytime' option isn't supported, don't hide end time
  if (!pThis->m_timerType->SupportsEndAnyTime())
    return true;

  std::string cond(condition);
  cond.erase(cond.find(END_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX));

  if ((cond == SETTING_TMR_END_DAY)   ||
      (cond == SETTING_TMR_END))
  {
    bool bAnytime = static_cast<const CSettingBool*>(setting)->GetValue();
    return !bAnytime;
  }
  return false;
}
