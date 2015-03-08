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

import nodefunctions
NODE = nodefunctions.NodeFunctions()

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
    
class DataFunctions():
    def __init__(self):
        pass
        
    
    def _get_labelID( self, labelID, action, getDefaultID = False, includeAddOnID = True ):
        # This gets the unique labelID for the item we've been passed. We'll also store it, to make sure
        # we don't give it to any other item.
        
        labelID = self.createNiceName( self.slugify( labelID.replace( " ", "" ).lower() ) )
        
        if includeAddOnID:
            addon_labelID = self._get_addon_labelID( action )
            if addon_labelID is not None:
                labelID = addon_labelID
        
        # If we're getting the defaultID, just return this
        if getDefaultID == True:
            return labelID
        
        # Check if the labelID exists in the list
        if labelID in self.labelIDList:
            # We're going to add an --[int] to the end of this
            count = 0
            while labelID + "--" + str( count ) in self.labelIDList:
                count += 1
            
            # We can now use this one
            self.labelIDList.append( labelID + "--" + str( count ) )
            return labelID + "--" + str( count )
        else:
            # We can use this one
            self.labelIDList.append( labelID )
            return labelID
            
    def _get_addon_labelID( self, action ):
        # This will check the action to see if this is a program or the root of a plugin and, if so, return that as the labelID
        
        if action is None:
            return None
        
        try:
            if action.startswith( "RunAddOn(" ) and "," not in action:
                return action[9:-1]
                
            if action.startswith( "RunScript(" ) and "," not in action:
                return action[10:-1]
                
            if "plugin://" in action and "?" not in action:
                # Return the action
                # - less ActivateWindow(
                # - The second group after being split by comma
                # - Less plugin://
                return action[15:-1].split( "," )[1].replace( '"', '' )[9:]
        except:
            return None
            
        return None
    
    def _clear_labelID( self ):
        # This clears our stored list of labelID's
        self.labelIDList = []
        
    
    def _pop_labelID( self ):
        self.labelIDList.pop()
    
                
    def _get_shortcuts( self, group, defaultGroup = None, isXML = False, profileDir = None, defaultsOnly = False ):
        # This will load the shortcut file
        # Additionally, if the override files haven't been loaded, we'll load them too
        log( "Loading shortcuts for group " + group )
                
        if profileDir is None:
            profileDir = xbmc.translatePath( "special://profile/" ).decode( "utf-8" )
        
        userShortcuts = os.path.join( profileDir, "addon_data", __addonid__, self.slugify( group ) + ".DATA.xml" )#.encode('utf-8')
        skinShortcuts = os.path.join( __skinpath__ , self.slugify( group ) + ".DATA.xml")#.encode('utf-8')
        defaultShortcuts = os.path.join( __defaultpath__ , self.slugify( group ) + ".DATA.xml" )#.encode('utf-8')
        if defaultGroup is not None:
            skinShortcuts = os.path.join( __skinpath__ , self.slugify( defaultGroup ) + ".DATA.xml")#.encode('utf-8')    
            defaultShortcuts = os.path.join( __defaultpath__ , self.slugify( defaultGroup ) + ".DATA.xml" )#.encode('utf-8')

        if defaultsOnly:
            paths = [skinShortcuts, defaultShortcuts ]
        else:
            paths = [userShortcuts, skinShortcuts, defaultShortcuts ]
        
        for path in paths:
            try:
                path = path.decode( "utf-8" )
            except:
                pass
                
            tree = None
            altPath = path.replace( ".DATA.xml", ".shortcuts" )
            if xbmcvfs.exists( path ):
                file = xbmcvfs.File( path ).read()
                self._save_hash( path, file )
                tree = xmltree.parse( path )
            elif xbmcvfs.exists( altPath ):
                file = xbmcvfs.File( altPath ).read()
                self._save_hash( altPath, file )
                tree = UpgradeFunctions().upgrade_xmlfile( altPath, mixedVersion = True, saveFile = False )
                    
            if tree is not None:
                # If this is a user-selected list of shortcuts...
                if path == userShortcuts:
                    if group == "mainmenu":
                        self._get_skin_required( tree, group, profileDir )
                    # Process shortcuts, marked as user-selected                    
                    self._process_shortcuts( tree, group, profileDir, True )
                    
                else:
                    if group == "mainmenu":
                        self._get_skin_required( tree, group, profileDir )
                    self._process_shortcuts( tree, group, profileDir )
                                        
                log( " - Loaded file " + path ) 
                return tree
            else:
                self._save_hash( path, None )
                
        # No file loaded
        log( " - No shortcuts" )
        return xmltree.ElementTree( xmltree.Element( "shortcuts" ) )
                            
    def _process_shortcuts( self, tree, group, profileDir = "special:\\profile", isUserShortcuts = False, allowAdditionalRequired = True ):
        # This function will process any overrides and add them to the tree ready to be displayed
        #  - We will process graphics overrides, action overrides, visibility conditions
        skinoverrides = self._get_overrides_skin()
        useroverrides = self._get_overrides_user( profileDir )
        
        self._clear_labelID()
        
        # Iterate through all <shortcut/> nodes
        for node in tree.getroot().findall( "shortcut" ):
            # If not user shortcuts, remove locked and defaultid nodes (in case of naughty skinners!)
            if isUserShortcuts == False:
                searchNode = node.find( "locked" )
                if searchNode is not None:
                    node.remove( searchNode )
                    
            # Remove any labelID node (because it confuses us!)
            searchNode = node.find( "labelID" )
            if searchNode is not None:
                node.remove( searchNode )
                    
            # Get the action
            action = node.find( "action" )
            
            # Generate the labelID
            labelID = self._get_labelID( self.local( node.find( "label" ).text )[3].replace( " ", "" ).lower(), action.text )
            xmltree.SubElement( node, "labelID" ).text = labelID
            
            # If there's no defaultID, set it to the labelID
            defaultID = node.find( "defaultID" )
            if defaultID == None:
                xmltree.SubElement( node, "defaultID" ).text = labelID
            else:
                defaultID = defaultID.text
            
            # Check that any version node matches current XBMC version
            version = node.find( "version" )
            if version is not None:
                if __xbmcversion__ != version.text:
                    tree.getroot().remove( node )
                    self._pop_labelID()
                    continue
                    
            # Check that any skin-required shortcut matches current skin
            xmltree.SubElement( node, "additional-properties" ).text = repr( self.checkAdditionalProperties( group, labelID, defaultID, isUserShortcuts ) )
                        
            # Get a skin-overriden icon
            overridenIcon = self._get_icon_overrides( skinoverrides, node.find( "icon" ).text, group, labelID )
            if overridenIcon is not None:
                # Add a new node with the overriden icon
                xmltree.SubElement( node, "override-icon" ).text = overridenIcon
            
            # If the action uses the special://skin protocol, translate it
            if "special://skin/" in action.text:
                action.text = xbmc.translatePath( action.text )
                
            # Get visibility condition
            visibilityCondition = self.checkVisibility( action.text )
            visibilityNode = None
            if visibilityCondition != "":
                visibilityNode = xmltree.SubElement( node, "visibility" )
                visibilityNode.text = visibilityCondition
            
            # Get action and visibility overrides
            overrideTrees = [useroverrides, skinoverrides]
            hasOverriden = False
            for overrideTree in overrideTrees:
                if tree is not None:
                    if hasOverriden == True:
                        continue
                    if overrideTree is not None:
                        for elem in overrideTree.findall( "override" ):
                            # Retrieve group property
                            checkGroup = None
                            if "group" in elem.attrib:
                                checkGroup = elem.attrib.get( "group" )
                                
                            # If the action and (if provided) the group match...
                            if elem.attrib.get( "action" ) == action.text and (checkGroup == None or checkGroup == group):
                                # Check the XBMC version matches
                                if "version" in elem.attrib:
                                    if elem.attrib.get( "version" ) != __xbmcversion__:
                                        continue
                                    
                                hasOverriden = True
                                # Get the visibility condition
                                condition = elem.find( "condition" )
                                overrideVisibility = None
                                if condition is not None:
                                    overrideVisibility = condition.text
                                
                                # Get the new action
                                for actions in elem.findall( "action" ):
                                    newaction = xmltree.SubElement( node, "override-action" )
                                    newaction.text = actions.text
                                    if overrideVisibility is not None:
                                        newaction.set( "condition", overrideVisibility )
                                        
                                # If there's no action, and there is a visibility condition
                                if len( elem.findall( "action" ) ) == 0:
                                    newaction = xmltree.SubElement( node, "override-action" )
                                    newaction.text = action.text
                                    if overrideVisibility is not None:
                                        newaction.set( "condition", overrideVisibility )
                                    
            # Get visibility condition of any skin-provided shortcuts
            if hasOverriden == False and skinoverrides is not None:
                for elem in skinoverrides.findall( "shortcut" ):
                    if elem.text == action.text and "condition" in elem.attrib:
                        if visibilityNode == None:
                            xmltree.SubElement( node, "visibility" ).text = elem.attrib.get( "condition" )
                        else:
                            visibilityNode.text = "[" + visibilityNode.text + "] + [" + elem.attrib.get( "condition" ) + "]"
                            
            # Get any visibility conditions in the .DATA.xml file
            if hasOverriden == False:
                additionalVisibility = node.find( "visible" )
                if additionalVisibility is not None:
                    if visibilityNode == None:
                        xmltree.SubElement( node, "visibility" ).text = additionalVisibility.text
                    else:
                        visibilityNode.text = "[" + visibilityNode.text + "] + [" + additionalVisibility.text + "]"
        
        return tree
        
    def _get_skin_required( self, listitems, group, profileDir ):
        # This function builds a tree of any skin-required shortcuts not currently in the menu
        # Once the tree is built, it sends them to _process_shortcuts for any overrides, etc, then adds them to the menu tree
        
        tree = self._get_overrides_skin()
        if tree is None:
            return
            
        # Get an array of all actions currently in the menu
        actions = []
        for node in listitems.getroot().findall( "shortcut" ):
            for action in node.findall( "action" ):
                actions.append( action.text )
                
        # Get a list of all skin-required shortcuts
        requiredShortcuts = []
        for elem in tree.findall( "requiredshortcut" ):
            if not elem.text in actions:
                # We need to add this shortcut - add it to the listitems
                requiredShortcut = xmltree.SubElement( listitems.getroot(), "shortcut" )
                
                # Label and label2
                xmltree.SubElement( requiredShortcut, "label" ).text = elem.attrib.get( "label" )
                xmltree.SubElement( requiredShortcut, "label2" ).text = xbmc.getSkinDir()
                
                # Icon and thumbnail
                if "icon" in elem.attrib:
                    xmltree.SubElement( requiredShortcut, "icon" ).text = elem.attrib.get( "icon" )
                else:
                    xmltree.SubElement( requiredShortcut, "icon" ).text = "DefaultShortcut.png"
                if "thumb" in elem.attrib:
                    xmltree.SubElement( requiredShortcut, "thumb" ).text = elem.attrib.get( "thumbnail" )
                    
                # Action
                xmltree.SubElement( requiredShortcut, "action" ).text = elem.text
                
                # Locked
                # - This is set to the skin directory, so it will only be locked in the management directory when using this skin
                xmltree.SubElement( requiredShortcut, "lock" ).text = xbmc.getSkinDir()
                
                
    def _get_icon_overrides( self, tree, icon, group, labelID, setToDefault = True ):        
        # This function will get any icon overrides based on labelID or group
        if icon is None:
            return
            
        # If the icon is a VAR or an INFO, we aren't going to override
        if icon.startswith( "$" ):
            return icon
            
        oldicon = None
        newicon = icon
        
        # Check for overrides
        if tree is not None:
            for elem in tree.findall( "icon" ):
                if oldicon is None:
                    if ("labelID" in elem.attrib and elem.attrib.get( "labelID" ) == labelID) or ("image" in elem.attrib and elem.attrib.get( "image" ) == icon):
                        # LabelID matched
                        if "group" in elem.attrib:
                            if elem.attrib.get( "group" ) == group:
                                # Group also matches - change icon
                                oldicon = icon
                                newicon = elem.text
                                
                        elif "grouping" not in elem.attrib:
                            # No group - change icon
                            oldicon = icon
                            newicon = elem.text
        
        if not xbmc.skinHasImage( newicon ) and setToDefault == True:
            newicon = self._get_icon_overrides( tree, "DefaultShortcut.png", group, labelID, False )
        return newicon

        
    def _get_overrides_script( self ):
        # If we haven't already loaded skin overrides, or if the skin has changed, load the overrides file
        if not xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-overrides-script-data" ) or not xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-overrides-script" ) == __defaultpath__:
            xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-overrides-script", __defaultpath__ )
            overridepath = os.path.join( __defaultpath__ , "overrides.xml" )
            try:
                tree = xmltree.parse( overridepath )
                self._save_hash( overridepath, xbmcvfs.File( overridepath ).read() )
                xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-overrides-script-data", pickle.dumps( tree ) )
                return tree
            except:
                self._save_hash( overridepath, None )
                xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-overrides-script-data", "No overrides" )
                return None
   
        # Return the overrides
        returnData = xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-overrides-script-data" )
        if returnData == "No overrides":
            return None
        else:
            return pickle.loads( returnData )


    def _get_overrides_skin( self ):
        # If we haven't already loaded skin overrides, or if the skin has changed, load the overrides file
        if not xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-overrides-skin-data" ) or not xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-overrides-skin" ) == __skinpath__:
            xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-overrides-skin", __skinpath__ )
            overridepath = os.path.join( __skinpath__ , "overrides.xml" )
            try:
                tree = xmltree.parse( overridepath )
                self._save_hash( overridepath, xbmcvfs.File( overridepath ).read() )
                xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-overrides-skin-data", pickle.dumps( tree ) )
                return tree
            except:
                self._save_hash( overridepath, None )
                xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-overrides-skin-data", "No overrides" )
                return None
   
        # Return the overrides
        returnData = xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-overrides-skin-data" )
        if returnData == "No overrides":
            return None
        else:
            return pickle.loads( returnData )


    def _get_overrides_user( self, profileDir = "special://profile" ):
        # If we haven't already loaded user overrides
        profileDir = profileDir.encode( "utf-8" )
        if not xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-overrides-user-data" + profileDir ) or not xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-overrides-user" + profileDir ) == __profilepath__:
            xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-overrides-user" + profileDir, profileDir )
            overridepath = os.path.join( profileDir , "overrides.xml" )
            try:
                tree = xmltree.parse( overridepath )
                self._save_hash( overridepath, xbmcvfs.File( overridepath ).read() )
                xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-overrides-user-data" + profileDir, pickle.dumps( tree ) )
                return tree
            except:
                self._save_hash( overridepath, None )
                xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-overrides-user-data" + profileDir, "No overrides" )
                return None
                
        # Return the overrides
        returnData = xbmcgui.Window( 10000 ).getProperty( "skinshortcuts-overrides-user-data" + profileDir )
        if returnData == "No overrides":
            return None
        else:
            return pickle.loads( returnData )


    def _get_additionalproperties( self ):
        # Load all saved properties (widgets, backgrounds, custom properties)
            
        currentProperties = []
        defaultProperties = []
        
        path = os.path.join( __datapath__ , xbmc.getSkinDir().decode('utf-8') + ".properties" )
        if xbmcvfs.exists( path ):
            # The properties file exists, load from it
            try:
                file = xbmcvfs.File( path ).read()
                listProperties = eval( file )
                self._save_hash( path, file )
                
                for listProperty in listProperties:
                    # listProperty[0] = groupname
                    # listProperty[1] = labelID
                    # listProperty[2] = property name
                    # listProperty[3] = property value
                    currentProperties.append( [listProperty[0], listProperty[1], listProperty[2], listProperty[3]] )
            except:
                pass
            
        # Load skin defaults (in case we need them...)
        tree = self._get_overrides_skin()
        if tree is not None:
            for elemSearch in [["widget", tree.findall( "widgetdefault" )], ["background", tree.findall( "backgrounddefault" )], ["custom", tree.findall( "propertydefault" )] ]:
                for elem in elemSearch[1]:
                    if elemSearch[0] == "custom":
                        # Custom property
                        if "group" not in elem.attrib:
                            defaultProperties.append( ["mainmenu", elem.attrib.get( 'labelID' ), elem.attrib.get( 'property' ), elem.text, elem.attrib.get( 'defaultID' ) ] )
                        else:
                            defaultProperties.append( [elem.attrib.get( "group" ), elem.attrib.get( 'labelID' ), elem.attrib.get( 'property' ), elem.text, elem.attrib.get( 'defaultID' ) ] )
                    else:
                        # Widget or background
                        if "group" not in elem.attrib:
                            defaultProperties.append( [ "mainmenu", elem.attrib.get( 'labelID' ), elemSearch[0], elem.text, elem.attrib.get( 'defaultID' ) ] )
                            
                            if elemSearch[ 0 ] == "background":
                                # Get and set the background name
                                backgroundName = self._getBackgroundName( elem.text )
                                if backgroundName is not None:
                                    defaultProperties.append( [ "mainmenu", elem.attrib.get( "labelID" ), "backgroundName", backgroundName, elem.attrib.get( 'defaultID' ) ] )
                                
                            if elemSearch[0] == "widget":
                                # Get and set widget type and name
                                widgetDetails = self._getWidgetNameAndType( elem.text )
                                if widgetDetails is not None:
                                    defaultProperties.append( [ "mainmenu", elem.attrib.get( "labelID" ), "widgetName", widgetDetails[0], elem.attrib.get( 'defaultID' ) ] )
                                    if widgetDetails[1] is not None:
                                        defaultProperties.append( [ "mainmenu", elem.attrib.get( "labelID" ), "widgetType", widgetDetails[1], elem.attrib.get( 'defaultID' ) ] )
                        else:
                            defaultProperties.append( [ elem.attrib.get( "group" ), elem.attrib.get( 'labelID' ), elemSearch[0], elem.text, elem.attrib.get( 'defaultID' ) ] )
                            
                            if elemSearch[ 0 ] == "background":
                                # Get and set the background name
                                backgroundName = self._getBackgroundName( elem.text )
                                if backgroundName is not None:
                                    defaultProperties.append( [ "mainmenu", elem.attrib.get( "labelID" ), "backgroundName", backgroundName, elem.attrib.get( 'defaultID' ) ] )
                            
                            if elemSearch[0] == "widget":
                                # Get and set widget type and name
                                widgetDetails = self._getWidgetNameAndType( elem.text )
                                if widgetDetails is not None:
                                    defaultProperties.append( [ elem.attrib.get( "group" ), elem.attrib.get( "labelID" ), "widgetName", widgetDetails[0], elem.attrib.get( 'defaultID' ) ] )
                                    if widgetDetails[1] is not None:
                                        defaultProperties.append( [ elem.attrib.get( "group" ), elem.attrib.get( "labelID" ), "widgetType", widgetDetails[1], elem.attrib.get( 'defaultID' ) ] )                
                                        
        returnVal = [currentProperties, defaultProperties]
        return returnVal
        
    def _getWidgetNameAndType( self, widgetID ):
        tree = self._get_overrides_skin()
        if tree is not None:
            for elem in tree.findall( "widget" ):
                if elem.text == widgetID:
                    if "type" in elem.attrib:
                        return [elem.attrib.get( "label" ), elem.attrib.get( "type" )]
                    else:
                        return [ elem.attrib.get( "label" ), None ]
                        
        return None
        
    def _getBackgroundName( self, backgroundID ):
        tree = self._get_overrides_skin()
        if tree is not None:
            for elem in tree.findall( "background" ):
                if elem.text == backgroundID:
                    return elem.attrib.get( "label" )
                        
        return None
                
    def _reset_backgroundandwidgets( self ):
        # This function resets all skin properties used to identify if specific backgrounds or widgets are active
        tree = self._get_overrides_skin()
        if tree is not None:
            for elem in tree.findall( "widget" ):
                xbmc.executebuiltin( "Skin.Reset(skinshortcuts-widget-" + elem.text + ")" )
            for elem in tree.findall( "background" ):
                xbmc.executebuiltin( "Skin.Reset(skinshortcuts-background-" + elem.text + ")" )
                
    
    def createNiceName ( self, item ):
        # Translate certain localized strings into non-localized form for labelID
        if item == "10006":
            return "videos"
        if item == "342":
            return "movies"
        if item == "20343":
            return "tvshows"
        if item == "32022":
            return "livetv"
        if item == "10005":
            return "music"
        if item == "20389":
            return "musicvideos"
        if item == "10002":
            return "pictures"
        if item == "12600":
            return "weather"
        if item == "10001":
            return "programs"
        if item == "32032":
            return "dvd"
        if item == "10004":
            return "settings"
        if item == "32087":
            return "radio"
        else:
            return item.lower( ).replace( " ", "" )
            
    def checkVisibility ( self, action ):
        action = action.lower().replace( " ", "" )
        
        # Return whether mainmenu items should be displayed
        if action == "activatewindow(weather)":
            return "!IsEmpty(Weather.Plugin)"
        elif action.startswith( "activatewindowandfocus(mypvr" ) or action.startswith( "playpvr" ):
            return "system.getbool(pvrmanager.enabled)"
        elif action.startswith( "activatewindow(videos,movie" ):
            return "Library.HasContent(Movies)"
        elif action.startswith( "activatewindow(videos,recentlyaddedmovies" ):
            return "Library.HasContent(Movies)"
        elif action.startswith( "activatewindow(videos,tvshow" ) or action.startswith( "activatewindow(videos,tvshow" ):
            return "Library.HasContent(TVShows)"
        elif action.startswith( "activatewindow(videos,recentlyaddedepisodes" ):
            return "Library.HasContent(TVShows)"
        elif action.startswith( "activatewindow(videos,musicvideo" ):
            return "Library.HasContent(MusicVideos)"
        elif action.startswith( "activatewindow(musiclibrary,musicvideo" ):
            return "Library.HasContent(MusicVideos)"
        elif action.startswith( "activatewindow(videos,recentlyaddedmusicvideos" ):
            return "Library.HasContent(MusicVideos)"
        elif action.startswith( "activatewindow(musiclibrary," ):
            return "Library.HasContent(Music)"
        elif action == "xbmc.playdvd()":
            return "System.HasMediaDVD"
            
        # Power menu visibilities
        elif action == "quit()" or action == "quit":
            return "System.ShowExitButton"
        elif action == "powerdown()" or action == "powerdown":
            return "System.CanPowerDown"
        elif action == "alarmclock(shutdowntimer,shutdown())":
            return "!System.HasAlarm(shutdowntimer) + [System.CanPowerDown | System.CanSuspend | System.CanHibernate]"
        elif action == "cancelalarm(shutdowntimer)":
            return "System.HasAlarm(shutdowntimer)"
        elif action == "suspend()" or action == "suspend":
            return "System.CanSuspend"
        elif action == "hibernate()" or action == "hibernate":
            return "System.CanHibernate"
        elif action == "reset()" or action == "reset":
            return "System.CanReboot"
        elif action == "system.logoff":
            return "[System.HasLoginScreen | IntegerGreaterThan(System.ProfileCount,1)] + System.Loggedon"
        elif action == "mastermode":
            return "System.HasLocks"
        elif action == "inhibitidleshutdown(true)":
            return "System.HasShutdown +!System.IsInhibit"
        elif action == "inhibitidleshutdown(false)":
            return "System.HasShutdown + System.IsInhibit"
            
        # New Helix visibility conditions
        elif action.startswith( "activatewindow(tv" ):
            return "PVR.HasTVChannels"
        elif action.startswith( "activatewindow(radio" ):
            return "PVR.HasRadioChannels"
            
        # Video node visibility
        elif action.startswith( "activatewindow(videos,videodb://" ) or action.startswith( "activatewindow(10025,videodb://" ) or action.startswith( "activatewindow(Videos,library://video/" ) or action.startswith( "activatewindow(10025,library://video/" ):
            path = action.split( "," )
            if path[ 1 ].endswith( ")" ):
                path[ 1 ] = path[ 1 ][:-1]
            return NODE.get_visibility( path[ 1 ] )
            
        return ""
        
        
    def checkAdditionalProperties( self, group, labelID, defaultID, isUserShortcuts ):
        # Return any additional properties, including widgets and backgrounds
        allProperties = self._get_additionalproperties()
        currentProperties = allProperties[1]
        
        returnProperties = []
        
        # This returns two lists...
        #  allProperties[0] = Saved properties
        #  allProperties[1] = Default properties
        
        if isUserShortcuts:
            currentProperties = allProperties[0]
            
        # Loop through the current properties, looking for the current item
        for currentProperty in currentProperties:
            # currentProperty[0] = Group name
            # currentProperty[1] = labelID
            # currentProperty[2] = Property name
            # currentProperty[3] = Property value
            # currentProperty[4] = defaultID
            if labelID is not None and currentProperty[0] == group and currentProperty[1] == labelID:
                returnProperties.append( [ currentProperty[2], currentProperty[3] ] )
            if len( currentProperty ) is not 4:
                if defaultID is not None and currentProperty[0] == group and currentProperty[4] == defaultID:
                    returnProperties.append( [ currentProperty[2], currentProperty[3] ] )
                
        return returnProperties
            
        
    def checkShortcutLabelOverride( self, action ):
        tree = self._get_overrides_skin()
        if tree is not None:
            elemSearch = tree.findall( "availableshortcutlabel" )
            for elem in elemSearch:
                if elem.attrib.get( "action" ).lower() == action.lower():
                    # This matches :) Check if we're also overriding the type
                    if "type" in elem.attrib:
                        return [ elem.text, elem.attrib.get( "type" ) ]
                    else:
                        return [ elem.text ]

        return None
        
        
    def _save_hash( self, filename, file ):
        if file is not None:
            hasher = hashlib.md5()
            hasher.update( file )
            hashlist.list.append( [filename, hasher.hexdigest()] )
        else:
            hashlist.list.append( [filename, None] )
            
            
    # in-place prettyprint formatter
    def indent( self, elem, level=0 ):
        i = "\n" + level*"\t"
        if len(elem):
            if not elem.text or not elem.text.strip():
                elem.text = i + "\t"
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
            for elem in elem:
                self.indent(elem, level+1)
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
        else:
            if level and (not elem.tail or not elem.tail.strip()):
                elem.tail = i
                
                
    def local( self, data ):
        # This is our function to manage localisation
        # It accepts strings in one of the following formats:
        #   #####, ::LOCAL::#####, ::SCRIPT::#####
        #   $LOCALISE[#####], $SKIN[####|skin.id|last translation]
        #   $ADDON[script.skinshortcuts #####]
        # If returns a list containing:
        #   [Number/$SKIN, $LOCALIZE/$ADDON/Local string, Local string]
        #   [Used for saving, used for building xml, used for displaying in dialog]
        
        if data is None:
            return ["","","",""]
        
        try:
            data = data.decode( "utf-8" )
        except:
            pass

        skinid = None
        lasttranslation = None
        
        # Get just the integer of the string, for the input forms where this is valid
        if not data.find( "::SCRIPT::" ) == -1:
            data = data[10:]
        elif not data.find( "::LOCAL::" ) == -1:            
            data = data[9:]
        elif not data.find( "$LOCALIZE[" ) == -1:
            data = data.replace( "$LOCALIZE[", "" ).replace( "]", "" ).replace( " ", "" )
        elif not data.find( "$ADDON[script.skinshortcuts" ) == -1:
            data = data.replace( "$ADDON[script.skinshortcuts", "" ).replace( "]", "" ).replace( " ", "" )
        
        # Get the integer and skin id, from $SKIN input forms
        elif not data.find( "$SKIN[" ) == -1:
            splitdata = data[6:-1].split( "|" )
            data = splitdata[0]
            skinid = splitdata[1]
            lasttranslation = splitdata[2]
            
        if data.isdigit():
            if int( data ) >= 31000 and int( data ) < 32000:
                # A number from a skin - we're going to return a $SKIN[#####|skin.id|last translation] unit
                if skinid is None:
                    # Set the skinid to the current skin id
                    skinid = xbmc.getSkinDir()
                    
                # If we're on the same skin as the skinid, get the latest translation
                if skinid == xbmc.getSkinDir():
                    lasttranslation = xbmc.getLocalizedString( int( data ) )
                    returnString = "$SKIN[" + data + "|" + skinid + "|" + lasttranslation + "]"
                    return [ returnString, "$LOCALIZE[" + data + "]", lasttranslation, data ]
                    
                returnString = "$SKIN[" + data + "|" + skinid + "|" + lasttranslation + "]"
                return [ returnString, lasttranslation, lasttranslation, data ]
                
            elif int( data ) >= 32000 and int( data ) < 33000:
                # A number from the script
                return [ data, "$ADDON[script.skinshortcuts " + data + "]", __language__( int( data ) ), data ]
                
            else:
                # A number from XBMC itself (probably)
                return [ data, "$LOCALIZE[" + data + "]", xbmc.getLocalizedString( int( data ) ), data ]
                
        # This isn't anything we can localize, just return it (in triplicate ;))
        return[ data, data, data, data ]
    def smart_truncate(string, max_length=0, word_boundaries=False, separator=' '):
        string = string.strip(separator)

        if not max_length:
            return string

        if len(string) < max_length:
            return string

        if not word_boundaries:
            return string[:max_length].strip(separator)

        if separator not in string:
            return string[:max_length]

        truncated = ''
        for word in string.split(separator):
            if word:
                next_len = len(truncated) + len(word) + len(separator)
                if next_len <= max_length:
                    truncated += '{0}{1}'.format(word, separator)
        if not truncated:
            truncated = string[:max_length]
        return truncated.strip(separator)

    def slugify(self, text, entities=True, decimal=True, hexadecimal=True, max_length=0, word_boundary=False, separator='-', convertInteger=False):
        # Handle integers
        if convertInteger and text.isdigit():
            text = "NUM-" + text
    
        # text to unicode
        if type(text) != types.UnicodeType:
            text = unicode(text, 'utf-8', 'ignore')

        # decode unicode ( ??? = Ying Shi Ma)
        text = unidecode(text)

        # text back to unicode
        if type(text) != types.UnicodeType:
            text = unicode(text, 'utf-8', 'ignore')

        # character entity reference
        if entities:
            text = CHAR_ENTITY_REXP.sub(lambda m: unichr(name2codepoint[m.group(1)]), text)

        # decimal character reference
        if decimal:
            try:
                text = DECIMAL_REXP.sub(lambda m: unichr(int(m.group(1))), text)
            except:
                pass

        # hexadecimal character reference
        if hexadecimal:
            try:
                text = HEX_REXP.sub(lambda m: unichr(int(m.group(1), 16)), text)
            except:
                pass

        # translate
        text = unicodedata.normalize('NFKD', text)
        if sys.version_info < (3,):
            text = text.encode('ascii', 'ignore')

        # replace unwanted characters
        text = REPLACE1_REXP.sub('', text.lower()) # replace ' with nothing instead with -
        text = REPLACE2_REXP.sub('-', text.lower())

        # remove redundant -
        text = REMOVE_REXP.sub('-', text).strip('-')

        # smart truncate if requested
        if max_length > 0:
            text = smart_truncate(text, max_length, word_boundary, '-')

        if separator != '-':
            text = text.replace('-', separator)

        return text
        
class UpgradeFunctions():
    def __init__(self):
        pass
    
    def upgrade_labels( self ):
        # This function will upgrade all the saved labels (label and label2) for the .shortcuts files
        # For localised strings, this will be either to just numeric, or for skin strings, to $SKIN[number|skinID|LastTranslation]
        # For non-localised strings, nothing will happen
        
        # Get all profiles
        profile_file = xbmc.translatePath( 'special://userdata/profiles.xml' ).decode("utf-8")
        tree = None
        if xbmcvfs.exists( profile_file ):
            tree = xmltree.parse( profile_file )
        
        profilelist = []
        if tree is not None:
            profiles = tree.findall( "profile" )
            for profile in profiles:
                name = profile.find( "name" ).text.encode( "utf-8" )
                dir = profile.find( "directory" ).text.encode( "utf-8" )
                # Localise the directory
                if "://" in dir:
                    dir = xbmc.translatePath( os.path.join( dir, "addon_data", "script.skinshortcuts" ) ).decode( "utf-8" )
                else:
                    # Base if off of the master profile
                    dir = xbmc.translatePath( os.path.join( "special://masterprofile", dir, "addon_data", "script.skinshortcuts" ) ).decode( "utf-8" )
                profilelist.append( dir )
                
        else:
            profilelist = [xbmc.translatePath( "special://masterprofile/addon_data/script.skinshortcuts" )]
            
        for folder in profilelist:
            for root, subdirs, files in os.walk( folder ):
                for file in files:
                    if file.endswith( ".shortcuts" ):
                        self.upgrade_file( os.path.join( folder, file ) )
                break
        
    def upgrade_file( self, path ):
        list = xbmcvfs.File( path ).read()
        shortcuts = eval( list )
        
        # Save the original file as a backup
        f = xbmcvfs.File( path + ".backup", 'w' )
        f.write( repr( shortcuts ).replace( "],", "],\n" ) )
        f.close()
        
        for shortcut in shortcuts:
            # Save the original file as a backup
            
            # Upgrade label1 and label2
            shortcut[0] = DataFunctions().local( shortcut[0] )[0]
            shortcut[1] = DataFunctions().local( shortcut[1] )[0]
            
            # If there is a skinID, ensure this is set in label1 and label2
            if len( shortcut ) is not 5:
                skinID = shortcut[5]
                if not shortcut[0].find( "$SKIN[" ) == -1:
                    splitdata = shortcut[0][6:-1].split( "|" )
                    stringid = splitdata[0]
                    lasttranslation = splitdata[2]
                    shortcut[0] = "$SKIN[" + stringid + "|" + skinID + "|" + lasttranslation + "]"
                if not shortcut[1].find( "$SKIN[" ) == -1:
                    splitdata = shortcut[1][6:-1].split( "|" )
                    stringid = splitdata[0]
                    lasttranslation = splitdata[2]
                    shortcut[1] = "$SKIN[" + stringid + "|" + skinID + "|" + lasttranslation + "]"
                    
                shortcut = [shortcut[0], shortcut[1], shortcut[2], shortcut[3], shortcut[4]]
                
        # Save the file
        f = xbmcvfs.File( path, 'w' )
        f.write( repr( shortcuts ).replace( "],", "],\n" ) )
        f.close()
        
    def upgrade_toxml( self ):
        # This function will upgrade all user .shortcuts files (already upgraded to the new labels) to the new xml format
        
        # Get all profiles
        profile_file = xbmc.translatePath( 'special://userdata/profiles.xml' ).decode("utf-8")
        tree = None
        if xbmcvfs.exists( profile_file ):
            tree = xmltree.parse( profile_file )
        
        profilelist = []
        if tree is not None:
            profiles = tree.findall( "profile" )
            for profile in profiles:
                name = profile.find( "name" ).text.encode( "utf-8" )
                dir = profile.find( "directory" ).text.encode( "utf-8" )
                # Localise the directory
                if "://" in dir:
                    dir = xbmc.translatePath( os.path.join( dir, "addon_data", "script.skinshortcuts" ) ).decode( "utf-8" )
                else:
                    # Base if off of the master profile
                    dir = xbmc.translatePath( os.path.join( "special://masterprofile", dir, "addon_data", "script.skinshortcuts" ) ).decode( "utf-8" )
                profilelist.append( dir )
                
        else:
            profilelist = [xbmc.translatePath( "special://masterprofile/addon_data/script.skinshortcuts" )]
            
        for folder in profilelist:
            for root, subdirs, files in os.walk( folder ):
                for file in files:
                    if file.endswith( ".shortcuts" ):
                        self.upgrade_xmlfile( os.path.join( folder, file ) )
                break
    
    def upgrade_xmlfile( self, path, mixedVersion = False, saveFile = True ):
        list = xbmcvfs.File( path ).read()
        shortcuts = eval( list )
        
        # Create the new tree
        tree = xmltree.ElementTree( xmltree.Element( "shortcuts" ) )
        root = tree.getroot()
        
        DataFunctions()._clear_labelID()
        
        for shortcut in shortcuts:
            # Create the element
            actionTree = xmltree.SubElement( root, "shortcut" )
            
            # Action
            try:
                action = urllib.unquote( shortcut[4] ).decode( "utf-8" )
            except:
                action = urllib.unquote( shortcut[4] )
            xmltree.SubElement( actionTree, "action" ).text = action
            
            # Label and label2, defaultID
            xmltree.SubElement( actionTree, "label" ).text = DataFunctions().local( shortcut[0] )[0]
            xmltree.SubElement( actionTree, "label2" ).text = DataFunctions().local( shortcut[1] )[0]
            xmltree.SubElement( actionTree, "defaultID" ).text = DataFunctions()._get_labelID( DataFunctions().local( shortcut[0] )[3], action, True )
            
            # Icon and thumbnail
            try:
                xmltree.SubElement( actionTree, "icon" ).text = shortcut[2].decode( "utf-8" )
            except:
                xmltree.SubElement( actionTree, "icon" ).text = shortcut[2] 
                
            try:
                xmltree.SubElement( actionTree, "thumb" ).text = shortcut[3].decode( "utf-8" )
            except:
                xmltree.SubElement( actionTree, "thumb" ).text = shortcut[3]
            
            # mixedVersion will be True if we're upgrading a skin's defaults
            if mixedVersion == True:
                # If this is a PVR item, we'll mark the element we've just created as being for version 13,
                # and create a new element for the new Helix TV links
                newAction = None
                if action == "ActivateWindow(MyPVR)":
                    newAction = "ActivateWindow(TVGuide)"
                if action == "ActivateWindowAndFocus(MyPVR,32,0 ,11,0)":
                    newAction = "ActivateWindow(TVChannels)"
                if action == "ActivateWindowAndFocus(MyPVR,33,0 ,12,0)":
                    newAction = "REMOVE"
                if action == "ActivateWindowAndFocus(MyPVR,31,0 ,10,0)":
                    newAction = "ActivateWindow(TVGuide)"
                if action == "ActivateWindowAndFocus(MyPVR,34,0 ,13,0)":
                    newAction = "ActivateWindow(TVRecordings)"
                if action == "ActivateWindowAndFocus(MyPVR,35,0 ,14,0)":
                    newAction = "ActivateWindow(TVTimers)"
                    
                if newAction is not None:
                    xmltree.SubElement( actionTree, "version" ).text = "13"
                    
                    if newAction != "REMOVE":
                        # Create the element
                        newactionTree = xmltree.SubElement( root, "shortcut" )
                        
                        # Label and label2
                        xmltree.SubElement( newactionTree, "label" ).text = DataFunctions().local( shortcut[0] )[0]
                        xmltree.SubElement( newactionTree, "label2" ).text = DataFunctions().local( shortcut[1] )[0]
                        
                        # Icon and thumbnail
                        xmltree.SubElement( newactionTree, "icon" ).text = shortcut[2]
                        xmltree.SubElement( newactionTree, "thumb" ).text = shortcut[3]
                        
                        # Action
                        xmltree.SubElement( newactionTree, "action" ).text = newAction
                        
                        # Version
                        xmltree.SubElement( newactionTree, "version" ).text = "14"
                
        # Save the tree
        if saveFile == True:
            DataFunctions().indent( root )
            tree.write( path.replace( ".shortcuts", ".DATA.xml" ), encoding="UTF-8"  )
        else:
            return tree
        
        # Delete the .shortcuts file
        xbmcvfs.delete( path )
        
    def upgrade_addon_labelID( self, path = None ):
        # This function will upgrade the labelIDs of addons (and any other out of date labelIDs) to the new format
       
        if path is not None:
            profilelist = [path]
        else:
            # Get all profiles
            profile_file = xbmc.translatePath( 'special://userdata/profiles.xml' ).decode("utf-8")
            tree = None
            if xbmcvfs.exists( profile_file ):
                tree = xmltree.parse( profile_file )
            
            profilelist = []
            if tree is not None:
                profiles = tree.findall( "profile" )
                for profile in profiles:
                    name = profile.find( "name" ).text.encode( "utf-8" )
                    dir = profile.find( "directory" ).text.encode( "utf-8" )
                    # Localise the directory
                    if "://" in dir:
                        dir = xbmc.translatePath( os.path.join( dir, "addon_data", "script.skinshortcuts" ) ).decode( "utf-8" )
                    else:
                        # Base if off of the master profile
                        dir = xbmc.translatePath( os.path.join( "special://masterprofile", dir, "addon_data", "script.skinshortcuts" ) ).decode( "utf-8" )
                    profilelist.append( dir )
                    
            else:
                profilelist = [xbmc.translatePath( "special://masterprofile/addon_data/script.skinshortcuts" )]
            
        DATA = DataFunctions()

        for folder in profilelist:
            file = os.path.join( folder, "mainmenu.DATA.xml" )
            if xbmcvfs.exists( file ):
                DATA._clear_labelID()
                root = xmltree.parse( file ).getroot()
                
                oldLabelID = []
                newLabelID = []
                
                for shortcut in root.findall( "shortcut" ):
                    DATA.labelIDList = oldLabelID
                    oldLabel = DATA._get_labelID( shortcut.find( "label" ).text, shortcut.find( "action" ).text, includeAddOnID = False )
                    oldLabelID = DATA.labelIDList
                    
                    DATA.labelIDList = newLabelID
                    newLabel = DATA._get_labelID( shortcut.find( "label" ).text, shortcut.find( "action" ).text )
                    newLabelID = DATA.labelIDList
                    
                    if oldLabel != newLabel:
                        if xbmcvfs.exists( DATA.slugify( os.path.join( folder, oldLabel + ".DATA.xml" ) ) ):
                            xbmcvfs.rename( DATA.slugify( os.path.join( folder, oldLabel + ".DATA.xml" ) ), DATA.slugify( os.path.join( folder, newLabel + ".DATA.xml" ) ) )
                

    def upgrade_newtv( self ):
        # This function will upgrade the .DATA.xml files to the new pvr functions, add a new radio function
        # and add a new radio link to the mainmenu.DATA.xml file (if it exists) plus copy the skin/scripts default
        # radio.DATA.xml file to the users shortcut directory
        pass
