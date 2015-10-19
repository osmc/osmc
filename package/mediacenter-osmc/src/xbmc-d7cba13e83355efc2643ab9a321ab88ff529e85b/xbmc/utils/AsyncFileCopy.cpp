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

#include "threads/SystemClock.h"
#include "AsyncFileCopy.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "log.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "utils/Variant.h"

CAsyncFileCopy::CAsyncFileCopy() : CThread("AsyncFileCopy")
{
  m_cancelled = false;
  m_succeeded = false;
  m_running = false;
  m_percent = 0;
  m_speed = 0;
}

CAsyncFileCopy::~CAsyncFileCopy()
{
  StopThread();
}

bool CAsyncFileCopy::Copy(const std::string &from, const std::string &to, const std::string &heading)
{
  // reset the variables to their appropriate states
  m_from = from;
  m_to = to;
  m_cancelled = false;
  m_succeeded = false;
  m_percent = 0;
  m_speed = 0;
  m_running = true;
  CURL url1(from);
  CURL url2(to);

  // create our thread, which starts the file copy operation
  Create();
  CGUIDialogProgress *dlg = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  unsigned int time = XbmcThreads::SystemClockMillis();
  while (m_running)
  {
    m_event.WaitMSec(1000 / 30);
    if (!m_running)
      break;
    // start the dialog up as needed
    if (dlg && !dlg->IsDialogRunning() && (XbmcThreads::SystemClockMillis() - time) > 500) // wait 0.5 seconds before starting dialog
    {
      dlg->SetHeading(CVariant{heading});
      dlg->SetLine(0, CVariant{url1.GetWithoutUserDetails()});
      dlg->SetLine(1, CVariant{url2.GetWithoutUserDetails()});
      dlg->SetPercentage(0);
      dlg->Open();
    }
    // and update the dialog as we go
    if (dlg && dlg->IsDialogRunning())
    {
      dlg->SetHeading(CVariant{heading});
      dlg->SetLine(0, CVariant{url1.Get()});
      dlg->SetLine(1, CVariant{url2.Get()});
      dlg->SetLine(2, CVariant{ StringUtils::Format("%2.2f KB/s", m_speed / 1024) });
      dlg->SetPercentage(m_percent);
      dlg->Progress();
      m_cancelled = dlg->IsCanceled();
    }
  }
  if (dlg)
    dlg->Close();
  return !m_cancelled && m_succeeded;
}

bool CAsyncFileCopy::OnFileCallback(void *pContext, int ipercent, float avgSpeed)
{
  m_percent = ipercent;
  m_speed = avgSpeed;
  m_event.Set();
  return !m_cancelled;
}

void CAsyncFileCopy::Process()
{
  try
  {
    m_succeeded = XFILE::CFile::Copy(m_from, m_to, this);
  }
  catch (...)
  {
    m_succeeded = false;
    CLog::Log(LOGERROR, "%s: unhandled exception copying file", __FUNCTION__);
  }
  m_running = false;
}
