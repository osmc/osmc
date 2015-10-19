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

#include "GUIContainerBuiltins.h"

#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "utils/StringUtils.h"

/*! \brief Change sort method.
 *  \param params (ignored)
 *
 *  Set the Dir template parameter to 1 to switch to next sort method
 *  or -1 to switch to previous sort method.
 */
  template<int Dir>
static int ChangeSortMethod(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, g_windowManager.GetActiveWindow(), 0, 0, Dir);
  g_windowManager.SendMessage(message);

  return 0;
}

/*! \brief Change view mode.
 *  \param params (ignored)
 *
 *  Set the Dir template parameter to 1 to switch to next view mode
 *  or -1 to switch to previous view mode.
 */
  template<int Dir>
static int ChangeViewMode(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, g_windowManager.GetActiveWindow(), 0, 0, Dir);
  g_windowManager.SendMessage(message);

  return 0;
}

/*! \brief Refresh a media window.
 *  \param params The parameters.
 *  \details params[0] = The URL to refresh window at.
 */
static int Refresh(const std::vector<std::string>& params)
{ // NOTE: These messages require a media window, thus they're sent to the current activewindow.
  //       This shouldn't stop a dialog intercepting it though.
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 1); // 1 to reset the history
  message.SetStringParam(params[0]);
  g_windowManager.SendMessage(message);

  return 0;
}

/*! \brief Set sort method.
 *  \param params The parameters.
 *  \details params[0] = ID of sort method.
 */
static int SetSortMethod(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, g_windowManager.GetActiveWindow(), 0, atoi(params[0].c_str()));
  g_windowManager.SendMessage(message);

  return 0;
}

/*! \brief Set view mode.
 *  \param params The parameters.
 *  \details params[0] = ID of view mode.
 */
static int SetViewMode(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, g_windowManager.GetActiveWindow(), 0, atoi(params[0].c_str()));
  g_windowManager.SendMessage(message);

  return 0;
}

/*! \brief Toggle sort direction.
 *  \param params (ignored)
 */
static int ToggleSortDirection(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_CHANGE_SORT_DIRECTION, g_windowManager.GetActiveWindow(), 0, 0);
  g_windowManager.SendMessage(message);

  return 0;
}

/*! \brief Update a listing in a media window.
 *  \param params The parameters.
 *  \details params[0] = The URL to update listing at.
 *           params[1] = "replace" to reset history (optional).
 */
static int Update(const std::vector<std::string>& params)
{
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
  message.SetStringParam(params[0]);
  if (params.size() > 1 && StringUtils::EqualsNoCase(params[1], "replace"))
    message.SetParam2(1); // reset the history
  g_windowManager.SendMessage(message);

  return 0;
}

CBuiltins::CommandMap CGUIContainerBuiltins::GetOperations() const
{
  return {
           {"container.nextsortmethod",     {"Change to the next sort method", 0, ChangeSortMethod<1>}},
           {"container.nextviewmode",       {"Move to the next view type (and refresh the listing)", 0, ChangeViewMode<1>}},
           {"container.previoussortmethod", {"Change to the previous sort method", 0, ChangeSortMethod<-1>}},
           {"container.previousviewmode",   {"Move to the previous view type (and refresh the listing)", 0, ChangeViewMode<-1>}},
           {"container.refresh",            {"Refresh current listing", 1, Refresh}},
           {"container.setsortdirection",   {"Toggle the sort direction", 0, ToggleSortDirection}},
           {"container.setsortmethod",      {"Change to the specified sort method", 1, SetSortMethod}},
           {"container.setviewmode",        {"Move to the view with the given id", 1, SetViewMode}},
           {"container.update",             {"Update current listing. Send Container.Update(path,replace) to reset the path history", 1, Update}}
         };
}
