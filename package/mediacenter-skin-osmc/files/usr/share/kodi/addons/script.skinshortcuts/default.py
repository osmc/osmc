# coding=utf-8
import os, sys
import xbmc, xbmcaddon, xbmcgui, xbmcplugin, urllib, xbmcvfs
import xml.etree.ElementTree as xmltree
import cPickle as pickle
import cProfile
import pstats
import random
import time
from time import gmtime, strftime
from datetime import datetime
from traceback import print_exc

if sys.version_info < (2, 7):
    import simplejson
else:
    import json as simplejson

__addon__        = xbmcaddon.Addon()
__addonid__      = __addon__.getAddonInfo('id').decode( 'utf-8' )
__addonversion__ = __addon__.getAddonInfo('version')
__language__     = __addon__.getLocalizedString
__cwd__          = __addon__.getAddonInfo('path').decode("utf-8")
__addonname__    = __addon__.getAddonInfo('name').decode("utf-8")
__resource__     = xbmc.translatePath( os.path.join( __cwd__, 'resources', 'lib' ) ).decode("utf-8")
__datapath__     = os.path.join( xbmc.translatePath( "special://profile/" ).decode( 'utf-8' ), "addon_data", __addonid__ )
__masterpath__   = os.path.join( xbmc.translatePath( "special://masterprofile/" ).decode( 'utf-8' ), "addon_data", __addonid__ )
__profilepath__  = xbmc.translatePath( "special://profile/" ).decode('utf-8')
__skinpath__     = xbmc.translatePath( "special://skin/shortcuts/" ).decode('utf-8')
__defaultpath__  = xbmc.translatePath( os.path.join( __cwd__, 'resources', 'shortcuts').encode("utf-8") ).decode("utf-8")
__xbmcversion__  = xbmc.getInfoLabel( "System.BuildVersion" ).split(".")[0]

sys.path.append(__resource__)

import xmlfunctions, datafunctions, library, nodefunctions
XML = xmlfunctions.XMLFunctions()
DATA = datafunctions.DataFunctions()
LIBRARY = library.LibraryFunctions()

hashlist = []

def log(txt):
    if __xbmcversion__ == "13" or __addon__.getSetting( "enable_logging" ) == "true":
        if isinstance (txt,str):
            txt = txt.decode('utf-8')
        message = u'%s: %s' % (__addonid__, txt)
        xbmc.log(msg=message.encode('utf-8'), level=xbmc.LOGDEBUG)
        
class Main:
    # MAIN ENTRY POINT
    def __init__(self):
        self._parse_argv()
        self.WINDOW = xbmcgui.Window(10000)    
        
        # Create data and master paths if not exists
        if not xbmcvfs.exists(__datapath__):
            xbmcvfs.mkdir(__datapath__)
        if not xbmcvfs.exists(__masterpath__):
            xbmcvfs.mkdir(__masterpath__)
        
        # Perform action specified by user
        if not self.TYPE:
            line1 = "This addon is for skin developers, and requires skin support"
            xbmcgui.Dialog().ok(__addonname__, line1)
            
        if self.TYPE=="buildxml":
            XML.buildMenu( self.MENUID, self.GROUP, self.LEVELS, self.MODE, self.OPTIONS )
            
        if self.TYPE=="launch":
            xbmcplugin.setResolvedUrl( handle=int( sys.argv[1]), succeeded=False, listitem=xbmcgui.ListItem() )
            self._launch_shortcut( self.PATH )
        if self.TYPE=="launchpvr":
            xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Player.Open", "params": { "item": {"channelid": ' + self.CHANNEL + '} } }')
        if self.TYPE=="manage":
            self._manage_shortcuts( self.GROUP, self.DEFAULTGROUP, self.NOLABELS, self.GROUPNAME )
            
        if self.TYPE=="hidesubmenu":
            self._hidesubmenu( self.MENUID )
        if self.TYPE=="resetlist":
            self._resetlist( self.MENUID, self.NEXTACTION )
            
        if self.TYPE=="shortcuts":
            # We're just going to choose a shortcut, and save its details to the given
            # skin labels
            
            if self.GROUPING is not None:
                selectedShortcut = LIBRARY.selectShortcut( "", grouping = self.GROUPING, custom = self.CUSTOM, showNone = self.NONE )
            else:
                selectedShortcut = LIBRARY.selectShortcut( "", custom = self.CUSTOM, showNone = self.NONE )
            
            # Now set the skin strings
            if selectedShortcut is not None and selectedShortcut.getProperty( "Path" ):
                path = urllib.unquote( selectedShortcut.getProperty( "Path" ) )
                if selectedShortcut.getProperty( "chosenPath" ):
                    path = urllib.unquote( selectedShortcut.getProperty( "chosenPath" ) )

                if path.startswith( "pvr-channel://" ):
                    path = "RunScript(script.skinshortcuts,type=launchpvr&channel=" + path.replace( "pvr-channel://", "" ) + ")"
                if self.LABEL is not None and selectedShortcut.getLabel() != "":
                    xbmc.executebuiltin( "Skin.SetString(" + self.LABEL + "," + selectedShortcut.getLabel() + ")" )
                if self.ACTION is not None:
                    xbmc.executebuiltin( "Skin.SetString(" + self.ACTION + "," + path + " )" )
                if self.SHORTCUTTYPE is not None:
                    xbmc.executebuiltin( "Skin.SetString(" + self.SHORTCUTTYPE + "," + selectedShortcut.getLabel2() + ")" )
                if self.THUMBNAIL is not None and selectedShortcut.getProperty( "icon" ):
                    xbmc.executebuiltin( "Skin.SetString(" + self.THUMBNAIL + "," + selectedShortcut.getProperty( "icon" ) + ")" )
                if self.THUMBNAIL is not None and selectedShortcut.getProperty( "thumbnail" ):
                    xbmc.executebuiltin( "Skin.SetString(" + self.THUMBNAIL + "," + selectedShortcut.getProperty( "thumbnail" ) + ")" )
                if self.LIST is not None:
                    xbmc.executebuiltin( "Skin.SetString(" + self.LIST + "," + DATA.getListProperty( path ) + ")" )
            elif selectedShortcut is not None and selectedShortcut.getLabel() == "::NONE::":
                # Clear the skin strings
                if self.LABEL is not None:
                    xbmc.executebuiltin( "Skin.Reset(" + self.LABEL + ")" )
                if self.ACTION is not None:
                    xbmc.executebuiltin( "Skin.Reset(" + self.ACTION + " )" )
                if self.SHORTCUTTYPE is not None:
                    xbmc.executebuiltin( "Skin.Reset(" + self.SHORTCUTTYPE + ")" )
                if self.THUMBNAIL is not None:
                    xbmc.executebuiltin( "Skin.Reset(" + self.THUMBNAIL + ")" )
                if self.THUMBNAIL is not None:
                    xbmc.executebuiltin( "Skin.Reset(" + self.THUMBNAIL + ")" )
                if self.LIST is not None:
                    xbmc.executebuiltin( "Skin.Reset(" + self.LIST + ")" )
                    
        if self.TYPE=="addNode":
            # We've been sent a node from plugin.program.video.node.editor
            targetDir = "library://video" + self.OPTIONS[ 0 ]
            
            icon = "DefaultShortcut.png"
            if self.OPTIONS[ 2 ] != "None":
                icon = self.OPTIONS[ 2 ].decode( "utf-8" )
            
            result = nodefunctions.NodeFunctions().addNodeToMenu( targetDir, urllib.unquote( self.OPTIONS[ 1 ] ).decode( "utf-8" ), icon, DATA )
            
            if result == False:
                # The item failed to add to the menu
                xbmcgui.dialog().ok( __addon__.getAddonInfo( "name" ), __language__(32091) )
                
        if self.TYPE=="resetall":
            # Tell XBMC not to try playing any media
            try:
                xbmcplugin.setResolvedUrl( handle=int( sys.argv[1]), succeeded=False, listitem=xbmcgui.ListItem() )
            except:
                log( "Not launched from a list item" )
            self._reset_all_shortcuts()

    def _parse_argv( self ):
        try:
            params = dict( arg.split( "=" ) for arg in sys.argv[ 1 ].split( "&" ) )
            self.TYPE = params.get( "type", "" )
        except:
            #print_exc()
            try:
                params = dict( arg.split( "=" ) for arg in sys.argv[ 2 ].split( "&" ) )
                self.TYPE = params.get( "?type", "" )
            except:
                self.TYPE = ""
                params = {}
        
        self.GROUP = params.get( "group", "" )
        self.GROUPNAME = params.get( "groupname", None )
        self.GROUPING = params.get( "grouping", None )
        self.PATH = params.get( "path", "" )
        self.MENUID = params.get( "mainmenuID", "0" )
        self.NEXTACTION = params.get( "action", "0" )
        self.LEVELS = params.get( "levels", "0" )
        self.MODE = params.get( "mode", None )
        self.CHANNEL = params.get( "channel", None )
        
        # Properties when using LIBRARY._displayShortcuts
        self.LABEL = params.get( "skinLabel", None )
        self.ACTION = params.get( "skinAction", None )
        self.SHORTCUTTYPE = params.get( "skinType", None )
        self.THUMBNAIL = params.get( "skinThumbnail", None )
        self.LIST = params.get( "skinList", None )
        self.CUSTOM = params.get( "custom", "False" )
        self.NONE = params.get( "showNone", "False" )
        
        self.NOLABELS = params.get( "nolabels", "false" ).lower()
        self.OPTIONS = params.get( "options", "" ).split( "|" )
        self.WARNING = params.get( "warning", None )
        self.DEFAULTGROUP = params.get( "defaultGroup", None )
    
    
    # -----------------
    # PRIMARY FUNCTIONS
    # -----------------

    def _launch_shortcut( self, path ):
        action = urllib.unquote( self.PATH )
        
        if action.find("::MULTIPLE::") == -1:
            # Single action, run it
            xbmc.executebuiltin( action )
        else:
            # Multiple actions, separated by |
            actions = action.split( "|" )
            for singleAction in actions:
                if singleAction != "::MULTIPLE::":
                    xbmc.executebuiltin( singleAction )
        
    
    def _manage_shortcuts( self, group, defaultGroup, nolabels, groupname ):
        homeWindow = xbmcgui.Window( 10000 )
        if homeWindow.getProperty( "skinshortcuts-loading" ):
            return

        homeWindow.setProperty( "skinshortcuts-loading", "True" )
        import gui
        ui= gui.GUI( "script-skinshortcuts.xml", __cwd__, "default", group=group, defaultGroup=defaultGroup, nolabels=nolabels, groupname=groupname )
        ui.doModal()
        del ui
        
        # Update home window property (used to automatically refresh type=settings)
        homeWindow.setProperty( "skinshortcuts",strftime( "%Y%m%d%H%M%S",gmtime() ) )
        
        # Clear window properties for this group, and for backgrounds, widgets, properties
        homeWindow.clearProperty( "skinshortcuts-" + group )        
        homeWindow.clearProperty( "skinshortcutsWidgets" )        
        homeWindow.clearProperty( "skinshortcutsCustomProperties" )        
        homeWindow.clearProperty( "skinshortcutsBackgrounds" )        
        
        
    def _reset_all_shortcuts( self ):
        log( "### Resetting all shortcuts" )
        log( repr( self.WARNING) )
        dialog = xbmcgui.Dialog()
        
        shouldRun = None
        if self.WARNING is not None and self.WARNING.lower() == "false":
            shouldRun = True
        
        # Ask the user if they're sure they want to do this
        if shouldRun is None:
            shouldRun = dialog.yesno( __language__( 32037 ), __language__( 32038 ) )
        
        if shouldRun:
            for files in xbmcvfs.listdir( __datapath__ ):
                # Try deleting all shortcuts
                if files:
                    for file in files:
                        if file != "settings.xml":
                            file_path = os.path.join( __datapath__, file.decode( 'utf-8' ) ).encode( 'utf-8' )
                            if xbmcvfs.exists( file_path ):
                                try:
                                    xbmcvfs.delete( file_path )
                                except:
                                    print_exc()
                                    log( "### ERROR could not delete file %s" % file[0] )
        
            # Update home window property (used to automatically refresh type=settings)
            xbmcgui.Window( 10000 ).setProperty( "skinshortcuts",strftime( "%Y%m%d%H%M%S",gmtime() ) )   
    
    # Functions for providing whoe menu in single list
    def _hidesubmenu( self, menuid ):
        count = 0
        while xbmc.getCondVisibility( "!IsEmpty(Container(" + menuid + ").ListItem(" + str( count ) + ").Property(isSubmenu))" ):
            count -= 1
            
        if count != 0:
            xbmc.executebuiltin( "Control.Move(" + menuid + "," + str( count ) + " )" )
        
        xbmc.executebuiltin( "ClearProperty(submenuVisibility, 10000)" )
        
    def _resetlist( self, menuid, action ):
        count = 0
        while xbmc.getCondVisibility( "!IsEmpty(Container(" + menuid + ").ListItemNoWrap(" + str( count ) + ").Label)" ):
            count -= 1
            
        count += 1
            
        if count != 0:
            xbmc.executebuiltin( "Control.Move(" + menuid + "," + str( count ) + " )" )
            
        xbmc.executebuiltin( urllib.unquote( action ) )
        
if ( __name__ == "__main__" ):
    log('script version %s started' % __addonversion__)
    
    # Profiling
    #filename = os.path.join( __datapath__, strftime( "%Y%m%d%H%M%S",gmtime() ) + "-" + str( random.randrange(0,100000) ) + ".log" )
    #cProfile.run( 'Main()', filename )
    
    #stream = open( filename + ".txt", 'w')
    #p = pstats.Stats( filename, stream = stream )
    #p.sort_stats( "cumulative" )
    #p.print_stats()
    
    # No profiling
    Main()
    
    log('script stopped')
