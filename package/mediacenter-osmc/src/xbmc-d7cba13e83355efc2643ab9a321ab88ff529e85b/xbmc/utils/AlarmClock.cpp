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

#include "AlarmClock.h"

#include <utility>

#include "dialogs/GUIDialogKaiToast.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "guilib/LocalizeStrings.h"
#include "log.h"
#include "messaging/ApplicationMessenger.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"

using namespace KODI::MESSAGING;

CAlarmClock::CAlarmClock() : CThread("AlarmClock"), m_bIsRunning(false)
{
}

CAlarmClock::~CAlarmClock()
{
}

void CAlarmClock::Start(const std::string& strName, float n_secs, const std::string& strCommand, bool bSilent /* false */, bool bLoop /* false */)
{
  // make lower case so that lookups are case-insensitive
  std::string lowerName(strName);
  StringUtils::ToLower(lowerName);
  Stop(lowerName);
  SAlarmClockEvent event;
  event.m_fSecs = n_secs;
  event.m_strCommand = strCommand;
  event.m_loop = bLoop;
  if (!m_bIsRunning)
  {
    StopThread();
    Create();
    m_bIsRunning = true;
  }

  uint32_t labelAlarmClock;
  uint32_t labelStarted;
  if (StringUtils::EqualsNoCase(strName, "shutdowntimer"))
  {
    labelAlarmClock = 20144;
    labelStarted = 20146;
  }
  else
  {
    labelAlarmClock = 13208;
    labelStarted = 13210;
  }

  EventPtr alarmClockActivity(new CNotificationEvent(labelAlarmClock,
    StringUtils::Format(g_localizeStrings.Get(labelStarted).c_str(), static_cast<int>(event.m_fSecs) / 60, static_cast<int>(event.m_fSecs) % 60)));
  if (bSilent)
    CEventLog::GetInstance().Add(alarmClockActivity);
  else
    CEventLog::GetInstance().AddWithNotification(alarmClockActivity);

  event.watch.StartZero();
  CSingleLock lock(m_events);
  m_event.insert(make_pair(lowerName,event));
  CLog::Log(LOGDEBUG,"started alarm with name: %s",lowerName.c_str());
}

void CAlarmClock::Stop(const std::string& strName, bool bSilent /* false */)
{
  CSingleLock lock(m_events);

  std::string lowerName(strName);
  StringUtils::ToLower(lowerName);          // lookup as lowercase only
  std::map<std::string,SAlarmClockEvent>::iterator iter = m_event.find(lowerName);

  if (iter == m_event.end())
    return;

  uint32_t labelAlarmClock;
  if (StringUtils::EqualsNoCase(strName, "shutdowntimer"))
    labelAlarmClock = 20144;
  else
    labelAlarmClock = 13208;

  std::string strMessage;
  float       elapsed     = 0.f;

  if (iter->second.watch.IsRunning())
    elapsed = iter->second.watch.GetElapsedSeconds();

  if (elapsed > iter->second.m_fSecs)
    strMessage = g_localizeStrings.Get(13211);
  else
  {
    float remaining = static_cast<float>(iter->second.m_fSecs - elapsed);
    strMessage = StringUtils::Format(g_localizeStrings.Get(13212).c_str(), static_cast<int>(remaining) / 60, static_cast<int>(remaining) % 60);
  }

  if (iter->second.m_strCommand.empty() || iter->second.m_fSecs > elapsed)
  {
    EventPtr alarmClockActivity(new CNotificationEvent(labelAlarmClock, strMessage));
    if (bSilent)
      CEventLog::GetInstance().Add(alarmClockActivity);
    else
      CEventLog::GetInstance().AddWithNotification(alarmClockActivity);
  }
  else
  {
    CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, iter->second.m_strCommand);
    if (iter->second.m_loop)
    {
      iter->second.watch.Reset();
      return;
    }
  }

  iter->second.watch.Stop();
  m_event.erase(iter);
}

void CAlarmClock::Process()
{
  while( !m_bStop)
  {
    std::string strLast;
    {
      CSingleLock lock(m_events);
      for (std::map<std::string,SAlarmClockEvent>::iterator iter=m_event.begin();iter != m_event.end(); ++iter)
        if ( iter->second.watch.IsRunning()
          && iter->second.watch.GetElapsedSeconds() >= iter->second.m_fSecs)
        {
          Stop(iter->first);
          if ((iter = m_event.find(strLast)) == m_event.end())
            break;
        }
        else
          strLast = iter->first;
    }
    Sleep(100);
  }
}

