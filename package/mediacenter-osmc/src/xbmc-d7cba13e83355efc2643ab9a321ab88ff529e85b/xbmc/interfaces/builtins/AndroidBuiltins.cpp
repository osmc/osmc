/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "AndroidBuiltins.h"

#include "messaging/ApplicationMessenger.h"

/*! \brief Launch an android system activity.
 *  \param params The parameters.
 *  \details params[0] = package
 *           params[1] = intent (optional)
 *           params[2] = datatype (optional)
 *           params[2] = dataURI (optional)
 */
static int LaunchAndroidActivity(const std::vector<std::string>& params)
{
  KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(
    TMSG_START_ANDROID_ACTIVITY, -1, -1, nullptr, "", params);

  return 0;
}

CBuiltins::CommandMap CAndroidBuiltins::GetOperations() const
{
  return {
           {"startandroidactivity",    {"Launch an Android native app with the given package name.  Optional parms (in order): intent, dataType, dataURI.", 1, LaunchAndroidActivity}}
         };
}
