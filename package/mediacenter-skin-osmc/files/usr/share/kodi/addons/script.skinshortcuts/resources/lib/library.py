# coding=utf-8
import os, sys, datetime, unicodedata
import xbmc, xbmcgui, xbmcvfs, urllib
import xml.etree.ElementTree as xmltree
import thread
from xml.dom.minidom import parse
from xml.sax.saxutils import escape as escapeXML
from traceback import print_exc
from unidecode import unidecode
from unicodeutils import try_decode
import datafunctions, nodefunctions
DATA = datafunctions.DataFunctions()
NODE = nodefunctions.NodeFunctions()

if sys.version_info < (2, 7):
    import simplejson
else:
    import json as simplejson

__addon__        = sys.modules[ "__main__" ].__addon__
__addonid__      = sys.modules[ "__main__" ].__addonid__
__addonversion__ = sys.modules[ "__main__" ].__addonversion__
__cwd__          = __addon__.getAddonInfo('path').decode("utf-8")
__datapath__     = os.path.join( xbmc.translatePath( "special://profile/addon_data/" ).decode('utf-8'), __addonid__ )
__datapathalt__  = os.path.join( "special://profile/", "addon_data", __addonid__ )
__skinpath__     = xbmc.translatePath( "special://skin/shortcuts/" ).decode('utf-8')
__defaultpath__  = xbmc.translatePath( os.path.join( __cwd__, 'resources', 'shortcuts').encode("utf-8") ).decode("utf-8")
__language__     = sys.modules[ "__main__" ].__language__
__cwd__          = sys.modules[ "__main__" ].__cwd__
__xbmcversion__  = xbmc.getInfoLabel( "System.BuildVersion" ).split(".")[0]

def log(txt):
    if __xbmcversion__ == "13" or __addon__.getSetting( "enable_logging" ) == "true":
        try:
            if isinstance (txt,str):
                txt = txt.decode('utf-8')
            message = u'%s: %s' % (__addonid__, txt)
            xbmc.log(msg=message.encode('utf-8'), level=xbmc.LOGDEBUG)
        except:
            pass

def kodilistdir(path, walkEverything = False, stringForce = False):
    json_query = xbmc.executeJSONRPC('{"jsonrpc":"2.0","method":"Files.GetDirectory","params":{"directory":"%s","media":"files"},"id":1}' % str(path))
    json_query = unicode(json_query, 'utf-8', errors='ignore')
    json_response = simplejson.loads(json_query)
    dirs = []
    files = []
    if json_response.has_key('result') and json_response['result'].has_key('files') and json_response['result']['files'] is not None:
        for item in json_response['result']['files']:
            if item.has_key('file') and item.has_key('filetype') and item.has_key('label'):
                if item['filetype'] == 'directory' and (walkEverything or ((not item['file'].endswith('.xsp')) and (not item['file'].endswith('.xml/')) and (not item['file'].endswith('.xml')))):
                    if stringForce and item['file'].startswith(stringForce):
                        dirs.append({'path':xbmc.translatePath(item['file']), 'label':item['label']})
                    else:
                        dirs.append({'path':item['file'], 'label':item['label']})
                else:
                    if stringForce and item['file'].startswith(stringForce):
                        files.append({'path':xbmc.translatePath(item['file']), 'label':item['label']})
                    else:
                        files.append({'path':item['file'], 'label':item['label']})
    return [path,dirs,files]
        
def kodiwalk(path, walkEverything = False, stringForce = False, paths = None):
    if paths is None or len(paths) == 0:
        paths = [kodilistdir(path, walkEverything, stringForce)]
    else:
        paths.append( kodilistdir(path, walkEverything, stringForce) )
    dirs = paths[-1][1]
    for dir in dirs:
        if stringForce:
            return kodiwalk(dir['path'].replace(xbmc.translatePath(stringForce),stringForce).replace('\\','/'), walkEverything, stringForce, paths)
        else:
            return kodiwalk(dir['path'], walkEverything, stringForce, paths)
    return paths

class LibraryFunctions():
    def __init__( self, *args, **kwargs ):
        
        # values to mark whether data from different areas of the library have been loaded
        self.loadedCommon = False
        self.loadedMoreCommands = False
        self.loadedVideoLibrary = False
        self.loadedMusicLibrary = False
        self.loadedLibrarySources = False
        self.loadedPVRLibrary = False
        self.loadedRadioLibrary = False
        self.loadedPlaylists = False
        self.loadedAddOns = False
        self.loadedFavourites = False
        self.loadedUPNP = False
        self.loadedSettings = False
        self.loadedWidgets = False
        
        self.widgetPlaylistsList = []
        
        # Empty dictionary for different shortcut types
        self.dictionaryGroupings = {"common":None,
                "commands":None,
                "video":None, "movie":None,
                "movie-flat":None,
                "tvshow":None,
                "tvshow-flat":None,
                "musicvideo":None,
                "musicvideo-flat":None,
                "customvideonode":None,
                "customvideonode-flat":None,
                "videosources":None,
                "pvr":None,
                "radio":None,
                "pvr-tv":None,
                "pvr-radio":None,
                "music":None,
                "musicsources":None,
                "picturesources":None,
                "playlist-video":None,
                "playlist-audio":None,
                "addon-program":None,
                "addon-video":None,
                "addon-audio":None,
                "addon-image":None,
                "favourite":None,
                "settings":None,
                "widgets":None,
                "widgets-classic":[] }
        self.folders = {}
        self.foldersCount = 0
        
        self.useDefaultThumbAsIcon = None
        
    def loadLibrary( self ):
        # Load all library data, for use with threading
        self.common()
        self.more()
        self.videolibrary()
        self.musiclibrary()
        self.pvrlibrary()
        self.radiolibrary()
        self.librarysources()
        self.playlists()
        self.addons()                
        self.favourites()
        self.settings()
        self.widgets()
        
        # Do a JSON query for upnp sources (so that they'll show first time the user asks to see them)
        if self.loadedUPNP == False:
            json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Files.GetDirectory", "params": { "properties": ["title", "file", "thumbnail"], "directory": "upnp://", "media": "files" } }')
            self.loadedUPNP = True

    # ==============================================
    # === BUILD/DISPLAY AVAILABLE SHORTCUT NODES ===
    # ==============================================
    
    def retrieveGroup( self, group, flat = True, grouping = None ):
        trees = [DATA._get_overrides_skin(), DATA._get_overrides_script()]
        nodes = None
        for tree in trees:
            if tree is not None:
                if flat:
                    nodes = tree.find( "flatgroupings" )
                    if nodes is not None:
                        nodes = nodes.findall( "node" )
                elif grouping is None:
                    nodes = tree.find( "groupings" )
                else:
                    nodes = tree.find( "%s-groupings" %( grouping ) )
            if nodes is not None:
                break                
                
        if nodes is None:
            return [ "Error", [] ]
            
        returnList = []
        
        if flat:
            # Flat groupings
            count = 0
            # Cycle through nodes till we find the one specified
            for node in nodes:
                count += 1
                if "condition" in node.attrib:
                    if not xbmc.getCondVisibility( node.attrib.get( "condition" ) ):
                        group += 1
                        continue
                if "version" in node.attrib:
                    version = node.attrib.get( "version" )
                    if __xbmcversion__ != version and DATA.checkVersionEquivalency( version, node.attrib.get( "condition" ), "groupings" ) == False:
                        group += 1
                        continue
                if count == group:
                    # We found it :)
                    return( node.attrib.get( "label" ), self.buildNodeListing( node, True ) )
                    for subnode in node:
                        if subnode.tag == "content":
                            returnList = returnList + self.retrieveContent( subnode.text )
                        if subnode.tag == "shortcut":
                            returnList.append( self._create( [subnode.text, subnode.attrib.get( "label" ), subnode.attrib.get( "type" ), {"icon": subnode.attrib.get( "icon" )}] ) )

                    return [node.attrib.get( "label" ), returnList]
                    
            return ["Error", []]
            
        else:
            # Heirachical groupings
            if group == "":
                # We're going to get the root nodes
                windowTitle = __language__(32048)
                if grouping == "widget":
                    windowTitle = __language__(32044)
                return [ windowTitle, self.buildNodeListing( nodes, False ) ]
            else:
                groups = group.split( "," )
                
                nodes = [ "", nodes ]
                for groupNum in groups:
                    nodes = self.getNode( nodes[1], int( groupNum ) )
                    
                return [ nodes[0], self.buildNodeListing( nodes[1], False ) ]
                        
    def getNode( self, tree, number ):
        count = 0
        for subnode in tree:
            count += 1
            # If not visible, skip it
            if "condition" in subnode.attrib:
                if not xbmc.getCondVisibility( subnode.attrib.get( "condition" ) ):
                    number += 1
                    continue
            if "version" in subnode.attrib:
                version = subnode.attrib.get( "version" )
                if __xbmcversion__ != version and DATA.checkVersionEquivalency( version, subnode.attrib.get( "condition" ), "groupings" ) == False:
                    number += 1
                    continue

            if count == number:
                label = DATA.local( subnode.attrib.get( "label" ) )[2]
                return [ label, subnode ]
                
    def buildNodeListing( self, nodes, flat ):
        returnList = []
        count = 0
        for node in nodes:
            if "condition" in node.attrib:
                if not xbmc.getCondVisibility( node.attrib.get( "condition" ) ):
                    continue
            if "version" in node.attrib:
                version = node.attrib.get( "version" )
                if __xbmcversion__ != version and DATA.checkVersionEquivalency( version, node.attrib.get( "condition" ), "groupings" ) == False:
                    continue
            count += 1
            if node.tag == "content":
                returnList = returnList + self.retrieveContent( node.text )
            if node.tag == "shortcut":
                shortcutItem = self._create( [node.text, node.attrib.get( "label" ), node.attrib.get( "type" ), {"icon": node.attrib.get( "icon" )}] )
                if "widget" in node.attrib:
                    # This is a widget shortcut, so add the relevant widget information
                    shortcutItem.setProperty( "widget", node.attrib.get( "widget" ) )
                    shortcutItem.setProperty( "widgetName", node.attrib.get( "label" ) )
                    shortcutItem.setProperty( "widgetPath", node.text )
                    if "widgetType" in node.attrib:
                        shortcutItem.setProperty( "widgetType", node.attrib.get( "widgetType" ) )
                    else:
                        shortcutItem.setProperty( "widgetType", "" )
                    if "widgetTarget" in node.attrib:
                        shortcutItem.setProperty( "widgetTarget", node.attrib.get( "widgetTarget" ) )
                    else:
                        shortcutItem.setProperty( "widgetTarget", "" )
                returnList.append( shortcutItem )
                #returnList.append( self._create( [node.text, node.attrib.get( "label" ), node.attrib.get( "type" ), {"icon": node.attrib.get( "icon" )}] ) )
            if node.tag == "node" and flat == False:
                returnList.append( self._create( ["||NODE||" + str( count ), node.attrib.get( "label" ), "", {"icon": "DefaultFolder.png"}] ) )
                
        return returnList
                
    def retrieveContent( self, content ):
        if content == "upnp-video":
            items = [ self._create(["||UPNP||", "32070", "32069", {"icon": "DefaultFolder.png"}]) ]
        elif content == "upnp-music":
            items = [ self._create(["||UPNP||", "32070", "32073", {"icon": "DefaultFolder.png"}]) ]
            
        elif self.dictionaryGroupings[ content ] is None:
            # The data hasn't been loaded yet
            items = self.loadGrouping( content )
        else:
            items = self.dictionaryGroupings[ content ]

        if items is not None:
            items = self.checkForFolder( items )
        else:
            items = []

        # Add widget information for video/audio nodes
        if content in [ "video", "music" ]:
            # Video nodes - add widget information
            for listitem in items:
                path = listitem.getProperty( "path" )
                if path.lower().startswith( "activatewindow" ):
                    path = DATA.getListProperty( path )
                    listitem.setProperty( "widget", "Library" )
                    if content == "video":
                        listitem.setProperty( "widgetType", "video" )
                        listitem.setProperty( "widgetTarget", "video" )
                    else:
                        listitem.setProperty( "widgetType", "audio" )
                        listitem.setProperty( "widgetTarget", "music" )
                    listitem.setProperty( "widgetName", listitem.getLabel() )
                    listitem.setProperty( "widgetPath", path )

                    widgetType = NODE.get_mediaType( path )
                    if widgetType != "unknown":
                        listitem.setProperty( "widgetType", widgetType )
            
        # Check for any icon overrides for these items
        tree = DATA._get_overrides_skin()
        if tree is None:
            return items
            
        for item in items:
            item = self._get_icon_overrides( tree, item, content )
            
        return items
            
    def checkForFolder( self, items ):
        # This function will check for any folders in the listings that are being returned
        # and, if found, move their sub-items into a property
        returnItems = []
        for item in items:
            if isinstance( item, list ):
                self.foldersCount += 1
                self.folders[ str( self.foldersCount ) ] = item[1]
                newItem = item[0]
                newItem.setProperty( "folder", str( self.foldersCount ) )
                returnItems.append( newItem )
            else:
                returnItems.append( item )
            
        return( returnItems )
        
    def loadGrouping( self, content ):
        # We'll be called if the data for a wanted group hasn't been loaded yet
        if content == "common":
            self.common()
        if content  == "commands":
            self.more()            
        if content == "movie" or content == "tvshow" or content == "musicvideo" or content == "customvideonode" or content == "movie-flat" or content == "tvshow-flat" or content == "musicvideo-flat" or content == "customvideonode-flat":
            # These have been deprecated
            return []
        if content == "video":
            self.videolibrary()
        if content == "videosources" or content == "musicsources" or content == "picturesources":
            self.librarysources()
        if content == "music":
            self.musiclibrary()
        if content == "pvr" or content == "pvr-tv" or content == "pvr-radio":
            self.pvrlibrary()
        if content == "radio":
            self.radiolibrary()
        if content == "playlist-video" or content == "playlist-audio":
            self.playlists()
        if content == "addon-program" or content == "addon-video" or content == "addon-audio" or content == "addon-image":
            self.addons()
        if content == "favourite":
            self.favourites()
        if content == "settings":
            self.settings()
        if content == "widgets":
            self.widgets()
            
        # The data has now been loaded, return it
        return self.dictionaryGroupings[ content ]
        
    def flatGroupingsCount( self ):
        # Return how many nodes there are in the the flat grouping
        tree = DATA._get_overrides_script()
        if tree is None:
            return 1
        groupings = tree.find( "flatgroupings" )
        nodes = groupings.findall( "node" )
        count = 0
        for node in nodes:
            if "condition" in node.attrib:
                if not xbmc.getCondVisibility( node.attrib.get( "condition" ) ):
                    continue
            if "version" in node.attrib:
                if __xbmcversion__ != node.attrib.get( "version" ):
                    continue
                    
            count += 1
                
        return count
        
    
    def addToDictionary( self, group, content ):
        # This function adds content to the dictionaryGroupings - including
        # adding any skin-provided shortcuts to the group
        tree = DATA._get_overrides_skin()
        if tree is None:
            # There are no overrides to check for extra shortcuts
            self.dictionaryGroupings[ group ] = content
            return
            
        # Search for skin-provided shortcuts for this group
        originalGroup = group
        if group.endswith( "-flat" ):
            group = group.replace( "-flat", "" )
            
        if group not in [ "movie", "tvshow", "musicvideo" ]:
            for elem in tree.findall( "shortcut" ):
                if "grouping" in elem.attrib:
                    if group == elem.attrib.get( "grouping" ):
                        # We want to add this shortcut
                        label = elem.attrib.get( "label" )
                        type = elem.attrib.get( "type" )
                        thumb = elem.attrib.get( "thumbnail" )
                        icon = elem.attrib.get( "icon" )
                        
                        action = elem.text
                        
                        #if label.isdigit():
                        #    label = "::LOCAL::" + label
                            
                        if type is None:
                            type = "32024"
                        #elif type.isdigit():
                        #    type = "::LOCAL::" + type
                        
                        if icon is None:
                            icon = ""
                        if thumb is None:
                            thumb = ""

                        listitem = self._create( [action, label, type, {"icon": icon, "thumb": thumb}] )
                        
                        if "condition" in elem.attrib:
                            if xbmc.getCondVisibility( elem.attrib.get( "condition" ) ):
                                content.insert( 0, listitem )
                        else:
                            content.insert( 0, listitem )

                elif group == "common":
                    # We want to add this shortcut
                    label = elem.attrib.get( "label" )
                    type = elem.attrib.get( "type" )
                    thumb = elem.attrib.get( "thumbnail" )
                    icon = elem.attrib.get( "icon" )
                    
                    action = elem.text
                    
                    #if label.isdigit():
                    #    label = "::LOCAL::" + label
                        
                    if type is None:
                        type = "32024"
                    #elif type.isdigit():
                    #    type = "::LOCAL::" + type
                        
                    if type is None or type == "":
                        type = "Skin Provided"
                        
                    if icon is None:
                        icon = ""
                        
                    if thumb is None:
                        thumb = ""

                    listitem = self._create( [action, label, type, {"icon": icon, "thumb":thumb}] )
                    
                    if "condition" in elem.attrib:
                        if xbmc.getCondVisibility( elem.attrib.get( "condition" ) ):
                            content.append( listitem )
                    else:
                        content.append( listitem )
                    
        self.dictionaryGroupings[ originalGroup ] = content
        
    # ================================
    # === BUILD AVAILABLE SHORTCUT ===
    # ================================
    
    def _create ( self, item, allowOverrideLabel = True ):
        # Retrieve label
        localLabel = DATA.local( item[1] )[0]
        
        # Create localised label2
        displayLabel2 = DATA.local( item[2] )[2]
        shortcutType = DATA.local( item[2] )[0]
        
        if allowOverrideLabel:
            # Check for a replaced label
            replacementLabel = DATA.checkShortcutLabelOverride( item[0] )
            if replacementLabel is not None:
                
                localLabel = DATA.local( replacementLabel[0] )[0]
                    
                if len( replacementLabel ) == 2:
                    # We're also overriding the type
                    displayLabel2 = DATA.local( replacementLabel[1] )[2]
                    shortcutType = DATA.local( replacementLabel[1] )[0]
                    
        
        # Try localising it
        displayLabel = DATA.local( localLabel )[2]
        labelID = DATA.createNiceName( DATA.local( localLabel )[0] )
        
        if displayLabel.startswith( "$NUMBER[" ):
            displayLabel = displayLabel[8:-1]
        
        # Create localised label2
        displayLabel2 = DATA.local( displayLabel2 )[2]
        shortcutType = DATA.local( shortcutType )[0]
        
        # If either displayLabel starts with a $, ask Kodi to parse it for us
        if displayLabel.startswith( "$" ):
            displayLabel = xbmc.getInfoLabel( displayLabel )
        if displayLabel2.startswith( "$" ):
            displayLabel2 = xbmc.getInfoLabel( displayLabel2 )
            
        # If this launches our explorer, append a notation to the displayLabel
        if item[0].startswith( "||" ):
            displayLabel = displayLabel + "  >"
            
        # Retrieve icon and thumbnail
        if item[3]:
            if "icon" in item[3].keys() and item[ 3 ][ "icon" ] is not None:
                icon = item[3]["icon"]
            else:
                icon = "DefaultShortcut.png"
            if "thumb" in item[3].keys():
                thumbnail = item[3]["thumb"]
            else:
                thumbnail = None
        else:
            icon = "DefaultShortcut.png"
            thumbnail = None
                        
        # Check if the option to use the thumb as the icon is enabled
        if self.useDefaultThumbAsIcon is None:
            # Retrieve the choice from the overrides.xml
            tree = DATA._get_overrides_skin()
            if tree is None:
                self.useDefaultThumbAsIcon = False
            else:
                node = tree.getroot().find( "useDefaultThumbAsIcon" )
                if node is None:
                    self.useDefaultThumbAsIcon = False
                else:
                    if node.text.lower() == "true":
                        self.useDefaultThumbAsIcon = True
                    else:
                        self.useDefaultThumbAsIcon = False
            
        usedDefaultThumbAsIcon = False
        if self.useDefaultThumbAsIcon == True and thumbnail is not None:            
            icon = thumbnail
            thumbnail = None
            usedDefaultThumbAsIcon = True
            
        oldicon = None
        
        # If the icon starts with a $, ask Kodi to parse it for us
        displayIcon = icon
        iconIsVar = False
        if icon.startswith( "$" ):
            displayIcon = xbmc.getInfoLabel( icon )
            iconIsVar = True
        
        # Get a temporary labelID
        DATA._clear_labelID()
        labelID = DATA._get_labelID( labelID, item[0] )
                        
        # If the skin doesn't have the icon, replace it with DefaultShortcut.png
        if ( not displayIcon or not xbmc.skinHasImage( displayIcon ) ) and not iconIsVar:
            if not usedDefaultThumbAsIcon:
                displayIcon = "DefaultShortcut.png"
                            
        # Build listitem
        if thumbnail is not None:
            listitem = xbmcgui.ListItem(label=displayLabel, label2=displayLabel2, iconImage=displayIcon, thumbnailImage=thumbnail)
            listitem.setProperty( "thumbnail", thumbnail)
        else:
            listitem = xbmcgui.ListItem(label=displayLabel, label2=displayLabel2, iconImage=thumbnail)
        listitem.setProperty( "path", item[0] )
        listitem.setProperty( "localizedString", localLabel )
        listitem.setProperty( "shortcutType", shortcutType )
        listitem.setProperty( "icon", displayIcon )
        listitem.setProperty( "tempLabelID", labelID )
        listitem.setProperty( "defaultLabel", labelID )
        
        if displayIcon != icon:
            listitem.setProperty( "untranslatedIcon", icon )
        
        return( listitem )
                
    def _get_icon_overrides( self, tree, item, content, setToDefault = True ):
        if tree is None:
            return item
            
        oldicon = None
        newicon = item.getProperty( "icon" )
        for elem in tree.findall( "icon" ):
            if oldicon is None:
                if ("labelID" in elem.attrib and elem.attrib.get( "labelID" ) == item.getProperty( "tempLabelID" )) or ("image" in elem.attrib and elem.attrib.get( "image" ) == item.getProperty( "icon" )):
                    # LabelID matched
                    if "grouping" in elem.attrib:
                        if elem.attrib.get( "grouping" ) == content:
                            # Group also matches - change icon
                            oldicon = item.getProperty( "icon" )
                            newicon = elem.text
                    elif "group" not in elem.attrib:
                        # No group - change icon
                        oldicon = item.getProperty( "icon" )
                        newicon = elem.text
                        
        # If the icon doesn't exist, set icon to default
        setDefault = False
        if not xbmc.skinHasImage( newicon ) and setToDefault == True:
            oldicon = item.getProperty( "icon" )
            icon = "DefaultShortcut.png"
            setDefault == True

        if oldicon is not None:
            # we found an icon override
            item.setProperty( "icon", newicon )
            item.setIconImage( newicon )
            
        if setDefault == True:
            item = self._get_icon_overrides( tree, item, content, False )
            
        return item

    # ===================================
    # === LOAD VIDEO LIBRARY HEIRACHY ===
    # ===================================
    
    def videolibrary( self ):
        if self.loadedVideoLibrary is True:
            # The List has already been populated, return it
            return self.loadedVideoLibrary
        elif self.loadedVideoLibrary == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedVideoLibrary is True:
                    return self.loadedVideoLibrary
        else:
            # We're going to populate the list
            self.loadedVideoLibrary = "Loading"
            
        # Try loading custom nodes first
        try:
            if self._parse_libraryNodes( "video", "custom" ) == False:
                log( "Failed to load custom video nodes" )
                self._parse_libraryNodes( "video", "default" )
        except:
            log( "Failed to load custom video nodes" )
            print_exc()
            try:
                # Try loading default nodes
                self._parse_libraryNodes( "video", "default" )
            except:
                # Empty library
                log( "Failed to load default video nodes" )
                print_exc()
                
        self.loadedVideoLibrary = True
        return self.loadedVideoLibrary

    def _parse_libraryNodes( self, library, type ):
        #items = {"video":[], "movies":[], "tvshows":[], "musicvideos":[], "custom":{}}
        items = {}

        if library == "video":
            windowID = "10025"
            prefix = "library://video"
            action = "||VIDEO||"
        elif library == "music":
            windowID = "10502"
            prefix = "library://music"
            action = "||AUDIO||"

        rootdir = os.path.join( xbmc.translatePath( "special://profile".decode('utf-8') ), "library", library )
        if type == "custom":
            log( "Listing custom %s nodes..." %( library ) )
        else:
            rootdir = os.path.join( xbmc.translatePath( "special://xbmc".decode('utf-8') ), "system", "library", library )
            log( "Listing default %s nodes..." %( library ) )
            
        nodes = NODE.get_nodes( rootdir, prefix )
        if nodes == False or len( nodes ) == 0:
            return False
        
        items = []
        
        for key in nodes:
            # 0 = Label
            # 1 = Icon
            # 2 = Path
            # 3 = Type
            # 4 = Order
            # 5 = Media type (not folders...?)
            if nodes[ key ][ 3 ] == "folder":
                item = self._create( [ "%s%s" % ( action, nodes[ key ][ 2 ] ), nodes[ key ][ 0 ], nodes[ key ][ 3 ], { "icon": nodes[ key ][ 1 ] } ] )
            elif nodes[ key ][ 3 ] == "grouped":
                item = self._create( [ "%s%s" % ( action, nodes[ key ][ 2 ] ), nodes[ key ][ 0 ], nodes[ key ][ 3 ], { "icon": nodes[ key ][ 1 ] } ] )
            else:
                item = self._create( [ "ActivateWindow(%s,%s,return)" %( windowID, nodes[ key ][ 2 ] ), nodes[ key ][ 0 ], nodes[ key ][ 3 ], { "icon": nodes[ key ][ 1 ] } ] )
            if nodes[ key ][ 5 ] is not None:
                item.setProperty( "widgetType", nodes[ key ][ 5 ] )
                item.setProperty( "widgetTarget", library )
            items.append( item )
            
        self.addToDictionary( library, items )
    

    # ============================
    # === LOAD OTHER LIBRARIES ===
    # ============================
                
    def common( self ):
        if self.loadedCommon is True:
            return True
        elif self.loadedCommon == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedCommon is True:
                    return True
        else:
            # We're going to populate the list
            self.loadedCommon = "Loading"
        
        listitems = []
        log('Listing xbmc common items...')
        
        # Videos, Movies, TV Shows, Live TV, Music, Music Videos, Pictures, Weather, Programs,
        # Play dvd, eject tray
        # Settings, File Manager, Profiles, System Info
        listitems.append( self._create(["ActivateWindow(Videos)", "10006", "32034", {"icon": "DefaultVideo.png"} ]) )
        listitems.append( self._create(["ActivateWindow(Videos,videodb://movies/titles/,return)", "342", "32034", {"icon": "DefaultMovies.png"} ]) )
        listitems.append( self._create(["ActivateWindow(Videos,videodb://tvshows/titles/,return)", "20343", "32034", {"icon": "DefaultTVShows.png"} ]) )

        if __xbmcversion__ == "13":
            listitems.append( self._create(["ActivateWindowAndFocus(MyPVR,34,0 ,13,0)", "32022", "32034", {"icon": "DefaultTVShows.png"} ]) )
        else:
            listitems.append( self._create(["ActivateWindow(TVGuide)", "32022", "32034", {"icon": "DefaultTVShows.png"} ]) )
            listitems.append( self._create(["ActivateWindow(RadioGuide)", "32087", "32034", {"icon": "DefaultTVShows.png"} ]) )
            
        listitems.append( self._create(["ActivateWindow(Music)", "10005", "32034", {"icon": "DefaultMusicAlbums.png"} ]) )
        listitems.append( self._create(["ActivateWindow(Videos,videodb://musicvideos/titles/,return)", "20389", "32034", {"icon": "DefaultMusicVideos.png"} ] ) )
        listitems.append( self._create(["ActivateWindow(Pictures)", "10002", "32034", {"icon": "DefaultPicture.png"} ] ) )
        listitems.append( self._create(["ActivateWindow(Weather)", "12600", "32034", {} ]) )
        listitems.append( self._create(["ActivateWindow(Programs,Addons,return)", "10001", "32034", {"icon": "DefaultProgram.png"} ] ) )

        listitems.append( self._create(["XBMC.PlayDVD()", "32032", "32034", {"icon": "DefaultDVDFull.png"} ] ) )
        listitems.append( self._create(["EjectTray()", "32033", "32034", {"icon": "DefaultDVDFull.png"} ] ) )
                
        listitems.append( self._create(["ActivateWindow(Settings)", "10004", "32034", {} ]) )
        listitems.append( self._create(["ActivateWindow(FileManager)", "7", "32034", {"icon": "DefaultFolder.png"} ] ) )
        listitems.append( self._create(["ActivateWindow(Profiles)", "13200", "32034", {"icon": "UnknownUser.png"} ] ) )
        listitems.append( self._create(["ActivateWindow(SystemInfo)", "10007", "32034", {} ]) )
        
        listitems.append( self._create(["ActivateWindow(Favourites)", "1036", "32034", {} ]) )
            
        self.addToDictionary( "common", listitems )
        
        self.loadedCommon = True
        
        return self.loadedCommon
        
    def more( self ):
        if self.loadedMoreCommands is True:
            # The List has already been populated, return it
            return True
        elif self.loadedMoreCommands == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedMoreCommands is True:
                    return True
        else:
            # We're going to populate the list
            self.loadedMoreCommands = "Loading"

        listitems = []
        log( 'Listing more XBMC commands...' )
        
        listitems.append( self._create(["Reboot", "13013", "32054", {} ]) )
        listitems.append( self._create(["ShutDown", "13005", "32054", {} ]) )
        listitems.append( self._create(["PowerDown", "13016", "32054", {} ]) )
        listitems.append( self._create(["Quit", "13009", "32054", {} ]) )
        listitems.append( self._create(["Hibernate", "13010", "32054", {} ]) )
        listitems.append( self._create(["Suspend", "13011", "32054", {} ]) )
        listitems.append( self._create(["AlarmClock(shutdowntimer,XBMC.Shutdown())", "19026", "32054", {} ]) )
        listitems.append( self._create(["CancelAlarm(shutdowntimer)", "20151", "32054", {} ]) )
        if xbmc.getCondVisibility( "System.HasLoginScreen" ):
            listitems.append( self._create(["System.LogOff", "20126", "32054", {} ]) )
        listitems.append( self._create(["ActivateScreensaver", "360", "32054", {} ]) )
        listitems.append( self._create(["Minimize", "13014", "32054", {} ]) )

        listitems.append( self._create(["Mastermode", "20045", "32054", {} ]) )
        
        listitems.append( self._create(["RipCD", "600", "32054", {} ]) )
        
        if __xbmcversion__ == "13":
            listitems.append( self._create(["UpdateLibrary(video)", "32046", "32054", {} ]) )
            listitems.append( self._create(["UpdateLibrary(music)", "32047", "32054", {} ]) )
        else:
            listitems.append( self._create(["UpdateLibrary(video,,true)", "32046", "32054", {} ]) )
            listitems.append( self._create(["UpdateLibrary(music,,true)", "32047", "32054", {} ]) )
        
        if __xbmcversion__ == "13":
            listitems.append( self._create(["CleanLibrary(video)", "32055", "32054", {} ]) )
            listitems.append( self._create(["CleanLibrary(music)", "32056", "32054", {} ]) )
        else:
            listitems.append( self._create(["CleanLibrary(video,true)", "32055", "32054", {} ]) )
            listitems.append( self._create(["CleanLibrary(music,true)", "32056", "32054", {} ]) )
        
        self.addToDictionary( "commands", listitems )
            
        self.loadedMoreCommands = True
        return self.loadedMoreCommands
        
    def settings( self ):
        if self.loadedSettings is True:
            # The List has already been populated, return it
            return True
        elif self.loadedSettings == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedSettings is True:
                    return True
        else:
            # We're going to populate the list
            self.loadedSettings = "Loading"

        listitems = []
        log( 'Listing XBMC settings...' )
        
        listitems.append( self._create(["ActivateWindow(Settings)", "10004", "10004", {} ]) )
        
        listitems.append( self._create(["ActivateWindow(AppearanceSettings)", "480", "10004", {} ]) )
        listitems.append( self._create(["ActivateWindow(VideosSettings)", "3", "10004", {} ]) )
        listitems.append( self._create(["ActivateWindow(PVRSettings)", "19020", "10004", {} ]) )
        listitems.append( self._create(["ActivateWindow(MusicSettings)", "2", "10004", {} ]) )
        listitems.append( self._create(["ActivateWindow(PicturesSettings)", "1", "10004", {} ]) )
        listitems.append( self._create(["ActivateWindow(WeatherSettings)", "8", "10004", {} ]) )
        listitems.append( self._create(["ActivateWindow(AddonBrowser)", "24001", "10004", {} ]) )
        listitems.append( self._create(["ActivateWindow(ServiceSettings)", "14036", "10004", {} ]) )
        listitems.append( self._create(["ActivateWindow(SystemSettings)", "13000", "10004", {} ]) )
        listitems.append( self._create(["ActivateWindow(SkinSettings)", "20077", "10004", {} ]) )
        
        self.addToDictionary( "settings", listitems )
            
        self.loadedSettings = True
        return self.loadedSettings
        
    
    def pvrlibrary( self ):
        if self.loadedPVRLibrary is True:
            # The List has already been populated, return it
            return self.loadedPVRLibrary
        elif self.loadedPVRLibrary == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedPVRLibrary is True:
                    return self.loadedPVRLibrary
        else:
            # We're going to populate the list
            self.loadedPVRLibrary = "Loading"

        try:
            listitems = []
            log('Listing pvr library...')
            
            # PVR
            if __xbmcversion__ == "13":
                listitems.append( self._create(["ActivateWindowAndFocus(MyPVR,32,0 ,11,0)", "19023", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["ActivateWindowAndFocus(MyPVR,33,0 ,12,0)", "19024", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["ActivateWindowAndFocus(MyPVR,31,0 ,10,0)", "19069", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["ActivateWindowAndFocus(MyPVR,34,0 ,13,0)", "19163", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["ActivateWindowAndFocus(MyPVR,35,0 ,14,0)", "32023", "32017", {"icon": "DefaultTVShows.png"} ] ) )

                listitems.append( self._create(["PlayPvrTV", "32066", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["PlayPvrRadio", "32067", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["PlayPvr", "32068", "32017", {"icon": "DefaultTVShows.png"} ] ) )
            else:
                listitems.append( self._create(["ActivateWindow(TVChannels)", "19019", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(TVGuide)", "22020", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(TVRecordings)", "19163", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(TVTimers)", "19040", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(TVSearch)", "137", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                
                listitems.append( self._create(["PlayPvrTV", "32066", "32017", {"icon": "DefaultTVShows.png"} ] ) )
                listitems.append( self._create(["PlayPvr", "32068", "32017", {"icon": "DefaultTVShows.png"} ] ) )

            self.addToDictionary( "pvr", listitems )            
            
            # Add tv channels
            listitems = []
            json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "PVR.GetChannels", "params": { "channelgroupid": "alltv", "properties": ["thumbnail", "channeltype", "hidden", "locked", "channel", "lastplayed"] } }')
            json_query = unicode(json_query, 'utf-8', errors='ignore')
            json_response = simplejson.loads(json_query)
            
            # Add all directories returned by the json query
            if json_response.has_key('result') and json_response['result'].has_key('channels') and json_response['result']['channels'] is not None:
                for item in json_response['result']['channels']:
                    listitems.append( self._create(["pvr-channel://" + str( item['channelid'] ), item['label'], "::SCRIPT::32076", {"icon": "DefaultTVShows.png", "thumb": item[ "thumbnail"]}]) )
            
            self.addToDictionary( "pvr-tv", listitems )
            
            # Add radio channels
            listitems = []
            json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "PVR.GetChannels", "params": { "channelgroupid": "allradio", "properties": ["thumbnail", "channeltype", "hidden", "locked", "channel", "lastplayed"] } }')
            json_query = unicode(json_query, 'utf-8', errors='ignore')
            json_response = simplejson.loads(json_query)
            
            # Add all directories returned by the json query
            if json_response.has_key('result') and json_response['result'].has_key('channels') and json_response['result']['channels'] is not None:
                for item in json_response['result']['channels']:
                    listitems.append( self._create(["pvr-channel://" + str( item['channelid'] ), item['label'], "::SCRIPT::32077", {"icon": "DefaultTVShows.png", "thumb": item[ "thumbnail"]}]) )
            
            log( "Found " + str( len( listitems ) ) + " radio channels" )
            self.addToDictionary( "pvr-radio", listitems )

        except:
            log( "Failed to load pvr library" )
            print_exc()

        self.loadedPVRLibrary = True
        return self.loadedPVRLibrary
        
    def radiolibrary( self ):
        if self.loadedRadioLibrary is True:
            # The List has already been populated, return it
            return self.loadedRadioLibrary
        elif self.loadedRadioLibrary == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedRadioLibrary is True:
                    return self.loadedRadioLibrary
        else:
            # We're going to populate the list
            self.loadedRadioLibrary = "Loading"

        listitems = []
        log('Listing pvr-radio library...')
        
        # PVR
        listitems.append( self._create(["ActivateWindow(RadioChannels)", "19019", "32087", {"icon": "DefaultAudio.png"} ] ) )
        listitems.append( self._create(["ActivateWindow(RadioGuide)", "22020", "32087", {"icon": "DefaultAudio.png"} ] ) )
        listitems.append( self._create(["ActivateWindow(RadioRecordings)", "19163", "32087", {"icon": "DefaultAudio.png"} ] ) )
        listitems.append( self._create(["ActivateWindow(RadioTimers)", "19040", "32087", {"icon": "DefaultAudio.png"} ] ) )
        listitems.append( self._create(["ActivateWindow(RadioSearch)", "137", "32087", {"icon": "DefaultAudio.png"} ] ) )
        
        listitems.append( self._create(["PlayPvrRadio", "32067", "32087", {"icon": "DefaultAudio.png"} ] ) )
        listitems.append( self._create(["PlayPvr", "32068", "32087", {"icon": "DefaultAudio.png"} ] ) )

        self.addToDictionary( "radio", listitems )

        self.loadedRadioLibrary = True
        return self.loadedRadioLibrary
        
    def musiclibrary( self ):
        if self.loadedMusicLibrary is True:
            # The List has already been populated, return it
            return self.loadedMusicLibrary
        elif self.loadedMusicLibrary == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if loadedMusicLibrary is True:
                    return self.loadedMusicLibrary
        else:
            # We're going to populate the list
            self.loadedMusicLibrary = "Loading"

        if int( __xbmcversion__ ) >= 15:
            # Isengard or later - load audio nodes
            # Try loading custom nodes first
            try:
                if self._parse_libraryNodes( "music", "custom" ) == False:
                    log( "Failed to load custom music nodes" )
                    self._parse_libraryNodes( "music", "default" )
            except:
                log( "Failed to load custom music nodes" )
                print_exc()
                try:
                    # Try loading default nodes
                    self._parse_libraryNodes( "music", "default" )
                except:
                    # Empty library
                    log( "Failed to load default music nodes" )
                    print_exc()
                    
            self.loadedMusicLibrary = True
            return self.loadedMusicLibrary

        else:
            # Gotham or earlier - load static entries
            try:
                listitems = []
                log('Listing music library...')
                            
                # Music
                listitems.append( self._create(["ActivateWindow(MusicFiles)", "744", "32019", {"icon": "DefaultFolder.png"} ]) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,MusicLibrary,return)", "15100", "32019", {"icon": "DefaultFolder.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,Genres,return)", "135", "32019", {"icon": "DefaultMusicGenres.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,Artists,return)", "133", "32019", {"icon": "DefaultMusicArtists.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,Albums,return)", "132", "32019", {"icon": "DefaultMusicAlbums.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,Songs,return)", "134", "32019", {"icon": "DefaultMusicSongs.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,Years,return)", "652", "32019", {"icon": "DefaultMusicYears.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,Top100,return)", "271", "32019", {"icon": "DefaultMusicTop100.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,Top100Songs,return)", "10504", "32019", {"icon": "DefaultMusicTop100Songs.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,Top100Albums,return)", "10505", "32019", {"icon": "DefaultMusicTop100Albums.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,RecentlyAddedAlbums,return)", "359", "32019", {"icon": "DefaultMusicRecentlyAdded.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,RecentlyPlayedAlbums,return)", "517", "32019", {"icon": "DefaultMusicRecentlyPlayed.png"} ] ) )
                listitems.append( self._create(["ActivateWindow(MusicLibrary,Playlists,return)", "136", "32019", {"icon": "DefaultMusicPlaylists.png"} ] ) )
                
                # Do a JSON query for upnp sources (so that they'll show first time the user asks to see them)
                if self.loadedUPNP == False:
                    json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Files.GetDirectory", "params": { "properties": ["title", "file", "thumbnail"], "directory": "upnp://", "media": "files" } }')
                    self.loadedUPNP = True
                    
                self.addToDictionary( "music", listitems )
            except:
                log( "Failed to load music library" )
            print_exc()

            self.loadedMusicLibrary = True
            return self.loadedMusicLibrary
    
    def librarysources( self ):
        if self.loadedLibrarySources is True:
            # The List has already been populated, return it
            return self.loadedLibrarySources
        elif self.loadedLibrarySources == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedLibrarySources is True:
                    return self.loadedLibrarySources
        else:
            # We're going to populate the list
            self.loadedLibrarySources = "Loading"
            
        log('Listing library sources...')
        # Add video sources
        listitems = []
        json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Files.GetSources", "params": { "media": "video" } }')
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        json_response = simplejson.loads(json_query)
            
        # Add all directories returned by the json query
        if json_response.has_key('result') and json_response['result'].has_key('sources') and json_response['result']['sources'] is not None:
            for item in json_response['result']['sources']:
                listitems.append( self._create(["||SOURCE||" + item['file'], item['label'], "32069", {"icon": "DefaultFolder.png"} ]) )
        self.addToDictionary( "videosources", listitems )
        
        log( " - " + str( len( listitems ) ) + " video sources" )
                
        # Add audio sources
        listitems = []
        json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Files.GetSources", "params": { "media": "music" } }')
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        json_response = simplejson.loads(json_query)
            
        # Add all directories returned by the json query
        if json_response.has_key('result') and json_response['result'].has_key('sources') and json_response['result']['sources'] is not None:
            for item in json_response['result']['sources']:
                listitems.append( self._create(["||SOURCE||" + item['file'], item['label'], "32073", {"icon": "DefaultFolder.png"} ]) )
        self.addToDictionary( "musicsources", listitems )
        
        log( " - " + str( len( listitems ) ) + " audio sources" )
        
        # Add picture sources
        listitems = []
        json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Files.GetSources", "params": { "media": "pictures" } }')
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        json_response = simplejson.loads(json_query)
            
        # Add all directories returned by the json query
        if json_response.has_key('result') and json_response['result'].has_key('sources') and json_response['result']['sources'] is not None:
            for item in json_response['result']['sources']:
                listitems.append( self._create(["||SOURCE||" + item['file'], item['label'], "32089", {"icon": "DefaultFolder.png"} ]) )
        self.addToDictionary( "picturesources", listitems )
        
        log( " - " + str( len( listitems ) ) + " picture sources" )
        
        self.loadedLibrarySources = True
        return self.loadedLibrarySources
            
    def playlists( self ):
        if self.loadedPlaylists is True:
            # The List has already been populated, return it
            return self.loadedPlaylists
        elif self.loadedPlaylists == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedPlaylists is True:
                    return self.loadedPlaylists
        else:
            # We're going to populate the list
            self.loadedPlaylists = "Loading"
            
        try:
            audiolist = []
            videolist = []
            log('Loading playlists...')
            paths = [['special://videoplaylists/','32004','VideoLibrary'], ['special://musicplaylists/','32005','MusicLibrary'], ["special://skin/playlists/",'32059',None], ["special://skin/extras/",'32059',None]]
            for path in paths:
                count = 0
                for root, subdirs, files in kodiwalk( path[0], stringForce = "special://skin/" ):
                    for file in files:
                        playlist = file['path']
                        label = file['label']
                        playlistfile = xbmc.translatePath( playlist ).decode('utf-8')
                        mediaLibrary = path[2]
                        
                        if playlist.endswith( '.xsp' ):
                            contents = xbmcvfs.File(playlistfile, 'r')
                            contents_data = contents.read().decode('utf-8')
                            xmldata = xmltree.fromstring(contents_data.encode('utf-8'))
                            mediaType = "unknown"
                            for line in xmldata.getiterator():
                                if line.tag == "smartplaylist":
                                    mediaType = line.attrib['type']
                                    if mediaType == "movies" or mediaType == "tvshows" or mediaType == "seasons" or mediaType == "episodes" or mediaType == "musicvideos" or mediaType == "sets":
                                        mediaLibrary = "VideoLibrary"
                                        mediaContent = "video"
                                    elif mediaType == "albums" or mediaType == "artists" or mediaType == "songs":
                                        mediaLibrary = "MusicLibrary"
                                        mediaContent = "music"
                                    
                                if line.tag == "name" and mediaLibrary is not None:
                                    name = line.text
                                    if not name:
                                        name = label
                                    # Create a list item
                                    listitem = self._create(["::PLAYLIST::", name, path[1], {"icon": "DefaultPlaylist.png"} ])
                                    listitem.setProperty( "action-play", "PlayMedia(" + playlist + ")" )
                                    listitem.setProperty( "action-show", "ActivateWindow(" + mediaLibrary + "," + playlist + ",return)".encode( 'utf-8' ) )

                                    # Add widget information
                                    listitem.setProperty( "widget", "Playlist" )
                                    listitem.setProperty( "widgetType", mediaType )
                                    listitem.setProperty( "widgetTarget", mediaContent )
                                    listitem.setProperty( "widgetName", name )
                                    listitem.setProperty( "widgetPath", playlist )
                                    
                                    if mediaLibrary == "VideoLibrary":
                                        videolist.append( listitem )
                                    else:
                                        audiolist.append( listitem )
                                    # Save it for the widgets list
                                    self.widgetPlaylistsList.append( [playlist, "(" + __language__( int( path[1] ) ) + ") " + name, name] )
                                    
                                    count += 1
                                    break

                        elif playlist.endswith( '.m3u' ):
                            name = label
                            listitem = self._create( ["::PLAYLIST::", name, "32005", {"icon": "DefaultPlaylist.png"} ] )
                            listitem.setProperty( "action-play", "PlayMedia(" + playlist + ")" )
                            listitem.setProperty( "action-show", "ActivateWindow(MusicLibrary," + playlist + ",return)".encode( 'utf-8' ) )
                            
                            audiolist.append( listitem )
                            
                            count += 1
                            
                log( " - [" + path[0] + "] " + str( count ) + " playlists found" )
            
            self.addToDictionary( "playlist-video", videolist )
            self.addToDictionary( "playlist-audio", audiolist )
            
        except:
            log( "Failed to load playlists" )
            print_exc()
            
        self.loadedPlaylists = True
        return self.loadedPlaylists
                
    def scriptPlaylists( self ):
        # Lazy loading of random source playlists auto-generated by the script
        # (loaded lazily as these can be created/deleted after gui has loaded)
        returnPlaylists = []
        try:
            log('Loading script generated playlists...')
            path = "special://profile/addon_data/" + __addonid__ + "/"
            count = 0
            for root, subdirs, files in kodiwalk( path ):
                for file in files:
                    playlist = file['path']
                    label = file['label']
                    playlistfile = xbmc.translatePath( playlist ).decode('utf-8')
                    
                    if playlist.endswith( '-randomversion.xsp' ):
                        contents = xbmcvfs.File(playlistfile, 'r')
                        contents_data = contents.read().decode('utf-8')
                        xmldata = xmltree.fromstring(contents_data.encode('utf-8'))
                        for line in xmldata.getiterator():                               
                            if line.tag == "name":
                                    
                                # Save it for the widgets list
                                # TO-DO - Localize display name
                                returnPlaylists.append( [playlist.encode( 'utf-8' ), "(Source) " + name, name] )
                                
                                count += 1
                                break
                        
            log( " - [" + path[0] + "] " + str( count ) + " playlists found" )
            
        except:
            log( "Failed to load script generated playlists" )
            print_exc()
            
        return returnPlaylists
                
    def favourites( self ):
        if self.loadedFavourites is True:
            # The List has already been populated, return it
            return self.loadedFavourites
        elif self.loadedFavourites == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedFavourites is True:
                    return self.loadedFavourites
        else:
            # We're going to populate the list
            self.loadedFavourites = "Loading"
            
        try:
            log('Loading favourites...')
            
            listitems = []
            listing = None
            
            fav_file = xbmc.translatePath( 'special://profile/favourites.xml' ).decode("utf-8")
            if xbmcvfs.exists( fav_file ):
                doc = parse( fav_file )
                listing = doc.documentElement.getElementsByTagName( 'favourite' )
            else:
                # No favourites file found
                self.addToDictionary( "favourite", [] )
                self.loadedFavourites = True
                return True
                
            for count, favourite in enumerate(listing):
                name = favourite.attributes[ 'name' ].nodeValue
                path = favourite.childNodes [ 0 ].nodeValue
                if ('RunScript' not in path) and ('StartAndroidActivity' not in path) and not (path.endswith(',return)') ):
                    path = path.rstrip(')')
                    path = path + ',return)'

                try:
                    thumb = favourite.attributes[ 'thumb' ].nodeValue
                    
                except:
                    thumb = None
                
                listitems.append( self._create( [ path, name, "32006", { "icon": "DefaultFolder.png", "thumb": thumb} ] ) )
            
            log( " - " + str( len( listitems ) ) + " favourites found" )
            
            self.addToDictionary( "favourite", listitems )
            
        except:
            log( "Failed to load favourites" )
            print_exc()
            
        self.loadedFavourites = True            
        return self.loadedFavourites
        
    def addons( self ):
        if self.loadedAddOns is True:
            # The List has already been populated, return it
            return self.loadedAddOns
        elif self.loadedAddOns == "Loading":
            # The list is currently being populated, wait and then return it
            for i in ( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedAddOns is True:
                    return self.loadedAddOns
        else:
            # We're going to populate the list
            self.loadedAddOns = "Loading"
            
        try:
            log( 'Loading add-ons' )

            executableItems = {}
            videoItems = {}
            audioItems = {}
            imageItems = {}
                        
            contenttypes = [ ( "executable", executableItems ),  ( "video", videoItems ), ( "audio", audioItems ), ( "image", imageItems ) ]
            for contenttype, listitems in contenttypes:
                #listitems = {}
                if contenttype == "executable":
                    contentlabel = __language__(32009)
                    shortcutType = "::SCRIPT::32009"
                elif contenttype == "video":
                    contentlabel = __language__(32010)
                    shortcutType = "::SCRIPT::32010"
                elif contenttype == "audio":
                    contentlabel = __language__(32011)
                    shortcutType = "::SCRIPT::32011"
                elif contenttype == "image":
                    contentlabel = __language__(32012)
                    shortcutType = "::SCRIPT::32012"
                    
                json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Addons.Getaddons", "params": { "content": "%s", "properties": ["name", "path", "thumbnail", "enabled"] } }' % contenttype)
                json_query = unicode(json_query, 'utf-8', errors='ignore')
                json_response = simplejson.loads(json_query)
                
                if json_response.has_key('result') and json_response['result'].has_key('addons') and json_response['result']['addons'] is not None:
                    for item in json_response['result']['addons']:
                        if item['enabled'] == True:                            
                            path = "RunAddOn(" + item['addonid'].encode('utf-8') + ")"
                            action = None

                            thumb = "DefaultAddon.png"
                            if item['thumbnail'] != "":
                                thumb = item[ 'thumbnail' ]
                            else:   
                                thumb = None  
                            listitem = self._create([path, item['name'], shortcutType, {"icon": "DefaultAddon.png", "thumb": thumb} ])

                            # If this is a plugin, mark that we can browse it
                            if item[ "type" ] == "xbmc.python.pluginsource":
                                path = "||BROWSE||" + item['addonid'].encode('utf-8')
                                action = "RunAddOn(" + item['addonid'].encode('utf-8') + ")"
                            elif contenttype == "executable":
                                # Check if it's a program that can be run as an exectuble
                                provides = self.hasPluginEntryPoint( item[ "path" ] )
                                if provides is None:
                                    continue
                                
                                for content in provides:
                                    # For each content that it provides, add it to the add-ons for that type
                                    contentData = { "video": [ "::SCRIPT::32010", videoItems ], "audio": [ "::SCRIPT::32011", audioItems ], "image": [ "::SCRIPT::32012", imageItems ], "executable": [ "::SCRIPT::32009", executableItems ] }
                                    if content in contentData:
                                        otherItem = self._create([path, item['name'] + "  >", contentData[ content ][ 0 ], {"icon": "DefaultAddon.png", "thumb": thumb} ])
                                        otherItem.setProperty( "path", "||BROWSE||" + item['addonid'].encode('utf-8') )
                                        otherItem.setProperty( "action", "RunAddOn(" + item['addonid'].encode('utf-8') + ")" )
                                        contentData[ content ][ 1 ][ item[ "name" ] ] = otherItem                                        

                            if action is not None:
                                listitem.setProperty( "path", path )
                                listitem.setProperty( "action", action )
                                listitem.setLabel( listitem.getLabel() + "  >" )

                            listitems[ item[ "name" ] ] = listitem
                            
                if contenttype == "executable":
                    self.addToDictionary( "addon-program", self.sortDictionary( listitems ) )
                    log( " - " + str( len( listitems ) ) + " programs found" )
                elif contenttype == "video":
                    self.addToDictionary( "addon-video", self.sortDictionary( listitems ) )
                    log( " - " + str( len( listitems ) ) + " video add-ons found" )
                elif contenttype == "audio":
                    self.addToDictionary( "addon-audio", self.sortDictionary( listitems ) )
                    log( " - " + str( len( listitems ) ) + " audio add-ons found" )
                elif contenttype == "image":
                    self.addToDictionary( "addon-image", self.sortDictionary( listitems ) )
                    log( " - " + str( len( listitems ) ) + " image add-ons found" )
            
        except:
            log( "Failed to load addons" )
            print_exc()
        
        self.loadedAddOns = True
        return self.loadedAddOns

    def hasPluginEntryPoint( self, path ):
        # Check if an addon has a plugin entry point by parsing its addon.xml file
        try:
            tree = xmltree.parse( os.path.join( path, "addon.xml" ) ).getroot()
            for extension in tree.findall( "extension" ):
                if "point" in extension.attrib and extension.attrib.get( "point" ) == "xbmc.python.pluginsource":
                    # Find out what content type it provides
                    provides = extension.find( "provides" )
                    if provides is None:
                        return None
                    return provides.text.split( " " )


        except:
            return None
        return None
    
    def detectPluginContent(self, item):
        #based on the properties in the listitem we try to detect the content
        
        if not item.has_key("showtitle") and not item.has_key("artist"):
            #these properties are only returned in the json response if we're looking at actual file content...
            # if it's missing it means this is a main directory listing and no need to scan the underlying listitems.
            return None

        if not item.has_key("showtitle") and item.has_key("artist"):
            ##### AUDIO ITEMS ####
            if item["artist"][0] == item["title"]:
                return "artists"
            elif item["album"] == item["title"]:
                return "albums"
            elif (item["type"] == "song" or (item["artist"] and item["album"])):
                return "songs"
        else:    
            ##### VIDEO ITEMS ####
            if (item["showtitle"] and not item["artist"]):
                #this is a tvshow, episode or season...
                if (item["season"] > -1 and item["episode"] == -1):
                    return "seasons"
                elif item["season"] > -1 and item["episode"] > -1:
                    return "episodes"
                else:
                    return "tvshows"
            elif (item["artist"]):
                #this is a musicvideo!
                return "musicvideos"
            elif (item["imdbnumber"] or item["mpaa"] or item["trailer"] or item["studio"]):
                return "movies"

        return None

    def widgets( self ):
        if self.loadedWidgets is True:
            # The list has already been populated
            return self.loadedWidgets
        elif self.loadedWidgets == "Loading":
            # The list is currently being populated, wait and then return it
            for i in range( 0, 20 ):
                xbmc.sleep( 100 )
                if self.loadedWidgets is True:
                    return self.loadedWidgets
        else:
            # We're going to populat the list
            self.loadedWidgets = "Loading"

        # Load skin overrides
        tree = DATA._get_overrides_skin()

        # Get widgets
        listitems = []
        log( "Loading skin widgets" )
        if tree is not None:
            elems = tree.getroot().findall( "widget" )
            for elem in elems:
                widgetType = None
                widgetPath = None
                widgetTarget = None
                widgetIcon = ""
                if "type" in elem.attrib:
                    widgetType = elem.attrib.get( "type" )
                if "condition" in elem.attrib:
                    if not xbmc.getCondVisibility( elem.attrib.get( "condition" ) ):
                        continue
                if "path" in elem.attrib:
                    widgetPath = elem.attrib.get( "path" )
                if "target" in elem.attrib:
                    widgetTarget = elem.attrib.get( "target" )
                if "icon" in elem.attrib:
                    widgetIcon = elem.attrib.get( "icon" )

                # Save widget for button 309
                self.dictionaryGroupings[ "widgets-classic" ].append( [elem.text, DATA.local( elem.attrib.get( 'label' ) )[2], widgetType, widgetPath, widgetIcon, widgetTarget ] )

                # Save widgets for button 312
                listitem = self._create( [ elem.text, DATA.local( elem.attrib.get( 'label' ) )[2], "::SCRIPT::32099", {"icon": widgetIcon } ] )
                listitem.setProperty( "widget", elem.text )
                listitem.setProperty( "widgetName", DATA.local( elem.attrib.get( 'label' ) )[2] )
                if widgetType is not None:
                    listitem.setProperty( "widgetType", widgetType )
                if widgetPath is not None:
                    listitem.setProperty( "widgetPath", widgetPath )
                if widgetTarget is not None:
                    listitem.setProperty( "widgetTarget", widgetTarget )
                listitems.append( listitem )

        self.addToDictionary( "widgets", listitems )

        self.loadedWidgets = True
        return self.loadedWidgets
        
    def sortDictionary( self, dictionary ):
        listitems = []
        for key in sorted( dictionary.keys() ): #, reverse = True):
            listitems.append( dictionary[ key ] )
        return listitems
            
    # =============================
    # === ADDON/SOURCE EXPLORER ===
    # =============================
    
    def explorer( self, history, location, label, thumbnail, itemType, isWidget = False ):
        isLibrary = False
        widgetType = None
        addonType = None

        dialogLabel = try_decode( label[0] ).replace( "  >", "" )
        if len( label ) != 1:
            dialogLabel = try_decode( label[0] ).replace( "  >", "" ) + " - " + try_decode( label[ -1 ] ).replace( "  >", "" )

        listings = []
        
        tree = DATA._get_overrides_skin()

        # Shortcut to go 'up'
        if len( label ) == 1:
            # This is the root, create a link to go back to selectShortcut
            listitem = self._create( [ "::UP::", "..", "", {} ] )
        else:
            # This isn't the root, create a link to go up the heirachy
            listitem = self._create( [ "::BACK::", "..", "", {} ] )
        listings.append( listitem )
            
        
        # Default action - create shortcut
        createLabel = "32058"
        if isWidget:
            createLabel = "32100"
        listings.append( self._get_icon_overrides( tree, self._create( ["::CREATE::", createLabel, "", {}] ), "" ) )
                
        log( "Getting %s - %s" %( dialogLabel, location ) )
            
        # Show a waiting dialog, then get the listings for the directory
        dialog = xbmcgui.DialogProgress()
        dialog.create( dialogLabel, __language__( 32063 ) )
        
        #we retrieve a whole bunch of properties, needed to guess the content type properly
        json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Files.GetDirectory", "params": { "properties": ["title", "file", "thumbnail", "episode", "showtitle", "season", "album", "artist", "imdbnumber", "firstaired", "mpaa", "trailer", "studio", "art"], "directory": "' + location + '", "media": "files" } }')
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        json_response = simplejson.loads(json_query)
            
        # Add all directories returned by the json query
        if json_response.has_key('result') and json_response['result'].has_key('files') and json_response['result']['files']:
            json_result = json_response['result']['files']
            
            for item in json_result:
                # Handle numeric labels
                altLabel = item[ "label" ]
                if item[ "label" ].isnumeric():
                    altLabel = "$NUMBER[" + item[ "label" ] + "]"
                if location.startswith( "library://" ):
                    # Process this as a library node
                    isLibrary = True
                    if widgetType is None:
                        widgetType = NODE.get_mediaType( location )

                    if itemType == "32014":
                        # Video node
                        windowID = "10025"
                        if widgetType == "unknown":
                            widgetType = "video"
                        widgetTarget = "video"
                    else:
                        # Audio node
                        windowID = "10502"
                        if widgetType == "unknown":
                            widgetType = "audio"
                        widgetTarget = "music"

                    if item[ "filetype" ] == "directory":
                        thumb = None
                        if item[ "thumbnail" ] is not "":
                            thumb = item[ "thumbnail" ]
                            
                        listitem = self._create( [ "ActivateWindow(%s,%s,return)" %( windowID, item[ "file" ] ), altLabel, "", {"icon": "DefaultFolder.png", "thumb": thumb} ] )

                        if item[ "file" ].endswith( ".xml/" ) and NODE.isGrouped( item[ "file" ] ):
                            listitem = self._create( [ item[ "file" ], "%s  >" %( item[ "label" ] ), "", {"icon": "DefaultFolder.png", "thumb": thumb} ] )

                        # Add widget properties
                        widgetName = try_decode(label[0]).replace( "  >", "" ) + " - " + item[ "label" ]
                        listitem.setProperty( "widget", "Library" )
                        listitem.setProperty( "widgetName", widgetName )
                        listitem.setProperty( "widgetType", widgetType )
                        listitem.setProperty( "widgetTarget", widgetTarget )
                        listitem.setProperty( "widgetPath", item[ "file" ] )

                        listings.append( self._get_icon_overrides( tree, listitem, "" ) )
                
                #some special code for smart shortcuts in script.skin.helper.service
                elif item.get("title",None) == "smartshortcut":

                    smartShortCutsData = eval(item.get("mpaa"))                  
                    thumb = smartShortCutsData["background"]
                    
                    listitem = self._create( [ item[ "file" ], altLabel, "", {"icon": item.get("icon"), "thumb": thumb} ] )
                    # add all passed properties to the gui to set default background, widget etc.
                    properties = []
                    for key, value in smartShortCutsData.iteritems():
                        properties.append( [key, value ] )
                    listitem.setProperty( "smartShortcutProperties", repr( properties ) )
                    listitem.setProperty( "untranslatedIcon", thumb )
                    listitem.setProperty( "widget", "addon" )
                    listitem.setProperty( "widgetName", item["label"] )
                    listitem.setProperty( "widgetType", smartShortCutsData["type"] )
                    if smartShortCutsData["type"] == "music" or smartShortCutsData["type"] == "artists" or smartShortCutsData["type"] == "albums":
                        listitem.setProperty( "widgetTarget", "music" )
                    else:
                        listitem.setProperty( "widgetTarget", "video" )
                    listitem.setProperty( "widgetPath", smartShortCutsData["list"] )
                    listings.append( self._get_icon_overrides( tree, listitem, "" ) )
                    
                else:
                    # Process this as a plugin
                    if item["filetype"] == "directory":
                        thumb = None
                        if item[ "thumbnail" ] is not "":
                            thumb = item[ "thumbnail" ]
                        listitem = self._create( [item[ "file" ], item[ "label" ] + "  >", "", {"icon": "DefaultFolder.png", "thumb": thumb} ] )
                        listings.append( self._get_icon_overrides( tree, listitem, "" ) )
                    else:
                        contentType = self.detectPluginContent( item )
                        if contentType is not None:
                            if addonType is not None:
                                addonType = contentType
                            else:
                                if addonType != contentType and addonType != "mixed":
                                    addonType = "mixed"
            
        # Close progress dialog
        dialog.close()

        # Show select dialog
        w = ShowDialog( "DialogSelect.xml", __cwd__, listing=listings, windowtitle=dialogLabel )
        w.doModal()
        selectedItem = w.result
        del w
        
        if selectedItem != -1:
            selectedAction = listings[ selectedItem ].getProperty( "path" )
            if selectedAction == "::UP::":
                # User wants to go out of explorer, back to selectShortcut
                listitem = xbmcgui.ListItem( label="back" )
                listitem.setProperty( "path", "::UP::" )

                return listitem
            if selectedAction == "::CREATE::":
                # User has chosen the shortcut they want

                # Localize strings
                localItemType = DATA.local( itemType )[2]
                
                # Create a listitem
                listitem = xbmcgui.ListItem(label=label[ len( label ) - 1 ].replace( "  >", "" ), label2=localItemType, iconImage="DefaultShortcut.png", thumbnailImage=thumbnail[ len( thumbnail ) - 1 ])
                
                # Build the action
                if itemType == "32010" or itemType == "32014" or itemType == "32069":
                    action = 'ActivateWindow(10025,"' + location + '",return)'
                    listitem.setProperty( "windowID", "10025" )
                    listitem.setProperty( "widgetType", "video" )

                    # Add widget details
                    if isLibrary:
                        listitem.setProperty( "widget", "Library" )
                        widgetType = NODE.get_mediaType( location )
                        if widgetType != "unknown":
                            listitem.setProperty( "widgetType", widgetType )
                    else:
                        listitem.setProperty( "widget", "Addon" )

                    if addonType is not None:
                        listitem.setProperty( "widgetType", addonType)

                    listitem.setProperty( "widgetTarget", "video" )
                    listitem.setProperty( "widgetName", dialogLabel )
                    listitem.setProperty( "widgetPath", location )

                elif itemType == "32011" or itemType == "32019" or itemType == "32073":
                    action = 'ActivateWindow(10501,"' + location + '",return)'
                    listitem.setProperty( "windowID", "10501" )

                    # Add widget details
                    listitem.setProperty( "widgetType", "audio" )
                    if isLibrary:
                        listitem.setProperty( "widget", "Library" )
                        widgetType = NODE.get_mediaType( location )
                        if widgetType != "unknown":
                            listitem.setProperty( "widgetType", widgetType )
                    else:
                        listitem.setProperty( "widget", "Addon" )
                    if addonType is not None:
                        listitem.setProperty( "widgetType", addonType)
                    
                    listitem.setProperty( "widgetTarget", "music" )
                    listitem.setProperty( "widgetName", dialogLabel )
                    listitem.setProperty( "widgetPath", location )

                elif itemType == "32012" or itemType == "32089":
                    action = 'ActivateWindow(10002,"' + location + '",return)'
                    listitem.setProperty( "windowID", "10002" )

                    # Add widget details
                    listitem.setProperty( "widget", "Addon" )
                    listitem.setProperty( "widgetType", "picture" )
                    listitem.setProperty( "widgetTarget", "pictures" )
                    listitem.setProperty( "widgetName", dialogLabel )
                    listitem.setProperty( "widgetPath", location )
                elif itemType == "32009":
                    action = 'ActivateWindow(10001,"' + location + '",return)'
                else:
                    action = "RunAddon(" + location + ")"

                listitem.setProperty( "path", action )
                listitem.setProperty( "displayPath", action )
                listitem.setProperty( "shortcutType", itemType )
                listitem.setProperty( "icon", "DefaultShortcut.png" )
                if thumbnail[ len( thumbnail ) -1 ] == "":
                    listitem.setProperty( "thumbnail", thumbnail[ 0 ] )
                else:
                    listitem.setProperty( "thumbnail", thumbnail[ len( thumbnail ) - 1 ] )
                listitem.setProperty( "location", location )
                
                return listitem
                
            elif selectedAction == "::BACK::":
                # User is going up the heirarchy, remove current level and re-call this function
                history.pop()
                label.pop()
                thumbnail.pop()
                return self.explorer( history, history[ len( history ) -1 ], label, thumbnail, itemType, isWidget = isWidget )
                
            elif selectedAction.startswith( "ActivateWindow(" ) or selectedAction.startswith( "$INFO" ):
                # The user wants to create a shortcut to a specific shortcut listed
                listitem = listings[ selectedItem ]

                # Add widget details
                if isLibrary:
                    widgetType = NODE.get_mediaType( listitem.getProperty( "widgetPath" ) )
                    if widgetType != "unknown":
                        listitem.setProperty( "widgetType", widgetType )

                return listitem
                
            else:
                # User has chosen a sub-level to display, add details and re-call this function
                history.append( selectedAction )
                label.append( listings[ selectedItem ].getLabel() )
                thumbnail.append( listings[ selectedItem ].getProperty( "thumbnail" ) )
                return self.explorer( history, selectedAction, label, thumbnail, itemType, isWidget = isWidget )
    
    # ======================
    # === AUTO-PLAYLISTS ===
    # ======================
    
    def _sourcelink_choice( self, selectedShortcut ):
        # The user has selected a source. We're going to give them the choice of displaying it
        # in the files view, or view library content from the source
        dialog = xbmcgui.Dialog()
        
        mediaType = None
        windowID = selectedShortcut.getProperty( "windowID" )
        # Check if we're going to display this in the files view, or the library view
        if windowID == "10025":
            # Video library                                    Files view           Movies                TV Shows             Music videos         !Movies               !TV Shows            !Music Videos
            userChoice = dialog.select( __language__(32078), [__language__(32079), __language__(32015), __language__(32016), __language__(32018), __language__(32081), __language__(32082), __language__(32083) ] )            
            if userChoice == -1:
                return None
            elif userChoice == 0:
                # Escape any backslashes (Windows fix)
                newAction = selectedShortcut.getProperty( "Path" )
                newAction = newAction.replace( "\\", "\\\\" )
                selectedShortcut.setProperty( "Path", newAction )
                selectedShortcut.setProperty( "displayPath", newAction )
                return selectedShortcut
            elif userChoice == 1:
                mediaType = "movies"
                negative = False
            elif userChoice == 2:
                mediaType = "tvshows"
                negative = False
            elif userChoice == 3:
                mediaType = "musicvideo"
                negative = False
            elif userChoice == 4:
                mediaType = "movies"
                negative = True
            elif userChoice == 5:
                mediaType = "tvshows"
                negative = True
            elif userChoice == 6:
                mediaType = "musicvideo"
                negative = True
        elif windowID == "10501":
            # Music library                                    Files view           Songs                          Albums                         Mixed                           !Songs               !Albums               !Mixed
            userChoice = dialog.select( __language__(32078), [__language__(32079), xbmc.getLocalizedString(134), xbmc.getLocalizedString(132), xbmc.getLocalizedString(20395), __language__(32084), __language__(32085), __language__(32086) ] )            
            if userChoice == -1:
                return None
            elif userChoice == 0:
                # Escape any backslashes (Windows fix)
                newAction = selectedShortcut.getProperty( "Path" )
                newAction = newAction.replace( "\\", "\\\\" )
                selectedShortcut.setProperty( "Path", newAction )
                selectedShortcut.setProperty( "displayPath", newAction )
                return selectedShortcut
            elif userChoice == 1:
                mediaType = "songs"
                windowID = "10502"
                negative = False
            elif userChoice == 2:
                mediaType = "albums"
                windowID = "10502"
                negative = False
            elif userChoice == 3:
                mediaType = "mixed"
                windowID = "10502"
                negative = False
            elif userChoice == 4:
                mediaType = "songs"
                windowID = "10502"
                negative = True
            elif userChoice == 5:
                mediaType = "albums"
                windowID = "10502"
                negative = True
            elif userChoice == 6:
                mediaType = "mixed"
                windowID = "10502"
                negative = True
        else:
            # Pictures                                         Files view           Slideshow                     Slideshow (random)                                                             Recursive slideshow             Recursive slideshow (random)    
            userChoice = dialog.select( __language__(32078), [ __language__(32079), xbmc.getLocalizedString(108), "%s (%s)" %( xbmc.getLocalizedString( 108 ), xbmc.getLocalizedString( 590 ) ), xbmc.getLocalizedString( 361 ), "%s (%s)" %( xbmc.getLocalizedString( 361 ), xbmc.getLocalizedString( 590 ) ) ] )
            if userChoice == -1:
                return None
            elif userChoice == 0:
                # Escape any backslashes (Windows fix)
                newAction = selectedShortcut.getProperty( "Path" )
                newAction = newAction.replace( "\\", "\\\\" )
                selectedShortcut.setProperty( "Path", newAction )
                selectedShortcut.setProperty( "displayPath", newAction )
                return selectedShortcut
            else:
                if userChoice == 1:
                    newAction = "SlideShow(" + selectedShortcut.getProperty( "location" ) + ",notrandom)"
                elif userChoice == 2:
                    newAction = "SlideShow(" + selectedShortcut.getProperty( "location" ) + ",random)"
                elif userChoice == 3:
                    newAction = "SlideShow(" + selectedShortcut.getProperty( "location" ) + ",recursive,notrandom)"
                elif userChoice == 4:
                    newAction = "SlideShow(" + selectedShortcut.getProperty( "location" ) + ",recursive,random)"
                selectedShortcut.setProperty( "path", newAction )
                selectedShortcut.setProperty( "displayPath", newAction )
                return selectedShortcut
            
        # We're going to display it in the library
        filename = self._build_playlist( selectedShortcut.getProperty( "location" ), mediaType, selectedShortcut.getLabel(), negative )
        newAction = "ActivateWindow(" + windowID + "," +"special://profile/addon_data/" + __addonid__ + "/" + filename + ",return)"
        selectedShortcut.setProperty( "Path", newAction )
        selectedShortcut.setProperty( "displayPath", newAction )
        return selectedShortcut
    
    def _build_playlist( self, target, mediatype, name, negative ):
        # This function will build a playlist that displays the contents of a source in the library view
        # (that is to say, "path" "contains")
        tree = xmltree.ElementTree( xmltree.Element( "smartplaylist" ) )
        root = tree.getroot()
        root.set( "type", mediatype )
        
        if target.startswith ( "multipath://" ):
            temp_path = target.replace( "multipath://", "" ).split( "%2f/" )
            target = []
            for item in temp_path:
                if item is not "":
                    target.append( urllib.url2pathname( item ) )
        else:
            target = [target]
        
        xmltree.SubElement( root, "name").text = name
        if negative == False:
            xmltree.SubElement( root, "match").text = "one"
        else:
            xmltree.SubElement( root, "match").text = "all"
        
        for item in target:
            if negative == False:
                rule = xmltree.SubElement( root, "rule")
                rule.set( "field", "path" )
                rule.set( "operator", "startswith" )
                xmltree.SubElement( rule, "value" ).text = item
            else:
                rule = xmltree.SubElement( root, "rule")
                rule.set( "field", "path" )
                rule.set( "operator", "doesnotcontain" )
                xmltree.SubElement( rule, "value" ).text = item
        
        id = 1
        while xbmcvfs.exists( os.path.join( __datapath__, str( id ) + ".xsp" ) ) :
            id += 1
                
        # Write playlist we'll link to the menu item
        DATA.indent( tree.getroot() )
        tree.write( os.path.join( __datapath__, str( id ) + ".xsp" ), encoding="utf-8" )
        
        # Add a random property, and save this for use in playlists/backgrounds
        order = xmltree.SubElement( root, "order" )
        order.text = "random"
        DATA.indent( tree.getroot() )
        tree.write( os.path.join( __datapath__, str( id ) + "-randomversion.xsp" ), encoding="utf-8" )
        
        return str( id ) + ".xsp"
        
    def _delete_playlist( self, target ):
        # This function will check if the target links to an auto-generated playlist and, if so, delete it
        target = target
        if target.startswith( "ActivateWindow(" ):
            try:
                elements = target.split( "," )
                if len( elements ) > 1:
                    if elements[1].startswith( "special://profile/addon_data/" + __addonid__ + "/" ) and elements[1].endswith( ".xsp" ):
                        xbmcvfs.delete( xbmc.translatePath( elements[1] ) )
                        xbmcvfs.delete( xbmc.translatePath( elements[1].replace( ".xsp", "-randomversion.xsp" ) ) )                        
            except:
                return

    def _rename_playlist( self, target, newLabel ):
        # This function changes the label tag of an auto-generated playlist
        
        # First we will check that this is a playlist
        target = target
        if target.startswith( "ActivateWindow(" ):
            try:
                elements = target.split( "," )
            except:
                return
                    
            try:
                if elements[1].startswith( "special://profile/addon_data/" + __addonid__ + "/" ) and elements[1].endswith( ".xsp" ):
                    filename =  xbmc.translatePath( elements[1] )
                else:
                    return
            except:
                return
                    
            # Load the tree and change the name
            tree = xmltree.parse( filename )
            name = tree.getroot().find( "name" )
            name.text = newLabel
            
            # Write the tree
            DATA.indent( tree.getroot() )
            tree.write( filename, encoding="utf-8" )
                    
            # Load the random tree and change the name
            tree = xmltree.parse( filename.replace( ".xsp", "-randomversion.xsp" ) )
            name = tree.getroot().find( "name" )
            name.text = newLabel
            
            # Write the random tree
            DATA.indent( tree.getroot() )
            tree.write( filename.replace( ".xsp", "-randomversion.xsp" ), encoding="utf-8" )

    def getImagesFromVfsPath(self, path):
        #this gets images from a vfs path to be used as backgrounds or icons
        images = []
        json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Files.GetDirectory", "params": { "properties": ["title", "art", "file", "fanart"], "directory": "' + path + '", "media": "files" } }')
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        json_response = simplejson.loads(json_query)
        if json_response.has_key('result') and json_response['result'].has_key('files') and json_response['result']['files']:
            json_result = json_response['result']['files']
            for item in json_result:
                label = item["label"]
                image = None
                if item.has_key("art"):
                    if item["art"].has_key("fanart"):
                        image = item["art"]["fanart"]
                if item.get("file",None).lower().endswith(".jpg") or item.get("file",None).lower().endswith(".png"):
                    image = item["file"]
                    label = label.replace(".jpg","").replace(".png","")
                if image:
                    image = urllib.unquote(image).decode('utf8')
                    if "$INFO" in image:
                        image = image.replace("image://","")
                        if image.endswith("/"):
                            image = image[:-1]
                    images.append( [image, label ] )
        return images
# =====================================
# === COMMON SELECT SHORTCUT METHOD ===
# =====================================

    def selectShortcut( self, group = "", custom = False, availableShortcuts = None, windowTitle = None, showNone = False, currentAction = "", grouping = None ):
        # This function allows the user to select a shortcut
        
        # If group is empty, start background loading of shortcuts
        if group == "":
            thread.start_new_thread( self.loadLibrary, () )

        isWidget = False
        if grouping == "widget":
            isWidget = True
        
        if availableShortcuts is None:
            nodes = self.retrieveGroup( group, flat = False, grouping = grouping )
            availableShortcuts = nodes[1]
            windowTitle = nodes[0]
        else:
            availableShortcuts = self.checkForFolder( availableShortcuts )
            
        if showNone is not False and group == "":
            availableShortcuts.insert( 0, self._create(["::NONE::", __language__(32053), "", {"icon":"DefaultAddonNone.png"}] ) )
            
        if custom is not False and group == "":
            availableShortcuts.append( self._create(["||CUSTOM||", __language__(32024), "", {}] ) )

        if group != "":
            # Add a link to go 'up'
            availableShortcuts.insert( 0, self._create( ["::BACK::", "..", "", {}] ) )

        # Show select dialog
        w = ShowDialog( "DialogSelect.xml", __cwd__, listing=availableShortcuts, windowtitle=windowTitle )
        w.doModal()
        number = w.result
        del w
        
        if number != -1:
            selectedShortcut = availableShortcuts[ number ]
            path = selectedShortcut.getProperty( "Path" )
            if path.startswith( "::BACK::" ):
                # Go back up
                if "," in group:
                    # Remove last level from group
                    newGroup = group.rsplit( ",", 1 )[ 0 ]
                else:
                    # We're only one level in, so we'll just clear the group
                    newGroup = ""
                # Recall this function
                return self.selectShortcut( group = newGroup, grouping = grouping, custom = custom, showNone = showNone, currentAction = currentAction )
            if path.startswith( "||NODE||" ):
                if group == "":
                    group = path.replace( "||NODE||", "" )
                else:
                    group = group + "," + path.replace( "||NODE||", "" )
                return self.selectShortcut( group = group, grouping = grouping, custom = custom, showNone = showNone, currentAction = currentAction )
            elif path.startswith( "||BROWSE||" ):
                selectedShortcut = self.explorer( ["plugin://" + path.replace( "||BROWSE||", "" )], "plugin://" + path.replace( "||BROWSE||", "" ), [selectedShortcut.getLabel()], [selectedShortcut.getProperty("thumbnail")], selectedShortcut.getProperty("shortcutType"), isWidget = isWidget )
                # Convert backslashes to double-backslashes (windows fix)
                if selectedShortcut is not None:
                    newAction = selectedShortcut.getProperty( "Path" )
                    newAction = newAction.replace( "\\", "\\\\" )
                    selectedShortcut.setProperty( "Path", newAction )
                    selectedShortcut.setProperty( "displayPath", newAction )
            elif path.startswith( "||VIDEO||" ):
                # Video node
                selectedShortcut = self.explorer( [ path.replace( "||VIDEO||", "" )], path.replace( "||VIDEO||", "" ), [selectedShortcut.getLabel()], [selectedShortcut.getProperty("thumbnail")], "32014", isWidget = isWidget )
                # Convert backslashes to double-backslashes (windows fix)
                if selectedShortcut is not None:
                    newAction = selectedShortcut.getProperty( "Path" )
                    newAction = newAction.replace( "\\", "\\\\" )
                    selectedShortcut.setProperty( "Path", newAction )
                    selectedShortcut.setProperty( "displayPath", newAction )
            elif path.startswith( "||AUDIO||" ):
                # Audio node
                selectedShortcut = self.explorer( [ path.replace( "||AUDIO||", "" )], path.replace( "||AUDIO||", "" ), [selectedShortcut.getLabel()], [selectedShortcut.getProperty("thumbnail")], "32019", isWidget = isWidget )
                # Convert backslashes to double-backslashes (windows fix)
                if selectedShortcut is not None:
                    newAction = selectedShortcut.getProperty( "Path" )
                    newAction = newAction.replace( "\\", "\\\\" )
                    selectedShortcut.setProperty( "Path", newAction )
                    selectedShortcut.setProperty( "displayPath", newAction )
            elif path == "||UPNP||":
                selectedShortcut = self.explorer( ["upnp://"], "upnp://", [selectedShortcut.getLabel()], [selectedShortcut.getProperty("thumbnail")], selectedShortcut.getProperty("shortcutType"), isWidget = isWidget )
                path = selectedShortcut.getProperty( "Path" )
            elif path.startswith( "||SOURCE||" ):
                selectedShortcut = self.explorer( [path.replace( "||SOURCE||", "" )], path.replace( "||SOURCE||", "" ), [selectedShortcut.getLabel()], [selectedShortcut.getProperty("thumbnail")], selectedShortcut.getProperty("shortcutType"), isWidget = isWidget )
                if selectedShortcut is None or "upnp://" in selectedShortcut.getProperty( "Path" ):
                    return selectedShortcut
                if isWidget:
                    # Set widget to 'source'
                    selectedShortcut.setProperty( "widget", "source" )
                else:
                    # Find out what the user wants to do with the source
                    selectedShortcut = self._sourcelink_choice( selectedShortcut )
            elif path == "::PLAYLIST::" :
                if isWidget:
                    # Return actionShow as chosenPath
                    selectedShortcut.setProperty( "chosenPath", selectedShortcut.getProperty( "action-show" ) )
                else:
                    # Give the user the choice of playing or displaying the playlist
                    dialog = xbmcgui.Dialog()
                    userchoice = dialog.yesno( __language__( 32040 ), __language__( 32060 ), "", "", __language__( 32061 ), __language__( 32062 ) )
                    # False: Display
                    # True: Play
                    if not userchoice:
                        selectedShortcut.setProperty( "chosenPath", selectedShortcut.getProperty( "action-show" ) )
                    else:
                        selectedShortcut.setProperty( "chosenPath", selectedShortcut.getProperty( "action-play" ) )
                   
            elif path == "||CUSTOM||":
                # Let the user type a command
                keyboard = xbmc.Keyboard( currentAction, __language__(32027), False )
                keyboard.doModal()
                
                if ( keyboard.isConfirmed() ):
                    action = keyboard.getText()
                    if action != "":
                        # Create a really simple listitem to return
                        selectedShortcut = xbmcgui.ListItem( None, __language__(32024) )
                        selectedShortcut.setProperty( "Path", action )
                    else:
                        selectedShortcut = None
                            
                else:
                    selectedShortcut = None
                    
            elif path == "::NONE::":
                # Create a really simple listitem to return
                selectedShortcut = xbmcgui.ListItem( "::NONE::" )

            # Check that explorer hasn't sent us back here
            if selectedShortcut is not None and selectedShortcut.getProperty( "path" ) == "::UP::":
                return self.selectShortcut( group = group, custom = custom, availableShortcuts = None, windowTitle = windowTitle, showNone = showNone, grouping = grouping, currentAction = currentAction )

            return selectedShortcut
        else:
            return None

# ============================
# === PRETTY SELECT DIALOG ===
# ============================
            
class ShowDialog( xbmcgui.WindowXMLDialog ):
    def __init__( self, *args, **kwargs ):
        xbmcgui.WindowXMLDialog.__init__( self )
        self.listing = kwargs.get( "listing" )
        self.windowtitle = kwargs.get( "windowtitle" )
        self.result = -1

    def onInit(self):
        try:
            self.fav_list = self.getControl(6)
            self.getControl(3).setVisible(False)
        except:
            print_exc()
            self.fav_list = self.getControl(3)

        self.getControl(5).setVisible(False)
        self.getControl(1).setLabel(self.windowtitle)

        for item in self.listing :
            listitem = xbmcgui.ListItem(label=item.getLabel(), label2=item.getLabel2(), iconImage=item.getProperty( "icon" ), thumbnailImage=item.getProperty( "thumbnail" ))
            listitem.setProperty( "Addon.Summary", item.getLabel2() )
            self.fav_list.addItem( listitem )

        self.setFocus(self.fav_list)

    def onAction(self, action):
        if action.getId() in ( 9, 10, 92, 216, 247, 257, 275, 61467, 61448, ):
            self.result = -1
            self.close()

    def onClick(self, controlID):
        if controlID == 6 or controlID == 3:
            num = self.fav_list.getSelectedPosition()
            self.result = num
        else:
            self.result = -1

        self.close()

    def onFocus(self, controlID):
        pass
