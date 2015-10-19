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

#include "GUIDialogMusicOSD.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "input/InputManager.h"
#include "GUIUserMessages.h"
#include "settings/Settings.h"
#include "addons/GUIWindowAddonBrowser.h"

#define CONTROL_VIS_BUTTON       500
#define CONTROL_LOCK_BUTTON      501

CGUIDialogMusicOSD::CGUIDialogMusicOSD(void)
    : CGUIDialog(WINDOW_DIALOG_MUSIC_OSD, "MusicOSD.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogMusicOSD::~CGUIDialogMusicOSD(void)
{
}

bool CGUIDialogMusicOSD::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl == CONTROL_VIS_BUTTON)
      {
        std::string addonID;
        if (CGUIWindowAddonBrowser::SelectAddonID(ADDON::ADDON_VIZ, addonID, true) == 1)
        {
          CSettings::GetInstance().SetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION, addonID);
          CSettings::GetInstance().Save();
          g_windowManager.SendMessage(GUI_MSG_VISUALISATION_RELOAD, 0, 0);
        }
      }
      else if (iControl == CONTROL_LOCK_BUTTON)
      {
        CGUIMessage msg(GUI_MSG_VISUALISATION_ACTION, 0, 0, ACTION_VIS_PRESET_LOCK);
        g_windowManager.SendMessage(msg);
      }
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogMusicOSD::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_SHOW_OSD:
    Close();
    return true;

  default:
    break;
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogMusicOSD::FrameMove()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (CInputManager::GetInstance().IsMouseActive() ||
        g_windowManager.IsWindowActive(WINDOW_DIALOG_VIS_SETTINGS) ||
        g_windowManager.IsWindowActive(WINDOW_DIALOG_VIS_PRESET_LIST) ||
        g_windowManager.IsWindowActive(WINDOW_DIALOG_AUDIO_DSP_OSD_SETTINGS) ||
        g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_RADIO_RDS_INFO))
      // extend show time by original value
      SetAutoClose(m_showDuration);
  }
  CGUIDialog::FrameMove();
}
