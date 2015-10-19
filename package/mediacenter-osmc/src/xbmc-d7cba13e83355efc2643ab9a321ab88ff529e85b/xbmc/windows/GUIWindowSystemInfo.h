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

#include "guilib/GUIWindow.h"

class CGUIWindowSystemInfo : public CGUIWindow
{
public:
  CGUIWindowSystemInfo(void);
  virtual ~CGUIWindowSystemInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void FrameMove();
private:
  int  m_section;
  void ResetLabels();
  void SetControlLabel(int id, const char *format, int label, int info);
  std::vector<std::string> m_diskUsage;
};

