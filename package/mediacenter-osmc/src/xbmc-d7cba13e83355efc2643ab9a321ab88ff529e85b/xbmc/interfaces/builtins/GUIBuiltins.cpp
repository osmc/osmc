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

#include "GUIBuiltins.h"

#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "input/ButtonTranslator.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "utils/Screenshot.h"
#include "utils/RssManager.h"
#include "utils/AlarmClock.h"
#include "windows/GUIMediaWindow.h"

using namespace KODI::MESSAGING;

/*! \brief Execute a GUI action.
 *  \param params The parameters.
 *  \details params[0] = Action to execute.
 *           params[1] = Window to send action to (optional).
 */
static int Action(const std::vector<std::string>& params)
{
  // try translating the action from our ButtonTranslator
  int actionID;
  if (CButtonTranslator::TranslateActionString(params[0].c_str(), actionID))
  {
    int windowID = params.size() == 2 ? CButtonTranslator::TranslateWindow(params[1]) : WINDOW_INVALID;
    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, windowID, -1, static_cast<void*>(new CAction(actionID)));
  }

  return 0;
}

/*! \brief Activate a window.
 *  \param params The parameters.
 *  \details params[0] = The window name.
 *           params[1] = Window starting folder (optional).
 *
 *           Set the Replace template parameter to true to replace current
 *           window in history.
 */
  template<bool Replace>
static int ActivateWindow(const std::vector<std::string>& params2)
{
  std::vector<std::string> params(params2);
  // get the parameters
  std::string strWindow;
  if (params.size())
  {
    strWindow = params[0];
    params.erase(params.begin());
  }

  // confirm the window destination is valid prior to switching
  int iWindow = CButtonTranslator::TranslateWindow(strWindow);
  if (iWindow != WINDOW_INVALID)
  {
    // compate the given directory param with the current active directory
    bool bIsSameStartFolder = true;
    if (!params.empty())
    {
      CGUIWindow *activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
      if (activeWindow && activeWindow->IsMediaWindow())
        bIsSameStartFolder = ((CGUIMediaWindow*) activeWindow)->IsSameStartFolder(params[0]);
    }

    // activate window only if window and path differ from the current active window
    if (iWindow != g_windowManager.GetActiveWindow() || !bIsSameStartFolder)
    {
      g_application.WakeUpScreenSaverAndDPMS();
      g_windowManager.ActivateWindow(iWindow, params, Replace);
      return 0;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "Activate/ReplaceWindow called with invalid destination window: %s", strWindow.c_str());
    return false;
  }

  return 1;
}

/*! \brief Activate a window and give given controls focus.
 *  \param params The parameters.
 *  \details params[0] = The window name.
 *           params[1,...] = Pair of (container ID, focus item).
 *
 *           Set the Replace template parameter to true to replace current
 *           window in history.
 */
  template<bool Replace>
static int ActivateAndFocus(const std::vector<std::string>& params)
{
  std::string strWindow = params[0];

  // confirm the window destination is valid prior to switching
  int iWindow = CButtonTranslator::TranslateWindow(strWindow);
  if (iWindow != WINDOW_INVALID)
  {
    if (iWindow != g_windowManager.GetActiveWindow())
    {
      // disable the screensaver
      g_application.WakeUpScreenSaverAndDPMS();
      g_windowManager.ActivateWindow(iWindow, {}, Replace);

      unsigned int iPtr = 1;
      while (params.size() > iPtr + 1)
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, g_windowManager.GetFocusedWindow(),
                        atol(params[iPtr].c_str()),
                        (params.size() >= iPtr + 2) ? atol(params[iPtr + 1].c_str())+1 : 0);
        g_windowManager.SendMessage(msg);
        iPtr += 2;
      }
      return 0;
    }

  }
  else
    CLog::Log(LOGERROR, "Replace/ActivateWindowAndFocus called with invalid destination window: %s", strWindow.c_str());

  return 1;
}

/*! \brief Start an alarm clock
 *  \param params The parameters.
 *  \details param[0] = name
 *           param[1] = command
 *           param[2] = Length in seconds (optional).
 *           param[3] = "silent" to suppress notifications.
 *           param[3] = "loop" to loop the alarm.
 */
static int AlarmClock(const std::vector<std::string>& params)
{
  // format is alarmclock(name,command[,seconds,true]);
  float seconds = 0;
  if (params.size() > 2)
  {
    if (params[2].find(':') == std::string::npos)
      seconds = static_cast<float>(atoi(params[2].c_str())*60);
    else
      seconds = (float)StringUtils::TimeStringToSeconds(params[2]);
  }
  else
  { // check if shutdown is specified in particular, and get the time for it
    std::string strHeading;
    if (StringUtils::EqualsNoCase(params[0], "shutdowntimer"))
      strHeading = g_localizeStrings.Get(20145);
    else
      strHeading = g_localizeStrings.Get(13209);
    std::string strTime;
    if( CGUIDialogNumeric::ShowAndGetNumber(strTime, strHeading) )
      seconds = static_cast<float>(atoi(strTime.c_str())*60);
    else
      return false;
  }
  bool silent = false;
  bool loop = false;
  for (unsigned int i = 3; i < params.size() ; i++)
  {
    // check "true" for backward comp
    if (StringUtils::EqualsNoCase(params[i], "true") || StringUtils::EqualsNoCase(params[i], "silent"))
      silent = true;
    else if (StringUtils::EqualsNoCase(params[i], "loop"))
      loop = true;
  }

  if( g_alarmClock.IsRunning() )
    g_alarmClock.Stop(params[0],silent);
  // no negative times not allowed, loop must have a positive time
  if (seconds < 0 || (seconds == 0 && loop))
    return false;
  g_alarmClock.Start(params[0], seconds, params[1], silent, loop);

  return 0;
}

/*! \brief Cancel an alarm clock.
 *  \param params The parameters.
 *  \details params[0] = "true" to silently cancel alarm (optional).
 */
static int CancelAlarm(const std::vector<std::string>& params)
{
  bool silent = (params.size() > 1 &&
      (StringUtils::EqualsNoCase(params[1], "true") ||
       StringUtils::EqualsNoCase(params[1], "silent")));
  g_alarmClock.Stop(params[0],silent);

  return 0;
}

/*! \brief Clear a property in a window.
 *  \param params The parameters.
 *  \details params[0] = The property to clear.
 *           params[1] = The window to clear property in (optional).
 */
static int ClearProperty(const std::vector<std::string>& params)
{
  CGUIWindow *window = g_windowManager.GetWindow(params.size() > 1 ? CButtonTranslator::TranslateWindow(params[1]) : g_windowManager.GetFocusedWindow());
  if (window)
    window->SetProperty(params[0],"");

  return 0;
}

/*! \brief Close a dialog.
 *  \param params The parameters.
 *  \details params[0] = "all" to close all dialogs, or dialog name.
 *           params[1] = "true" to force close (skip animations) (optional).
 */
static int CloseDialog(const std::vector<std::string>& params)
{
  bool bForce = false;
  if (params.size() > 1 && StringUtils::EqualsNoCase(params[1], "true"))
    bForce = true;
  if (StringUtils::EqualsNoCase(params[0], "all"))
  {
    g_windowManager.CloseDialogs(bForce);
  }
  else
  {
    int id = CButtonTranslator::TranslateWindow(params[0]);
    CGUIWindow *window = (CGUIWindow *)g_windowManager.GetWindow(id);
    if (window && window->IsDialog())
      ((CGUIDialog *)window)->Close(bForce);
  }

  return 0;
}

/*! \brief Send a notification.
 *  \param params The parameters.
 *  \details params[0] = Notification title.
 *           params[1] = Notification text.
 *           params[2] = Display time in milliseconds (optional).
 *           params[3] = Notification icon (optional).
 */
static int Notification(const std::vector<std::string>& params)
{
  if (params.size() < 2)
    return -1;
  if (params.size() == 4)
    CGUIDialogKaiToast::QueueNotification(params[3],params[0],params[1],atoi(params[2].c_str()));
  else if (params.size() == 3)
    CGUIDialogKaiToast::QueueNotification("",params[0],params[1],atoi(params[2].c_str()));
  else
    CGUIDialogKaiToast::QueueNotification(params[0],params[1]);

  return 0;
}

/*! \brief Refresh RSS feed.
 *  \param params (ignored)
 */
static int RefreshRSS(const std::vector<std::string>& params)
{
  CRssManager::GetInstance().Reload();

  return 0;
}

/*! \brief Take a screenshot.
 *  \param params The parameters.
 *  \details params[0] = URL to save file to. Blank to use default.
 *           params[1] = "sync" to run synchronously (optional).
 */
static int Screenshot(const std::vector<std::string>& params)
{
  if (!params.empty())
  {
    // get the parameters
    std::string strSaveToPath = params[0];
    bool sync = false;
    if (params.size() >= 2)
      sync = StringUtils::EqualsNoCase(params[1], "sync");

    if (!strSaveToPath.empty())
    {
      if (XFILE::CDirectory::Exists(strSaveToPath))
      {
        std::string file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(strSaveToPath, "screenshot%03d.png"), 999);

        if (!file.empty())
        {
          CScreenShot::TakeScreenshot(file, sync);
        }
        else
        {
          CLog::Log(LOGWARNING, "Too many screen shots or invalid folder %s", strSaveToPath.c_str());
        }
      }
      else
        CScreenShot::TakeScreenshot(strSaveToPath, sync);
    }
  }
  else
    CScreenShot::TakeScreenshot();

  return 0;
}

/*! \brief Set GUI language.
 *  \param params The parameters.
 *  \details params[0] = The language to use.
 */
static int SetLanguage(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_SETLANGUAGE, -1, -1, nullptr, params[0]);

  return 0;
}

/*! \brief Set GUI resolution.
 *  \param params The parameters.
 *  \details params[0] = A resolution identifier.
 */
static int SetResolution(const std::vector<std::string>& params)
{
  RESOLUTION res = RES_PAL_4x3;
  std::string paramlow(params[0]);
  StringUtils::ToLower(paramlow);
  if (paramlow == "pal") res = RES_PAL_4x3;
  else if (paramlow == "pal16x9") res = RES_PAL_16x9;
  else if (paramlow == "ntsc") res = RES_NTSC_4x3;
  else if (paramlow == "ntsc16x9") res = RES_NTSC_16x9;
  else if (paramlow == "720p") res = RES_HDTV_720p;
  else if (paramlow == "720psbs") res = RES_HDTV_720pSBS;
  else if (paramlow == "720ptb") res = RES_HDTV_720pTB;
  else if (paramlow == "1080psbs") res = RES_HDTV_1080pSBS;
  else if (paramlow == "1080ptb") res = RES_HDTV_1080pTB;
  else if (paramlow == "1080i") res = RES_HDTV_1080i;
  if (g_graphicsContext.IsValidResolution(res))
  {
    CDisplaySettings::GetInstance().SetCurrentResolution(res, true);
    g_application.ReloadSkin();
  }

  return 0;
}

/*! \brief Set a property in a window.
 *  \param params The parameters.
 *  \details params[0] = The property to set.
 *           params[1] = The property value.
 *           params[2] = The window to set property in (optional).
 */
static int SetProperty(const std::vector<std::string>& params)
{
  CGUIWindow *window = g_windowManager.GetWindow(params.size() > 2 ? CButtonTranslator::TranslateWindow(params[2]) : g_windowManager.GetFocusedWindow());
  if (window)
    window->SetProperty(params[0],params[1]);

  return 0;
}

/*! \brief Set GUI stereo mode.
 *  \param params The parameters.
 *  \details param[0] = Stereo mode identifier.
 */
static int SetStereoMode(const std::vector<std::string>& params)
{
  CAction action = CStereoscopicsManager::GetInstance().ConvertActionCommandToAction("SetStereoMode", params[0]);
  if (action.GetID() != ACTION_NONE)
    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(action)));
  else
  {
    CLog::Log(LOGERROR,"Builtin 'SetStereoMode' called with unknown parameter: %s", params[0].c_str());
    return -2;
  }

  return 0;
}

/*! \brief Toggle visualization of dirty regions.
 *  \param params Ignored.
 */
static int ToggleDirty(const std::vector<std::string>&)
{
  g_advancedSettings.ToggleDirtyRegionVisualization();

  return 0;
}

CBuiltins::CommandMap CGUIBuiltins::GetOperations() const
{
  return {
           {"action",                         {"Executes an action for the active window (same as in keymap)", 1, Action}},
           {"cancelalarm",                    {"Cancels an alarm", 1, CancelAlarm}},
           {"alarmclock",                     {"Prompt for a length of time and start an alarm clock", 2, AlarmClock}},
           {"activatewindow",                 {"Activate the specified window", 1, ActivateWindow<false>}},
           {"activatewindowandfocus",         {"Activate the specified window and sets focus to the specified id", 1, ActivateAndFocus<false>}},
           {"clearproperty",                  {"Clears a window property for the current focused window/dialog (key,value)", 1, ClearProperty}},
           {"dialog.close",                   {"Close a dialog", 1, CloseDialog}},
           {"notification",                   {"Shows a notification on screen, specify header, then message, and optionally time in milliseconds and a icon.", 2, Notification}},
           {"refreshrss",                     {"Reload RSS feeds from RSSFeeds.xml", 0, RefreshRSS}},
           {"replacewindow",                  {"Replaces the current window with the new one and sets focus to the specified id", 1, ActivateWindow<true>}},
           {"replacewindowandfocus",          {"Replaces the current window with the new one and sets focus to the specified id", 1, ActivateAndFocus<true>}},
           {"resolution",                     {"Change Kodi's Resolution", 1, SetResolution}},
           {"setguilanguage",                 {"Set GUI Language", 1, SetLanguage}},
           {"setproperty",                    {"Sets a window property for the current focused window/dialog (key,value)", 2, SetProperty}},
           {"setstereomode",                  {"Changes the stereo mode of the GUI. Params can be: toggle, next, previous, select, tomono or any of the supported stereomodes (off, split_vertical, split_horizontal, row_interleaved, hardware_based, anaglyph_cyan_red, anaglyph_green_magenta, anaglyph_yellow_blue, monoscopic)", 1, SetStereoMode}},
           {"takescreenshot",                 {"Takes a Screenshot", 0, Screenshot}},
           {"toggledirtyregionvisualization", {"Enables/disables dirty-region visualization", 0, ToggleDirty}}
         };
}
