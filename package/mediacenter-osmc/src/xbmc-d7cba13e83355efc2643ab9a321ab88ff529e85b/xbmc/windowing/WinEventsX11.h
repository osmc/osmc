/*
*      Copyright (C) 2005-2012 Team XBMC
*      http://www.xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/
#pragma once

#include "WinEvents.h"
#include <X11/Xlib.h>
#include "threads/SystemClock.h"
#include <map>

class CWinEventsX11 : public IWinEvents
{
public:
  virtual bool MessagePump();
  virtual size_t GetQueueSize();
};

class CWinEventsX11Imp
{
public:
  CWinEventsX11Imp();
  virtual ~CWinEventsX11Imp();
  static bool Init(Display *dpy, Window win);
  static void Quit();
  static bool HasStructureChanged();
  static void PendingResize(int width, int height);
  static void SetXRRFailSafeTimer(int millis);
  static bool MessagePump();
  static size_t GetQueueSize();

protected:
  static XBMCKey LookupXbmcKeySym(KeySym keysym);
  static bool ProcessKey(XBMC_Event &event);
  static CWinEventsX11Imp *WinEvents;
  Display *m_display;
  Window m_window;
  Atom m_wmDeleteMessage;
  char *m_keybuf;
  size_t m_keybuf_len;
  XIM m_xim;
  XIC m_xic;
  std::map<uint32_t,uint32_t> m_symLookupTable;
  int m_keymodState;
  bool m_structureChanged;
  int m_RREventBase;
  XbmcThreads::EndTime m_xrrFailSafeTimer;
  bool m_xrrEventPending;
};
