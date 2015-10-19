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

#include "threads/SystemClock.h"
#include "PlayListPlayer.h"
#include "Application.h"
#include "PartyModeManager.h"
#include "settings/AdvancedSettings.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "playlists/PlayList.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "music/tags/MusicInfoTag.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "input/Key.h"
#include "URL.h"
#include "messaging/ApplicationMessenger.h"

using namespace PLAYLIST;
using namespace KODI::MESSAGING;

CPlayListPlayer::CPlayListPlayer(void)
{
  m_PlaylistMusic = new CPlayList(PLAYLIST_MUSIC);
  m_PlaylistVideo = new CPlayList(PLAYLIST_VIDEO);
  m_PlaylistEmpty = new CPlayList;
  m_iCurrentSong = -1;
  m_bPlayedFirstFile = false;
  m_bPlaybackStarted = false;
  m_iCurrentPlayList = PLAYLIST_NONE;
  for (int i = 0; i < 2; i++)
    m_repeatState[i] = REPEAT_NONE;
  m_iFailedSongs = 0;
  m_failedSongsStart = 0;
}

CPlayListPlayer::~CPlayListPlayer(void)
{
  Clear();
  delete m_PlaylistMusic;
  delete m_PlaylistVideo;
  delete m_PlaylistEmpty;
}

bool CPlayListPlayer::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PREV_ITEM && !IsSingleItemNonRepeatPlaylist())
  {
    PlayPrevious();
    return true;
  }
  else if (action.GetID() == ACTION_NEXT_ITEM && !IsSingleItemNonRepeatPlaylist())
  {
    PlayNext();
    return true;
  }
  else
    return false;
}

bool CPlayListPlayer::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
    {
      // update the items in our playlist(s) if necessary
      for (int i = PLAYLIST_MUSIC; i <= PLAYLIST_VIDEO; i++)
      {
        CPlayList &playlist = GetPlaylist(i);
        CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
        playlist.UpdateItem(item.get());
      }
    }
    break;
  case GUI_MSG_PLAYBACK_STOPPED:
    {
      if (m_iCurrentPlayList != PLAYLIST_NONE && m_bPlaybackStarted)
      {
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
        g_windowManager.SendThreadMessage(msg);
        Reset();
        m_iCurrentPlayList = PLAYLIST_NONE;
        return true;
      }
    }
    break;
  }

  return false;
}

int CPlayListPlayer::GetNextSong(int offset) const
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return -1;

  const CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return -1;

  int song = m_iCurrentSong;

  // party mode
  if (g_partyModeManager.IsEnabled() && GetCurrentPlaylist() == PLAYLIST_MUSIC)
    return song + offset;

  // wrap around in the case of repeating
  if (RepeatedOne(m_iCurrentPlayList))
    return song;

  song += offset;
  if (song >= playlist.size() && Repeated(m_iCurrentPlayList))
    song %= playlist.size();

  return song;
}

int CPlayListPlayer::GetNextSong()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return -1;
  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return -1;
  int iSong = m_iCurrentSong;

  // party mode
  if (g_partyModeManager.IsEnabled() && GetCurrentPlaylist() == PLAYLIST_MUSIC)
    return iSong + 1;

  // if repeat one, keep playing the current song if its valid
  if (RepeatedOne(m_iCurrentPlayList))
  {
    // otherwise immediately abort playback
    if (m_iCurrentSong >= 0 && m_iCurrentSong < playlist.size() && playlist[m_iCurrentSong]->GetProperty("unplayable").asBoolean())
    {
      CLog::Log(LOGERROR,"Playlist Player: RepeatOne stuck on unplayable item: %i, path [%s]", m_iCurrentSong, playlist[m_iCurrentSong]->GetPath().c_str());
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      g_windowManager.SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = PLAYLIST_NONE;
      return -1;
    }
    return iSong;
  }

  // if we've gone beyond the playlist and repeat all is enabled,
  // then we clear played status and wrap around
  iSong++;
  if (iSong >= playlist.size() && Repeated(m_iCurrentPlayList))
    iSong = 0;

  return iSong;
}

bool CPlayListPlayer::PlayNext(int offset, bool bAutoPlay)
{
  int iSong = GetNextSong(offset);
  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);

  if ((iSong < 0) || (iSong >= playlist.size()) || (playlist.GetPlayable() <= 0))
  {
    if(!bAutoPlay)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559), g_localizeStrings.Get(34201));

    CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
    g_windowManager.SendThreadMessage(msg);
    Reset();
    m_iCurrentPlayList = PLAYLIST_NONE;
    return false;
  }

  return Play(iSong, false);
}

bool CPlayListPlayer::PlayPrevious()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  int iSong = m_iCurrentSong;

  if (!RepeatedOne(m_iCurrentPlayList))
    iSong--;

  if (iSong < 0 && Repeated(m_iCurrentPlayList))
    iSong = playlist.size() - 1;

  if (iSong < 0 || playlist.size() <= 0)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559), g_localizeStrings.Get(34202));
    return false;
  }

  return Play(iSong, false, true);
}

bool CPlayListPlayer::IsSingleItemNonRepeatPlaylist() const
{
  const CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  return (playlist.size() <= 1 && !RepeatedOne(m_iCurrentPlayList) && !Repeated(m_iCurrentPlayList));
}

bool CPlayListPlayer::Play()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0) 
    return false;

  return Play(0);
}

bool CPlayListPlayer::PlaySongId(int songId)
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0) 
    return Play();

  for (int i = 0; i < playlist.size(); i++)
  {
    if (playlist[i]->HasMusicInfoTag() && playlist[i]->GetMusicInfoTag()->GetDatabaseId() == songId)
      return Play(i);
  }
  return Play();
}

bool CPlayListPlayer::Play(int iSong, bool bAutoPlay /* = false */, bool bPlayPrevious /* = false */)
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0) 
    return false;
  if (iSong < 0) 
    iSong = 0;
  if (iSong >= playlist.size()) 
    iSong = playlist.size() - 1;

  // check if the item itself is a playlist, and can be expanded
  // only allow a few levels, this could end up in a loop
  // if they refer to each other in a loop
  for(int i=0;i<5;i++)
  {
     if(!playlist.Expand(iSong))
        break;
  }

  m_iCurrentSong = iSong;
  CFileItemPtr item = playlist[m_iCurrentSong];
  playlist.SetPlayed(true);

  m_bPlaybackStarted = false;

  unsigned int playAttempt = XbmcThreads::SystemClockMillis();
  PlayBackRet ret = g_application.PlayFile(*item, bAutoPlay);
  if (ret == PLAYBACK_CANCELED)
    return false;
  if (ret == PLAYBACK_FAIL)
  {
    CLog::Log(LOGERROR,"Playlist Player: skipping unplayable item: %i, path [%s]", m_iCurrentSong, CURL::GetRedacted(item->GetPath()).c_str());
    playlist.SetUnPlayable(m_iCurrentSong);

    // abort on 100 failed CONSECTUTIVE songs
    if (!m_iFailedSongs)
      m_failedSongsStart = playAttempt;
    m_iFailedSongs++;
    if ((m_iFailedSongs >= g_advancedSettings.m_playlistRetries && g_advancedSettings.m_playlistRetries >= 0)
        || ((XbmcThreads::SystemClockMillis() - m_failedSongsStart  >= (unsigned int)g_advancedSettings.m_playlistTimeout * 1000) && g_advancedSettings.m_playlistTimeout))
    {
      CLog::Log(LOGDEBUG,"Playlist Player: one or more items failed to play... aborting playback");

      // open error dialog
      CGUIDialogOK::ShowAndGetInput(CVariant{16026}, CVariant{16027});

      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      g_windowManager.SendThreadMessage(msg);
      Reset();
      GetPlaylist(m_iCurrentPlayList).Clear();
      m_iCurrentPlayList = PLAYLIST_NONE;
      m_iFailedSongs = 0;
      m_failedSongsStart = 0;
      return false;
    }

    // how many playable items are in the playlist?
    if (playlist.GetPlayable() > 0)
    {
      return bPlayPrevious ? PlayPrevious() : PlayNext();
    }
    // none? then abort playback
    else
    {
      CLog::Log(LOGDEBUG,"Playlist Player: no more playable items... aborting playback");
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      g_windowManager.SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = PLAYLIST_NONE;
      return false;
    }
  }

  // reset the start offset of this item
  if (item->m_lStartOffset == STARTOFFSET_RESUME)
    item->m_lStartOffset = 0;

  // TODO - move the above failure logic and the below success logic
  //        to callbacks instead so we don't rely on the return value
  //        of PlayFile()

  // consecutive error counter so reset if the current item is playing
  m_iFailedSongs = 0;
  m_failedSongsStart = 0;
  m_bPlaybackStarted = true;
  m_bPlayedFirstFile = true;
  return true;
}

void CPlayListPlayer::SetCurrentSong(int iSong)
{
  if (iSong >= -1 && iSong < GetPlaylist(m_iCurrentPlayList).size())
    m_iCurrentSong = iSong;
}

int CPlayListPlayer::GetCurrentSong() const
{
  return m_iCurrentSong;
}

int CPlayListPlayer::GetCurrentPlaylist() const
{
  return m_iCurrentPlayList;
}

void CPlayListPlayer::SetCurrentPlaylist(int iPlaylist)
{
  if (iPlaylist == m_iCurrentPlayList)
    return;

  // changing the current playlist while party mode is on
  // disables party mode
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  m_iCurrentPlayList = iPlaylist;
  m_bPlayedFirstFile = false;
}

void CPlayListPlayer::ClearPlaylist(int iPlaylist)
{
  // clear our applications playlist file
  g_application.m_strPlayListFile.clear();

  CPlayList& playlist = GetPlaylist(iPlaylist);
  playlist.Clear();

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  g_windowManager.SendMessage(msg);
}

CPlayList& CPlayListPlayer::GetPlaylist(int iPlaylist)
{
  switch ( iPlaylist )
  {
  case PLAYLIST_MUSIC:
    return *m_PlaylistMusic;
    break;
  case PLAYLIST_VIDEO:
    return *m_PlaylistVideo;
    break;
  default:
    m_PlaylistEmpty->Clear();
    return *m_PlaylistEmpty;
    break;
  }
}

const CPlayList& CPlayListPlayer::GetPlaylist(int iPlaylist) const
{
  switch ( iPlaylist )
  {
  case PLAYLIST_MUSIC:
    return *m_PlaylistMusic;
    break;
  case PLAYLIST_VIDEO:
    return *m_PlaylistVideo;
    break;
  default:
    // NOTE: This playlist may not be empty if the caller of the non-const version alters it!
    return *m_PlaylistEmpty;
    break;
  }
}

int CPlayListPlayer::RemoveDVDItems()
{
  int nRemovedM = m_PlaylistMusic->RemoveDVDItems();
  int nRemovedV = m_PlaylistVideo->RemoveDVDItems();

  return nRemovedM + nRemovedV;
}

void CPlayListPlayer::Reset()
{
  m_iCurrentSong = -1;
  m_bPlayedFirstFile = false;
  m_bPlaybackStarted = false;

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  g_windowManager.SendMessage(msg);
}

bool CPlayListPlayer::HasPlayedFirstFile() const
{
  return m_bPlayedFirstFile;
}

bool CPlayListPlayer::Repeated(int iPlaylist) const
{
  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO)
    return (m_repeatState[iPlaylist] == REPEAT_ALL);
  return false;
}

bool CPlayListPlayer::RepeatedOne(int iPlaylist) const
{
  if (iPlaylist == PLAYLIST_MUSIC || iPlaylist == PLAYLIST_VIDEO)
    return (m_repeatState[iPlaylist] == REPEAT_ONE);
  return false;
}

void CPlayListPlayer::SetShuffle(int iPlaylist, bool bYesNo, bool bNotify /* = false */)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;

  // disable shuffle in party mode
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    return;

  // do we even need to do anything?
  if (bYesNo != IsShuffled(iPlaylist))
  {
    // save the order value of the current song so we can use it find its new location later
    int iOrder = -1;
    CPlayList &playlist = GetPlaylist(iPlaylist);
    if (m_iCurrentSong >= 0 && m_iCurrentSong < playlist.size())
      iOrder = playlist[m_iCurrentSong]->m_iprogramCount;

    // shuffle or unshuffle as necessary
    if (bYesNo)
      playlist.Shuffle();
    else
      playlist.UnShuffle();

    if (bNotify)
    {
      std::string shuffleStr = StringUtils::Format("%s: %s", g_localizeStrings.Get(191).c_str(), g_localizeStrings.Get(bYesNo ? 593 : 591).c_str()); // Shuffle: All/Off
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559),  shuffleStr);
    }

    // find the previous order value and fix the current song marker
    if (iOrder >= 0)
    {
      int iIndex = playlist.FindOrder(iOrder);
      if (iIndex >= 0)
        m_iCurrentSong = iIndex;
      // if iIndex < 0, something unexpected happened
      // so dont do anything
    }
  }
  
  AnnouncePropertyChanged(iPlaylist, "shuffled", IsShuffled(iPlaylist));
}

bool CPlayListPlayer::IsShuffled(int iPlaylist) const
{
  // even if shuffled, party mode says its not
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    return false;

  if (iPlaylist == PLAYLIST_MUSIC || iPlaylist == PLAYLIST_VIDEO)
    return GetPlaylist(iPlaylist).IsShuffled();

  return false;
}

void CPlayListPlayer::SetRepeat(int iPlaylist, REPEAT_STATE state, bool bNotify /* = false */)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;

  // disable repeat in party mode
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    state = REPEAT_NONE;

  // notify the user if there was a change in the repeat state
  if (m_repeatState[iPlaylist] != state && bNotify)
  {
    int iLocalizedString;
    if (state == REPEAT_NONE)
      iLocalizedString = 595; // Repeat: Off
    else if (state == REPEAT_ONE)
      iLocalizedString = 596; // Repeat: One
    else
      iLocalizedString = 597; // Repeat: All
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559), g_localizeStrings.Get(iLocalizedString));
  }

  m_repeatState[iPlaylist] = state;

  CVariant data;
  switch (state)
  {
  case REPEAT_ONE:
    data = "one";
    break;
  case REPEAT_ALL:
    data = "all";
    break;
  default:
    data = "off";
    break;
  }
  AnnouncePropertyChanged(iPlaylist, "repeat", data);
}

REPEAT_STATE CPlayListPlayer::GetRepeat(int iPlaylist) const
{
  if (iPlaylist == PLAYLIST_MUSIC || iPlaylist == PLAYLIST_VIDEO)
    return m_repeatState[iPlaylist];
  return REPEAT_NONE;
}

void CPlayListPlayer::ReShuffle(int iPlaylist, int iPosition)
{
  // playlist has not played yet so shuffle the entire list
  // (this only really works for new video playlists)
  if (!GetPlaylist(iPlaylist).WasPlayed())
  {
    GetPlaylist(iPlaylist).Shuffle();
  }
  // we're trying to shuffle new items into the curently playing playlist
  // so we shuffle starting at two positions below the current item
  else if (iPlaylist == m_iCurrentPlayList)
  {
    if (
      (g_application.m_pPlayer->IsPlayingAudio() && iPlaylist == PLAYLIST_MUSIC) ||
      (g_application.m_pPlayer->IsPlayingVideo() && iPlaylist == PLAYLIST_VIDEO)
      )
    {
      g_playlistPlayer.GetPlaylist(iPlaylist).Shuffle(m_iCurrentSong + 2);
    }
  }
  // otherwise, shuffle from the passed position
  // which is the position of the first new item added
  else
  {
    g_playlistPlayer.GetPlaylist(iPlaylist).Shuffle(iPosition);
  }
}

void CPlayListPlayer::Add(int iPlaylist, CPlayList& playlist)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(playlist);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
}

void CPlayListPlayer::Add(int iPlaylist, const CFileItemPtr &pItem)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(pItem);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
}

void CPlayListPlayer::Add(int iPlaylist, CFileItemList& items)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(items);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
}

void CPlayListPlayer::Insert(int iPlaylist, CPlayList& playlist, int iIndex)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Insert(playlist, iIndex);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
  else if (m_iCurrentPlayList == iPlaylist && m_iCurrentSong >= iIndex)
    m_iCurrentSong++;
}

void CPlayListPlayer::Insert(int iPlaylist, const CFileItemPtr &pItem, int iIndex)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Insert(pItem, iIndex);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
  else if (m_iCurrentPlayList == iPlaylist && m_iCurrentSong >= iIndex)
    m_iCurrentSong++;
}

void CPlayListPlayer::Insert(int iPlaylist, CFileItemList& items, int iIndex)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Insert(items, iIndex);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
  else if (m_iCurrentPlayList == iPlaylist && m_iCurrentSong >= iIndex)
    m_iCurrentSong++;
}

void CPlayListPlayer::Remove(int iPlaylist, int iPosition)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  list.Remove(iPosition);
  if (m_iCurrentPlayList == iPlaylist && m_iCurrentSong >= iPosition)
    m_iCurrentSong--;

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  g_windowManager.SendMessage(msg);
}

void CPlayListPlayer::Clear()
{
  if (m_PlaylistMusic)
    m_PlaylistMusic->Clear();
  if (m_PlaylistVideo)
    m_PlaylistVideo->Clear();
  if (m_PlaylistEmpty)
    m_PlaylistEmpty->Clear();
}

void CPlayListPlayer::Swap(int iPlaylist, int indexItem1, int indexItem2)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;

  CPlayList& list = GetPlaylist(iPlaylist);
  if (list.Swap(indexItem1, indexItem2) && iPlaylist == m_iCurrentPlayList)
  {
    if (m_iCurrentSong == indexItem1)
      m_iCurrentSong = indexItem2;
    else if (m_iCurrentSong == indexItem2)
      m_iCurrentSong = indexItem1;
  }

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  g_windowManager.SendMessage(msg);
}

void CPlayListPlayer::AnnouncePropertyChanged(int iPlaylist, const std::string &strProperty, const CVariant &value)
{
  if (strProperty.empty() || value.isNull() ||
     (iPlaylist == PLAYLIST_VIDEO && !g_application.m_pPlayer->IsPlayingVideo()) ||
     (iPlaylist == PLAYLIST_MUSIC && !g_application.m_pPlayer->IsPlayingAudio()))
    return;

  CVariant data;
  data["player"]["playerid"] = iPlaylist;
  data["property"][strProperty] = value;
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Player, "xbmc", "OnPropertyChanged", data);
}

int PLAYLIST::CPlayListPlayer::GetMessageMask()
{
  return TMSG_MASK_PLAYLISTPLAYER;
}

void PLAYLIST::CPlayListPlayer::OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_PLAYLISTPLAYER_PLAY:
    if (pMsg->param1 != -1)
      Play(pMsg->param1);
    else
      Play();
    break;

  case TMSG_PLAYLISTPLAYER_PLAY_SONG_ID:
    if (pMsg->param1 != -1)
    {
      bool *result = (bool*)pMsg->lpVoid;
      *result = PlaySongId(pMsg->param1);
    }
    else
      Play();
    break;

  case TMSG_PLAYLISTPLAYER_NEXT:
    PlayNext();
    break;

  case TMSG_PLAYLISTPLAYER_PREV:
    PlayPrevious();
    break;

  case TMSG_PLAYLISTPLAYER_ADD:
    if (pMsg->lpVoid)
    {
      CFileItemList *list = (CFileItemList *)pMsg->lpVoid;

      Add(pMsg->param1, (*list));
      delete list;
    }
    break;

  case TMSG_PLAYLISTPLAYER_INSERT:
    if (pMsg->lpVoid)
    {
      CFileItemList *list = (CFileItemList *)pMsg->lpVoid;
      Insert(pMsg->param1, (*list), pMsg->param2);
      delete list;
    }
    break;

  case TMSG_PLAYLISTPLAYER_REMOVE:
    if (pMsg->param1 != -1)
      Remove(pMsg->param1, pMsg->param2);
    break;

  case TMSG_PLAYLISTPLAYER_CLEAR:
    ClearPlaylist(pMsg->param1);
    break;

  case TMSG_PLAYLISTPLAYER_SHUFFLE:
    SetShuffle(pMsg->param1, pMsg->param2 > 0);
    break;

  case TMSG_PLAYLISTPLAYER_REPEAT:
    SetRepeat(pMsg->param1, (PLAYLIST::REPEAT_STATE)pMsg->param2);
    break;

  case TMSG_PLAYLISTPLAYER_GET_ITEMS:
    if (pMsg->lpVoid)
    {
      PLAYLIST::CPlayList playlist = GetPlaylist(pMsg->param1);
      CFileItemList *list = static_cast<CFileItemList*>(pMsg->lpVoid);

      for (int i = 0; i < playlist.size(); i++)
        list->Add(std::make_shared<CFileItem>(*playlist[i]));
    }
    break;

  case TMSG_PLAYLISTPLAYER_SWAP:
    if (pMsg->lpVoid)
    {
      auto indexes = static_cast<std::vector<int>*>(pMsg->lpVoid);
      if (indexes->size() == 2)
        Swap(pMsg->param1, indexes->at(0), indexes->at(1));
      delete indexes;
    }
    break;

  case TMSG_MEDIA_PLAY:
  {
    // first check if we were called from the PlayFile() function
    if (pMsg->lpVoid && pMsg->param2 == 0)
    {
      CFileItem *item = (CFileItem *)pMsg->lpVoid;
      g_application.PlayFile(*item, pMsg->param1 != 0);
      delete item;
      return;
    }
    // restore to previous window if needed
    if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
      g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
      g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      g_windowManager.PreviousWindow();

    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();

    //g_application.StopPlaying();
    // play file
    if (pMsg->lpVoid)
    {
      CFileItemList *list = (CFileItemList *)pMsg->lpVoid;

      if (list->Size() > 0)
      {
        int playlist = PLAYLIST_MUSIC;
        for (int i = 0; i < list->Size(); i++)
        {
          if ((*list)[i]->IsVideo())
          {
            playlist = PLAYLIST_VIDEO;
            break;
          }
        }

        ClearPlaylist(playlist);
        SetCurrentPlaylist(playlist);
        //For single item lists try PlayMedia. This covers some more cases where a playlist is not appropriate
        //It will fall through to PlayFile
        if (list->Size() == 1 && !(*list)[0]->IsPlayList())
          g_application.PlayMedia(*((*list)[0]), playlist);
        else
        {
          // Handle "shuffled" option if present
          if (list->HasProperty("shuffled") && list->GetProperty("shuffled").isBoolean())
            SetShuffle(playlist, list->GetProperty("shuffled").asBoolean(), false);
          // Handle "repeat" option if present
          if (list->HasProperty("repeat") && list->GetProperty("repeat").isInteger())
            SetRepeat(playlist, (PLAYLIST::REPEAT_STATE)list->GetProperty("repeat").asInteger(), false);

          Add(playlist, (*list));
          Play(pMsg->param1);
        }
      }

      delete list;
    }
    else if (pMsg->param1 == PLAYLIST_MUSIC || pMsg->param1 == PLAYLIST_VIDEO)
    {
      if (GetCurrentPlaylist() != pMsg->param1)
        SetCurrentPlaylist(pMsg->param1);

      CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_PLAY, pMsg->param2);
    }
  }
  break;

  case TMSG_MEDIA_RESTART:
    g_application.Restart(true);
    break;

  case TMSG_MEDIA_STOP:
  {
    // restore to previous window if needed
    bool stopSlideshow = true;
    bool stopVideo = true;
    bool stopMusic = true;
    if (pMsg->param1 >= PLAYLIST_MUSIC && pMsg->param1 <= PLAYLIST_PICTURE)
    {
      stopSlideshow = (pMsg->param1 == PLAYLIST_PICTURE);
      stopVideo = (pMsg->param1 == PLAYLIST_VIDEO);
      stopMusic = (pMsg->param1 == PLAYLIST_MUSIC);
    }

    if ((stopSlideshow && g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) ||
      (stopVideo && g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO) ||
      (stopMusic && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION))
      g_windowManager.PreviousWindow();

    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();

    // stop playing file
    if (g_application.m_pPlayer->IsPlaying()) g_application.StopPlaying();
  }
  break;

  case TMSG_MEDIA_PAUSE:
    if (g_application.m_pPlayer->HasPlayer())
    {
      g_application.ResetScreenSaver();
      g_application.WakeUpScreenSaverAndDPMS();
      g_application.m_pPlayer->Pause();
    }
    break;

  case TMSG_MEDIA_UNPAUSE:
    if (g_application.m_pPlayer->IsPausedPlayback())
    {
      g_application.ResetScreenSaver();
      g_application.WakeUpScreenSaverAndDPMS();
      g_application.m_pPlayer->Pause();
    }
    break;

  case TMSG_MEDIA_PAUSE_IF_PLAYING:
    if (g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
    {
      g_application.ResetScreenSaver();
      g_application.WakeUpScreenSaverAndDPMS();
      g_application.m_pPlayer->Pause();
    }
    break;
  default:
    break;
  }
}


