# coding=utf-8
import os, sys, datetime, unicodedata, re
import xbmc, xbmcgui, xbmcvfs, xbmcaddon
import xml.etree.ElementTree as xmltree
from xml.sax.saxutils import escape as escapeXML
import copy
import ast
from traceback import print_exc
from unicodeutils import try_decode

if sys.version_info < (2, 7):
    import simplejson
else:
    import json as simplejson

__addon__        = xbmcaddon.Addon()
__addonid__      = sys.modules[ "__main__" ].__addonid__
__addonversion__ = __addon__.getAddonInfo('version')
__xbmcversion__  = xbmc.getInfoLabel( "System.BuildVersion" ).split(".")[0]
__datapath__     = os.path.join( xbmc.translatePath( "special://profile/addon_data/" ).decode('utf-8'), __addonid__ ).encode('utf-8')
__masterpath__     = os.path.join( xbmc.translatePath( "special://masterprofile/addon_data/" ).decode('utf-8'), __addonid__ ).encode('utf-8')
__skin__         = xbmc.translatePath( "special://skin/" )    
__language__     = __addon__.getLocalizedString

import datafunctions, template
DATA = datafunctions.DataFunctions()
import hashlib, hashlist

def log(txt):
    if __xbmcversion__ == "13" or __addon__.getSetting( "enable_logging" ) == "true":
        if isinstance (txt,str):
            txt = txt.decode('utf-8')
        message = u'%s: %s' % (__addonid__, txt)
        xbmc.log(msg=message.encode('utf-8'), level=xbmc.LOGDEBUG)
    
class XMLFunctions():
    def __init__(self):
        self.MAINWIDGET = {}
        self.MAINBACKGROUND = {}
        self.hasSettings = False
        self.widgetCount = 1

        self.skinDir = xbmc.translatePath( "special://skin" )
        
        self.checkForShorctcuts = []
        
    def buildMenu( self, mainmenuID, groups, numLevels, buildMode, options, weEnabledSystemDebug = False, weEnabledScriptDebug = False ): 
        # Entry point for building includes.xml files
        if xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-isrunning" ) == "True":
            return
        
        xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-isrunning", "True" )
 
        # Get a list of profiles
        fav_file = xbmc.translatePath( 'special://userdata/profiles.xml' ).decode("utf-8")
        tree = None
        if xbmcvfs.exists( fav_file ):
            f = xbmcvfs.File( fav_file )
            tree = xmltree.fromstring( f.read() )
        
        profilelist = []
        if tree is not None:
            profiles = tree.findall( "profile" )
            for profile in profiles:
                name = profile.find( "name" ).text.encode( "utf-8" )
                dir = profile.find( "directory" ).text.encode( "utf-8" )
                log( "Profile found: " + name + " (" + dir + ")" )
                # Localise the directory
                if "://" in dir:
                    dir = xbmc.translatePath( dir ).decode( "utf-8" )
                else:
                    # Base if off of the master profile
                    dir = xbmc.translatePath( os.path.join( "special://masterprofile", dir ) ).decode( "utf-8" )
                profilelist.append( [ dir, "StringCompare(System.ProfileName," + name.decode( "utf-8" ) + ")", name.decode( "utf-8" ) ] )
                
        else:
            profilelist = [["special://masterprofile", None]]
 
        if self.shouldwerun( profilelist ) == False:
            log( "Menu is up to date" )
            xbmcgui.Window( 10000 ).clearProperty( "skinshortcuts-isrunning" )
            return

        progress = None
        # Create a progress dialog
        progress = xbmcgui.DialogProgressBG()
        progress.create(__addon__.getAddonInfo( "name" ), __language__( 32049 ) )
        progress.update( 0 )
        
        # Write the menus
        try:
            self.writexml( profilelist, mainmenuID, groups, numLevels, buildMode, progress, options )
            complete = True
        except:
            log( "Failed to write menu" )
            print_exc()
            complete = False
        
        # Mark that we're no longer running, clear the progress dialog
        xbmcgui.Window( 10000 ).clearProperty( "skinshortcuts-isrunning" )
        progress.close()
        
        if complete == True:
            # Menu is built, reload the skin
            xbmc.executebuiltin( "XBMC.ReloadSkin()" )
        else:
            # Menu couldn't be built - if the user has script.xbmc.debug.log offer to upload a debug log
            if xbmc.getCondVisibility( "System.HasAddon( script.xbmc.debug.log )" ):
                # If we enabled debug logging
                if weEnabledSystemDebug or weEnabledScriptDebug:
                    # Disable any logging we enabled
                    if weEnabledSystemDebug:
                        json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method":"Settings.setSettingValue", "params": {"setting":"debug.showloginfo", "value":false} } ' )
                    if weEnabledScriptDebug:
                        __addon__.setSetting( "enable_logging", "false" )
                        
                    # Offer to upload a debug log
                    ret = xbmcgui.Dialog().yesno( __addon__.getAddonInfo( "name" ), __language__( 32092 ), __language__( 32093 ) )
                    if ret:
                        xbmc.executebuiltin( "RunScript(script.xbmc.debug.log)" )                    
                        
                else:
                    # Enable any debug logging needed                        
                    json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method": "Settings.getSettings", "params": { "filter":{"section":"system", "category":"debug"} } }')
                    json_query = unicode(json_query, 'utf-8', errors='ignore')
                    json_response = simplejson.loads(json_query)
                    
                    enabledSystemDebug = False
                    enabledScriptDebug = False
                    if json_response.has_key('result') and json_response['result'].has_key('settings') and json_response['result']['settings'] is not None:
                        for item in json_response['result']['settings']:
                            if item["id"] == "debug.showloginfo":
                                if item["value"] == False:
                                    json_query = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "id": 0, "method":"Settings.setSettingValue", "params": {"setting":"debug.showloginfo", "value":true} } ' )
                                    enabledSystemDebug = True
                    
                    if __addon__.getSetting( "enable_logging" ) != "true":
                        __addon__.setSetting( "enable_logging", "true" )
                        enabledScriptDebug = True
                        
                    if enabledSystemDebug or enabledScriptDebug:
                        # We enabled one or more of the debug options, re-run this function
                        self.buildMenu( mainmenuID, groups, numLevels, buildMode, options, enabledSystemDebug, enabledScriptDebug )
                    else:
                        # Debug logging already enabled - offer to upload a debug log
                        ret = xbmcgui.Dialog().yesno( __addon__.getAddonInfo( "name" ), __language__( 32092 ), __language__( 32093 ) )
                        if ret:
                            xbmc.executebuiltin( "RunScript(script.xbmc.debug.log)" )                    
                            
            else:
                xbmcgui.Dialog().ok( __addon__.getAddonInfo( "name" ), __language__( 32092 ), __language__( 32094 ) )
        
    def shouldwerun( self, profilelist ):
        try:
            property = xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-reloadmainmenu" )
            xbmcgui.Window( 10000 ).clearProperty( "skinshortcuts-reloadmainmenu" )
            if property == "True":
                log( "Menu has been edited")
                return True
        except:
            pass
            
        # Get the skins addon.xml file
        addonpath = xbmc.translatePath( os.path.join( "special://skin/", 'addon.xml').encode("utf-8") ).decode("utf-8")
        addon = xmltree.parse( addonpath )
        extensionpoints = addon.findall( "extension" )
        paths = []
        skinpaths = []
        
        # Get the skin version
        skinVersion = addon.getroot().attrib.get( "version" )
        
        # Get the directories for resolutions this skin supports
        for extensionpoint in extensionpoints:
            if extensionpoint.attrib.get( "point" ) == "xbmc.gui.skin":
                resolutions = extensionpoint.findall( "res" )
                for resolution in resolutions:
                    path = xbmc.translatePath( os.path.join( "special://skin/", resolution.attrib.get( "folder" ), "script-skinshortcuts-includes.xml").encode("utf-8") ).decode("utf-8")
                    paths.append( path )
                    skinpaths.append( path )
        
        # Check for the includes file
        for path in paths:
            if not xbmcvfs.exists( path ):
                log( "Includes file does not exist" )
                return True
            else:
                pass
                


        try:
            hashes = ast.literal_eval( xbmcvfs.File( os.path.join( __masterpath__ , xbmc.getSkinDir() + ".hash" ) ).read() )
        except:
            # There is no hash list, return True
            log( "No hash list" )
            print_exc()
            return True
        
        checkedXBMCVer = False
        checkedSkinVer = False
        checkedScriptVer = False
        checkedProfileList = False
            
        for hash in hashes:
            if hash[1] is not None:
                if hash[0] == "::XBMCVER::":
                    # Check the skin version is still the same as hash[1]
                    checkedXBMCVer = True
                    if __xbmcversion__ != hash[1]:
                        log( "Now running a different version of Kodi" )
                        return True
                elif hash[0] == "::SKINVER::":
                    # Check the skin version is still the same as hash[1]
                    checkedSkinVer = True
                    if skinVersion != hash[1]:
                        log( "Now running a different skin version" )
                        return True
                elif hash[0] == "::SCRIPTVER::":
                    # Check the script version is still the same as hash[1]
                    checkedScriptVer = True
                    if __addonversion__ != hash[1]:
                        log( "Now running a different script version" )
                        return True
                elif hash[0] == "::PROFILELIST::":
                    # Check the profilelist is still the same as hash[1]
                    checkedProfileList = True
                    if profilelist != hash[1]:
                        log( "Profiles have changes" )
                        return True
                elif hash[0] == "::LANGUAGE::":
                    # We no longer need to rebuild on a system language change
                    pass
                elif hash[0] == "::SKINBOOL::":
                    # A boolean we need to set (if profile matches)
                    if xbmc.getCondVisibility( hash[ 1 ][ 0 ] ):
                        if hash[ 1 ][ 2 ] == "True":
                            xbmc.executebuiltin( "Skin.SetBool(%s)" %( hash[ 1 ][ 1 ] ) )
                        else:
                            xbmc.executebuiltin( "Skin.Reset(%s)" %( hash[ 1 ][ 1 ] ) )
                else:
                    try:
                        hasher = hashlib.md5()
                        hasher.update( xbmcvfs.File( hash[0] ).read() )
                        if hasher.hexdigest() != hash[1]:
                            log( "Hash does not match on file " + hash[0] )
                            log( "(" + hash[1] + " > " + hasher.hexdigest() + ")" )
                            return True
                    except:
                        log( "Unable to generate hash for %s" %( hash[ 0 ] ) )
                        log( "(%s > ?)" %( hash[ 1 ] ) )
            else:
                if xbmcvfs.exists( hash[0] ):
                    log( "File now exists " + hash[0] )
                    return True
                
        # If the skin or script version, or profile list, haven't been checked, we need to rebuild the menu 
        # (most likely we're running an old version of the script)
        if checkedXBMCVer == False or checkedSkinVer == False or checkedScriptVer == False or checkedProfileList == False:
            return True
        
            
        # If we get here, the menu does not need to be rebuilt.
        return False


    def writexml( self, profilelist, mainmenuID, groups, numLevels, buildMode, progress, options ): 
        # Reset the hashlist, add the profile list and script version
        hashlist.list = []
        hashlist.list.append( ["::PROFILELIST::", profilelist] )
        hashlist.list.append( ["::SCRIPTVER::", __addonversion__] )
        hashlist.list.append( ["::XBMCVER::", __xbmcversion__] )
        
        # Clear any skin settings for backgrounds and widgets
        DATA._reset_backgroundandwidgets()
        self.widgetCount = 1
        
        # Create a new tree and includes for the various groups
        tree = xmltree.ElementTree( xmltree.Element( "includes" ) )
        root = tree.getroot()
        
        # Create a Template object and pass it the root
        Template = template.Template()
        Template.includes = root
        
        # Get any shortcuts we're checking for
        self.checkForShortcuts = []
        overridestree = DATA._get_overrides_skin()
        if overridestree is not None:
            checkForShorctcutsOverrides = overridestree.getroot().findall( "checkforshortcut" )
            for checkForShortcutOverride in checkForShorctcutsOverrides:
                if "property" in checkForShortcutOverride.attrib:
                    # Add this to the list of shortcuts we'll check for
                    self.checkForShortcuts.append( ( checkForShortcutOverride.text.lower(), checkForShortcutOverride.attrib.get( "property" ), "False" ) )
        
        mainmenuTree = xmltree.SubElement( root, "include" )
        mainmenuTree.set( "name", "skinshortcuts-mainmenu" )
        
        submenuTrees = []
        for level in range( 0,  int( numLevels) + 1 ):
            subelement = xmltree.SubElement(root, "include")
            subtree = xmltree.SubElement( root, "include" )
            if level == 0:
                subtree.set( "name", "skinshortcuts-submenu" )
            else:
                subtree.set( "name", "skinshortcuts-submenu-" + str( level ) )
            if not subtree in submenuTrees:
                submenuTrees.append( subtree )
        
        if buildMode == "single":
            allmenuTree = xmltree.SubElement( root, "include" )
            allmenuTree.set( "name", "skinshortcuts-allmenus" )
        
        profilePercent = 100 / len( profilelist )
        profileCount = -1
        
        submenuNodes = {}
        
        for profile in profilelist:
            log( "Building menu for profile %s" %( profile[ 2 ] ) )
            # Load profile details
            profileDir = profile[0]
            profileVis = profile[1]
            profileCount += 1
            
            # Reset whether we have settings
            self.hasSettings = False
            
            # Reset any checkForShortcuts to say we haven't found them
            newCheckForShortcuts = []
            for checkforShortcut in self.checkForShortcuts:
                newCheckForShortcuts.append( ( checkforShortcut[ 0 ], checkforShortcut[ 1 ], "False" ) )
            self.checkForShortcuts = newCheckForShortcuts

            # Clear any previous labelID's
            DATA._clear_labelID()
            
            # Create objects to hold the items
            menuitems = []
            templateMainMenuItems = xmltree.Element( "includes" )
            
            # If building the main menu, split the mainmenu shortcut nodes into the menuitems list
            if groups == "" or groups.split( "|" )[0] == "mainmenu":
                # Set a skinstring that marks that we're providing the whole menu
                xbmc.executebuiltin( "Skin.SetBool(SkinShortcuts-FullMenu)" )
                for node in DATA._get_shortcuts( "mainmenu", None, True, profile[0] ).findall( "shortcut" ):
                    menuitems.append( node )
            else:
                # Clear any skinstring marking that we're providing the whole menu
                xbmc.executebuiltin( "Skin.Reset(SkinShortcuts-FullMenu)" )
                    
            # If building specific groups, split them into the menuitems list
            count = 0
            if groups != "":
                for group in groups.split( "|" ):
                    if count != 0 or group != "mainmenu":
                        menuitems.append( group )
                        
            if len( menuitems ) == 0:
                # No groups to build
                break
                
            itemidmainmenu = 0
            percent = profilePercent / len( menuitems )
            
            i = 0
            for item in menuitems:
                i += 1
                itemidmainmenu += 1
                progress.update( ( profilePercent * profileCount) + percent * i )
                submenuDefaultID = None

                if not isinstance( item, basestring ):
                    # This is a main menu item (we know this because it's an element, not a string)
                    submenu = item.find( "labelID" ).text

                    # Build the menu item
                    menuitem = self.buildElement( item, "mainmenu", None, profile[1], DATA.slugify( submenu, convertInteger=True ), itemid = itemidmainmenu, options = options )

                    # Add the menu item to the various includes, retaining a reference to them
                    mainmenuItemA = copy.deepcopy( menuitem )
                    mainmenuTree.append( mainmenuItemA )

                    if buildMode == "single":
                        mainmenuItemB = copy.deepcopy( menuitem )
                        allmenuTree.append( mainmenuItemB )

                    templateMainMenuItems.append( copy.deepcopy( menuitem ) )

                    # Get submenu defaultID
                    submenuDefaultID = item.find( "defaultID" ).text
                else:
                    # It's an additional menu, so get its labelID
                    submenu = DATA._get_labelID( item, None )
                    
                # Build the submenu
                count = 0 # Used to keep track of additional submenu
                for submenuTree in submenuTrees:
                    submenuVisibilityName = submenu
                    if count == 1:
                        submenu = submenu + "." + str( count )
                    elif count != 0:
                        submenu = submenu[:-1] + str( count )
                        submenuVisibilityName = submenu[:-2]
                        
                    # Get the tree's we're going to write the menu to
                    if submenu in submenuNodes:
                        justmenuTreeA = submenuNodes[ submenu ][ 0 ]
                        justmenuTreeB = submenuNodes[ submenu ][ 1 ]
                    else:
                        # Create these nodes
                        justmenuTreeA = xmltree.SubElement( root, "include" )
                        justmenuTreeB = xmltree.SubElement( root, "include" )
                        
                        justmenuTreeA.set( "name", "skinshortcuts-group-" + DATA.slugify( submenu ) )
                        justmenuTreeB.set( "name", "skinshortcuts-group-alt-" + DATA.slugify( submenu ) )
                        
                        submenuNodes[ submenu ] = [ justmenuTreeA, justmenuTreeB ]
                        
                    itemidsubmenu = 0
                    
                    # Get the shortcuts for the submenu
                    if count == 0:
                        submenudata = DATA._get_shortcuts( submenu, submenuDefaultID, True, profile[0] )
                    else:
                        submenudata = DATA._get_shortcuts( submenu, None, True, profile[0] )
                        
                    if type( submenudata ) == list:
                        submenuitems = submenudata
                    else:
                        submenuitems = submenudata.findall( "shortcut" )
                    
                    # Are there any submenu items for the main menu?
                    if count == 0:
                        if len( submenuitems ) != 0:
                            try:
                                hasSubMenu = xmltree.SubElement( mainmenuItemA, "property" )
                                hasSubMenu.set( "name", "hasSubmenu" )
                                hasSubMenu.text = "True"
                                if buildMode == "single":
                                    hasSubMenu = xmltree.SubElement( mainmenuItemB, "property" )
                                    hasSubMenu.set( "name", "hasSubmenu" )
                                    hasSubMenu.text = "True"
                            except:
                                # There probably isn't a main menu
                                pass
                        else:   
                            try:
                                hasSubMenu = xmltree.SubElement( mainmenuItemA, "property" )
                                hasSubMenu.set( "name", "hasSubmenu" )
                                hasSubMenu.text = "False"
                                if buildMode == "single":
                                    hasSubMenu = xmltree.SubElement( mainmenuItemB, "property" )
                                    hasSubMenu.set( "name", "hasSubmenu" )
                                    hasSubMenu.text = "False"
                            except:
                                # There probably isn't a main menu
                                pass
                
                    # If we're building a single menu, update the onclicks of the main menu
                    if buildMode == "single" and not len( submenuitems ) == 0:
                        for onclickelement in mainmenuItemB.findall( "onclick" ):
                            if "condition" in onclickelement.attrib:
                                onclickelement.set( "condition", "StringCompare(Window(10000).Property(submenuVisibility)," + DATA.slugify( submenuVisibilityName, convertInteger=True ) + ") + [" + onclickelement.attrib.get( "condition" ) + "]" )
                                newonclick = xmltree.SubElement( mainmenuItemB, "onclick" )
                                newonclick.text = "SetProperty(submenuVisibility," + DATA.slugify( submenuVisibilityName, convertInteger=True ) + ",10000)"
                                newonclick.set( "condition", onclickelement.attrib.get( "condition" ) )
                            else:
                                onclickelement.set( "condition", "StringCompare(Window(10000).Property(submenuVisibility)," + DATA.slugify( submenuVisibilityName, convertInteger=True ) + ")" )
                                newonclick = xmltree.SubElement( mainmenuItemB, "onclick" )
                                newonclick.text = "SetProperty(submenuVisibility," + DATA.slugify( submenuVisibilityName, convertInteger=True ) + ",10000)"
                    
                    # Build the submenu items
                    for submenuItem in submenuitems:
                        itemidsubmenu += 1
                        # Build the item without any visibility conditions
                        menuitem = self.buildElement( submenuItem, submenu, None, profile[1], itemid = itemidsubmenu, options = options )
                        isSubMenuElement = xmltree.SubElement( menuitem, "property" )
                        isSubMenuElement.set( "name", "isSubmenu" )
                        isSubMenuElement.text = "True"

                        # Add it, with appropriate visibility conditions, to the various submenu includes
                        justmenuTreeA.append( copy.deepcopy( menuitem ) )

                        menuitemCopy = copy.deepcopy( menuitem )
                        visibilityElement = menuitemCopy.find( "visible" )
                        visibilityElement.text = "[%s] + %s" %( visibilityElement.text, "StringCompare(Window(10000).Property(submenuVisibility)," + DATA.slugify( submenuVisibilityName, convertInteger=True ) + ")" )
                        justmenuTreeB.append( menuitemCopy )

                        if buildMode == "single":
                            allmenuTree.append( copy.deepcopy( menuitemCopy ) )

                        menuitemCopy = copy.deepcopy( menuitem )
                        visibilityElement = menuitemCopy.find( "visible" )
                        visibilityElement.text = "[%s] + %s" %( visibilityElement.text, "StringCompare(Container(" + mainmenuID + ").ListItem.Property(submenuVisibility)," + DATA.slugify( submenuVisibilityName, convertInteger=True ) + ")" )
                        submenuTree.append( menuitemCopy )
                            
                    # Build the template for the submenu
                    Template.parseItems( "submenu", count, justmenuTreeA, profile[ 2 ], profile[ 1 ], "StringCompare(Container(" + mainmenuID + ").ListItem.Property(submenuVisibility)," + DATA.slugify( submenuVisibilityName, convertInteger=True ) + ")", item )
                        
                    count += 1

            if self.hasSettings == False:
                # Check if the overrides asks for a forced settings...
                overridestree = DATA._get_overrides_skin()
                if overridestree is not None:
                    forceSettings = overridestree.getroot().find( "forcesettings" )
                    if forceSettings is not None:
                        # We want a settings option to be added
                        newelement = xmltree.SubElement( mainmenuTree, "item" )
                        xmltree.SubElement( newelement, "label" ).text = "$LOCALIZE[10004]"
                        xmltree.SubElement( newelement, "icon" ).text = "DefaultShortcut.png"
                        xmltree.SubElement( newelement, "onclick" ).text = "ActivateWindow(settings)" 
                        xmltree.SubElement( newelement, "visible" ).text = profile[1]
                        
                        if buildMode == "single":
                            newelement = xmltree.SubElement( mainmenuTree, "item" )
                            xmltree.SubElement( newelement, "label" ).text = "$LOCALIZE[10004]"
                            xmltree.SubElement( newelement, "icon" ).text = "DefaultShortcut.png"
                            xmltree.SubElement( newelement, "onclick" ).text = "ActivateWindow(settings)" 
                            xmltree.SubElement( newelement, "visible" ).text = profile[1]
                            
            if len( self.checkForShortcuts ) != 0:
                # Add a value to the variable for all checkForShortcuts
                for checkForShortcut in self.checkForShortcuts:
                    if profile[ 1 ] is not None and xbmc.getCondVisibility( profile[ 1 ] ):
                        # Current profile - set the skin bool
                        if checkForShortcut[ 2 ] == "True":
                            xbmc.executebuiltin( "Skin.SetBool(%s)" %( checkForShortcut[ 1 ] ) )
                        else:
                            xbmc.executebuiltin( "Skin.Reset(%s)" %( checkForShortcut[ 1 ] ) )
                    # Save this to the hashes file, so we can set it on profile changes
                    hashlist.list.append( [ "::SKINBOOL::", [ profile[ 1 ], checkForShortcut[ 1 ], checkForShortcut[ 2 ] ] ] )

            # Build the template for the main menu
            Template.parseItems( "mainmenu", 0, templateMainMenuItems, profile[ 2 ], profile[ 1 ], "", "", mainmenuID )
                    
        # Build any 'Other' templates
        Template.writeOthers()
        
        progress.update( 100, message = __language__( 32098 ) )
                
        # Get the skins addon.xml file
        addonpath = xbmc.translatePath( os.path.join( "special://skin/", 'addon.xml').encode("utf-8") ).decode("utf-8")
        addon = xmltree.parse( addonpath )
        extensionpoints = addon.findall( "extension" )
        paths = []
        for extensionpoint in extensionpoints:
            if extensionpoint.attrib.get( "point" ) == "xbmc.gui.skin":
                resolutions = extensionpoint.findall( "res" )
                for resolution in resolutions:
                    path = xbmc.translatePath( os.path.join( self.skinDir, resolution.attrib.get( "folder" ), "script-skinshortcuts-includes.xml").encode("utf-8") ).decode("utf-8")
                    paths.append( path )
        skinVersion = addon.getroot().attrib.get( "version" )
        
        # Save the tree
        DATA.indent( tree.getroot() )
        for path in paths:
            tree.write( path, encoding="UTF-8" )
            
            # Save the hash of the file we've just written
            with open(path, "r+") as f:
                DATA._save_hash( path, f.read() )
                f.close()
            
        # Save the hashes
        # Append the skin version to the hashlist
        hashlist.list.append( ["::SKINVER::", skinVersion] )

        # Save the hashes
        file = xbmcvfs.File( os.path.join( __masterpath__ , xbmc.getSkinDir() + ".hash" ), "w" )
        file.write( repr( hashlist.list ) )
        file.close
        
        
    def buildElement( self, item, groupName, visibilityCondition, profileVisibility, submenuVisibility = None, itemid=-1, options=[] ):
        # This function will build an element for the passed Item in
        newelement = xmltree.Element( "item" )

        if itemid is not -1:
            newelement.set( "id", str( itemid ) )
            
        # Label and label2
        xmltree.SubElement( newelement, "label" ).text = DATA.local( item.find( "label" ).text )[1]
        xmltree.SubElement( newelement, "label2" ).text = DATA.local( item.find( "label2" ).text )[1]
            
        # Icon and thumb
        icon = item.find( "override-icon" )
        if icon is None:
            icon = item.find( "icon" )
        if icon is None:
            xmltree.SubElement( newelement, "icon" ).text = "DefaultShortcut.png"
        else:
            xmltree.SubElement( newelement, "icon" ).text = icon.text
        thumb = item.find( "thumb" )
        if thumb is not None:
            xmltree.SubElement( newelement, "thumb" ).text = item.find( "thumb" ).text
        
        # labelID and defaultID
        labelID = xmltree.SubElement( newelement, "property" )
        labelID.text = item.find( "labelID" ).text
        labelID.set( "name", "labelID" )
        defaultID = xmltree.SubElement( newelement, "property" )
        defaultID.text = item.find( "defaultID" ).text
        defaultID.set( "name", "defaultID" )
        
        # Additional properties
        properties = eval( item.find( "additional-properties" ).text )
        if len( properties ) != 0:
            repr( properties )
            for property in properties:
                if property[0] == "node.visible":
                    visibleProperty = xmltree.SubElement( newelement, "visible" )
                    visibleProperty.text = try_decode( property[1] )                    
                else:
                    additionalproperty = xmltree.SubElement( newelement, "property" )
                    additionalproperty.set( "name", property[0].decode( "utf-8" ) )
                    additionalproperty.text = DATA.local( property[1] )[1]
                        
                    # If this is a widget or background, set a skin setting to say it's enabled
                    if property[0] == "widget":
                        xbmc.executebuiltin( "Skin.SetBool(skinshortcuts-widget-" + property[1] + ")" )
                        # And if it's the main menu, list it
                        if groupName == "mainmenu":
                            xbmc.executebuiltin( "Skin.SetString(skinshortcuts-widget-" + str( self.widgetCount ) + "," + property[ 1 ] + ")" )
                            self.widgetCount += 1
                    elif property[0] == "background":
                        try:
                            xbmc.executebuiltin( "Skin.SetBool(skinshortcuts-background-" + property[1] + ")" )
                        except UnicodeEncodeError:							
                            xbmc.executebuiltin( "Skin.SetBool(skinshortcuts-background-" + property[1].encode('utf-8') + ")" )
                        
                    # If this is the main menu, and we're cloning widgets or backgrounds...
                    if groupName == "mainmenu":
                        if "clonewidgets" in options:
                            if property[0] == "widget" or property[0] == "widgetName" or property[0] == "widgetType" or property[0] == "widgetPlaylist":
                                self.MAINWIDGET[ property[0] ] = property[1]
                        if "clonebackgrounds" in options:
                            if property[0] == "background" or property[0] == "backgroundName" or property[0] == "backgroundPlaylist" or property[0] == "backgroundPlaylistName":
                                self.MAINBACKGROUND[ property[0] ] = property[1]

                    # For backwards compatibility, save widgetPlaylist as widgetPath too
                    if property[ 0 ] == "widgetPlaylist":
                        additionalproperty = xmltree.SubElement( newelement, "property" )
                        additionalproperty.set( "name", "widgetPath" )
                        try:
                            additionalproperty.text = DATA.local( property[1].decode( "utf-8" ) )[1]
                        except:
                            additionalproperty.text = DATA.local( property[1] )[1]
        
        # Primary visibility
        visibility = item.find( "visibility" )
        if visibility is not None:
            xmltree.SubElement( newelement, "visible" ).text = visibility.text
        
        #additional onclick (group overrides)
        onclicks = item.findall( "additional-action" )
        for onclick in onclicks:
            onclickelement = xmltree.SubElement( newelement, "onclick" )
            onclickelement.text = onclick.text
            if "condition" in onclick.attrib:
                onclickelement.set( "condition", onclick.attrib.get( "condition" ) )
        
        # Onclick
        onclicks = item.findall( "override-action" )
        if len( onclicks ) == 0:
            onclicks = item.findall( "action" )
            
        for onclick in onclicks:
            onclickelement = xmltree.SubElement( newelement, "onclick" )
            # PVR Action
            if onclick.text.startswith( "pvr-channel://" ):
                # PVR action
                onclickelement.text = "RunScript(script.skinshortcuts,type=launchpvr&channel=" + onclick.text.replace( "pvr-channel://", "" ) + ")"
            elif onclick.text.startswith( "ActivateWindow(" ) and xbmc.translatePath( "special://skin/" ) in onclick.text:
                # Skin-relative links
                try:
                    actionParts = onclick.text[15:-1].split( "," )
                    actionParts[1] = actionParts[1].replace( xbmc.translatePath( "special://skin/" ), "" )
                    path = actionParts[1].split( os.sep )
                    newAction = "special://skin"
                    for actionPart in actionParts[1].split( os.sep ):
                        if actionPart != "":
                            newAction = newAction + "/" + actionPart
                    if len( actionParts ) == 2:
                        onclickelement.text = "ActivateWindow(" + actionParts[0] + "," + newAction + ")"
                    else:
                        onclickelement.text = "ActivateWindow(" + actionParts[0] + "," + newAction + "," + actionParts[2] + ")"
                except:
                    pass
            else:
                onclickelement.text = onclick.text
                
            # Also add it as a path property
            if not self.propertyExists( "path", newelement ):
                # we only add the path property if there isn't already one in the list because it has to be unique in Kodi lists
                pathelement = xmltree.SubElement( newelement, "property" )
                pathelement.set( "name", "path" )
                pathelement.text = onclickelement.text
            
            # Get 'list' property (the action property of an ActivateWindow shortcut)
            if not self.propertyExists( "list", newelement ):
                # we only add the list property if there isn't already one in the list because it has to be unique in Kodi lists
                listElement = xmltree.SubElement( newelement, "property" )
                listElement.set( "name", "list" )
                listElement.text = DATA.getListProperty( onclickelement.text )
                
            if onclick.text == "ActivateWindow(Settings)":
                self.hasSettings = True
                
            if "condition" in onclick.attrib:
                onclickelement.set( "condition", onclick.attrib.get( "condition" ) )
                
            if len( self.checkForShortcuts ) != 0:
                # Check if we've been asked to watch for this shortcut
                newCheckForShortcuts = []
                for checkforShortcut in self.checkForShortcuts:
                    if onclick.text.lower() == checkforShortcut[ 0 ]:
                        # They match, change the value to True
                        newCheckForShortcuts.append( ( checkforShortcut[ 0 ], checkforShortcut[ 1 ], "True" ) )
                    else:
                        newCheckForShortcuts.append( checkforShortcut )
                self.checkForShortcuts = newCheckForShortcuts

        # Visibility
        if visibilityCondition is not None:
            visibilityElement = xmltree.SubElement( newelement, "visible" )
            if profileVisibility is not None:
                visibilityElement.text = profileVisibility + " + [" + visibilityCondition + "]"
            else:
                visibilityElement.text = visibilityCondition
            issubmenuElement = xmltree.SubElement( newelement, "property" )
            issubmenuElement.set( "name", "isSubmenu" )
            issubmenuElement.text = "True"
        elif profileVisibility is not None:
            visibilityElement = xmltree.SubElement( newelement, "visible" )
            visibilityElement.text = profileVisibility
                
        # Submenu visibility
        if submenuVisibility is not None:
            submenuVisibilityElement = xmltree.SubElement( newelement, "property" )
            submenuVisibilityElement.set( "name", "submenuVisibility" )
            if submenuVisibility.isdigit():
                submenuVisibilityElement.text = "$NUMBER[" + submenuVisibility + "]"
            else:
                submenuVisibilityElement.text = DATA.slugify( submenuVisibility )
                
        # Group name
        group = xmltree.SubElement( newelement, "property" )
        group.set( "name", "group" )
        group.text = try_decode( groupName )

        if groupName == "mainmenu":
            self.MAINWIDGET = {}
            self.MAINBACKGROUND = {}
        
        # If this isn't the main menu, and we're cloning widgets or backgrounds...
        if groupName != "mainmenu":
            if "clonewidgets" in options and len( self.MAINWIDGET ) is not 0:
                for key in self.MAINWIDGET:
                    additionalproperty = xmltree.SubElement( newelement, "property" )
                    additionalproperty.set( "name", key )
                    additionalproperty.text = try_decode( self.MAINWIDGET[ key ] )
            if "clonebackgrounds" in options and len( self.MAINBACKGROUND ) is not 0:
                for key in self.MAINBACKGROUND:
                    additionalproperty = xmltree.SubElement( newelement, "property" )
                    additionalproperty.set( "name", key )
                    additionalproperty.text = DATA.local( self.MAINBACKGROUND[ key ] )[1]

        propertyPatterns = self.getPropertyPatterns(labelID.text, groupName)
        if len(propertyPatterns) > 0:
            propertyReplacements = self.getPropertyReplacements(newelement)
            for propertyName in propertyPatterns:
                propertyPattern = propertyPatterns[propertyName][0]
                for original, replacement in propertyReplacements:
                    regexpPattern = re.compile(re.escape(original), re.IGNORECASE)
                    propertyPattern = regexpPattern.sub(replacement, propertyPattern)
    
                additionalproperty = xmltree.SubElement(newelement, "property")
                additionalproperty.set("name", propertyName.decode("utf-8"))
                additionalproperty.text = propertyPattern.decode("utf-8")
            
        return newelement


    def getPropertyPatterns(self, labelID, group):
        overrides = DATA._get_overrides_skin()
        propertyPatterns = {}
        
        if overrides is not None:
            propertyPatternElements = overrides.getroot().findall("propertypattern")
            for propertyPatternElement in propertyPatternElements:
                propertyName = propertyPatternElement.get("property")
                propertyGroup = propertyPatternElement.get("group")
              
                if not propertyName or not propertyGroup or propertyGroup != group or not propertyPatternElement.text:
                    continue
                  
                propertyLabelID = propertyPatternElement.get("labelID")
                if not propertyLabelID:
                    if propertyName not in propertyPatterns:
                        propertyPatterns[propertyName] = [propertyPatternElement.text, False]
                elif propertyLabelID == labelID:
                    if propertyName not in propertyPatterns or propertyPatterns[propertyName][1] == False:
                        propertyPatterns[propertyName] = [propertyPatternElement.text, True]

        return propertyPatterns
    
        
    def getPropertyReplacements(self, element):
        propertyReplacements = []
        for subElement in list(element):
            if subElement.tag == "property":
                propertyName = subElement.get("name")
                if propertyName and subElement.text:
                    propertyReplacements.append(("::%s::" % propertyName, subElement.text))
            elif subElement.text:
                propertyReplacements.append(("::%s::" % subElement.tag, subElement.text))

        return propertyReplacements


    def propertyExists( self, propertyName, element ):
        for item in element.findall( "property" ):
            if propertyName in item.attrib:
                return True
        return False


      
    def findIncludePosition( self, list, item ):
        try:
            return list.index( item )
        except:
            return None
            
