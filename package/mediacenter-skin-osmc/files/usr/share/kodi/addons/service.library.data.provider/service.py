#!/usr/bin/python
# -*- coding: utf-8 -*-
#
#     Copyright (C) 2012 Team-XBMC
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program. If not, see <http://www.gnu.org/licenses/>.
#
#    This script is based on service.skin.widgets
#    Thanks to the original authors

import os
import sys
import xbmc
import xbmcgui
import xbmcplugin
import xbmcaddon
import xbmcvfs
import random
import urllib
import datetime
from traceback import print_exc
from time import gmtime, strftime

if sys.version_info < (2, 7):
    import simplejson
else:
    import json as simplejson

__addon__        = xbmcaddon.Addon()
__addonversion__ = __addon__.getAddonInfo('version')
__addonid__      = __addon__.getAddonInfo('id')
__addonname__    = __addon__.getAddonInfo('name')
__localize__     = __addon__.getLocalizedString

import library
LIBRARY = library.LibraryFunctions()

def log(txt):
    message = '%s: %s' % (__addonname__, txt.encode('ascii', 'ignore'))
    xbmc.log(msg=message, level=xbmc.LOGDEBUG)

class Main:
    def __init__(self):
        self.WINDOW = xbmcgui.Window(10000)

        # clear our property, if another instance is already running it should stop now
        self._init_vars()
        self.WINDOW.clearProperty('LibraryDataProvider_Running')
        a_total = datetime.datetime.now()
        self._fetch_random()
        self._fetch_recent()
        self._fetch_recommended()
        b_total = datetime.datetime.now()
        c_total = b_total - a_total
        log('Total time needed for all queries: %s' % c_total)
        # give a possible other instance some time to notice the empty property
        self.WINDOW.setProperty('LibraryDataProvider_Running', 'true')
        self._daemon()
            
            
    def _init_vars(self):
        self.WINDOW = xbmcgui.Window(10000)
        self.Player = Widgets_Player(action = self._update)
        self.Monitor = Widgets_Monitor(update_listitems = self._update)

            
    def _fetch_random( self ):
        LIBRARY._fetch_random_movies()
        LIBRARY._fetch_random_episodes()
        LIBRARY._fetch_random_songs()
        LIBRARY._fetch_random_albums()

        
    def _fetch_recent( self ):
        LIBRARY._fetch_recent_movies()
        LIBRARY._fetch_recent_episodes()
        LIBRARY._fetch_recent_albums()
            
    
    def _fetch_recommended( self ):
        LIBRARY._fetch_recommended_movies()
        LIBRARY._fetch_recommended_episodes()
        LIBRARY._fetch_recommended_albums()
        
        
    def _daemon(self):
        # deamon is meant to keep script running at all time
        count = 0
        home_update = False
        while (not xbmc.abortRequested) and self.WINDOW.getProperty('LibraryDataProvider_Running') == 'true':
            xbmc.sleep(1000)
            if not xbmc.Player().isPlayingVideo():
                # Update random items
                count += 1
                if count == 1200: # 10 minutes
                    self._fetch_random()
                    count = 0    # reset counter
                    
    def _update(self, type):
        xbmc.sleep(1000)
        if type == 'movie':
            LIBRARY._fetch_recommended_movies()
            LIBRARY._fetch_recent_movies()
        elif type == 'episode':
            LIBRARY._fetch_recommended_episodes()
            LIBRARY._fetch_recent_episodes()
        elif type == 'video':
            #only on db update
            LIBRARY._fetch_recommended_movies()
            LIBRARY._fetch_recommended_episodes()
            LIBRARY._fetch_recent_movies()
            LIBRARY._fetch_recent_episodes()
        elif type == 'music':
            LIBRARY._fetch_recommended_albums()
            LIBRARY._fetch_recent_albums()
    
class Widgets_Monitor(xbmc.Monitor):
    def __init__(self, *args, **kwargs):
        xbmc.Monitor.__init__(self)
        self.update_listitems = kwargs['update_listitems']

    def onDatabaseUpdated(self, database):
        self.update_listitems(database)
        

class Widgets_Player(xbmc.Player):
    def __init__(self, *args, **kwargs):
        xbmc.Player.__init__(self)
        self.type = ""
        self.action = kwargs[ "action" ]
        self.substrings = [ '-trailer', 'http://' ]

    def onPlayBackStarted(self):
        xbmc.sleep(1000)
        # Set values based on the file content
        if (self.isPlayingAudio()):
            self.type = "music"  
        else:
            if xbmc.getCondVisibility('VideoPlayer.Content(movies)'):
                filename = ''
                isMovie = True
                try:
                    filename = self.getPlayingFile()
                except:
                    pass
                if filename != '':
                    for string in self.substrings:
                        if string in filename:
                            isMovie = False
                            break
                if isMovie:
                    self.type = "movie"
            elif xbmc.getCondVisibility('VideoPlayer.Content(episodes)'):
                # Check for tv show title and season to make sure it's really an episode
                if xbmc.getInfoLabel('VideoPlayer.Season') != "" and xbmc.getInfoLabel('VideoPlayer.TVShowTitle') != "":
                    self.type = "episode"

    def onPlayBackEnded(self):
        self.onPlayBackStopped()

    def onPlayBackStopped(self):
        if self.type == 'movie':
            self.action('movie')
        elif self.type == 'episode':
            self.action('episode')
        elif self.type == 'music':
            self.action('music')
        self.type = ""
    
log('service version %s started' % __addonversion__)
Main()
log('service version %s stopped' % __addonversion__)