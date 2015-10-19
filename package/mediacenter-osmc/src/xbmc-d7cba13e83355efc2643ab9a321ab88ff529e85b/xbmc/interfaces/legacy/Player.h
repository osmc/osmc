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

#pragma once

#include "cores/playercorefactory/PlayerCoreFactory.h"

#include "ListItem.h"
#include "PlayList.h"
#include "InfoTagVideo.h"
#include "Exception.h"
#include "AddonString.h"
#include "InfoTagMusic.h"
#include "InfoTagRadioRDS.h"
#include "AddonCallback.h"
#include "Alternative.h"

#include "swighelper.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(PlayerException);

    typedef Alternative<String, const PlayList* > PlayParameter;

    // This class is a merge of what was previously in xbmcmodule/player.h
    //  and xbmcmodule/PythonPlayer.h without the python references. The
    //  queuing and handling of asynchronous callbacks is done internal to
    //  this class.

    /**
     * Player class.
     * 
     * Player() -- Creates a new Player class.
     */
    class Player : public AddonCallback, public IPlayerCallback
    {
    private:
      int iPlayList;
      EPLAYERCORES playerCore;

      void playStream(const String& item = emptyString, const XBMCAddon::xbmcgui::ListItem* listitem = NULL, bool windowed = false);
      void playPlaylist(const PlayList* playlist = NULL,
                        bool windowed = false, int startpos=-1);
      void playCurrent(bool windowed = false);

    public:
#ifndef SWIG
	  static PlayParameter defaultPlayParameter;
#endif

      // Construct a Player proxying the given generated binding. The 
      //  construction of a Player needs to identify whether or not any 
      //  callbacks will be executed asynchronously or not.
      Player(int playerCore = EPC_NONE);
      virtual ~Player(void);

      /**
       * play([item, listitem, windowed, startpos]) -- Play this item.\n
       * \n
       * item           : [opt] string - filename, url or playlist.\n
       * listitem       : [opt] listitem - used with setInfo() to set different infolabels.\n
       * windowed       : [opt] bool - true=play video windowed, false=play users preference.(default)\n
       * startpos       : [opt] int - starting position when playing a playlist. Default = -1\n
       * \n
       * *Note, If item is not given then the Player will try to play the current item\n
       *        in the current playlist.\n
       * \n
       *        You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * example:\n
       *   - listitem = xbmcgui.ListItem('Ironman')\n
       *   - listitem.setInfo('video', {'Title': 'Ironman', 'Genre': 'Science Fiction'})\n
       *   - xbmc.Player().play(url, listitem, windowed)\n
       *   - xbmc.Player().play(playlist, listitem, windowed, startpos)\n
       */
      void play(const PlayParameter& item = Player::defaultPlayParameter, 
                const XBMCAddon::xbmcgui::ListItem* listitem = NULL, bool windowed = false, int startpos = -1);

      /**
       * stop() -- Stop playing.
       */
      void stop();

      /**
       * pause() -- Pause or resume playing if already paused.
       */
      void pause();

      /**
       * playnext() -- Play next item in playlist.
       */
      void playnext();

      /**
       * playprevious() -- Play previous item in playlist.
       */
      void playprevious();

      /**
       * playselected() -- Play a certain item from the current playlist.
       */
      void playselected(int selected);

      /**
       * onPlayBackStarted() -- onPlayBackStarted method.
       * 
       * Will be called when Kodi starts playing a file
       */
      // Player_OnPlayBackStarted
      virtual void onPlayBackStarted();


      /**
       * onPlayBackEnded() -- onPlayBackEnded method.
       * 
       * Will be called when Kodi stops playing a file
       */
      // Player_OnPlayBackEnded
      virtual void onPlayBackEnded();

      /**
       * onPlayBackStopped() -- onPlayBackStopped method.
       * 
       * Will be called when user stops Kodi playing a file
       */
      // Player_OnPlayBackStopped
      virtual void onPlayBackStopped();

      /**
       * onPlayBackPaused() -- onPlayBackPaused method.
       * 
       * Will be called when user pauses a playing file
       */
      // Player_OnPlayBackPaused
      virtual void onPlayBackPaused();

      /**
       * onPlayBackResumed() -- onPlayBackResumed method.
       * 
       * Will be called when user resumes a paused file
       */
      // Player_OnPlayBackResumed
      virtual void onPlayBackResumed();

      /**
       * onQueueNextItem() -- onQueueNextItem method.
       * 
       * Will be called when user queues the next item
       */
      virtual void onQueueNextItem();

      /**
       * onPlayBackSpeedChanged(speed) -- onPlayBackSpeedChanged method.
       * 
       * speed          : integer - current speed of player.
       * 
       * *Note, negative speed means player is rewinding, 1 is normal playback speed.
       * 
       * Will be called when players speed changes. (eg. user FF/RW)
       */
      virtual void onPlayBackSpeedChanged(int speed);

      /**
       * onPlayBackSeek(time, seekOffset) -- onPlayBackSeek method.
       * 
       * time           : integer - time to seek to.\n
       * seekOffset     : integer - ?.
       * 
       * Will be called when user seeks to a time
       */
      virtual void onPlayBackSeek(int time, int seekOffset);

      /**
       * onPlayBackSeekChapter(chapter) -- onPlayBackSeekChapter method.
       * 
       * chapter        : integer - chapter to seek to.
       * 
       * Will be called when user performs a chapter seek
       */
      virtual void onPlayBackSeekChapter(int chapter);

      /**
       * isPlaying() -- returns True is Kodi is playing a file.
       */
      // Player_IsPlaying
      bool isPlaying();

      /**
       * isPlayingAudio() -- returns True is Kodi is playing an audio file.
       */
      // Player_IsPlayingAudio
      bool isPlayingAudio();

      /**
       * isPlayingVideo() -- returns True if Kodi is playing a video.
       */
      // Player_IsPlayingVideo
      bool isPlayingVideo();

      /**
       * isPlayingRDS() -- returns True if xbmc is playing a radio data system (RDS).
       */
      // Player_IsPlayingRDS
      bool isPlayingRDS();

      /**
       * getPlayingFile() -- returns the current playing file as a string.\n
       * Note: For LiveTV, returns a pvr:// url which is not translatable to an OS specific file or external url\n
       * \n
       * Throws: Exception, if player is not playing a file.\n
       */
      // Player_GetPlayingFile
      String getPlayingFile();

      /**
       * getTime() -- Returns the current time of the current playing media as fractional seconds.
       * 
       * Throws: Exception, if player is not playing a file.
       */
      // Player_GetTime
      double getTime();

      /**
       * seekTime() -- Seeks the specified amount of time as fractional seconds.
       *               The time specified is relative to the beginning of the
       *               currently playing media file.
       * 
       * Throws: Exception, if player is not playing a file.
       */
      // Player_SeekTime
      void seekTime(double seekTime);

      /**
       * setSubtitles() -- set subtitle file and enable subtitlesn
       */
      // Player_SetSubtitles
      void setSubtitles(const char* subtitleFile);

      // Player_ShowSubtitles
      /**
       * showSubtitles(visible) -- enable/disable subtitles
       * 
       * visible        : boolean - True for visible subtitles.
       *
       * example:
       * - xbmc.Player().showSubtitles(True)
       */
      void showSubtitles(bool bVisible);

      /**
       * getSubtitles() -- get subtitle stream name
       */
      // Player_GetSubtitles
      String getSubtitles();

      /**
       * DisableSubtitles() -- disable subtitles
       */
      // Player_DisableSubtitles
      void disableSubtitles();

      // Player_getAvailableSubtitleStreams
      /**
       * getAvailableSubtitleStreams() -- get Subtitle stream names
       */
      std::vector<String> getAvailableSubtitleStreams();

      // Player_setSubtitleStream
      /**
       * setSubtitleStream(stream) -- set Subtitle Stream 
       * 
       * stream           : int
       * 
       * example:
       *   - setSubtitleStream(1)
       */
      void setSubtitleStream(int iStream);

      /**
       * getVideoInfoTag() -- returns the VideoInfoTag of the current playing Movie.
       * 
       * Throws: Exception, if player is not playing a file or current file is not a movie file.
       */
      InfoTagVideo* getVideoInfoTag();

      /**
       * getMusicInfoTag() -- returns the MusicInfoTag of the current playing 'Song'.
       * 
       * Throws: Exception, if player is not playing a file or current file is not a music file.
       */
      // Player_GetMusicInfoTag
      InfoTagMusic* getMusicInfoTag();

      /**
       * getRadioRDSInfoTag() -- returns the RadioRDSInfoTag of the current playing 'Radio Song if present'.
       *
       * Throws: Exception, if player is not playing a file or current file is not a rds file.
       */
      // Player_GetRadioRDSInfoTag
      InfoTagRadioRDS* getRadioRDSInfoTag() throw (PlayerException);

      /**
       * getTotalTime() -- Returns the total time of the current playing media in
       *                   seconds.  This is only accurate to the full second.
       *
       * Throws: Exception, if player is not playing a file.
       */
      double getTotalTime();

      // Player_getAvailableAudioStreams
      /**
       * getAvailableAudioStreams() -- get Audio stream names
       */
      std::vector<String> getAvailableAudioStreams();

      /**
       * setAudioStream(stream) -- set Audio Stream.
       * 
       * stream           : int
       * 
       * example:
       * 
       *    - setAudioStream(1)
       */
      void setAudioStream(int iStream);

#ifndef SWIG
      SWIGHIDDENVIRTUAL void OnPlayBackStarted();
      SWIGHIDDENVIRTUAL void OnPlayBackEnded();
      SWIGHIDDENVIRTUAL void OnPlayBackStopped();
      SWIGHIDDENVIRTUAL void OnPlayBackPaused();
      SWIGHIDDENVIRTUAL void OnPlayBackResumed();
      SWIGHIDDENVIRTUAL void OnQueueNextItem();
      SWIGHIDDENVIRTUAL void    OnPlayBackSpeedChanged(int iSpeed);
      SWIGHIDDENVIRTUAL void    OnPlayBackSeek(int iTime, int seekOffset);
      SWIGHIDDENVIRTUAL void    OnPlayBackSeekChapter(int iChapter);
#endif

    protected:
    };
  }
}

