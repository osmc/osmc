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
from time import gmtime, strftime

if sys.version_info < (2, 7):
    import simplejson
else:
    import json as simplejson

__addon__        = xbmcaddon.Addon()

PLOT_ENABLE = True

class LibraryFunctions():
    def __init__(self):
        self.WINDOW = xbmcgui.Window(10000)
        self.LIMIT = int(__addon__.getSetting("limit"))
        self.RECENTITEMS_UNPLAYED = __addon__.getSetting("recentitems_unplayed")  == 'true'
        self.RANDOMITEMS_UNPLAYED = __addon__.getSetting("randomitems_unplayed")  == 'true'
        
        
    def _get_data( self, type, useCache ):
        # Check if data is being refreshed elsewhere
        if self.WINDOW.getProperty( type + "-data" ) == "LOADING":
            count = 0
            while count < 30:
                xbmc.sleep( 100 )
                count += 1
                if not self.WINDOW.getProperty( type + "-data" ) == "LOADING":
                    # Data has just been refreshed, return it
                    return self.WINDOW.getProperty( type + "-data" )
        
        if useCache:
            # Check whether there is saved data
            if self.WINDOW.getProperty( type + "-data" ) is not "":
                return self.WINDOW.getProperty( type + "-data" )
                
        # We haven't got any data, so don't send back anything
        return None
        
        
    def _fetch_random_movies( self, useCache = False ):
        data = self._get_data( "randommovies", useCache )
        if data is not None:
            return data
        
        # Set that we're getting updated data
        self.WINDOW.setProperty( "randommovies-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0",  "id": 1, "method": "VideoLibrary.GetMovies", "params": {"properties": ["title", "originaltitle", "votes", "playcount", "year", "genre", "studio", "country", "tagline", "plot", "runtime", "file", "plotoutline", "lastplayed", "trailer", "rating", "resume", "art", "streamdetails", "mpaa", "director", "writer", "cast", "dateadded"], "limits": {"end": %d},' % self.LIMIT
        if self.RANDOMITEMS_UNPLAYED:
            json_query = xbmc.executeJSONRPC('%s "sort": {"method": "random" }, "filter": {"field": "playcount", "operator": "lessthan", "value": "1"}}}' %json_string)
        else:
            json_query = xbmc.executeJSONRPC('%s "sort": {"method": "random" } }}' %json_string)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        
        self.WINDOW.setProperty( "randommovies-data", json_query )
        self.WINDOW.setProperty( "randommovies",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query
        
    def _fetch_random_episodes( self, useCache = False ):
        data = self._get_data( "randomepisodes", useCache )
        if data is not None:
            return data
            
        # Set that we're getting updated data
        self.WINDOW.setProperty( "randomepisodes-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0", "id": 1, "method": "VideoLibrary.GetEpisodes", "params": { "properties": ["title", "playcount", "season", "episode", "showtitle", "plot", "file", "rating", "resume", "tvshowid", "art", "streamdetails", "firstaired", "runtime", "writer", "cast", "dateadded", "lastplayed"], "limits": {"end": %d},' %self.LIMIT
        if self.RANDOMITEMS_UNPLAYED:
            json_query = xbmc.executeJSONRPC('%s "sort": {"method": "random" }, "filter": {"field": "playcount", "operator": "lessthan", "value": "1"}}}' %json_string)
        else:
            json_query = xbmc.executeJSONRPC('%s "sort": {"method": "random" }}}' %json_string)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        
        self.WINDOW.setProperty( "randomepisodes-data", json_query )
        self.WINDOW.setProperty( "randomepisodes",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query

    def _fetch_random_songs( self, useCache = False ):
        data = self._get_data( "randomsongs", useCache )
        if data is not None:
            return data
            
        # Set that we're getting updated data
        self.WINDOW.setProperty( "randomsongs-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0", "id": 1, "method": "AudioLibrary.GetSongs", "params": {"properties": ["title", "playcount", "genre", "artist", "album", "year", "file", "thumbnail", "fanart", "rating", "lastplayed"], "filter": {"field": "playcount", "operator": "lessthan", "value": "1"}, "limits": {"end": %d},' %self.LIMIT
        if self.RANDOMITEMS_UNPLAYED == "True":
            json_query = xbmc.executeJSONRPC('%s "sort": {"method": "random"}, "filter": {"field": "playcount", "operator": "lessthan", "value": "1"}}}'  %json_string)
        else:
            json_query = xbmc.executeJSONRPC('%s  "sort": {"method": "random"}}}'  %json_string)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        
        self.WINDOW.setProperty( "randomsongs-data", json_query )
        self.WINDOW.setProperty( "randomsongs",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query
        
    def _fetch_random_albums( self, useCache = False ):
        data = self._get_data( "randomalbums", useCache )
        if data is not None:
            return data
            
        # Set that we're getting updated data
        self.WINDOW.setProperty( "randomalbums-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0", "id": 1, "method": "AudioLibrary.GetAlbums", "params": {"properties": ["title", "description", "albumlabel", "theme", "mood", "style", "type", "artist", "genre", "year", "thumbnail", "fanart", "rating", "playcount"], "limits": {"end": %d},' %self.LIMIT
        json_query = xbmc.executeJSONRPC('%s "sort": {"method": "random"}}}' %json_string)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        
        self.WINDOW.setProperty( "randomalbums-data", json_query )
        self.WINDOW.setProperty( "randomalbums",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query

            
    def _fetch_recent_movies( self, useCache = False ):
        data = self._get_data( "recentmovies", useCache )
        if data is not None:
            return data
            
        # Set that we're getting updated data
        self.WINDOW.setProperty( "recentmovies-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0",  "id": 1, "method": "VideoLibrary.GetMovies", "params": {"properties": ["title", "originaltitle", "votes", "playcount", "year", "genre", "studio", "country", "tagline", "plot", "runtime", "file", "plotoutline", "lastplayed", "trailer", "rating", "resume", "art", "streamdetails", "mpaa", "director", "writer", "cast", "dateadded"], "limits": {"end": %d},' % self.LIMIT
        if self.RECENTITEMS_UNPLAYED:
            json_query = xbmc.executeJSONRPC('%s "sort": {"order": "descending", "method": "dateadded"}, "filter": {"field": "playcount", "operator": "is", "value": "0"}}}' %json_string)
        else:
            json_query = xbmc.executeJSONRPC('%s "sort": {"order": "descending", "method": "dateadded"}}}' %json_string)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        
        self.WINDOW.setProperty( "recentmovies-data", json_query )
        self.WINDOW.setProperty( "recentmovies",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        self.WINDOW.setProperty( "recentvideos",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query
        
    def _fetch_recent_episodes( self, useCache = False ):
        data = self._get_data( "recentepisodes", useCache )
        if data is not None:
            return data
            
        # Set that we're getting updated data
        self.WINDOW.setProperty( "recentepisodes-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0", "id": 1, "method": "VideoLibrary.GetEpisodes", "params": { "properties": ["title", "playcount", "season", "episode", "showtitle", "plot", "file", "rating", "resume", "tvshowid", "art", "streamdetails", "firstaired", "runtime", "writer", "cast", "dateadded", "lastplayed"], "limits": {"end": %d},' %self.LIMIT
        if self.RECENTITEMS_UNPLAYED:
            json_query = xbmc.executeJSONRPC('%s "sort": {"order": "descending", "method": "dateadded"}, "filter": {"field": "playcount", "operator": "lessthan", "value": "1"}}}' %json_string)
        else:
            json_query = xbmc.executeJSONRPC('%s "sort": {"order": "descending", "method": "dateadded"}}}' %json_string)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        
        self.WINDOW.setProperty( "recentepisodes-data", json_query )
        self.WINDOW.setProperty( "recentepisodes",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        self.WINDOW.setProperty( "recentvideos",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query
        
    def _fetch_recent_albums( self, useCache = False ):
        data = self._get_data( "recentalbums", useCache )
        if data is not None:
            return data
            
        # Set that we're getting updated data
        self.WINDOW.setProperty( "recentalbums-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0", "id": 1, "method": "AudioLibrary.GetAlbums", "params": {"properties": ["title", "description", "albumlabel", "theme", "mood", "style", "type", "artist", "genre", "year", "thumbnail", "fanart", "rating", "playcount"], "limits": {"end": %d},' %self.LIMIT
        json_query = xbmc.executeJSONRPC('%s "sort": {"order": "descending", "method": "dateadded" }}}' %json_string)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        
        self.WINDOW.setProperty( "recentalbums-data", json_query )
        self.WINDOW.setProperty( "recentalbums",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query
        
        
    def _fetch_recommended_movies( self, useCache = False ):
        data = self._get_data( "recommendedmovies", useCache )
        if data is not None:
            return data
            
        # Set that we're getting updated data
        self.WINDOW.setProperty( "recommendedmovies-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0",  "id": 1, "method": "VideoLibrary.GetMovies", "params": {"properties": ["title", "originaltitle", "votes", "playcount", "year", "genre", "studio", "country", "tagline", "plot", "runtime", "file", "plotoutline", "lastplayed", "trailer", "rating", "resume", "art", "streamdetails", "mpaa", "director", "writer", "cast", "dateadded"], "limits": {"end": %d},' % self.LIMIT
        json_query = xbmc.executeJSONRPC('%s "sort": {"order": "descending", "method": "lastplayed"}, "filter": {"field": "inprogress", "operator": "true", "value": ""}}}' %json_string)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        
        self.WINDOW.setProperty( "recommendedmovies-data", json_query )
        self.WINDOW.setProperty( "recommendedmovies",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query
    
    def _fetch_recommended_episodes( self, useCache = False ):
        data = self._get_data( "recommendedepisodes", useCache )
        if data is not None:
            return data
            
        # Set that we're getting updated data
        self.WINDOW.setProperty( "recommendedepisodes-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0", "id": 1, "method": "VideoLibrary.GetEpisodes", "params": { "properties": ["title", "playcount", "season", "episode", "showtitle", "plot", "file", "rating", "resume", "tvshowid", "art", "streamdetails", "firstaired", "runtime", "writer", "cast", "dateadded", "lastplayed"], "limits": {"end": %d},' %self.LIMIT
        json_query = xbmc.executeJSONRPC('{"jsonrpc": "2.0", "method": "VideoLibrary.GetTVShows", "params": {"properties": ["title", "studio", "mpaa", "file", "art"], "sort": {"order": "descending", "method": "lastplayed"}, "filter": {"field": "inprogress", "operator": "true", "value": ""}, "limits": {"end": %d}}, "id": 1}' %self.LIMIT)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        json_query1 = simplejson.loads(json_query)
        if json_query1.has_key('result') and json_query1['result'].has_key('tvshows'):
            for item in json_query1['result']['tvshows']:
                if xbmc.abortRequested:
                    break
                json_query2 = xbmc.executeJSONRPC('{"jsonrpc": "2.0", "method": "VideoLibrary.GetEpisodes", "params": {"tvshowid": %d, "properties": ["title", "playcount", "plot", "season", "episode", "showtitle", "file", "lastplayed", "rating", "resume", "art", "streamdetails", "firstaired", "runtime", "writer", "cast", "dateadded"], "sort": {"method": "episode"}, "filter": {"field": "playcount", "operator": "is", "value": "0"}, "limits": {"end": 1}}, "id": 1}' %item['tvshowid'])
                json_query2 = unicode(json_query2, 'utf-8', errors='ignore')
                self.WINDOW.setProperty( "recommendedepisodes-data-" + str(item['tvshowid']), json_query2)
        
        self.WINDOW.setProperty( "recommendedepisodes-data", json_query )
        self.WINDOW.setProperty( "recommendedepisodes",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query
        
    def _fetch_recommended_albums( self, useCache = False ):
        data = self._get_data( "recommendedalbums", useCache )
        if data is not None:
            return data
            
        # Set that we're getting updated data
        self.WINDOW.setProperty( "recommendedalbums-data", "LOADING" )
        
        json_string = '{"jsonrpc": "2.0", "id": 1, "method": "AudioLibrary.GetAlbums", "params": {"properties": ["title", "description", "albumlabel", "theme", "mood", "style", "type", "artist", "genre", "year", "thumbnail", "fanart", "rating", "playcount"], "limits": {"end": %d},' %self.LIMIT
        json_query = xbmc.executeJSONRPC('%s "sort": {"order": "descending", "method": "playcount" }}}' %json_string)
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        
        self.WINDOW.setProperty( "recommendedalbums-data", json_query )
        self.WINDOW.setProperty( "recommendedalbums",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        return json_query
            