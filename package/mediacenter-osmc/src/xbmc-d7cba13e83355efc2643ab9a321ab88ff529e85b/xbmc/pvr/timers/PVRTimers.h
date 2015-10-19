#pragma once
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

#include <map>
#include <memory>

#include "addons/include/xbmc_pvr_types.h"
#include "PVRTimerInfoTag.h"
#include "utils/Observer.h"
#include "XBDateTime.h"

class CFileItem;
class CFileItemList;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace EPG
{
  class CEpgInfoTag;
}

namespace PVR
{
  class CPVRTimersPath;

  class CPVRTimers : public Observer,
                     public Observable
  {
  public:
    CPVRTimers(void);
    virtual ~CPVRTimers(void);

    /**
     * (re)load the timers from the clients.
     * True when loaded, false otherwise.
     */
    bool Load(void);

    /**
     * @brief refresh the channel list from the clients.
     */
    bool Update(void);

    /**
     * Add a timer entry to this container, called by the client callback.
     */
    bool UpdateFromClient(const CPVRTimerInfoTagPtr &timer);

    /*!
     * @return The timer that will be active next (state scheduled), or an empty fileitemptr if none.
     */
    CFileItemPtr GetNextActiveTimer(void) const;

    /*!
     * @return All timers that are active (states scheduled or recording)
     */
    std::vector<CFileItemPtr> GetActiveTimers(void) const;

    /*!
     * Get all timers
     * @param items The list to add the timers to
     */
    void GetAll(CFileItemList& items) const;

    /*!
     * @return True when there is at least one timer that is active (states scheduled or recording), false otherwise.
     */
    bool HasActiveTimers(void) const;

    /*!
     * @return The amount of timers that are active (states scheduled or recording)
     */
    int AmountActiveTimers(void) const;

    /*!
     * @return All timers that are recording
     */
    std::vector<CFileItemPtr> GetActiveRecordings(void) const;

    /*!
     * @return True when recording, false otherwise.
     */
    bool IsRecording(void) const;

    /*!
     * @brief Check if a recording is running on the given channel.
     * @param channel The channel to check.
     * @return True when recording, false otherwise.
     */
    bool IsRecordingOnChannel(const CPVRChannel &channel) const;

    /*!
     * @return The amount of timers that are currently recording
     */
    int AmountActiveRecordings(void) const;

    /*!
     * @brief Get all timers for the given path.
     * @param strPath The vfs path to get the timers for.
     * @param items The results.
     * @return True when the path was valid, false otherwise.
     */
    bool GetDirectory(const std::string& strPath, CFileItemList &items) const;

    /*!
     * @brief Delete all timers on a channel.
     * @param channel The channel to delete the timers for.
     * @param bDeleteRepeating True to delete repeating events too, false otherwise.
     * @param bCurrentlyActiveOnly True to delete timers that are currently running only.
     * @return True if timers any were deleted, false otherwise.
     */
    bool DeleteTimersOnChannel(const CPVRChannelPtr &channel, bool bDeleteRepeating = true, bool bCurrentlyActiveOnly = false);

    /*!
     * @brief Create a new instant timer on a channel.
     * @param channel The channel to create the timer on.
     * @return True if the timer was created, false otherwise.
     */
    bool InstantTimer(const CPVRChannelPtr &channel);

    /*!
     * @return Next event time (timer or daily wake up)
     */
    CDateTime GetNextEventTime(void) const;

    /*!
     * @brief Add a timer to the client. Doesn't add the timer to the container. The backend will do this.
     * @return True if it was sent correctly, false if not.
     */
    static bool AddTimer(const CPVRTimerInfoTagPtr &item);

    /*!
     * @brief Delete a timer on the client. Doesn't delete the timer from the container. The backend will do this.
     * @param bDeleteSchedule Also delete repeating schedule instead of single timer only.
     * @return True if it was sent correctly, false if not.
     */
    static bool DeleteTimer(const CFileItem &item, bool bForce = false, bool bDeleteSchedule = false);

    /*!
     * @brief Rename a timer on the client. Doesn't update the timer in the container. The backend will do this.
     * @return True if it was sent correctly, false if not.
     */
    static bool RenameTimer(CFileItem &item, const std::string &strNewName);

    /**
     * @brief Update the timer on the client. Doesn't update the timer in the container. The backend will do this.
     * @return True if it was sent correctly, false if not.
     */
    static bool UpdateTimer(CFileItem &item);

    /*!
     * @brief Get the timer tag that matches the given epg tag.
     * @param item The epg tag.
     * @return The requested timer tag, or an empty fileitemptr if none was found.
     */
    CFileItemPtr GetTimerForEpgTag(const CFileItem *item) const;

    /*!
     * @brief Update the channel pointers.
     */
    void UpdateChannels(void);

    void Notify(const Observable &obs, const ObservableMessage msg);

    /*!
     * Get a timer tag given it's unique ID
     * @param iTimerId The ID to find
     * @return The tag, or an empty one when not found
     */
    CPVRTimerInfoTagPtr GetById(unsigned int iTimerId) const;

  private:
    typedef std::map<CDateTime, std::vector<CPVRTimerInfoTagPtr>* > MapTags;
    typedef std::vector<CPVRTimerInfoTagPtr> VecTimerInfoTag;

    void Unload(void);
    void UpdateEpgEvent(CPVRTimerInfoTagPtr timer);
    bool UpdateEntries(const CPVRTimers &timers);
    CPVRTimerInfoTagPtr GetByClient(int iClientId, unsigned int iClientTimerId) const;
    bool GetRootDirectory(const CPVRTimersPath &path, CFileItemList &items) const;
    bool GetSubDirectory(const CPVRTimersPath &path, CFileItemList &items) const;

    CCriticalSection  m_critSection;
    bool              m_bIsUpdating;
    MapTags           m_tags;
    unsigned int      m_iLastId;
  };

  class CPVRTimersPath
  {
  public:
    static const std::string PATH_ADDTIMER;
    static const std::string PATH_NEW;

    CPVRTimersPath(const std::string &strPath);
    CPVRTimersPath(const std::string &strPath, int iClientId, unsigned int iParentId);
    CPVRTimersPath(bool bRadio, bool bGrouped);

    bool IsValid() const { return m_bValid; }

    const std::string &GetPath() const        { return m_path; }
    bool              IsTimersRoot() const    { return m_bRoot; }
    bool              IsTimerSchedule() const { return !IsTimersRoot(); }
    bool              IsRadio() const         { return m_bRadio; }
    bool              IsGrouped() const       { return m_bGrouped; }
    int               GetClientId() const     { return m_iClientId; }
    unsigned int      GetParentId() const     { return m_iParentId; }

  private:
    bool Init(const std::string &strPath);

    std::string  m_path;
    bool         m_bValid;
    bool         m_bRoot;
    bool         m_bRadio;
    bool         m_bGrouped;
    int          m_iClientId;
    unsigned int m_iParentId;
  };
}
