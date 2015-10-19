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

#include "filesystem/File.h"
#include "threads/Thread.h"
#include "threads/SystemClock.h"
#include <string>

class CGUIDialogProgress;

class CGUIDialogCache : public CThread, public XFILE::IFileCallback
{
public:
  CGUIDialogCache(DWORD dwDelay = 0, const std::string& strHeader="", const std::string& strMsg="");
  virtual ~CGUIDialogCache();
  void SetHeader(const std::string& strHeader);
  void SetHeader(int nHeader);
  void SetMessage(const std::string& strMessage);
  bool IsCanceled() const;
  void ShowProgressBar(bool bOnOff);
  void SetPercentage(int iPercentage);

  void Close(bool bForceClose = false);

  virtual void Process();
  virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed);

protected:

  void OpenDialog();

  XbmcThreads::EndTime m_endtime;
  CGUIDialogProgress* m_pDlg;
  std::string m_strHeader;
  std::string m_strLinePrev;
  std::string m_strLinePrev2;
  bool bSentCancel;
};
