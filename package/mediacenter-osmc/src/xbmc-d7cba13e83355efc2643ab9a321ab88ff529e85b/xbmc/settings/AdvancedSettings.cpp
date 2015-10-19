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

#include "AdvancedSettings.h"

#include <climits>
#include <algorithm>
#include <string>
#include <vector>

#include "addons/AddonManager.h"
#include "addons/AudioDecoder.h"
#include "addons/IAddon.h"
#include "Application.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "LangInfo.h"
#include "network/DNSNameCache.h"
#include "profiles/ProfilesManager.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "settings/SettingUtils.h"
#include "system.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

#if defined(TARGET_DARWIN_IOS)
#include "osx/DarwinUtils.h"
#endif

using namespace ADDON;
using namespace XFILE;

CAdvancedSettings::CAdvancedSettings()
{
  m_initialized = false;
  m_fullScreen = false;
}

void CAdvancedSettings::OnSettingsLoaded()
{
  // load advanced settings
  Load();

  // default players?
  CLog::Log(LOGNOTICE, "Default DVD Player: %s", m_videoDefaultDVDPlayer.c_str());
  CLog::Log(LOGNOTICE, "Default Video Player: %s", m_videoDefaultPlayer.c_str());
  CLog::Log(LOGNOTICE, "Default Audio Player: %s", m_audioDefaultPlayer.c_str());

  // setup any logging...
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_DEBUG_SHOWLOGINFO))
  {
    m_logLevel = std::max(m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
    CLog::Log(LOGNOTICE, "Enabled debug logging due to GUI setting (%d)", m_logLevel);
  }
  else
  {
    m_logLevel = std::min(m_logLevelHint, LOG_LEVEL_DEBUG/*LOG_LEVEL_NORMAL*/);
    CLog::Log(LOGNOTICE, "Disabled debug logging due to GUI setting. Level %d.", m_logLevel);
  }
  CLog::SetLogLevel(m_logLevel);

  m_extraLogEnabled = CSettings::GetInstance().GetBool(CSettings::SETTING_DEBUG_EXTRALOGGING);
  setExtraLogLevel(CSettings::GetInstance().GetList(CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL));
}

void CAdvancedSettings::OnSettingsUnloaded()
{
  m_initialized = false;
}

void CAdvancedSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_DEBUG_SHOWLOGINFO)
    SetDebugMode(((CSettingBool*)setting)->GetValue());
  else if (settingId == CSettings::SETTING_DEBUG_EXTRALOGGING)
    m_extraLogEnabled = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL)
    setExtraLogLevel(CSettingUtils::GetList(static_cast<const CSettingList*>(setting)));
}

void CAdvancedSettings::Initialize()
{
  if (m_initialized)
    return;

  m_audioHeadRoom = 0;
  m_ac3Gain = 12.0f;
  m_audioApplyDrc = -1.0f;
  m_dvdplayerIgnoreDTSinWAV = false;

  //default hold time of 25 ms, this allows a 20 hertz sine to pass undistorted
  m_limiterHold = 0.025f;
  m_limiterRelease = 0.1f;

  m_seekSteps = { 10, 30, 60, 180, 300, 600, 1800 };

  m_omxHWAudioDecode = false;
  m_omxDecodeStartWithValidFrame = true;

  m_karaokeSyncDelayCDG = 0.0f;
  m_karaokeSyncDelayLRC = 0.0f;
  m_karaokeChangeGenreForKaraokeSongs = false;
  m_karaokeKeepDelay = true;
  m_karaokeStartIndex = 1;
  m_karaokeAlwaysEmptyOnCdgs = 1;
  m_karaokeUseSongSpecificBackground = 0;

  m_audioDefaultPlayer = "paplayer";
  m_audioPlayCountMinimumPercent = 90.0f;

  m_videoSubsDelayRange = 60;
  m_videoAudioDelayRange = 10;
  m_videoUseTimeSeeking = true;
  m_videoTimeSeekForward = 30;
  m_videoTimeSeekBackward = -30;
  m_videoTimeSeekForwardBig = 600;
  m_videoTimeSeekBackwardBig = -600;
  m_videoPercentSeekForward = 2;
  m_videoPercentSeekBackward = -2;
  m_videoPercentSeekForwardBig = 10;
  m_videoPercentSeekBackwardBig = -10;

  m_videoBlackBarColour = 0;
  m_videoPPFFmpegDeint = "linblenddeint";
  m_videoPPFFmpegPostProc = "ha:128:7,va,dr";
  m_videoDefaultPlayer = "dvdplayer";
  m_videoDefaultDVDPlayer = "dvdplayer";
  m_videoIgnoreSecondsAtStart = 3*60;
  m_videoIgnorePercentAtEnd   = 8.0f;
  m_videoPlayCountMinimumPercent = 90.0f;
  m_videoVDPAUScaling = -1;
  m_videoVAAPIforced = false;
  m_videoNonLinStretchRatio = 0.5f;
  m_videoEnableHighQualityHwScalers = false;
  m_videoAutoScaleMaxFps = 30.0f;
  m_videoDisableBackgroundDeinterlace = false;
  m_videoCaptureUseOcclusionQuery = -1; //-1 is auto detect
  m_videoVDPAUtelecine = false;
  m_videoVDPAUdeintSkipChromaHD = false;
  m_useFfmpegVda = true;
  m_DXVACheckCompatibility = false;
  m_DXVACheckCompatibilityPresent = false;
  m_DXVAForceProcessorRenderer = true;
  m_DXVANoDeintProcForProgressive = false;
  m_DXVAAllowHqScaling = true;
  m_videoFpsDetect = 1;
  m_videoBusyDialogDelay_ms = 500;
  m_stagefrightConfig.useAVCcodec = -1;
  m_stagefrightConfig.useHEVCcodec = -1;
  m_stagefrightConfig.useVC1codec = -1;
  m_stagefrightConfig.useVPXcodec = -1;
  m_stagefrightConfig.useMP4codec = -1;
  m_stagefrightConfig.useMPEG2codec = -1;
  m_stagefrightConfig.useSwRenderer = false;
  m_stagefrightConfig.useInputDTS = false;

  m_mediacodecForceSoftwareRendring = false;

  m_videoDefaultLatency = 0.0;

  m_musicUseTimeSeeking = true;
  m_musicTimeSeekForward = 10;
  m_musicTimeSeekBackward = -10;
  m_musicTimeSeekForwardBig = 60;
  m_musicTimeSeekBackwardBig = -60;
  m_musicPercentSeekForward = 1;
  m_musicPercentSeekBackward = -1;
  m_musicPercentSeekForwardBig = 10;
  m_musicPercentSeekBackwardBig = -10;

  m_slideshowPanAmount = 2.5f;
  m_slideshowZoomAmount = 5.0f;
  m_slideshowBlackBarCompensation = 20.0f;

  m_songInfoDuration = 10;

  m_cddbAddress = "freedb.freedb.org";

  m_handleMounting = false;

  m_fullScreenOnMovieStart = true;
  m_cachePath = "special://temp/";

  m_videoCleanDateTimeRegExp = "(.*[^ _\\,\\.\\(\\)\\[\\]\\-])[ _\\.\\(\\)\\[\\]\\-]+(19[0-9][0-9]|20[0-9][0-9])([ _\\,\\.\\(\\)\\[\\]\\-]|[^0-9]$)?";

  m_videoCleanStringRegExps.clear();
  m_videoCleanStringRegExps.push_back("[ _\\,\\.\\(\\)\\[\\]\\-](ac3|dts|custom|dc|remastered|divx|divx5|dsr|dsrip|dutch|dvd|dvd5|dvd9|dvdrip|dvdscr|dvdscreener|screener|dvdivx|cam|fragment|fs|hdtv|hdrip|hdtvrip|internal|limited|multisubs|ntsc|ogg|ogm|pal|pdtv|proper|repack|rerip|retail|r3|r5|bd5|se|svcd|swedish|german|read.nfo|nfofix|unrated|extended|ws|telesync|ts|telecine|tc|brrip|bdrip|480p|480i|576p|576i|720p|720i|1080p|1080i|3d|hrhd|hrhdtv|hddvd|bluray|x264|h264|xvid|xvidvd|xxx|www.www|cd[1-9]|\\[.*\\])([ _\\,\\.\\(\\)\\[\\]\\-]|$)");
  m_videoCleanStringRegExps.push_back("(\\[.*\\])");

  m_moviesExcludeFromScanRegExps.clear();
  m_moviesExcludeFromScanRegExps.push_back("-trailer");
  m_moviesExcludeFromScanRegExps.push_back("[!-._ \\\\/]sample[-._ \\\\/]");
  m_moviesExcludeFromScanRegExps.push_back("[\\/](proof|subs)[\\/]");
  m_tvshowExcludeFromScanRegExps.push_back("[!-._ \\\\/]sample[-._ \\\\/]");

  m_folderStackRegExps.clear();
  m_folderStackRegExps.push_back("((cd|dvd|dis[ck])[0-9]+)$");

  m_videoStackRegExps.clear();
  m_videoStackRegExps.push_back("(.*?)([ _.-]*(?:cd|dvd|p(?:(?:ar)?t)|dis[ck])[ _.-]*[0-9]+)(.*?)(\\.[^.]+)$");
  m_videoStackRegExps.push_back("(.*?)([ _.-]*(?:cd|dvd|p(?:(?:ar)?t)|dis[ck])[ _.-]*[a-d])(.*?)(\\.[^.]+)$");
  m_videoStackRegExps.push_back("(.*?)([ ._-]*[a-d])(.*?)(\\.[^.]+)$");
  // This one is a bit too greedy to enable by default.  It will stack sequels
  // in a flat dir structure, but is perfectly safe in a dir-per-vid one.
  //m_videoStackRegExps.push_back("(.*?)([ ._-]*[0-9])(.*?)(\\.[^.]+)$");

  m_tvshowEnumRegExps.clear();
  // foo.s01.e01, foo.s01_e01, S01E02 foo, S01 - E02
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"s([0-9]+)[ ._-]*e([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([^\\\\/]*)$"));
  // foo.ep01, foo.EP_01, foo.E01
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"[\\._ -]()e(?:p[ ._-]?)?([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([^\\\\/]*)$"));
  // foo.yyyy.mm.dd.* (byDate=true)
  m_tvshowEnumRegExps.push_back(TVShowRegexp(true,"([0-9]{4})[\\.-]([0-9]{2})[\\.-]([0-9]{2})"));
  // foo.mm.dd.yyyy.* (byDate=true)
  m_tvshowEnumRegExps.push_back(TVShowRegexp(true,"([0-9]{2})[\\.-]([0-9]{2})[\\.-]([0-9]{4})"));
  // foo.1x09* or just /1x09*
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"[\\\\/\\._ \\[\\(-]([0-9]+)x([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([^\\\\/]*)$"));
  // foo.103*, 103 foo
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"[\\\\/\\._ -]([0-9]+)([0-9][0-9](?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([\\._ -][^\\\\/]*)$"));
  // Part I, Pt.VI, Part 1
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,"[\\/._ -]p(?:ar)?t[_. -]()([ivx]+|[0-9]+)([._ -][^\\/]*)$"));

  m_tvshowMultiPartEnumRegExp = "^[-_ex]+([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)";

  m_remoteDelay = 3;
  m_controllerDeadzone = 0.2f;

  m_playlistAsFolders = true;
  m_detectAsUdf = false;

  m_fanartRes = 1080;
  m_imageRes = 720;
  m_useDDSFanart = false;
  m_imageScalingAlgorithm = CPictureScalingAlgorithm::Default;

  m_sambaclienttimeout = 10;
  m_sambadoscodepage = "";
  m_sambastatfiles = true;

  m_bHTTPDirectoryStatFilesize = false;

  m_bFTPThumbs = false;

  m_musicThumbs = "folder.jpg|Folder.jpg|folder.JPG|Folder.JPG|cover.jpg|Cover.jpg|cover.jpeg|thumb.jpg|Thumb.jpg|thumb.JPG|Thumb.JPG";
  m_fanartImages = "fanart.jpg|fanart.png";

  m_bMusicLibraryAllItemsOnBottom = false;
  m_bMusicLibraryCleanOnUpdate = false;
  m_iMusicLibraryRecentlyAddedItems = 25;
  m_strMusicLibraryAlbumFormat = "";
  m_prioritiseAPEv2tags = false;
  m_musicItemSeparator = " / ";
  m_videoItemSeparator = " / ";
  m_iMusicLibraryDateAdded = 1; // prefer mtime over ctime and current time

  m_bVideoLibraryAllItemsOnBottom = false;
  m_iVideoLibraryRecentlyAddedItems = 25;
  m_bVideoLibraryHideEmptySeries = false;
  m_bVideoLibraryCleanOnUpdate = false;
  m_bVideoLibraryUseFastHash = true;
  m_bVideoLibraryExportAutoThumbs = false;
  m_bVideoLibraryImportWatchedState = false;
  m_bVideoLibraryImportResumePoint = false;
  m_bVideoScannerIgnoreErrors = false;
  m_iVideoLibraryDateAdded = 1; // prefer mtime over ctime and current time

  m_iEpgLingerTime = 60 * 24;           /* keep 24 hours by default */
  m_iEpgUpdateCheckInterval = 300; /* check if tables need to be updated every 5 minutes */
  m_iEpgCleanupInterval = 900;     /* remove old entries from the EPG every 15 minutes */
  m_iEpgActiveTagCheckInterval = 60; /* check for updated active tags every minute */
  m_iEpgRetryInterruptedUpdateInterval = 30; /* retry an interrupted epg update after 30 seconds */
  m_iEpgUpdateEmptyTagsInterval = 60; /* override user selectable EPG update interval for empty EPG tags */
  m_bEpgDisplayUpdatePopup = true; /* display a progress popup while updating EPG data from clients */
  m_bEpgDisplayIncrementalUpdatePopup = false; /* also display a progress popup while doing incremental EPG updates */

  m_bEdlMergeShortCommBreaks = false;      // Off by default
  m_iEdlMaxCommBreakLength = 8 * 30 + 10;  // Just over 8 * 30 second commercial break.
  m_iEdlMinCommBreakLength = 3 * 30;       // 3 * 30 second commercial breaks.
  m_iEdlMaxCommBreakGap = 4 * 30;          // 4 * 30 second commercial breaks.
  m_iEdlMaxStartGap = 5 * 60;              // 5 minutes.
  m_iEdlCommBreakAutowait = 0;             // Off by default
  m_iEdlCommBreakAutowind = 0;             // Off by default

  m_curlconnecttimeout = 10;
  m_curllowspeedtime = 20;
  m_curlretries = 2;
  m_curlDisableIPV6 = false;      //Certain hardware/OS combinations have trouble
                                  //with ipv6.

  m_startFullScreen = false;
  m_showExitButton = true;
  m_splashImage = true;

  m_playlistRetries = 100;
  m_playlistTimeout = 20; // 20 seconds timeout
  m_GLRectangleHack = false;
  m_iSkipLoopFilter = 0;
  m_RestrictCapsMask = 0;
  m_sleepBeforeFlip = 0;
  m_bVirtualShares = true;
  m_bAllowDeferredRendering = true;

//caused lots of jerks
//#ifdef TARGET_WINDOWS
//  m_ForcedSwapTime = 2.0;
//#else
  m_ForcedSwapTime = 0.0;
//#endif

  m_cpuTempCmd = "";
  m_gpuTempCmd = "";
#if defined(TARGET_DARWIN)
  // default for osx is fullscreen always on top
  m_alwaysOnTop = true;
#else
  // default for windows is not always on top
  m_alwaysOnTop = false;
#endif

  m_iPVRTimeCorrection             = 0;
  m_iPVRInfoToggleInterval         = 3000;
  m_iPVRMinVideoCacheLevel         = 5;
  m_iPVRMinAudioCacheLevel         = 10;
  m_bPVRCacheInDvdPlayer           = true;
  m_bPVRChannelIconsAutoScan       = true;
  m_bPVRAutoScanIconsUserSet       = false;
  m_iPVRNumericChannelSwitchTimeout = 1000;

  m_cacheMemBufferSize = 1024 * 1024 * 20;
  m_networkBufferMode = 0; // Default (buffer all internet streams/filesystems)
  // the following setting determines the readRate of a player data
  // as multiply of the default data read rate
  m_readBufferFactor = 4.0f;
  m_addonPackageFolderSize = 200;

  m_jsonOutputCompact = true;
  m_jsonTcpPort = 9090;

  m_enableMultimediaKeys = false;

  m_canWindowed = true;
  m_guiVisualizeDirtyRegions = false;
  m_guiAlgorithmDirtyRegions = 3;
  m_guiDirtyRegionNoFlipTimeout = 0;
  m_airTunesPort = 36666;
  m_airPlayPort = 36667;

  m_databaseMusic.Reset();
  m_databaseVideo.Reset();

  m_pictureExtensions = ".png|.jpg|.jpeg|.bmp|.gif|.ico|.tif|.tiff|.tga|.pcx|.cbz|.zip|.cbr|.rar|.dng|.nef|.cr2|.crw|.orf|.arw|.erf|.3fr|.dcr|.x3f|.mef|.raf|.mrw|.pef|.sr2|.rss";
  m_musicExtensions = ".nsv|.m4a|.flac|.aac|.strm|.pls|.rm|.rma|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u|.gdm|.imf|.m15|.sfx|.uni|.ac3|.dts|.cue|.aif|.aiff|.wpl|.ape|.mac|.mpc|.mp+|.mpp|.shn|.zip|.rar|.wv|.dsp|.xsp|.xwav|.waa|.wvs|.wam|.gcm|.idsp|.mpdsp|.mss|.spt|.rsd|.sap|.cmc|.cmr|.dmc|.mpt|.mpd|.rmt|.tmc|.tm8|.tm2|.oga|.url|.pxml|.tta|.rss|.wtv|.mka|.tak|.opus|.dff|.dsf";
  m_videoExtensions = ".m4v|.3g2|.3gp|.nsv|.tp|.ts|.ty|.strm|.pls|.rm|.rmvb|.m3u|.m3u8|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.nrg|.img|.iso|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mp4|.mkv|.mk3d|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli|.flv|.rar|.001|.wpl|.zip|.vdr|.dvr-ms|.xsp|.mts|.m2t|.m2ts|.evo|.ogv|.sdp|.avs|.rec|.url|.pxml|.vc1|.h264|.rcv|.rss|.mpls|.webm|.bdmv|.wtv";
  m_subtitlesExtensions = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar|.zip";
  m_discStubExtensions = ".disc";
  // internal music extensions
  m_musicExtensions += "|.cdda";
  // internal video extensions
  m_videoExtensions += "|.pvr";

  m_stereoscopicregex_3d = "[-. _]3d[-. _]";
  m_stereoscopicregex_sbs = "[-. _]h?sbs[-. _]";
  m_stereoscopicregex_tab = "[-. _]h?tab[-. _]";

  m_videoAssFixedWorks = false;

  m_logLevelHint = m_logLevel = LOG_LEVEL_NORMAL;
  m_extraLogEnabled = false;
  m_extraLogLevels = 0;

  #if defined(TARGET_DARWIN)
    std::string logDir = getenv("HOME");
    #if defined(TARGET_DARWIN_OSX)
    logDir += "/Library/Logs/";
    #else // ios
    logDir += "/" + std::string(CDarwinUtils::GetAppRootFolder()) + "/";
    #endif
    m_logFolder = logDir;
  #else
    m_logFolder = "special://home/";              // log file location
  #endif

  m_userAgent = g_sysinfo.GetUserAgent();

  m_initialized = true;
}

bool CAdvancedSettings::Load()
{
  // NOTE: This routine should NOT set the default of any of these parameters
  //       it should instead use the versions of GetString/Integer/Float that
  //       don't take defaults in.  Defaults are set in the constructor above
  Initialize(); // In case of profile switch.
  ParseSettingsFile("special://xbmc/system/advancedsettings.xml");
  for (unsigned int i = 0; i < m_settingsFiles.size(); i++)
    ParseSettingsFile(m_settingsFiles[i]);
  ParseSettingsFile(CProfilesManager::GetInstance().GetUserDataItem("advancedsettings.xml"));

  // Add the list of disc stub extensions (if any) to the list of video extensions
  if (!m_discStubExtensions.empty())
    m_videoExtensions += "|" + m_discStubExtensions;

  return true;
}

void CAdvancedSettings::ParseSettingsFile(const std::string &file)
{
  CXBMCTinyXML advancedXML;
  if (!CFile::Exists(file))
  {
    CLog::Log(LOGNOTICE, "No settings file to load (%s)", file.c_str());
    return;
  }

  if (!advancedXML.LoadFile(file))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", file.c_str(), advancedXML.ErrorRow(), advancedXML.ErrorDesc());
    return;
  }

  TiXmlElement *pRootElement = advancedXML.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"advancedsettings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <advancedsettings> node", file.c_str());
    return;
  }

  // succeeded - tell the user it worked
  CLog::Log(LOGNOTICE, "Loaded settings file from %s", file.c_str());

  // Dump contents of AS.xml to debug log
  TiXmlPrinter printer;
  printer.SetLineBreak("\n");
  printer.SetIndent("  ");
  advancedXML.Accept(&printer);
  CLog::Log(LOGNOTICE, "Contents of %s are...\n%s", file.c_str(), printer.CStr());

  TiXmlElement *pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    XMLUtils::GetFloat(pElement, "ac3downmixgain", m_ac3Gain, -96.0f, 96.0f);
    XMLUtils::GetInt(pElement, "headroom", m_audioHeadRoom, 0, 12);
    XMLUtils::GetString(pElement, "defaultplayer", m_audioDefaultPlayer);
    // 101 on purpose - can be used to never automark as watched
    XMLUtils::GetFloat(pElement, "playcountminimumpercent", m_audioPlayCountMinimumPercent, 0.0f, 101.0f);

    XMLUtils::GetBoolean(pElement, "usetimeseeking", m_musicUseTimeSeeking);
    XMLUtils::GetInt(pElement, "timeseekforward", m_musicTimeSeekForward, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackward", m_musicTimeSeekBackward, -6000, 0);
    XMLUtils::GetInt(pElement, "timeseekforwardbig", m_musicTimeSeekForwardBig, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackwardbig", m_musicTimeSeekBackwardBig, -6000, 0);

    XMLUtils::GetInt(pElement, "percentseekforward", m_musicPercentSeekForward, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackward", m_musicPercentSeekBackward, -100, 0);
    XMLUtils::GetInt(pElement, "percentseekforwardbig", m_musicPercentSeekForwardBig, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackwardbig", m_musicPercentSeekBackwardBig, -100, 0);

    TiXmlElement* pAudioExcludes = pElement->FirstChildElement("excludefromlisting");
    if (pAudioExcludes)
      GetCustomRegexps(pAudioExcludes, m_audioExcludeFromListingRegExps);

    pAudioExcludes = pElement->FirstChildElement("excludefromscan");
    if (pAudioExcludes)
      GetCustomRegexps(pAudioExcludes, m_audioExcludeFromScanRegExps);

    XMLUtils::GetFloat(pElement, "applydrc", m_audioApplyDrc);
    XMLUtils::GetBoolean(pElement, "dvdplayerignoredtsinwav", m_dvdplayerIgnoreDTSinWAV);

    XMLUtils::GetFloat(pElement, "limiterhold", m_limiterHold, 0.0f, 100.0f);
    XMLUtils::GetFloat(pElement, "limiterrelease", m_limiterRelease, 0.001f, 100.0f);
  }

  pElement = pRootElement->FirstChildElement("omx");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "omxhwaudiodecode", m_omxHWAudioDecode);
    XMLUtils::GetBoolean(pElement, "omxdecodestartwithvalidframe", m_omxDecodeStartWithValidFrame);
  }

  pElement = pRootElement->FirstChildElement("karaoke");
  if (pElement)
  {
    XMLUtils::GetFloat(pElement, "syncdelaycdg", m_karaokeSyncDelayCDG, -3.0f, 3.0f); // keep the old name for comp
    XMLUtils::GetFloat(pElement, "syncdelaylrc", m_karaokeSyncDelayLRC, -3.0f, 3.0f);
    XMLUtils::GetBoolean(pElement, "alwaysreplacegenre", m_karaokeChangeGenreForKaraokeSongs );
    XMLUtils::GetBoolean(pElement, "storedelay", m_karaokeKeepDelay );
    XMLUtils::GetInt(pElement, "autoassignstartfrom", m_karaokeStartIndex, 1, 2000000000);
    XMLUtils::GetBoolean(pElement, "nocdgbackground", m_karaokeAlwaysEmptyOnCdgs );
    XMLUtils::GetBoolean(pElement, "lookupsongbackground", m_karaokeUseSongSpecificBackground );

    TiXmlElement* pKaraokeBackground = pElement->FirstChildElement("defaultbackground");
    if (pKaraokeBackground)
    {
      pKaraokeBackground->QueryStringAttribute("type", &m_karaokeDefaultBackgroundType);
      pKaraokeBackground->QueryStringAttribute("path", &m_karaokeDefaultBackgroundFilePath);
    }
  }

  pElement = pRootElement->FirstChildElement("video");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "assfixedworks", m_videoAssFixedWorks);
    XMLUtils::GetString(pElement, "stereoscopicregex3d", m_stereoscopicregex_3d);
    XMLUtils::GetString(pElement, "stereoscopicregexsbs", m_stereoscopicregex_sbs);
    XMLUtils::GetString(pElement, "stereoscopicregextab", m_stereoscopicregex_tab);
    XMLUtils::GetFloat(pElement, "subsdelayrange", m_videoSubsDelayRange, 10, 600);
    XMLUtils::GetFloat(pElement, "audiodelayrange", m_videoAudioDelayRange, 10, 600);
    XMLUtils::GetInt(pElement, "blackbarcolour", m_videoBlackBarColour, 0, 255);
    XMLUtils::GetString(pElement, "defaultplayer", m_videoDefaultPlayer);
    XMLUtils::GetString(pElement, "defaultdvdplayer", m_videoDefaultDVDPlayer);
    XMLUtils::GetBoolean(pElement, "fullscreenonmoviestart", m_fullScreenOnMovieStart);
    // 101 on purpose - can be used to never automark as watched
    XMLUtils::GetFloat(pElement, "playcountminimumpercent", m_videoPlayCountMinimumPercent, 0.0f, 101.0f);
    XMLUtils::GetInt(pElement, "ignoresecondsatstart", m_videoIgnoreSecondsAtStart, 0, 900);
    XMLUtils::GetFloat(pElement, "ignorepercentatend", m_videoIgnorePercentAtEnd, 0, 100.0f);

    XMLUtils::GetBoolean(pElement, "usetimeseeking", m_videoUseTimeSeeking);
    XMLUtils::GetInt(pElement, "timeseekforward", m_videoTimeSeekForward, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackward", m_videoTimeSeekBackward, -6000, 0);
    XMLUtils::GetInt(pElement, "timeseekforwardbig", m_videoTimeSeekForwardBig, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackwardbig", m_videoTimeSeekBackwardBig, -6000, 0);

    XMLUtils::GetInt(pElement, "percentseekforward", m_videoPercentSeekForward, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackward", m_videoPercentSeekBackward, -100, 0);
    XMLUtils::GetInt(pElement, "percentseekforwardbig", m_videoPercentSeekForwardBig, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackwardbig", m_videoPercentSeekBackwardBig, -100, 0);

    TiXmlElement* pVideoExcludes = pElement->FirstChildElement("excludefromlisting");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_videoExcludeFromListingRegExps);

    pVideoExcludes = pElement->FirstChildElement("excludefromscan");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_moviesExcludeFromScanRegExps);

    pVideoExcludes = pElement->FirstChildElement("excludetvshowsfromscan");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_tvshowExcludeFromScanRegExps);

    pVideoExcludes = pElement->FirstChildElement("cleanstrings");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_videoCleanStringRegExps);

    XMLUtils::GetString(pElement,"cleandatetime", m_videoCleanDateTimeRegExp);
    XMLUtils::GetString(pElement,"ppffmpegdeinterlacing",m_videoPPFFmpegDeint);
    XMLUtils::GetString(pElement,"ppffmpegpostprocessing",m_videoPPFFmpegPostProc);
    XMLUtils::GetInt(pElement,"vdpauscaling",m_videoVDPAUScaling);
    // There is a large amount of drivers implementing VAAPI in a non stable way
    // the forcevaapienabled setting let's the user decide to use it nevertheless
    XMLUtils::GetBoolean(pElement, "forcevaapienabled", m_videoVAAPIforced);
    XMLUtils::GetFloat(pElement, "nonlinearstretchratio", m_videoNonLinStretchRatio, 0.01f, 1.0f);
    XMLUtils::GetBoolean(pElement,"enablehighqualityhwscalers", m_videoEnableHighQualityHwScalers);
    XMLUtils::GetFloat(pElement,"autoscalemaxfps",m_videoAutoScaleMaxFps, 0.0f, 1000.0f);
    XMLUtils::GetBoolean(pElement, "disablebackgrounddeinterlace", m_videoDisableBackgroundDeinterlace);
    XMLUtils::GetInt(pElement, "useocclusionquery", m_videoCaptureUseOcclusionQuery, -1, 1);
    XMLUtils::GetBoolean(pElement,"vdpauInvTelecine",m_videoVDPAUtelecine);
    XMLUtils::GetBoolean(pElement,"vdpauHDdeintSkipChroma",m_videoVDPAUdeintSkipChromaHD);
    XMLUtils::GetBoolean(pElement,"useffmpegvda", m_useFfmpegVda);

    TiXmlElement* pStagefrightElem = pElement->FirstChildElement("stagefright");
    if (pStagefrightElem)
    {
      XMLUtils::GetInt(pStagefrightElem,"useavccodec",m_stagefrightConfig.useAVCcodec, -1, 1);
      XMLUtils::GetInt(pStagefrightElem,"usehevccodec",m_stagefrightConfig.useHEVCcodec, -1, 1);
      XMLUtils::GetInt(pStagefrightElem,"usevc1codec",m_stagefrightConfig.useVC1codec, -1, 1);
      XMLUtils::GetInt(pStagefrightElem,"usevpxcodec",m_stagefrightConfig.useVPXcodec, -1, 1);
      XMLUtils::GetInt(pStagefrightElem,"usemp4codec",m_stagefrightConfig.useMP4codec, -1, 1);
      XMLUtils::GetInt(pStagefrightElem,"usempeg2codec",m_stagefrightConfig.useMPEG2codec, -1, 1);
      XMLUtils::GetBoolean(pStagefrightElem,"useswrenderer",m_stagefrightConfig.useSwRenderer);
      XMLUtils::GetBoolean(pStagefrightElem,"useinputdts",m_stagefrightConfig.useInputDTS);
    }

    XMLUtils::GetBoolean(pElement,"mediacodecforcesoftwarerendering",m_mediacodecForceSoftwareRendring);

    TiXmlElement* pAdjustRefreshrate = pElement->FirstChildElement("adjustrefreshrate");
    if (pAdjustRefreshrate)
    {
      TiXmlElement* pRefreshOverride = pAdjustRefreshrate->FirstChildElement("override");
      while (pRefreshOverride)
      {
        RefreshOverride override = {0};

        float fps;
        if (XMLUtils::GetFloat(pRefreshOverride, "fps", fps))
        {
          override.fpsmin = fps - 0.01f;
          override.fpsmax = fps + 0.01f;
        }

        float fpsmin, fpsmax;
        if (XMLUtils::GetFloat(pRefreshOverride, "fpsmin", fpsmin) &&
            XMLUtils::GetFloat(pRefreshOverride, "fpsmax", fpsmax))
        {
          override.fpsmin = fpsmin;
          override.fpsmax = fpsmax;
        }

        float refresh;
        if (XMLUtils::GetFloat(pRefreshOverride, "refresh", refresh))
        {
          override.refreshmin = refresh - 0.01f;
          override.refreshmax = refresh + 0.01f;
        }

        float refreshmin, refreshmax;
        if (XMLUtils::GetFloat(pRefreshOverride, "refreshmin", refreshmin) &&
            XMLUtils::GetFloat(pRefreshOverride, "refreshmax", refreshmax))
        {
          override.refreshmin = refreshmin;
          override.refreshmax = refreshmax;
        }

        bool fpsCorrect     = (override.fpsmin > 0.0f && override.fpsmax >= override.fpsmin);
        bool refreshCorrect = (override.refreshmin > 0.0f && override.refreshmax >= override.refreshmin);

        if (fpsCorrect && refreshCorrect)
          m_videoAdjustRefreshOverrides.push_back(override);
        else
          CLog::Log(LOGWARNING, "Ignoring malformed refreshrate override, fpsmin:%f fpsmax:%f refreshmin:%f refreshmax:%f",
              override.fpsmin, override.fpsmax, override.refreshmin, override.refreshmax);

        pRefreshOverride = pRefreshOverride->NextSiblingElement("override");
      }

      TiXmlElement* pRefreshFallback = pAdjustRefreshrate->FirstChildElement("fallback");
      while (pRefreshFallback)
      {
        RefreshOverride fallback = {0};
        fallback.fallback = true;

        float refresh;
        if (XMLUtils::GetFloat(pRefreshFallback, "refresh", refresh))
        {
          fallback.refreshmin = refresh - 0.01f;
          fallback.refreshmax = refresh + 0.01f;
        }

        float refreshmin, refreshmax;
        if (XMLUtils::GetFloat(pRefreshFallback, "refreshmin", refreshmin) &&
            XMLUtils::GetFloat(pRefreshFallback, "refreshmax", refreshmax))
        {
          fallback.refreshmin = refreshmin;
          fallback.refreshmax = refreshmax;
        }

        if (fallback.refreshmin > 0.0f && fallback.refreshmax >= fallback.refreshmin)
          m_videoAdjustRefreshOverrides.push_back(fallback);
        else
          CLog::Log(LOGWARNING, "Ignoring malformed refreshrate fallback, fpsmin:%f fpsmax:%f refreshmin:%f refreshmax:%f",
              fallback.fpsmin, fallback.fpsmax, fallback.refreshmin, fallback.refreshmax);

        pRefreshFallback = pRefreshFallback->NextSiblingElement("fallback");
      }
    }

    m_DXVACheckCompatibilityPresent = XMLUtils::GetBoolean(pElement,"checkdxvacompatibility", m_DXVACheckCompatibility);

    XMLUtils::GetBoolean(pElement,"forcedxvarenderer", m_DXVAForceProcessorRenderer);
    XMLUtils::GetBoolean(pElement,"dxvanodeintforprogressive", m_DXVANoDeintProcForProgressive);
    XMLUtils::GetBoolean(pElement, "dxvaallowhqscaling", m_DXVAAllowHqScaling);
    //0 = disable fps detect, 1 = only detect on timestamps with uniform spacing, 2 detect on all timestamps
    XMLUtils::GetInt(pElement, "fpsdetect", m_videoFpsDetect, 0, 2);

    // controls the delay, in milliseconds, until
    // the busy dialog is shown when starting video playback.
    XMLUtils::GetInt(pElement, "busydialogdelayms", m_videoBusyDialogDelay_ms, 0, 1000);

    // Store global display latency settings
    TiXmlElement* pVideoLatency = pElement->FirstChildElement("latency");
    if (pVideoLatency)
    {
      float refresh, refreshmin, refreshmax, delay;
      TiXmlElement* pRefreshVideoLatency = pVideoLatency->FirstChildElement("refresh");

      while (pRefreshVideoLatency)
      {
        RefreshVideoLatency videolatency = {0};

        if (XMLUtils::GetFloat(pRefreshVideoLatency, "rate", refresh))
        {
          videolatency.refreshmin = refresh - 0.01f;
          videolatency.refreshmax = refresh + 0.01f;
        }
        else if (XMLUtils::GetFloat(pRefreshVideoLatency, "min", refreshmin) &&
                 XMLUtils::GetFloat(pRefreshVideoLatency, "max", refreshmax))
        {
          videolatency.refreshmin = refreshmin;
          videolatency.refreshmax = refreshmax;
        }
        if (XMLUtils::GetFloat(pRefreshVideoLatency, "delay", delay, -600.0f, 600.0f))
          videolatency.delay = delay;

        if (videolatency.refreshmin > 0.0f && videolatency.refreshmax >= videolatency.refreshmin)
          m_videoRefreshLatency.push_back(videolatency);
        else
          CLog::Log(LOGWARNING, "Ignoring malformed display latency <refresh> entry, min:%f max:%f", videolatency.refreshmin, videolatency.refreshmax);

        pRefreshVideoLatency = pRefreshVideoLatency->NextSiblingElement("refresh");
      }

      // Get default global display latency
      XMLUtils::GetFloat(pVideoLatency, "delay", m_videoDefaultLatency, -600.0f, 600.0f);
    }
  }

  pElement = pRootElement->FirstChildElement("musiclibrary");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "recentlyaddeditems", m_iMusicLibraryRecentlyAddedItems, 1, INT_MAX);
    XMLUtils::GetBoolean(pElement, "prioritiseapetags", m_prioritiseAPEv2tags);
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", m_bMusicLibraryAllItemsOnBottom);
    XMLUtils::GetBoolean(pElement, "cleanonupdate", m_bMusicLibraryCleanOnUpdate);
    XMLUtils::GetString(pElement, "albumformat", m_strMusicLibraryAlbumFormat);
    XMLUtils::GetString(pElement, "itemseparator", m_musicItemSeparator);
    XMLUtils::GetInt(pElement, "dateadded", m_iMusicLibraryDateAdded);
  }

  pElement = pRootElement->FirstChildElement("videolibrary");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", m_bVideoLibraryAllItemsOnBottom);
    XMLUtils::GetInt(pElement, "recentlyaddeditems", m_iVideoLibraryRecentlyAddedItems, 1, INT_MAX);
    XMLUtils::GetBoolean(pElement, "hideemptyseries", m_bVideoLibraryHideEmptySeries);
    XMLUtils::GetBoolean(pElement, "cleanonupdate", m_bVideoLibraryCleanOnUpdate);
    XMLUtils::GetBoolean(pElement, "usefasthash", m_bVideoLibraryUseFastHash);
    XMLUtils::GetString(pElement, "itemseparator", m_videoItemSeparator);
    XMLUtils::GetBoolean(pElement, "exportautothumbs", m_bVideoLibraryExportAutoThumbs);
    XMLUtils::GetBoolean(pElement, "importwatchedstate", m_bVideoLibraryImportWatchedState);
    XMLUtils::GetBoolean(pElement, "importresumepoint", m_bVideoLibraryImportResumePoint);
    XMLUtils::GetInt(pElement, "dateadded", m_iVideoLibraryDateAdded);
  }

  pElement = pRootElement->FirstChildElement("videoscanner");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "ignoreerrors", m_bVideoScannerIgnoreErrors);
  }

  // Backward-compatibility of ExternalPlayer config
  pElement = pRootElement->FirstChildElement("externalplayer");
  if (pElement)
  {
    CLog::Log(LOGWARNING, "External player configuration has been removed from advancedsettings.xml.  It can now be configed in userdata/playercorefactory.xml");
  }
  pElement = pRootElement->FirstChildElement("slideshow");
  if (pElement)
  {
    XMLUtils::GetFloat(pElement, "panamount", m_slideshowPanAmount, 0.0f, 20.0f);
    XMLUtils::GetFloat(pElement, "zoomamount", m_slideshowZoomAmount, 0.0f, 20.0f);
    XMLUtils::GetFloat(pElement, "blackbarcompensation", m_slideshowBlackBarCompensation, 0.0f, 50.0f);
  }

  pElement = pRootElement->FirstChildElement("network");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "curlclienttimeout", m_curlconnecttimeout, 1, 1000);
    XMLUtils::GetInt(pElement, "curllowspeedtime", m_curllowspeedtime, 1, 1000);
    XMLUtils::GetInt(pElement, "curlretries", m_curlretries, 0, 10);
    XMLUtils::GetBoolean(pElement,"disableipv6", m_curlDisableIPV6);
    XMLUtils::GetUInt(pElement, "cachemembuffersize", m_cacheMemBufferSize);
    XMLUtils::GetUInt(pElement, "buffermode", m_networkBufferMode, 0, 3);
    XMLUtils::GetFloat(pElement, "readbufferfactor", m_readBufferFactor);
  }

  pElement = pRootElement->FirstChildElement("jsonrpc");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "compactoutput", m_jsonOutputCompact);
    XMLUtils::GetUInt(pElement, "tcpport", m_jsonTcpPort);
  }

  pElement = pRootElement->FirstChildElement("samba");
  if (pElement)
  {
    XMLUtils::GetString(pElement,  "doscodepage",   m_sambadoscodepage);
    XMLUtils::GetInt(pElement, "clienttimeout", m_sambaclienttimeout, 5, 100);
    XMLUtils::GetBoolean(pElement, "statfiles", m_sambastatfiles);
  }

  pElement = pRootElement->FirstChildElement("httpdirectory");
  if (pElement)
    XMLUtils::GetBoolean(pElement, "statfilesize", m_bHTTPDirectoryStatFilesize);

  pElement = pRootElement->FirstChildElement("ftp");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "remotethumbs", m_bFTPThumbs);
  }

  pElement = pRootElement->FirstChildElement("loglevel");
  if (pElement)
  { // read the loglevel setting, so set the setting advanced to hide it in GUI
    // as altering it will do nothing - we don't write to advancedsettings.xml
    XMLUtils::GetInt(pRootElement, "loglevel", m_logLevelHint, LOG_LEVEL_NONE, LOG_LEVEL_MAX);
    const char* hide = pElement->Attribute("hide");
    if (hide == NULL || strnicmp("false", hide, 4) != 0)
    {
      CSetting *setting = CSettings::GetInstance().GetSetting(CSettings::SETTING_DEBUG_SHOWLOGINFO);
      if (setting != NULL)
        setting->SetVisible(false);
    }
    g_advancedSettings.m_logLevel = std::max(g_advancedSettings.m_logLevel, g_advancedSettings.m_logLevelHint);
    CLog::SetLogLevel(g_advancedSettings.m_logLevel);
  }

  XMLUtils::GetString(pRootElement, "cddbaddress", m_cddbAddress);

  //airtunes + airplay
  XMLUtils::GetInt(pRootElement,     "airtunesport", m_airTunesPort);
  XMLUtils::GetInt(pRootElement,     "airplayport", m_airPlayPort);  

  XMLUtils::GetBoolean(pRootElement, "handlemounting", m_handleMounting);

#if defined(HAS_SDL) || defined(TARGET_WINDOWS)
  XMLUtils::GetBoolean(pRootElement, "fullscreen", m_startFullScreen);
#endif
  XMLUtils::GetBoolean(pRootElement, "splash", m_splashImage);
  XMLUtils::GetBoolean(pRootElement, "showexitbutton", m_showExitButton);
  XMLUtils::GetBoolean(pRootElement, "canwindowed", m_canWindowed);

  XMLUtils::GetInt(pRootElement, "songinfoduration", m_songInfoDuration, 0, INT_MAX);
  XMLUtils::GetInt(pRootElement, "playlistretries", m_playlistRetries, -1, 5000);
  XMLUtils::GetInt(pRootElement, "playlisttimeout", m_playlistTimeout, 0, 5000);

  XMLUtils::GetBoolean(pRootElement,"glrectanglehack", m_GLRectangleHack);
  XMLUtils::GetInt(pRootElement,"skiploopfilter", m_iSkipLoopFilter, -16, 48);
  XMLUtils::GetFloat(pRootElement, "forcedswaptime", m_ForcedSwapTime, 0.0, 100.0);

  XMLUtils::GetUInt(pRootElement,"restrictcapsmask", m_RestrictCapsMask);
  XMLUtils::GetFloat(pRootElement,"sleepbeforeflip", m_sleepBeforeFlip, 0.0f, 1.0f);
  XMLUtils::GetBoolean(pRootElement,"virtualshares", m_bVirtualShares);
  XMLUtils::GetUInt(pRootElement, "packagefoldersize", m_addonPackageFolderSize);
  XMLUtils::GetBoolean(pRootElement, "allowdeferredrendering", m_bAllowDeferredRendering);

  // EPG
  pElement = pRootElement->FirstChildElement("epg");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "lingertime", m_iEpgLingerTime);
    XMLUtils::GetInt(pElement, "updatecheckinterval", m_iEpgUpdateCheckInterval);
    XMLUtils::GetInt(pElement, "cleanupinterval", m_iEpgCleanupInterval);
    XMLUtils::GetInt(pElement, "activetagcheckinterval", m_iEpgActiveTagCheckInterval);
    XMLUtils::GetInt(pElement, "retryinterruptedupdateinterval", m_iEpgRetryInterruptedUpdateInterval);
    XMLUtils::GetInt(pElement, "updateemptytagsinterval", m_iEpgUpdateEmptyTagsInterval);
    XMLUtils::GetBoolean(pElement, "displayupdatepopup", m_bEpgDisplayUpdatePopup);
    XMLUtils::GetBoolean(pElement, "displayincrementalupdatepopup", m_bEpgDisplayIncrementalUpdatePopup);
  }

  // EDL commercial break handling
  pElement = pRootElement->FirstChildElement("edl");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "mergeshortcommbreaks", m_bEdlMergeShortCommBreaks);
    XMLUtils::GetInt(pElement, "maxcommbreaklength", m_iEdlMaxCommBreakLength, 0, 10 * 60); // Between 0 and 10 minutes
    XMLUtils::GetInt(pElement, "mincommbreaklength", m_iEdlMinCommBreakLength, 0, 5 * 60);  // Between 0 and 5 minutes
    XMLUtils::GetInt(pElement, "maxcommbreakgap", m_iEdlMaxCommBreakGap, 0, 5 * 60);        // Between 0 and 5 minutes.
    XMLUtils::GetInt(pElement, "maxstartgap", m_iEdlMaxStartGap, 0, 10 * 60);               // Between 0 and 10 minutes
    XMLUtils::GetInt(pElement, "commbreakautowait", m_iEdlCommBreakAutowait, 0, 10);        // Between 0 and 10 seconds
    XMLUtils::GetInt(pElement, "commbreakautowind", m_iEdlCommBreakAutowind, 0, 10);        // Between 0 and 10 seconds
  }

  // picture exclude regexps
  TiXmlElement* pPictureExcludes = pRootElement->FirstChildElement("pictureexcludes");
  if (pPictureExcludes)
    GetCustomRegexps(pPictureExcludes, m_pictureExcludeFromListingRegExps);

  // picture extensions
  TiXmlElement* pExts = pRootElement->FirstChildElement("pictureextensions");
  if (pExts)
    GetCustomExtensions(pExts, m_pictureExtensions);

  // music extensions
  pExts = pRootElement->FirstChildElement("musicextensions");
  if (pExts)
    GetCustomExtensions(pExts, m_musicExtensions);

  // video extensions
  pExts = pRootElement->FirstChildElement("videoextensions");
  if (pExts)
    GetCustomExtensions(pExts, m_videoExtensions);

  // stub extensions
  pExts = pRootElement->FirstChildElement("discstubextensions");
  if (pExts)
    GetCustomExtensions(pExts, m_discStubExtensions);

  m_vecTokens.clear();
  CLangInfo::LoadTokens(pRootElement->FirstChild("sorttokens"),m_vecTokens);

  // TODO: Should cache path be given in terms of our predefined paths??
  //       Are we even going to have predefined paths??
  std::string tmp;
  if (XMLUtils::GetPath(pRootElement, "cachepath", tmp))
    m_cachePath = tmp;
  URIUtils::AddSlashAtEnd(m_cachePath);

  g_LangCodeExpander.LoadUserCodes(pRootElement->FirstChildElement("languagecodes"));

  // trailer matching regexps
  TiXmlElement* pTrailerMatching = pRootElement->FirstChildElement("trailermatching");
  if (pTrailerMatching)
    GetCustomRegexps(pTrailerMatching, m_trailerMatchRegExps);

  //everything thats a trailer is not a movie
  m_moviesExcludeFromScanRegExps.insert(m_moviesExcludeFromScanRegExps.end(),
                                        m_trailerMatchRegExps.begin(),
                                        m_trailerMatchRegExps.end());

  // video stacking regexps
  TiXmlElement* pVideoStacking = pRootElement->FirstChildElement("moviestacking");
  if (pVideoStacking)
    GetCustomRegexps(pVideoStacking, m_videoStackRegExps);

  // folder stacking regexps
  TiXmlElement* pFolderStacking = pRootElement->FirstChildElement("folderstacking");
  if (pFolderStacking)
    GetCustomRegexps(pFolderStacking, m_folderStackRegExps);

  //tv stacking regexps
  TiXmlElement* pTVStacking = pRootElement->FirstChildElement("tvshowmatching");
  if (pTVStacking)
    GetCustomTVRegexps(pTVStacking, m_tvshowEnumRegExps);

  //tv multipart enumeration regexp
  XMLUtils::GetString(pRootElement, "tvmultipartmatching", m_tvshowMultiPartEnumRegExp);

  // path substitutions
  TiXmlElement* pPathSubstitution = pRootElement->FirstChildElement("pathsubstitution");
  if (pPathSubstitution)
  {
    m_pathSubstitutions.clear();
    CLog::Log(LOGDEBUG,"Configuring path substitutions");
    TiXmlNode* pSubstitute = pPathSubstitution->FirstChildElement("substitute");
    while (pSubstitute)
    {
      std::string strFrom, strTo;
      TiXmlNode* pFrom = pSubstitute->FirstChild("from");
      if (pFrom)
        strFrom = CSpecialProtocol::TranslatePath(pFrom->FirstChild()->Value()).c_str();
      TiXmlNode* pTo = pSubstitute->FirstChild("to");
      if (pTo)
        strTo = pTo->FirstChild()->Value();

      if (!strFrom.empty() && !strTo.empty())
      {
        CLog::Log(LOGDEBUG,"  Registering substition pair:");
        CLog::Log(LOGDEBUG,"    From: [%s]", strFrom.c_str());
        CLog::Log(LOGDEBUG,"    To:   [%s]", strTo.c_str());
        m_pathSubstitutions.push_back(std::make_pair(strFrom,strTo));
      }
      else
      {
        // error message about missing tag
        if (strFrom.empty())
          CLog::Log(LOGERROR,"  Missing <from> tag");
        else
          CLog::Log(LOGERROR,"  Missing <to> tag");
      }

      // get next one
      pSubstitute = pSubstitute->NextSiblingElement("substitute");
    }
  }

  XMLUtils::GetInt(pRootElement, "remotedelay", m_remoteDelay, 1, 20);
  XMLUtils::GetFloat(pRootElement, "controllerdeadzone", m_controllerDeadzone, 0.0f, 1.0f);
  XMLUtils::GetUInt(pRootElement, "fanartres", m_fanartRes, 0, 1080);
  XMLUtils::GetUInt(pRootElement, "imageres", m_imageRes, 0, 1080);
#if !defined(TARGET_RASPBERRY_PI)
  XMLUtils::GetBoolean(pRootElement, "useddsfanart", m_useDDSFanart);
#endif
  if (XMLUtils::GetString(pRootElement, "imagescalingalgorithm", tmp))
    m_imageScalingAlgorithm = CPictureScalingAlgorithm::FromString(tmp);
  XMLUtils::GetBoolean(pRootElement, "playlistasfolders", m_playlistAsFolders);
  XMLUtils::GetBoolean(pRootElement, "detectasudf", m_detectAsUdf);

  // music thumbs
  TiXmlElement* pThumbs = pRootElement->FirstChildElement("musicthumbs");
  if (pThumbs)
    GetCustomExtensions(pThumbs,m_musicThumbs);

  // movie fanarts
  TiXmlElement* pFanart = pRootElement->FirstChildElement("fanart");
  if (pFanart)
    GetCustomExtensions(pFanart,m_fanartImages);

  // music filename->tag filters
  TiXmlElement* filters = pRootElement->FirstChildElement("musicfilenamefilters");
  if (filters)
  {
    TiXmlNode* filter = filters->FirstChild("filter");
    while (filter)
    {
      if (filter->FirstChild())
        m_musicTagsFromFileFilters.push_back(filter->FirstChild()->ValueStr());
      filter = filter->NextSibling("filter");
    }
  }

  TiXmlElement* pHostEntries = pRootElement->FirstChildElement("hosts");
  if (pHostEntries)
  {
    TiXmlElement* element = pHostEntries->FirstChildElement("entry");
    while(element)
    {
      if(!element->NoChildren())
      {
        std::string name  = XMLUtils::GetAttribute(element, "name");
        std::string value = element->FirstChild()->ValueStr();
        if (!name.empty())
          CDNSNameCache::Add(name, value);
      }
      element = element->NextSiblingElement("entry");
    }
  }

  XMLUtils::GetString(pRootElement, "cputempcommand", m_cpuTempCmd);
  XMLUtils::GetString(pRootElement, "gputempcommand", m_gpuTempCmd);

  XMLUtils::GetBoolean(pRootElement, "alwaysontop", m_alwaysOnTop);

  TiXmlElement *pPVR = pRootElement->FirstChildElement("pvr");
  if (pPVR)
  {
    XMLUtils::GetInt(pPVR, "timecorrection", m_iPVRTimeCorrection, 0, 1440);
    XMLUtils::GetInt(pPVR, "infotoggleinterval", m_iPVRInfoToggleInterval, 0, 30000);
    XMLUtils::GetInt(pPVR, "minvideocachelevel", m_iPVRMinVideoCacheLevel, 0, 100);
    XMLUtils::GetInt(pPVR, "minaudiocachelevel", m_iPVRMinAudioCacheLevel, 0, 100);
    XMLUtils::GetBoolean(pPVR, "cacheindvdplayer", m_bPVRCacheInDvdPlayer);
    XMLUtils::GetBoolean(pPVR, "channeliconsautoscan", m_bPVRChannelIconsAutoScan);
    XMLUtils::GetBoolean(pPVR, "autoscaniconsuserset", m_bPVRAutoScanIconsUserSet);
    XMLUtils::GetInt(pPVR, "numericchannelswitchtimeout", m_iPVRNumericChannelSwitchTimeout, 50, 60000);
  }

  TiXmlElement* pDatabase = pRootElement->FirstChildElement("videodatabase");
  if (pDatabase)
  {
    CLog::Log(LOGWARNING, "VIDEO database configuration is experimental.");
    XMLUtils::GetString(pDatabase, "type", m_databaseVideo.type);
    XMLUtils::GetString(pDatabase, "host", m_databaseVideo.host);
    XMLUtils::GetString(pDatabase, "port", m_databaseVideo.port);
    XMLUtils::GetString(pDatabase, "user", m_databaseVideo.user);
    XMLUtils::GetString(pDatabase, "pass", m_databaseVideo.pass);
    XMLUtils::GetString(pDatabase, "name", m_databaseVideo.name);
    XMLUtils::GetString(pDatabase, "key", m_databaseVideo.key);
    XMLUtils::GetString(pDatabase, "cert", m_databaseVideo.cert);
    XMLUtils::GetString(pDatabase, "ca", m_databaseVideo.ca);
    XMLUtils::GetString(pDatabase, "capath", m_databaseVideo.capath);
    XMLUtils::GetString(pDatabase, "ciphers", m_databaseVideo.ciphers);
    XMLUtils::GetBoolean(pDatabase, "compression", m_databaseVideo.compression);
  }

  pDatabase = pRootElement->FirstChildElement("musicdatabase");
  if (pDatabase)
  {
    XMLUtils::GetString(pDatabase, "type", m_databaseMusic.type);
    XMLUtils::GetString(pDatabase, "host", m_databaseMusic.host);
    XMLUtils::GetString(pDatabase, "port", m_databaseMusic.port);
    XMLUtils::GetString(pDatabase, "user", m_databaseMusic.user);
    XMLUtils::GetString(pDatabase, "pass", m_databaseMusic.pass);
    XMLUtils::GetString(pDatabase, "name", m_databaseMusic.name);
    XMLUtils::GetString(pDatabase, "key", m_databaseMusic.key);
    XMLUtils::GetString(pDatabase, "cert", m_databaseMusic.cert);
    XMLUtils::GetString(pDatabase, "ca", m_databaseMusic.ca);
    XMLUtils::GetString(pDatabase, "capath", m_databaseMusic.capath);
    XMLUtils::GetString(pDatabase, "ciphers", m_databaseMusic.ciphers);
    XMLUtils::GetBoolean(pDatabase, "compression", m_databaseMusic.compression);
  }

  pDatabase = pRootElement->FirstChildElement("tvdatabase");
  if (pDatabase)
  {
    XMLUtils::GetString(pDatabase, "type", m_databaseTV.type);
    XMLUtils::GetString(pDatabase, "host", m_databaseTV.host);
    XMLUtils::GetString(pDatabase, "port", m_databaseTV.port);
    XMLUtils::GetString(pDatabase, "user", m_databaseTV.user);
    XMLUtils::GetString(pDatabase, "pass", m_databaseTV.pass);
    XMLUtils::GetString(pDatabase, "name", m_databaseTV.name);
    XMLUtils::GetString(pDatabase, "key", m_databaseTV.key);
    XMLUtils::GetString(pDatabase, "cert", m_databaseTV.cert);
    XMLUtils::GetString(pDatabase, "ca", m_databaseTV.ca);
    XMLUtils::GetString(pDatabase, "capath", m_databaseTV.capath);
    XMLUtils::GetString(pDatabase, "ciphers", m_databaseTV.ciphers);
    XMLUtils::GetBoolean(pDatabase, "compression", m_databaseTV.compression);
  }

  pDatabase = pRootElement->FirstChildElement("adspdatabase");
  if (pDatabase)
  {
    XMLUtils::GetString(pDatabase, "type", m_databaseADSP.type);
    XMLUtils::GetString(pDatabase, "host", m_databaseADSP.host);
    XMLUtils::GetString(pDatabase, "port", m_databaseADSP.port);
    XMLUtils::GetString(pDatabase, "user", m_databaseADSP.user);
    XMLUtils::GetString(pDatabase, "pass", m_databaseADSP.pass);
    XMLUtils::GetString(pDatabase, "name", m_databaseADSP.name);
    XMLUtils::GetString(pDatabase, "key", m_databaseADSP.key);
    XMLUtils::GetString(pDatabase, "cert", m_databaseADSP.cert);
    XMLUtils::GetString(pDatabase, "ca", m_databaseADSP.ca);
    XMLUtils::GetString(pDatabase, "capath", m_databaseADSP.capath);
    XMLUtils::GetString(pDatabase, "ciphers", m_databaseADSP.ciphers);
  }

  pDatabase = pRootElement->FirstChildElement("epgdatabase");
  if (pDatabase)
  {
    XMLUtils::GetString(pDatabase, "type", m_databaseEpg.type);
    XMLUtils::GetString(pDatabase, "host", m_databaseEpg.host);
    XMLUtils::GetString(pDatabase, "port", m_databaseEpg.port);
    XMLUtils::GetString(pDatabase, "user", m_databaseEpg.user);
    XMLUtils::GetString(pDatabase, "pass", m_databaseEpg.pass);
    XMLUtils::GetString(pDatabase, "name", m_databaseEpg.name);
    XMLUtils::GetString(pDatabase, "key", m_databaseEpg.key);
    XMLUtils::GetString(pDatabase, "cert", m_databaseEpg.cert);
    XMLUtils::GetString(pDatabase, "ca", m_databaseEpg.ca);
    XMLUtils::GetString(pDatabase, "capath", m_databaseEpg.capath);
    XMLUtils::GetString(pDatabase, "ciphers", m_databaseEpg.ciphers);
    XMLUtils::GetBoolean(pDatabase, "compression", m_databaseEpg.compression);
  }

  pElement = pRootElement->FirstChildElement("enablemultimediakeys");
  if (pElement)
  {
    XMLUtils::GetBoolean(pRootElement, "enablemultimediakeys", m_enableMultimediaKeys);
  }
  
  pElement = pRootElement->FirstChildElement("gui");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "visualizedirtyregions", m_guiVisualizeDirtyRegions);
    XMLUtils::GetInt(pElement, "algorithmdirtyregions",     m_guiAlgorithmDirtyRegions);
    XMLUtils::GetInt(pElement, "nofliptimeout",             m_guiDirtyRegionNoFlipTimeout);
  }

  std::string seekSteps;
  XMLUtils::GetString(pRootElement, "seeksteps", seekSteps);
  if (!seekSteps.empty())
  {
    m_seekSteps.clear();
    std::vector<std::string> steps = StringUtils::Split(seekSteps, ',');
    for(std::vector<std::string>::iterator it = steps.begin(); it != steps.end(); ++it)
      m_seekSteps.push_back(atoi((*it).c_str()));
  }

  // load in the settings overrides
  CSettings::GetInstance().Load(pRootElement, true);  // true to hide the settings we read in
}

void CAdvancedSettings::Clear()
{
  m_videoCleanStringRegExps.clear();
  m_moviesExcludeFromScanRegExps.clear();
  m_tvshowExcludeFromScanRegExps.clear();
  m_videoExcludeFromListingRegExps.clear();
  m_videoStackRegExps.clear();
  m_folderStackRegExps.clear();
  m_audioExcludeFromScanRegExps.clear();
  m_audioExcludeFromListingRegExps.clear();
  m_pictureExcludeFromListingRegExps.clear();

  m_pictureExtensions.clear();
  m_musicExtensions.clear();
  m_videoExtensions.clear();
  m_discStubExtensions.clear();

  m_logFolder.clear();
  m_userAgent.clear();
}

void CAdvancedSettings::GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings)
{
  TiXmlElement *pElement = pRootElement;
  while (pElement)
  {
    int iAction = 0; // overwrite
    // for backward compatibility
    const char* szAppend = pElement->Attribute("append");
    if ((szAppend && stricmp(szAppend, "yes") == 0))
      iAction = 1;
    // action takes precedence if both attributes exist
    const char* szAction = pElement->Attribute("action");
    if (szAction)
    {
      iAction = 0; // overwrite
      if (stricmp(szAction, "append") == 0)
        iAction = 1; // append
      else if (stricmp(szAction, "prepend") == 0)
        iAction = 2; // prepend
    }
    if (iAction == 0)
      settings.clear();
    TiXmlNode* pRegExp = pElement->FirstChild("regexp");
    int i = 0;
    while (pRegExp)
    {
      if (pRegExp->FirstChild())
      {
        bool bByDate = false;
        int iDefaultSeason = 1;
        if (pRegExp->ToElement())
        {
          std::string byDate = XMLUtils::GetAttribute(pRegExp->ToElement(), "bydate");
          if (byDate == "true")
          {
            bByDate = true;
          }
          std::string defaultSeason = XMLUtils::GetAttribute(pRegExp->ToElement(), "defaultseason");
          if(!defaultSeason.empty())
          {
            iDefaultSeason = atoi(defaultSeason.c_str());
          }
        }
        std::string regExp = pRegExp->FirstChild()->Value();
        if (iAction == 2)
          settings.insert(settings.begin() + i++, 1, TVShowRegexp(bByDate,regExp,iDefaultSeason));
        else
          settings.push_back(TVShowRegexp(bByDate,regExp,iDefaultSeason));
      }
      pRegExp = pRegExp->NextSibling("regexp");
    }

    pElement = pElement->NextSiblingElement(pRootElement->Value());
  }
}

void CAdvancedSettings::GetCustomRegexps(TiXmlElement *pRootElement, std::vector<std::string>& settings)
{
  TiXmlElement *pElement = pRootElement;
  while (pElement)
  {
    int iAction = 0; // overwrite
    // for backward compatibility
    const char* szAppend = pElement->Attribute("append");
    if ((szAppend && stricmp(szAppend, "yes") == 0))
      iAction = 1;
    // action takes precedence if both attributes exist
    const char* szAction = pElement->Attribute("action");
    if (szAction)
    {
      iAction = 0; // overwrite
      if (stricmp(szAction, "append") == 0)
        iAction = 1; // append
      else if (stricmp(szAction, "prepend") == 0)
        iAction = 2; // prepend
    }
    if (iAction == 0)
      settings.clear();
    TiXmlNode* pRegExp = pElement->FirstChild("regexp");
    int i = 0;
    while (pRegExp)
    {
      if (pRegExp->FirstChild())
      {
        std::string regExp = pRegExp->FirstChild()->Value();
        if (iAction == 2)
          settings.insert(settings.begin() + i++, 1, regExp);
        else
          settings.push_back(regExp);
      }
      pRegExp = pRegExp->NextSibling("regexp");
    }

    pElement = pElement->NextSiblingElement(pRootElement->Value());
  }
}

void CAdvancedSettings::GetCustomExtensions(TiXmlElement *pRootElement, std::string& extensions)
{
  std::string extraExtensions;
  if (XMLUtils::GetString(pRootElement, "add", extraExtensions) && !extraExtensions.empty())
    extensions += "|" + extraExtensions;
  if (XMLUtils::GetString(pRootElement, "remove", extraExtensions) && !extraExtensions.empty())
  {
    std::vector<std::string> exts = StringUtils::Split(extraExtensions, '|');
    for (std::vector<std::string>::const_iterator i = exts.begin(); i != exts.end(); ++i)
    {
      size_t iPos = extensions.find(*i);
      if (iPos != std::string::npos)
        extensions.erase(iPos,i->size()+1);
    }
  }
}

void CAdvancedSettings::AddSettingsFile(const std::string &filename)
{
  m_settingsFiles.push_back(filename);
}

float CAdvancedSettings::GetDisplayLatency(float refreshrate)
{
  float delay = m_videoDefaultLatency / 1000.0f;
  for (int i = 0; i < (int) m_videoRefreshLatency.size(); i++)
  {
    RefreshVideoLatency& videolatency = m_videoRefreshLatency[i];
    if (refreshrate >= videolatency.refreshmin && refreshrate <= videolatency.refreshmax)
      delay = videolatency.delay / 1000.0f;
  }

  return delay; // in seconds
}

void CAdvancedSettings::SetDebugMode(bool debug)
{
  if (debug)
  {
    int level = std::max(m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
    m_logLevel = level;
    CLog::SetLogLevel(level);
    CLog::Log(LOGNOTICE, "Enabled debug logging due to GUI setting. Level %d.", level);
  }
  else
  {
    int level = std::min(m_logLevelHint, LOG_LEVEL_DEBUG/*LOG_LEVEL_NORMAL*/);
    CLog::Log(LOGNOTICE, "Disabled debug logging due to GUI setting. Level %d.", level);
    m_logLevel = level;
    CLog::SetLogLevel(level);
  }
}

bool CAdvancedSettings::CanLogComponent(int component) const
{
  if (!m_extraLogEnabled || component <= 0)
    return false;

  return ((m_extraLogLevels & component) == component);
}

void CAdvancedSettings::SettingOptionsLoggingComponentsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  list.push_back(std::make_pair(g_localizeStrings.Get(669), LOGSAMBA));
  list.push_back(std::make_pair(g_localizeStrings.Get(670), LOGCURL));
  list.push_back(std::make_pair(g_localizeStrings.Get(672), LOGFFMPEG));
  list.push_back(std::make_pair(g_localizeStrings.Get(676), LOGAUDIO));
  list.push_back(std::make_pair(g_localizeStrings.Get(680), LOGVIDEO));
#ifdef HAS_LIBRTMP
  list.push_back(std::make_pair(g_localizeStrings.Get(673), LOGRTMP));
#endif
#ifdef HAS_DBUS
  list.push_back(std::make_pair(g_localizeStrings.Get(674), LOGDBUS));
#endif
#ifdef HAS_JSONRPC
  list.push_back(std::make_pair(g_localizeStrings.Get(675), LOGJSONRPC));
#endif
#ifdef HAS_AIRTUNES
  list.push_back(std::make_pair(g_localizeStrings.Get(677), LOGAIRTUNES));
#endif
#ifdef HAS_UPNP
  list.push_back(std::make_pair(g_localizeStrings.Get(678), LOGUPNP));
#endif
#ifdef HAVE_LIBCEC
  list.push_back(std::make_pair(g_localizeStrings.Get(679), LOGCEC));
#endif
}

void CAdvancedSettings::setExtraLogLevel(const std::vector<CVariant> &components)
{
  m_extraLogLevels = 0;
  for (std::vector<CVariant>::const_iterator it = components.begin(); it != components.end(); ++it)
  {
    if (!it->isInteger())
      continue;

    m_extraLogLevels |= static_cast<int>(it->asInteger());
  }
}

std::string CAdvancedSettings::GetMusicExtensions() const
{
  std::string result(m_musicExtensions);

  VECADDONS codecs;
  CAddonMgr::GetInstance().GetAddons(ADDON_AUDIODECODER, codecs);
  for (size_t i=0;i<codecs.size();++i)
  {
    std::shared_ptr<CAudioDecoder> dec(std::static_pointer_cast<CAudioDecoder>(codecs[i]));
    result += '|';
    result += dec->GetExtensions();
  }

  return result;
}
