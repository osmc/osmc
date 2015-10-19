/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "ContextMenuManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "epg/EpgContainer.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "utils/Variant.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"

#include "GUIWindowPVRSearch.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRSearch::CGUIWindowPVRSearch(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_SEARCH : WINDOW_TV_SEARCH, "MyPVRSearch.xml"),
  m_bSearchConfirmed(false)
{
}

void CGUIWindowPVRSearch::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  if (pItem->HasEPGInfoTag())
  {
    if (pItem->GetEPGInfoTag()->EndAsLocalTime() > CDateTime::GetCurrentDateTime())
    {
      if (!pItem->GetEPGInfoTag()->HasTimer())
      {
        buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);      /* record */
        buttons.Add(CONTEXT_BUTTON_ADVANCED_RECORD, 19061); /* add timer */
      }
      else
      {
        if (pItem->GetEPGInfoTag()->StartAsLocalTime() < CDateTime::GetCurrentDateTime())
          buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19059); /* stop recording */
        else if (pItem->GetEPGInfoTag()->Timer()->HasTimerType())
        {
          if (!pItem->GetEPGInfoTag()->Timer()->GetTimerType()->IsReadOnly())
            buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19060); /* delete timer */
        }
      }
    }
    if (pItem->GetEPGInfoTag()->HasRecording())
      buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19687);       /* Play recording */

    buttons.Add(CONTEXT_BUTTON_INFO, 19047);              /* Epg info button */
    if (pItem->GetEPGInfoTag()->HasPVRChannel() &&
        g_PVRClients->HasMenuHooks(pItem->GetEPGInfoTag()->ChannelTag()->ClientID(), PVR_MENUHOOK_EPG))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */
  }

  buttons.Add(CONTEXT_BUTTON_CLEAR, 19232);             /* Clear search results */

  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
  CContextMenuManager::GetInstance().AddVisibleItems(pItem, buttons);
}

void CGUIWindowPVRSearch::OnWindowLoaded()
{
  CGUIMediaWindow::OnWindowLoaded();
  m_searchfilter.Reset();
}

bool CGUIWindowPVRSearch::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  return OnContextButtonClear(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonStopRecord(pItem.get(), button) ||
      OnContextButtonStartRecord(pItem.get(), button) ||
      OnContextButtonPlay(pItem.get(), button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRSearch::OnContextButton(const CFileItem &item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  switch(button)
  {
    case CONTEXT_BUTTON_FIND:
    {
      m_searchfilter.Reset();

      // construct the search term
      if (item.IsEPG())
        m_searchfilter.m_strSearchTerm = "\"" + item.GetEPGInfoTag()->Title() + "\"";
      else if (item.IsPVRChannel())
      {
        const CEpgInfoTagPtr tag(item.GetPVRChannelInfoTag()->GetEPGNow());
        if (tag)
          m_searchfilter.m_strSearchTerm = "\"" + tag->Title() + "\"";
      }
      else if (item.IsUsablePVRRecording())
        m_searchfilter.m_strSearchTerm = "\"" + item.GetPVRRecordingInfoTag()->m_strTitle + "\"";
      else if (item.IsPVRTimer())
      {
        const CPVRTimerInfoTagPtr info(item.GetPVRTimerInfoTag());
        const CEpgInfoTagPtr tag(info->GetEpgInfoTag());
        if (tag)
          m_searchfilter.m_strSearchTerm = "\"" + tag->Title() + "\"";
        else
          m_searchfilter.m_strSearchTerm = "\"" + info->m_strTitle + "\"";
      }
      m_bSearchConfirmed = true;
      Refresh(true);
      bReturn = true;
      break;
    }
    default:
      bReturn = false;
  }

  return bReturn;
}

void CGUIWindowPVRSearch::OnPrepareFileItems(CFileItemList &items)
{
  bool bAddSpecialSearchItem = items.IsEmpty();

  if (m_bSearchConfirmed)
  {
    m_bSearchConfirmed = false;

    bAddSpecialSearchItem = true;

    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (dlgProgress)
    {
      dlgProgress->SetHeading(CVariant{194}); // "Searching..."
      dlgProgress->SetText(CVariant{m_searchfilter.m_strSearchTerm});
      dlgProgress->Open();
      dlgProgress->Progress();
    }

    // TODO should we limit the find similar search to the selected group?
    g_EpgContainer.GetEPGSearch(items, m_searchfilter);

    if (dlgProgress)
      dlgProgress->Close();

    if (items.IsEmpty())
      CGUIDialogOK::ShowAndGetInput(CVariant{194},  // "Searching..."
                                    CVariant{284}); // "No results found"
  }

  if (bAddSpecialSearchItem)
  {
    CFileItemPtr item(new CFileItem("pvr://guide/searchresults/search/", true));
    item->SetLabel(g_localizeStrings.Get(19140)); // "Search..."
    item->SetLabelPreformated(true);
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
}

bool CGUIWindowPVRSearch::OnMessage(CGUIMessage &message)
{
  if (!IsValidMessage(message))
    return false;
  
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetSenderId() == m_viewControl.GetCurrentControl())
    {
      int iItem = m_viewControl.GetSelectedItem();
      if (iItem >= 0 && iItem < m_vecItems->Size())
      {
        CFileItemPtr pItem = m_vecItems->Get(iItem);

        /* process actions */
        switch (message.GetParam1())
        {
          case ACTION_SHOW_INFO:
          case ACTION_SELECT_ITEM:
          case ACTION_MOUSE_LEFT_CLICK:
          {
            if (URIUtils::PathEquals(pItem->GetPath(), "pvr://guide/searchresults/search/"))
              OpenDialogSearch();
            else
               ShowEPGInfo(pItem.get());
            return true;
          }

          case ACTION_CONTEXT_MENU:
          case ACTION_MOUSE_RIGHT_CLICK:
            OnPopupMenu(iItem);
            return true;

          case ACTION_RECORD:
            ActionRecord(pItem.get());
            return true;
        }
      }
    }
  }

  return CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRSearch::OnContextButtonClear(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_CLEAR)
  {
    bReturn = true;

    m_bSearchConfirmed = false;
    m_searchfilter.Reset();

    Refresh(true);
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_INFO)
  {
    bReturn = true;

    ShowEPGInfo(item);
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_PLAY_ITEM)
  {
    ActionPlayEpg(item, true /* play recording, not channel */);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnContextButtonStartRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if ((button == CONTEXT_BUTTON_START_RECORD) ||
      (button == CONTEXT_BUTTON_ADVANCED_RECORD))
  {
    AddTimer(item, button == CONTEXT_BUTTON_ADVANCED_RECORD);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRSearch::OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_STOP_RECORD)
  {
    StopRecordFile(item);
    bReturn = true;
  }

  return bReturn;
}

void CGUIWindowPVRSearch::OpenDialogSearch()
{
  CGUIDialogPVRGuideSearch* dlgSearch = (CGUIDialogPVRGuideSearch*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_SEARCH);

  if (!dlgSearch)
    return;

  dlgSearch->SetFilterData(&m_searchfilter);

  /* Set channel type filter */
  m_searchfilter.m_bIsRadio = m_bRadio;

  /* Open dialog window */
  dlgSearch->Open();

  if (dlgSearch->IsConfirmed())
  {
    m_bSearchConfirmed = true;
    Refresh(true);
  }
}
