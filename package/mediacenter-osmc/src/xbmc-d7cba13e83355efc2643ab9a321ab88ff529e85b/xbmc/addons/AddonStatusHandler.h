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

#include "threads/Thread.h"
#include "IAddon.h"
#include "include/xbmc_addon_types.h"
#include "threads/CriticalSection.h"
#include <string>

namespace ADDON
{
  /**
  * Class - CAddonStatusHandler
  * Used to informate the user about occurred errors and
  * changes inside Add-on's, and ask him what to do.
  * It can executed in the same thread as the calling
  * function or in a seperate thread.
  */
  class CAddonStatusHandler : private CThread
  {
    public:
      CAddonStatusHandler(const std::string &addonID, ADDON_STATUS status, std::string message, bool sameThread = true);
      ~CAddonStatusHandler();

      /* Thread handling */
      virtual void Process();
      virtual void OnStartup();
      virtual void OnExit();

    private:
      static CCriticalSection   m_critSection;
      AddonPtr                  m_addon;
      ADDON_STATUS              m_status;
      std::string               m_message;
  };


}
