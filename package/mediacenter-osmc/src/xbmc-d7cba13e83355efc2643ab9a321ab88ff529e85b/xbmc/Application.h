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

#include "system.h" // for HAS_DVD_DRIVE et. al.
#include "XBApplicationEx.h"

#include "guilib/IMsgTargetCallback.h"
#include "guilib/Resolution.h"
#include "utils/GlobalsHandling.h"
#include "messaging/IMessageTarget.h"

#include <map>
#include <memory>
#include <string>

class CAction;
class CFileItem;
class CFileItemList;
class CKey;
+
+  // allow memory mapped IO with sqlite
+
+  bool m_sqlitemm;
+  bool IsSqliteMm() const { return m_sqlitemm; }
+  void SetSqliteMmTrue() { m_sqlitemm = true; }
+

namespace ADDON
{
  class CSkinInfo;
  class IAddon;
  typedef std::shared_ptr<IAddon> AddonPtr;
}

namespace MEDIA_DETECT
{
  class CAutorun;
}

#include "cores/IPlayerCallback.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "PlayListPlayer.h"
#include "settings/lib/ISettingsHandler.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISubSettings.h"
#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
#include "storage/DetectDVDType.h"
#endif
#ifdef TARGET_WINDOWS
#include "win32/WIN32Util.h"
#endif
#include "utils/Stopwatch.h"
#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceStats.h"
#endif
#include "windowing/XBMC_events.h"
#include "threads/Thread.h"

#include "ApplicationPlayer.h"
#include "interfaces/IActionListener.h"

class CSeekHandler;
class CKaraokeLyricsManager;
class CInertialScrollingHandler;
class DPMSSupport;
class CSplash;
class CBookmark;
class CNetwork;

namespace VIDEO
{
  class CVideoInfoScanner;
}

namespace MUSIC_INFO
{
  class CMusicInfoScanner;
}

#define VOLUME_MINIMUM 0.0f        // -60dB
#define VOLUME_MAXIMUM 1.0f        // 0dB
#define VOLUME_DYNAMIC_RANGE 90.0f // 60dB
#define VOLUME_CONTROL_STEPS 90    // 90 steps

// replay gain settings struct for quick access by the player multiple
// times per second (saves doing settings lookup)
struct ReplayGainSettings
{
  int iPreAmp;
  int iNoGainPreAmp;
  int iType;
  bool bAvoidClipping;
};

class CBackgroundPlayer : public CThread
{
public:
  CBackgroundPlayer(const CFileItem &item, int iPlayList);
  virtual ~CBackgroundPlayer();
  virtual void Process();
protected:
  CFileItem *m_item;
  int       m_iPlayList;
};

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback,
                     public ISettingCallback, public ISettingsHandler, public ISubSettings,
                     public KODI::MESSAGING::IMessageTarget
{
  friend class CApplicationPlayer;
public:

  enum ESERVERS
  {
    ES_WEBSERVER = 1,
    ES_AIRPLAYSERVER,
    ES_JSONRPCSERVER,
    ES_UPNPRENDERER,
    ES_UPNPSERVER,
    ES_EVENTSERVER,
    ES_ZEROCONF
  };

  CApplication(void);
  virtual ~CApplication(void);
  virtual bool Initialize() override;
  virtual void FrameMove(bool processEvents, bool processGUI = true) override;
  virtual void Render() override;
  virtual bool RenderNoPresent();
  virtual void Preflight();
  virtual bool Create() override;
  virtual bool Cleanup() override;

  bool CreateGUI();
  bool InitWindow(RESOLUTION res = RES_INVALID);
  bool DestroyWindow();
  void StartServices();
  void StopServices();

  bool StartServer(enum ESERVERS eServer, bool bStart, bool bWait = false);

  void StartPVRManager();
  void StopPVRManager();
  bool IsCurrentThread() const;
  void Stop(int exitCode);
  void RestartApp();
  void UnloadSkin(bool forReload = false);
  bool LoadUserWindows();
  void ReloadSkin(bool confirm = false);
  const std::string& CurrentFile();
  CFileItem& CurrentFileItem();
  CFileItem& CurrentUnstackedItem();
  virtual bool OnMessage(CGUIMessage& message) override;
  PLAYERCOREID GetCurrentPlayer();
  virtual void OnPlayBackEnded() override;
  virtual void OnPlayBackStarted() override;
  virtual void OnPlayBackPaused() override;
  virtual void OnPlayBackResumed() override;
  virtual void OnPlayBackStopped() override;
  virtual void OnQueueNextItem() override;
  virtual void OnPlayBackSeek(int iTime, int seekOffset) override;
  virtual void OnPlayBackSeekChapter(int iChapter) override;
  virtual void OnPlayBackSpeedChanged(int iSpeed) override;

  virtual int  GetMessageMask() override;
  virtual void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;

  bool PlayMedia(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC);
  bool PlayMediaSync(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC);
  bool ProcessAndStartPlaylist(const std::string& strPlayList, PLAYLIST::CPlayList& playlist, int iPlaylist, int track=0);
  PlayBackRet PlayFile(const CFileItem& item, bool bRestart = false);
  void SaveFileState(bool bForeground = false);
  void UpdateFileState();
  void LoadVideoSettings(const CFileItem& item);
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  bool IsPlayingFullScreenVideo() const;
  bool IsStartingPlayback() const { return m_bPlaybackStarting; }
  bool IsFullScreen();
  bool OnAppCommand(const CAction &action);
  bool OnAction(const CAction &action);
  void CheckShutdown();
  void InhibitIdleShutdown(bool inhibit);
  bool IsIdleShutdownInhibited() const;
  // Checks whether the screensaver and / or DPMS should become active.
  void CheckScreenSaverAndDPMS();
  void CheckPlayingProgress();
  void ActivateScreenSaver(bool forceType = false);
  bool SetupNetwork();
  void CloseNetworkShares();

  void ShowAppMigrationMessage();
  virtual void Process() override;
  void ProcessSlow();
  void ResetScreenSaver();
  float GetVolume(bool percentage = true) const;
  void SetVolume(float iValue, bool isPercentage = true);
  bool IsMuted() const;
  bool IsMutedInternal() const { return m_muted; }
  void ToggleMute(void);
  void SetMute(bool mute);
  void ShowVolumeBar(const CAction *action = NULL);
  int GetSubtitleDelay() const;
  int GetAudioDelay() const;
  void ResetSystemIdleTimer();
  void ResetScreenSaverTimer();
  void StopScreenSaverTimer();
  // Wakes up from the screensaver and / or DPMS. Returns true if woken up.
  bool WakeUpScreenSaverAndDPMS(bool bPowerOffKeyPressed = false);
  bool WakeUpScreenSaver(bool bPowerOffKeyPressed = false);
  /*!
   \brief Returns the total time in fractional seconds of the currently playing media

   Beware that this method returns fractional seconds whereas IPlayer::GetTotalTime() returns milliseconds.
   */
  double GetTotalTime() const;
  /*!
   \brief Returns the current time in fractional seconds of the currently playing media

   Beware that this method returns fractional seconds whereas IPlayer::GetTime() returns milliseconds.
   */
  double GetTime() const;
  float GetPercentage() const;

  // Get the percentage of data currently cached/buffered (aq/vq + FileCache) from the input stream if applicable.
  float GetCachePercentage() const;

  void SeekPercentage(float percent);
  void SeekTime( double dTime = 0.0 );

  void StopShutdownTimer();
  void ResetShutdownTimers();

  void StopVideoScan();
  void StopMusicScan();
  bool IsMusicScanning() const;
  bool IsVideoScanning() const;

  /*!
   \brief Starts a video library cleanup.
   \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
   */
  void StartVideoCleanup(bool userInitiated = true);

  /*!
   \brief Starts a video library update.
   \param path The path to scan or "" (empty string) for a global scan.
   \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
   \param scanAll Whether to scan everything not already scanned (regardless of whether the user normally doesn't want a folder scanned).
   */
  void StartVideoScan(const std::string &path, bool userInitiated = true, bool scanAll = false);

  /*!
  \brief Starts a music library cleanup.
  \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
  */
  void StartMusicCleanup(bool userInitiated = true);

  /*!
   \brief Starts a music library update.
   \param path The path to scan or "" (empty string) for a global scan.
   \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
   \param flags Flags for controlling the scanning process.  See xbmc/music/infoscanner/MusicInfoScanner.h for possible values.
   */
  void StartMusicScan(const std::string &path, bool userInitiated = true, int flags = 0);
  void StartMusicAlbumScan(const std::string& strDirectory, bool refresh = false);
  void StartMusicArtistScan(const std::string& strDirectory, bool refresh = false);

  void UpdateLibraries();
  void CheckMusicPlaylist();

  bool ExecuteXBMCAction(std::string action, const CGUIListItemPtr &item = NULL);

  static bool OnEvent(XBMC_Event& newEvent);

  CNetwork& getNetwork();
#ifdef HAS_PERFORMANCE_SAMPLE
  CPerformanceStats &GetPerformanceStats();
#endif

#ifdef HAS_DVD_DRIVE
  MEDIA_DETECT::CAutorun* m_Autorun;
#endif

#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
  MEDIA_DETECT::CDetectDVDMedia m_DetectDVDType;
#endif

  CApplicationPlayer* m_pPlayer;

  inline bool IsInScreenSaver() { return m_bScreenSave; };
  inline bool IsDPMSActive() { return m_dpmsIsActive; };
  int m_iScreenSaveLock; // spiff: are we checking for a lock? if so, ignore the screensaver state, if -1 we have failed to input locks

  bool m_bPlaybackStarting;
  typedef enum
  {
    PLAY_STATE_NONE = 0,
    PLAY_STATE_STARTING,
    PLAY_STATE_PLAYING,
    PLAY_STATE_STOPPED,
    PLAY_STATE_ENDED,
  } PlayState;
  PlayState m_ePlayState;
  CCriticalSection m_playStateMutex;

  CKaraokeLyricsManager* m_pKaraokeMgr;

  PLAYERCOREID m_eForcedNextPlayer;
  std::string m_strPlayListFile;

  int GlobalIdleTime();

  void EnablePlatformDirectories(bool enable=true)
  {
    m_bPlatformDirectories = enable;
  }

  bool PlatformDirectoriesEnabled()
  {
    return m_bPlatformDirectories;
  }

  void SetStandAlone(bool value);

  bool IsStandAlone()
  {
    return m_bStandalone;
  }

  void SetEnableLegacyRes(bool value)
  {
    m_bEnableLegacyRes = value;
  }

  bool IsEnableLegacyRes()
  {
    return m_bEnableLegacyRes;
  }

  void SetEnableTestMode(bool value)
  {
    m_bTestMode = value;
  }

  bool IsEnableTestMode()
  {
    return m_bTestMode;
  }

  bool IsAppFocused() const { return m_AppFocused; }

  void Minimize();
  bool ToggleDPMS(bool manual);

  float GetDimScreenSaverLevel() const;

  bool SwitchToFullScreen(bool force = false);

  void SetRenderGUI(bool renderGUI) override;
  bool GetRenderGUI() const { return m_renderGUI; };

  bool SetLanguage(const std::string &strLanguage);
  bool LoadLanguage(bool reload);

  ReplayGainSettings& GetReplayGainSettings() { return m_replayGainSettings; }

  void SetLoggingIn(bool loggingIn) { m_loggingIn = loggingIn; }
  
  /*!
   \brief Register an action listener.
   \param listener The listener to register
   */
  void RegisterActionListener(IActionListener *listener);
  /*!
   \brief Unregister an action listener.
   \param listener The listener to unregister
   */
  void UnregisterActionListener(IActionListener *listener);

protected:
  virtual bool OnSettingsSaving() const override;

  virtual bool Load(const TiXmlNode *settings) override;
  virtual bool Save(TiXmlNode *settings) const override;

  virtual void OnSettingChanged(const CSetting *setting) override;
  virtual void OnSettingAction(const CSetting *setting) override;
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode) override;

  bool LoadSkin(const std::string& skinID);
  bool LoadSkin(const std::shared_ptr<ADDON::CSkinInfo>& skin);
  
  /*!
   \brief Delegates the action to all registered action handlers.
   \param action The action
   \return true, if the action was taken by one of the action listener.
   */
  bool NotifyActionListeners(const CAction &action) const;

  bool m_skinReverting;
  std::string m_skinReloadSettingIgnore;

  bool m_loggingIn;

#if defined(TARGET_DARWIN_IOS)
  friend class CWinEventsIOS;
#endif
#if defined(TARGET_ANDROID)
  friend class CWinEventsAndroid;
#endif
  // screensaver
  bool m_bScreenSave;
  ADDON::AddonPtr m_screenSaver;

  // timer information
#ifdef TARGET_WINDOWS
  CWinIdleTimer m_idleTimer;
  CWinIdleTimer m_screenSaverTimer;
#else
  CStopWatch m_idleTimer;
  CStopWatch m_screenSaverTimer;
#endif
  CStopWatch m_restartPlayerTimer;
  CStopWatch m_frameTime;
  CStopWatch m_navigationTimer;
  CStopWatch m_slowTimer;
  CStopWatch m_shutdownTimer;

  bool m_bInhibitIdleShutdown;

  DPMSSupport* m_dpms;
  bool m_dpmsIsActive;
  bool m_dpmsIsManual;

  CFileItemPtr m_itemCurrentFile;
  CFileItemList* m_currentStack;
  CFileItemPtr m_stackFileItemToUpdate;

  std::string m_prevMedia;
  ThreadIdentifier m_threadID;       // application thread ID.  Used in applicationMessanger to know where we are firing a thread with delay from.
  bool m_bInitializing;
  bool m_bPlatformDirectories;

  CBookmark& m_progressTrackingVideoResumeBookmark;
  CFileItemPtr m_progressTrackingItem;
  bool m_progressTrackingPlayCountUpdate;

  int m_currentStackPosition;
  int m_nextPlaylistItem;

  bool m_bPresentFrame;
  unsigned int m_lastFrameTime;
  unsigned int m_lastRenderTime;
  bool m_skipGuiRender;

  bool m_bStandalone;
  bool m_bEnableLegacyRes;
  bool m_bTestMode;
  bool m_bSystemScreenSaverEnable;

  MUSIC_INFO::CMusicInfoScanner *m_musicInfoScanner;

  bool m_muted;
  float m_volumeLevel;

  void Mute();
  void UnMute();

  void SetHardwareVolume(float hardwareVolume);

  void VolumeChanged() const;

  PlayBackRet PlayStack(const CFileItem& item, bool bRestart);
  int  GetActiveWindowID(void);

  float NavigationIdleTime();

  bool InitDirectoriesLinux();
  bool InitDirectoriesOSX();
  bool InitDirectoriesWin32();
  void CreateUserDirs();

  /*! \brief Helper method to determine how to handle TMSG_SHUTDOWN
  */
  void HandleShutdownMessage();

  CInertialScrollingHandler *m_pInertialScrollingHandler;
  CNetwork    *m_network;
#ifdef HAS_PERFORMANCE_SAMPLE
  CPerformanceStats m_perfStats;
#endif

  ReplayGainSettings m_replayGainSettings;
  
  std::vector<IActionListener *> m_actionListeners;

  bool m_fallbackLanguageLoaded;
  
private:
  CCriticalSection                m_critSection;                 /*!< critical section for all changes to this class, except for changes to triggers */
};

XBMC_GLOBAL_REF(CApplication,g_application);
#define g_application XBMC_GLOBAL_USE(CApplication)
