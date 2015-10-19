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
#include "storage/IStorageProvider.h"
#include "utils/Job.h"

class CWin32StorageProvider : public IStorageProvider
{
public:
  virtual ~CWin32StorageProvider() { }

  virtual void Initialize();
  virtual void Stop() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives);
  virtual void GetRemovableDrives(VECSOURCES &removableDrives);
  virtual std::string GetFirstOpticalDeviceFileName();

  virtual bool Eject(const std::string& mountpath);

  virtual std::vector<std::string> GetDiskUsage();

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback);

  static void SetEvent() { xbevent = true; }
  static bool xbevent;
};

class CDetectDisc : public CJob
{
public:
  CDetectDisc(const std::string &strPath, const bool bautorun);
  bool DoWork();

private:
  std::string  m_strPath;
  bool        m_bautorun;
};

