# coding=utf-8
import os, sys, datetime, unicodedata, re, types
import xbmc, xbmcaddon, xbmcgui, xbmcvfs, urllib
import xml.etree.ElementTree as xmltree
import hashlib, hashlist
import cPickle as pickle
from xml.dom.minidom import parse
from traceback import print_exc
from htmlentitydefs import name2codepoint
from unidecode import unidecode

if sys.version_info < (2, 7):
    import simplejson
else:
    import json as simplejson

__addon__        = xbmcaddon.Addon()
__addonid__      = __addon__.getAddonInfo('id').decode( 'utf-8' )
__addonversion__ = __addon__.getAddonInfo('version')
__xbmcversion__  = xbmc.getInfoLabel( "System.BuildVersion" ).split(".")[0]
__language__     = __addon__.getLocalizedString
__cwd__          = __addon__.getAddonInfo('path').decode("utf-8")
__addonname__    = __addon__.getAddonInfo('name').decode("utf-8")
__resource__   = xbmc.translatePath( os.path.join( __cwd__, 'resources', 'lib' ) ).decode("utf-8")
__datapath__     = os.path.join( xbmc.translatePath( "special://profile/addon_data/" ).decode('utf-8'), __addonid__ )
__profilepath__  = xbmc.translatePath( "special://profile/" ).decode('utf-8')
__skinpath__     = xbmc.translatePath( "special://skin/shortcuts/" ).decode('utf-8')
__defaultpath__  = xbmc.translatePath( os.path.join( __cwd__, 'resources', 'shortcuts').encode("utf-8") ).decode("utf-8")

# character entity reference
CHAR_ENTITY_REXP = re.compile('&(%s);' % '|'.join(name2codepoint))

# decimal character reference
DECIMAL_REXP = re.compile('&#(\d+);')

# hexadecimal character reference
HEX_REXP = re.compile('&#x([\da-fA-F]+);')

REPLACE1_REXP = re.compile(r'[\']+')
REPLACE2_REXP = re.compile(r'[^-a-z0-9]+')
REMOVE_REXP = re.compile('-{2,}')

def log(txt):
    if __xbmcversion__ == "13" or __addon__.getSetting( "enable_logging" ) == "true":
        try:
            if isinstance (txt,str):
                txt = txt.decode('utf-8')
            message = u'%s: %s' % (__addonid__, txt)
            xbmc.log(msg=message.encode('utf-8'), level=xbmc.LOGDEBUG)
        except:
            pass
    
class NodeFunctions():
    def __init__(self):
        self.indexCounter = 0
        
    ##############################################
    # Functions used by library.py to list nodes #
    ##############################################
        
    def get_nodes( self, path, prefix ):
        dirs, files = xbmcvfs.listdir( path )
        nodes = {}
        
        try:
            for dir in dirs:
                self.parse_node( os.path.join( path, dir ), dir, nodes, prefix )
            for file in files:
                self.parse_view( os.path.join( path, file.decode( "utf-8" ) ), nodes, origPath = "%s/%s" % ( prefix, file ), prefix = prefix )
        except:
            print_exc()
            return False
        
        return nodes
        
    def parse_node( self, node, dir, nodes, prefix ):
        # If the folder we've been passed contains an index.xml, send that file to be processed
        if xbmcvfs.exists( os.path.join( node, "index.xml" ) ):
            self.parse_view( os.path.join( node, "index.xml" ), nodes, True, "%s/%s/" % ( prefix, dir ), node, prefix = prefix )
    
    def parse_view( self, file, nodes, isFolder = False, origFolder = None, origPath = None, prefix = None ):
        if not isFolder and file.endswith( "index.xml" ):
            return
        try:
            # Load the xml file
            tree = xmltree.parse( file )
            root = tree.getroot()
            
            # Get the item index
            if "order" in root.attrib:
                index = root.attrib.get( "order" )
                origIndex = index
                while int( index ) in nodes:
                    index = int( index )
                    index += 1
                    index = str( index )
            else:
                self.indexCounter -= 1
                index = str( self.indexCounter )
                origIndex = "-"

            # Try to get media type from visibility condition
            mediaType = None
            if "visible" in root.attrib:
                visibleAttrib = root.attrib.get( "visible" )
                if not xbmc.getCondVisibility( visibleAttrib ):
                    # The node isn't visible
                    return
                if "Library.HasContent(" in visibleAttrib and "+" not in visibleAttrib and "|" not in visibleAttrib:
                    mediaType = visibleAttrib.split( "(" )[ 1 ].split( ")" )[ 0 ].lower()

            # Try to get media type from content node
            contentNode = root.find( "content" )
            if contentNode is not None:
                mediaType = contentNode.text

            # Get label and icon
            label = root.find( "label" ).text
            
            icon = root.find( "icon" )
            if icon is not None:
                icon = icon.text
            else:
                icon = ""

            if isFolder:
                # Add it to our list of nodes
                nodes[ int( index ) ] = [ label, icon, origFolder.decode( "utf-8" ), "folder", origIndex, mediaType ]
            else:
                # Check for a path
                path = root.find( "path" )
                if path is not None:
                    # Change the origPath (the url used as the shortcut address) to it
                    origPath = path.text
                    
                # Check for a grouping
                group = root.find( "group" )
                if group is None:
                    # Add it as an item
                    nodes[ int( index ) ] = [ label, icon, origPath, "item", origIndex, mediaType ]
                else:
                    # Add it as grouped
                    nodes[ int( index ) ] = [ label, icon, origPath, "grouped", origIndex, mediaType ]
        except:
            print_exc()
            
    def isGrouped( self, path ):        
        customPathVideo = path.replace( "library://video", os.path.join( xbmc.translatePath( "special://profile".decode('utf-8') ), "library", "video" ) )[:-1]
        defaultPathVideo = path.replace( "library://video", os.path.join( xbmc.translatePath( "special://xbmc".decode('utf-8') ), "system", "library", "video" ) )[:-1]
        customPathAudio = path.replace( "library://music", os.path.join( xbmc.translatePath( "special://profile".decode('utf-8') ), "library", "music" ) )[:-1]
        defaultPathAudio = path.replace( "library://music", os.path.join( xbmc.translatePath( "special://xbmc".decode('utf-8') ), "system", "library", "music" ) )[:-1]
        
        paths = [ customPathVideo, defaultPathVideo, customPathAudio, defaultPathAudio ]
        foundPath = False

        for tryPath in paths:
            if xbmcvfs.exists( tryPath ):
                path = tryPath
                foundPath = True
                break
        if foundPath == False:
            return False
        
        # Open the file
        try:
            # Load the xml file
            tree = xmltree.parse( path )
            root = tree.getroot()

            group = root.find( "group" )
            if group is None:
                return False
            else:
                return True
        except:
            return False

    #####################################
    # Function used by DataFunctions.py #
    #####################################
            
    def get_visibility( self, path ):
        path = path.replace( "videodb://", "library://video/" )
        path = path.replace( "musicdb://", "library://music/" )
        if path.endswith( ".xml" ):
            path = path[ :-3 ]
        if path.endswith( ".xml/" ):
            path = path[ :-4 ]

        if "library://video" in path:
            pathStart = "library://video"
            pathEnd = "video"
        elif "library://music" in path:
            pathStart = "library://music"
            pathEnd = "music"
        else:
            return ""

        customPath = path.replace( pathStart, os.path.join( xbmc.translatePath( "special://profile".decode('utf-8') ), "library", pathEnd ) ) + "index.xml"
        customFile = path.replace( pathStart, os.path.join( xbmc.translatePath( "special://profile".decode('utf-8') ), "library", pathEnd ) )[:-1] + ".xml"
        defaultPath = path.replace( pathStart, os.path.join( xbmc.translatePath( "special://xbmc".decode('utf-8') ), "system", "library", pathEnd ) ) + "index.xml"
        defaultFile = path.replace( pathStart, os.path.join( xbmc.translatePath( "special://xbmc".decode('utf-8') ), "system", "library", pathEnd ) )[:-1] + ".xml"

        # Check whether the node exists - either as a parent node (with an index.xml) or a view node (append .xml)
        # in first custom video nodes, then default video nodes
        if xbmcvfs.exists( customPath ):
            path = customPath
        elif xbmcvfs.exists( customFile ):
            path = customFile
        elif xbmcvfs.exists( defaultPath ):
            path = defaultPath
        elif xbmcvfs.exists( defaultFile ):
            path = defaultFile
        else:
            return ""
            
        # Open the file
        try:
            # Load the xml file
            tree = xmltree.parse( path )
            root = tree.getroot()

            if "visible" in root.attrib:
                return root.attrib.get( "visible" )
            else:
                return ""
        except:
            return False

    def get_mediaType( self, path ):
        path = path.replace( "videodb://", "library://video/" )
        path = path.replace( "musicdb://", "library://music/" )
        if path.endswith( ".xml" ):
            path = path[ :-3 ]
        if path.endswith( ".xml/" ):
            path = path[ :-4 ]

        if "library://video" in path:
            pathStart = "library://video"
            pathEnd = "video"
        elif "library://music" in path:
            pathStart = "library://music"
            pathEnd = "music"
        else:
            return "unknown"

        customPath = path.replace( pathStart, os.path.join( xbmc.translatePath( "special://profile".decode('utf-8') ), "library", pathEnd ) ) + "index.xml"
        customFile = path.replace( pathStart, os.path.join( xbmc.translatePath( "special://profile".decode('utf-8') ), "library", pathEnd ) )[:-1] + ".xml"
        defaultPath = path.replace( pathStart, os.path.join( xbmc.translatePath( "special://xbmc".decode('utf-8') ), "system", "library", pathEnd ) ) + "index.xml"
        defaultFile = path.replace( pathStart, os.path.join( xbmc.translatePath( "special://xbmc".decode('utf-8') ), "system", "library", pathEnd ) )[:-1] + ".xml"
        
        # Check whether the node exists - either as a parent node (with an index.xml) or a view node (append .xml)
        # in first custom video nodes, then default video nodes
        if xbmcvfs.exists( customPath ):
            path = customPath
        elif xbmcvfs.exists( customFile ):
            path = customFile
        elif xbmcvfs.exists( defaultPath ):
            path = defaultPath
        elif xbmcvfs.exists( defaultFile ):
            path = defaultFile
        else:
            return "unknown"
            
        # Open the file
        try:
            # Load the xml file
            tree = xmltree.parse( path )
            root = tree.getroot()

            mediaType = "unknown"
            if "visible" in root.attrib:
                visibleAttrib = root.attrib.get( "visible" )
                if "Library.HasContent(" in visibleAttrib and "+" not in visibleAttrib and "|" not in visibleAttrib:
                    mediaType = visibleAttrib.split( "(" )[ 1 ].split( ")" )[ 0 ].lower()

            contentNode = root.find( "content" )
            if contentNode is not None:
                mediaType = contentNode.text

            return mediaType

        except:
            return "unknown"
            
    ############################################
    # Functions used to add a node to the menu #
    ############################################

    def addNodeToMenu( self, node, label, icon, DATA ):
        # Get a list of all nodes within
        json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Files.GetDirectory", "params": { "properties": ["title", "file", "thumbnail"], "directory": "' + node + '", "media": "files" } }')
        json_query = unicode(json_query, 'utf-8', errors='ignore')
        json_response = simplejson.loads(json_query)
        
        labels = []
        paths = []
        
        # Add all directories returned by the json query
        if json_response.has_key('result') and json_response['result'].has_key('files') and json_response['result']['files'] is not None:
            for item in json_response['result']['files']:
                labels.append( item[ "label" ] )
                paths.append( item[ "file" ] )
        else:
            log( "Invalid JSON response returned" )
            return False
        
        # If there were some items returned by the JSON...
        if len( paths ) != 0:
            # Show a select dialog so the user can pick the default action
            selected = xbmcgui.Dialog().select( __language__( 32095 ), labels )
            
            if selected == -1 or selected is None:
                # User cancelled
                return True
            
        # We have all the information we need from the user to add the node to the menu :)
        
        # Load existing main menu items
        menuitems = DATA._get_shortcuts( "mainmenu" )
        DATA._clear_labelID()
        for menuitem in menuitems.findall( "shortcut" ):
            # Get existing items labelID's/
            DATA._get_labelID( DATA.local( menuitem.find( "label" ).text )[3], menuitem.find( "action" ).text )
            
        # Generate a new labelID
        labelID = DATA._get_labelID( label, paths[ selected ] )
        
        # Write the updated mainmenu.DATA.xml
        newelement = xmltree.SubElement( menuitems.getroot(), "shortcut" )
        xmltree.SubElement( newelement, "label" ).text = label
        xmltree.SubElement( newelement, "label2" ).text = "32024" # Custom shortcut
        xmltree.SubElement( newelement, "icon" ).text = icon
        xmltree.SubElement( newelement, "thumb" )
        xmltree.SubElement( newelement, "action" ).text = "ActivateWindow(10025," + paths[ selected ] + ",return)"
        
        DATA.indent( menuitems.getroot() )
        path = xbmc.translatePath( os.path.join( "special://profile", "addon_data", __addonid__, "mainmenu.DATA.xml" ).encode('utf-8') )
        menuitems.write( path, encoding="UTF-8" )
        
        if len( paths ) != 0:
            # Write the new [labelID].DATA.xml
            menuitems = xmltree.ElementTree( xmltree.Element( "shortcuts" ) )
            
            for item in json_response['result']['files']:
                newelement = xmltree.SubElement( menuitems.getroot(), "shortcut" )
                xmltree.SubElement( newelement, "label" ).text = item[ "label" ]
                xmltree.SubElement( newelement, "label2" ).text = "32024" # Custom shortcut
                xmltree.SubElement( newelement, "icon" ).text = item[ "thumbnail" ]
                xmltree.SubElement( newelement, "thumb" )
                xmltree.SubElement( newelement, "action" ).text = "ActivateWindow(10025," + item[ "file" ] + ",return)"
                
            DATA.indent( menuitems.getroot() )
            path = xbmc.translatePath( os.path.join( "special://profile", "addon_data", __addonid__, DATA.slugify( labelID ) + ".DATA.xml" ).encode('utf-8') )
            menuitems.write( path, encoding="UTF-8" )
        
        # Mark that the menu needs to be rebuilt
        xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-reloadmainmenu", "True" )
        
        # And tell the user it all worked
        xbmcgui.Dialog().ok( __addon__.getAddonInfo( "name" ), __language__(32090) )
        
        return True