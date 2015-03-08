# coding=utf-8
import os, sys, datetime, unicodedata
import xbmc, xbmcgui, xbmcvfs#, urllib
import xml.etree.ElementTree as xmltree
from xml.dom.minidom import parse
from xml.sax.saxutils import escape as escapeXML
import thread
from traceback import print_exc
from unidecode import unidecode
import random

import datafunctions
DATA = datafunctions.DataFunctions()

import library
LIBRARY = library.LibraryFunctions()

if sys.version_info < (2, 7):
    import simplejson
else:
    import json as simplejson

__addon__        = sys.modules[ "__main__" ].__addon__
__addonid__      = sys.modules[ "__main__" ].__addonid__
__addonversion__ = sys.modules[ "__main__" ].__addonversion__
__cwd__          = __addon__.getAddonInfo('path').decode("utf-8")
__datapath__     = os.path.join( xbmc.translatePath( "special://profile/addon_data/" ).decode('utf-8'), __addonid__ )
__skinpath__     = xbmc.translatePath( "special://skin/shortcuts/" ).decode('utf-8')
__defaultpath__  = xbmc.translatePath( os.path.join( __cwd__, 'resources', 'shortcuts').encode("utf-8") ).decode("utf-8")
__language__     = __addon__.getLocalizedString
__cwd__          = sys.modules[ "__main__" ].__cwd__
__xbmcversion__  = xbmc.getInfoLabel( "System.BuildVersion" ).split(".")[0]

ACTION_CANCEL_DIALOG = ( 9, 10, 92, 216, 247, 257, 275, 61467, 61448, )

if not xbmcvfs.exists(__datapath__):
    xbmcvfs.mkdir(__datapath__)

def log(txt):
    if __xbmcversion__ == "13" or __addon__.getSetting( "enable_logging" ) == "true":
        try:
            if isinstance (txt,str):
                txt = txt.decode('utf-8')
            message = u'%s: %s' % (__addonid__, txt)
            xbmc.log(msg=message.encode('utf-8'), level=xbmc.LOGDEBUG)
        except:
            pass

class GUI( xbmcgui.WindowXMLDialog ):
    def __init__( self, *args, **kwargs ):
        self.group = kwargs[ "group" ]
        try:
            self.defaultGroup = kwargs[ "defaultGroup" ]
            if self.defaultGroup == "":
                self.defaultGroup = None
        except:
            self.defaultGroup = None
        self.nolabels = kwargs[ "nolabels" ]
        self.groupname = kwargs[ "groupname" ]
        self.shortcutgroup = 1
        
        # Empty arrays for different shortcut types
        self.backgroundBrowse = False
        self.backgroundBrowseDefault = None
        self.widgetPlaylists = False
        self.widgetPlaylistsType = None
        
        self.allListItems = []
        
        self.changeMade = False
        
        log( 'Management module loaded' )
        
    def onInit( self ):
        xbmcgui.Window( 10000 ).clearProperty( "skinshortcuts-overrides-script" )
        xbmcgui.Window( 10000 ).clearProperty( "skinshortcuts-overrides-script-data" )
        xbmcgui.Window( 10000 ).clearProperty( "skinshortcuts-overrides-skin" )
        xbmcgui.Window( 10000 ).clearProperty( "skinshortcuts-overrides-skin-data" )
        xbmcgui.Window( 10000 ).clearProperty( "skinshortcuts-overrides-user" )
        xbmcgui.Window( 10000 ).clearProperty( "skinshortcuts-overrides-user-data" )
        xbmcgui.Window( 10000 ).clearProperty( "skinshortcutsAdditionalProperties" )
        if self.group == '':
            self._close()
        else:
            self.window_id = xbmcgui.getCurrentWindowDialogId()
            xbmcgui.Window(self.window_id).setProperty('groupname', self.group)
            if self.groupname is not None:
                xbmcgui.Window( self.window_id ).setProperty( 'groupDisplayName', self.groupname )
            
            # Load widget and background names
            self._load_widgetsbackgrounds()

            # Load current shortcuts
            self.load_shortcuts()
                        
            # Set window title label
            try:
                if self.getControl( 500 ).getLabel() == "":
                    if self.group == "mainmenu":
                        self.getControl( 500 ).setLabel( __language__(32071) )
                    elif self.groupname is not None:
                        self.getControl( 500 ).setLabel( __language__(32080).replace( "::MENUNAME::", self.groupname ) )
                    else:
                        self.getControl( 500 ).setLabel( __language__(32072) )
            except:
                pass
                
            # Set enabled condition for various controls
            has111 = True
            try:
                self.getControl( 111 ).setEnableCondition( "IsEmpty(Container(211).ListItem.Property(LOCKED))" )
            except:
                has111 = False
            try:
                self.getControl( 302 ).setEnableCondition( "IsEmpty(Container(211).ListItem.Property(LOCKED))" )
            except:
                pass
            try:
                self.getControl( 307 ).setEnableCondition( "IsEmpty(Container(211).ListItem.Property(LOCKED))" )
            except:
                pass
            try:
                self.getControl( 401 ).setEnableCondition( "IsEmpty(Container(211).ListItem.Property(LOCKED))" )
            except:
                pass
            
            # Set button labels
            if self.nolabels == "false":
                try:
                    if self.getControl( 301 ).getLabel() == "":
                        self.getControl( 301 ).setLabel( __language__(32000) )
                except:
                    log( "No add shortcut button on GUI (id 301)" )
                try:
                    if self.getControl( 302 ).getLabel() == "":
                        self.getControl( 302 ).setLabel( __language__(32001) )
                except:
                    log( "No delete shortcut button on GUI (id 302)" )
                try:
                    if self.getControl( 303 ).getLabel() == "":
                        self.getControl( 303 ).setLabel( __language__(32002) )
                except:
                    log( "No move shortcut up button on GUI (id 303)" )
                try:
                    if self.getControl( 304 ).getLabel() == "":
                        self.getControl( 304 ).setLabel( __language__(32003) )
                except:
                    log( "No move shortcut down button on GUI (id 304)" )
                
                try:
                    if self.getControl( 305 ).getLabel() == "":
                        self.getControl( 305 ).setLabel( __language__(32025) )
                except:
                    log( "Not set label button on GUI (id 305)" )
                    
                try:
                    if self.getControl( 306 ).getLabel() == "":
                        self.getControl( 306 ).setLabel( __language__(32026) )
                except:
                    log( "No edit thumbnail button on GUI (id 306)" )
                    
                try:
                    if self.getControl( 307 ).getLabel() == "":
                        self.getControl( 307 ).setLabel( __language__(32027) )
                except:
                    log( "Not edit action button on GUI (id 307)" )
                    
                try:
                    if self.getControl( 308 ).getLabel() == "":
                        self.getControl( 308 ).setLabel( __language__(32028) )
                except:
                    log( "No reset shortcuts button on GUI (id 308)" )
                    
                try:
                    if self.getControl( 309 ).getLabel() == "":
                        self.getControl( 309 ).setLabel( __language__(32044) )
                except:
                    log( "No widget button on GUI (id 309)" )
                try:
                    if self.getControl( 310 ).getLabel() == "":
                        self.getControl( 310 ).setLabel( __language__(32045) )
                except:
                    log( "No background button on GUI (id 310)" )
                    
                try:
                    if self.getControl( 401 ).getLabel() == "":
                        self.getControl( 401 ).setLabel( __language__(32048) )
                except:
                    log( "No widget button on GUI (id 401)" )
                    
            # Load library shortcuts in thread
            thread.start_new_thread( LIBRARY.loadLibrary, () )
            
            if has111:
                try:
                    self._display_shortcuts()
                except:
                    pass


                
    # ======================
    # === LOAD/SAVE DATA ===
    # ======================

    
    def load_shortcuts( self, includeUserShortcuts = True, addShortcutsToWindow = True ):
        log( "Loading shortcuts" )
        DATA._clear_labelID()
        
        if includeUserShortcuts:
            shortcuts = DATA._get_shortcuts( self.group, defaultGroup = self.defaultGroup )
        else:
            shortcuts = DATA._get_shortcuts( self.group, defaultGroup = self.defaultGroup, defaultsOnly = True )
        
        #listitems = []
        for shortcut in shortcuts.getroot().findall( "shortcut" ):
            # Parse the shortcut, and add it to the list of shortcuts
            item = self._parse_shortcut( shortcut )
            self.allListItems.append( item[1] )
        
        # Add all visible shortcuts to control 211
        self._display_listitems()
        
    def _display_listitems( self, focus = None ):
        # Displays listitems that are visible from self.allListItems
        
        # Initial properties
        count = 0
        DATA._clear_labelID()
        listitems = []
        
        # If there are no shortcuts, add a blank one
        if len( self.allListItems ) == 0:
            listitem = xbmcgui.ListItem( __language__(32013), iconImage = "DefaultShortcut.png" )
            listitem.setProperty( "Path", 'noop' )
            listitem.setProperty( "icon", "DefaultShortcut.png" )
            self.allListItems = [ listitem ]
        
        for listitem in self.allListItems:
            # Get icon overrides
            self._get_icon_overrides( listitem )
            
            # Set order index in case its changed
            listitem.setProperty( "skinshortcuts-orderindex", str( count ) )
            
            shouldDisplay = True
            # Check for a visibility condition
            if listitem.getProperty( "visible-condition" ):
                shouldDisplay = xbmc.getCondVisibility( listitem.getProperty( "visible-condition" ) )
                
            if shouldDisplay == True:
                listitems.append( listitem )
                
            # Increase our count
            count += 1
                
        self.getControl( 211 ).reset()
        self.getControl( 211 ).addItems( listitems )
        if focus is not None:
            self.getControl( 211 ).selectItem( focus )
            
        
    def _parse_shortcut( self, item ):
        # Parse a shortcut node
        localLabel = DATA.local( item.find( "label" ).text )
        localLabel2 = DATA.local( item.find( "label2" ).text )
        
        # Get icon and thumb (and set to None if there isn't any)
        icon = item.find( "icon" )
        
        if icon is not None:
            icon = icon.text
        else:
            icon = "DefaultShortcut.png"
            
        thumb = item.find( "thumb" )
        if thumb is not None:
            thumb = thumb.text
        else:
            thumb = ""
            
        # If either localLabel[ 2 ] starts with a $, ask Kodi to parse it for us
        if localLabel[ 2 ].startswith( "$" ):
            localLabel[ 2 ] = xbmc.getInfoLabel( localLabel[ 2 ] )
        if localLabel2[ 2 ].startswith( "$" ):
            localLabel2[ 2 ] = xbmc.getInfoLabel( localLabel2[ 2 ] )
        
        # Create the list item
        listitem = xbmcgui.ListItem( label=localLabel[2], label2 = localLabel2[2], iconImage = icon, thumbnailImage = thumb )
        listitem.setProperty( "localizedString", localLabel[0] )
        listitem.setProperty( "icon", icon )
        listitem.setProperty( "thumbnail", thumb )
        
        # Set the action
        action = item.find( "action" ).text
        if "special://skin/" in action:
            translate = xbmc.translatePath( "special://skin/" ).decode( "utf-8" )
            action = action.replace( "special://skin/", translate )
        listitem.setProperty( "path", action )
        listitem.setProperty( "displayPath", action )
        
        # If there's an overriden icon, use it
        overridenIcon = item.find( "override-icon" )
        if overridenIcon is not None:
            listitem.setIconImage( overridenIcon.text )
            listitem.setProperty( "icon", overridenIcon.text )
            listitem.setProperty( "original-icon", icon )
            
        # Set the labelID, displayID, shortcutType
        listitem.setProperty( "labelID", item.find( "labelID" ).text )
        listitem.setProperty( "defaultID", item.find( "defaultID" ).text )
        listitem.setProperty( "shortcutType", localLabel2[0] )
        
        # Set any visible condition
        isVisible = True
        visibleCondition = item.find( "visible" )
        if visibleCondition is not None:
            listitem.setProperty( "visible-condition", visibleCondition.text )
            isVisible = xbmc.getCondVisibility( visibleCondition.text )
        
        # Check if the shortcut is locked
        locked = item.find( "lock" )
        if locked is not None:
            if locked.text.lower() == "true" or locked.text == xbmc.getSkinDir():
                listitem.setProperty( "LOCKED", locked.text )
                
        # Additional properties
        additionalProperties = item.find( "additional-properties" )
        backgroundName = None
        backgroundPlaylistName = None
        if additionalProperties is not None:
            listitem.setProperty( "additionalListItemProperties", additionalProperties.text )
            for property in eval( additionalProperties.text ):
                listitem.setProperty( property[0], DATA.local( property[1] )[2] )
                
                # if this is backgroundName or backgroundPlaylistName, keep them so we can localise them properly
                if property[0] == "backgroundName":
                    backgroundName = property[1]
                if property[1] == "backgroundPlaylistName":
                    backgroundPlaylistName = property[1]
                    
            # If we've kept backgroundName, localise it with the updated playlist name
            if backgroundName is not None and backgroundPlaylistName is not None:
                listitem.setProperty( "backgroundName", DATA.local( backgroundName )[2].replace( "::PLAYLIST::", backgroundPlaylistName ) )
        else:
            listitem.setProperty( "additionalListItemProperties", "[]" )
        
        return [ isVisible, listitem ]

    def _get_icon_overrides( self, listitem, setToDefault = True ):
        # Start by getting the labelID
        labelID = listitem.getProperty( "localizedString" )
        if labelID == None or labelID == "":
            labelID = listitem.getLabel()
        labelID = DATA._get_labelID( labelID, listitem.getProperty( "path" ) )
        
        # Retrieve icon
        icon = listitem.getProperty( "icon" )
        oldicon = None
        iconIsVar = False
        
        if listitem.getProperty( "untranslatedIcon" ):
            iconIsVar = True
        
        # If the icon is a VAR or an INFO, we're going to translate it and set the untranslatedIcon property
        if icon.startswith( "$" ):
            listitem.setProperty( "untranslatedIcon", icon )
            icon = xbmc.getInfoLabel( icon )
            listitem.setProperty( "icon", icon )
            listitem.setIconImage( icon )
            iconIsVar = True
        
        # Check for overrides
        tree = DATA._get_overrides_skin()
        if tree is not None:
            for elem in tree.findall( "icon" ):
                if oldicon is None:
                    if ("labelID" in elem.attrib and elem.attrib.get( "labelID" ) == labelID) or ("image" in elem.attrib and elem.attrib.get( "image" ) == icon):
                        # LabelID matched
                        if "group" in elem.attrib:
                            if elem.attrib.get( "group" ) == self.group:
                                # Group also matches - change icon
                                oldicon = icon
                                icon = elem.text
                                
                        elif "grouping" not in elem.attrib:
                            # No group - change icon
                            oldicon = icon
                            icon = elem.text
                            
        # If the skin doesn't have the icon, replace it with DefaultShortcut.png
        setDefault = False
        if ( not xbmc.skinHasImage( icon ) and setToDefault == True ) and not iconIsVar:
            if oldicon == None:
                oldicon = icon
            setDefault = True
            icon = "DefaultShortcut.png"
                
        # If we changed the icon, update the listitem
        if oldicon is not None:
            listitem.setIconImage( icon )
            listitem.setProperty( "icon", icon )
            listitem.setProperty( "original-icon", oldicon )
            
        if setDefault == True and setToDefault == True:
            # We set this to the default icon, so we need to check if /that/ icon is overriden
            self._get_icon_overrides( listitem, False )
        
    def _save_shortcuts( self ):
        # Save shortcuts
        if self.changeMade == True:
            log( "Saving changes" )
            
            # Create a new tree
            tree = xmltree.ElementTree( xmltree.Element( "shortcuts" ) )
            root = tree.getroot()
            
            properties = []
            
            labelIDChanges = []
            labelIDChangesDict = {}
           
            DATA._clear_labelID()
            
            for listitem in self.allListItems:
                
                # If the item has a label...
                if listitem.getLabel() != __language__(32013):
                    # Generate labelID, and mark if it has changed
                    labelID = listitem.getProperty( "labelID" )
                    newlabelID = labelID
                    # defaultID
                    try:
                        defaultID = listitem.getProperty( "defaultID" ).decode( "utf-8" )
                    except:
                        defaultID = listitem.getProperty( "defaultID" )
                    
                    localizedString = listitem.getProperty( "localizedString" )
                    if localizedString is None or localizedString == "":
                        localLabel = DATA.local( listitem.getLabel() )
                    else:
                        localLabel = DATA.local( localizedString )
                    newlabelID = DATA._get_labelID( localLabel[3], listitem.getProperty( "path" ) )     
                    if self.group == "mainmenu":
                        labelIDChanges.append( [labelID, newlabelID, defaultID] )
                        labelIDChangesDict[ labelID ] = newlabelID
                        
                    # We want to save this
                    shortcut = xmltree.SubElement( root, "shortcut" )
                    xmltree.SubElement( shortcut, "defaultID" ).text = defaultID
                    
                    # Label and label2
                    xmltree.SubElement( shortcut, "label" ).text = localLabel[0]
                    xmltree.SubElement( shortcut, "label2" ).text = DATA.local( listitem.getLabel2() )[0]
                    
                    # Icon and thumbnail
                    if listitem.getProperty( "untranslatedIcon" ):
                        xmltree.SubElement( shortcut, "icon" ).text = listitem.getProperty( "untranslatedIcon" )
                    else:
                        if listitem.getProperty( "original-icon" ):
                            xmltree.SubElement( shortcut, "icon" ).text = listitem.getProperty( "original-icon" )
                        else:
                            xmltree.SubElement( shortcut, "icon" ).text = listitem.getProperty( "icon" )
                        
                    xmltree.SubElement( shortcut, "thumb" ).text = listitem.getProperty( "thumbnail" )
                    
                    # Action
                    try:
                        action = listitem.getProperty( "path" ).decode( "utf-8" )
                    except:
                        action = listitem.getProperty( "path" )
                    xmltree.SubElement( shortcut, "action" ).text = action
                    
                    # Visible
                    if listitem.getProperty( "visible-condition" ):
                        xmltree.SubElement( shortcut, "visible" ).text = listitem.getProperty( "visible-condition" )
                    
                    # Locked
                    if listitem.getProperty( "LOCKED" ):
                        xmltree.SubElement( shortcut, "lock" ).text = listitem.getProperty( "LOCKED" )
                    
                    # Additional properties
                    if listitem.getProperty( "additionalListItemProperties" ):
                        properties.append( [ newlabelID, eval( listitem.getProperty( "additionalListItemProperties" ) ) ] )
                        
            # Save the shortcuts
            DATA.indent( root )
            path = os.path.join( __datapath__ , DATA.slugify( self.group ) + ".DATA.xml" )
            
            try:
                path = path.decode( "utf-8" )
            except:
                pass
            
            tree.write( path.replace( ".shortcuts", ".DATA.xml" ), encoding="UTF-8"  )
            
            # Now make any labelID changes
            while not len( labelIDChanges ) == 0:
                # Get the first labelID change, and check that we're not changing anything from that
                labelIDFrom = labelIDChanges[0][0]
                labelIDTo = labelIDChanges[0][1]
                defaultIDFrom = labelIDChanges[0][2]
                
                # If labelIDFrom is empty. this is a new item so we want to set the From the same as the To
                # (this will ensure any default .shortcuts file is copied across)
                if labelIDFrom == "" or labelIDFrom is None:
                    labelIDFrom = labelIDTo
                
                # Check that there isn't another item in the list whose 'From' is the same as our 'To'
                # - if so, we're going to move our items elsewhere, and move 'em to the correct place later
                # (This ensures we don't overwrite anything incorrectly)
                if not len( labelIDChanges ) == 1:
                    for x in range( 1, len( labelIDChanges ) ):
                        if labelIDChanges[x][0] == labelIDTo:
                            tempLocation = str( random.randrange(0,9999999999999999) )
                            labelIDChanges[0][1] = tempLocation
                            labelIDChanges.append( [tempLocation, labelIDTo, defaultIDFrom] )
                            labelIDTo = tempLocation
                            break
                            
                # Make the change (0 - the main sub-menu, 1-5 - additional submenus )
                for i in range( 0, 6 ):
                    if i == 0:
                        paths = [[os.path.join( __datapath__, DATA.slugify( labelIDFrom ) + ".DATA.xml" ).encode( "utf-8" ), "Move"], [os.path.join( __skinpath__, DATA.slugify( defaultIDFrom ) + ".DATA.xml" ).encode( "utf-8" ), "Copy"], [os.path.join( __defaultpath__, DATA.slugify( defaultIDFrom ) + ".DATA.xml" ).encode( "utf-8" ), "Copy"], [None, "New"]]
                        target = os.path.join( __datapath__, DATA.slugify( labelIDTo ) + ".DATA.xml" ).encode( "utf-8" )
                    else:
                        paths = [[os.path.join( __datapath__, DATA.slugify( labelIDFrom ) + "." + str( i ) + ".DATA,xml" ).encode( "utf-8" ), "Move"], [os.path.join( __skinpath__, DATA.slugify( defaultIDFrom ) + "." + str( i ) + ".DATA.xml" ).encode( "utf-8" ), "Copy"], [os.path.join( __defaultpath__, DATA.slugify( defaultIDFrom ) + "." + str( i ) + ".DATA.xml" ).encode( "utf-8" ), "Copy"]]
                        target = os.path.join( __datapath__, DATA.slugify( labelIDTo ) + "." + str( i ) + ".DATA.xml" ).encode( "utf-8" )
                        
                    try:
                        target = target.decode( "utf-8" )
                    except:
                        pass
                    
                    for path in paths:
                        try:
                            path[ 0 ] = path[ 0 ].decode( "utf-8" )
                        except:
                            pass
                        try:
                            path[ 1 ] = path[ 1 ].decode( "utf-8" )
                        except:
                            pass
                            
                        if path[1] == "New":
                            tree = xmltree.ElementTree( xmltree.Element( "shortcuts" ) )
                            tree.write( target, encoding="UTF-8"  )
                            break
                            
                        elif xbmcvfs.exists( path[0] ):
                            # The XML file exists
                            if path[1] == "Move":
                                # Move the original to the target path
                                log( "### Moving " + path[0] + " > " + target )
                                xbmcvfs.rename( path[0], target )
                            else:
                                # We're copying the file (actually, we'll re-write the file without
                                # any LOCKED elements)
                                newtree = xmltree.parse( path[0] )
                                for newnode in newtree.getroot().findall( "shortcut" ):
                                    searchNode = newnode.find( "locked" )
                                    if searchNode is not None:
                                        newnode.remove( searchNode )
                                    #searchNode = newnode.find( "defaultID" )
                                    #if searchNode is not None:
                                    #    newnode.remove( searchNode )
                                        
                                # Write it to the target
                                DATA.indent( newtree.getroot() )
                                newtree.write( target, encoding="utf-8" )
                                log( "### Copying " + path[0] + " > " + target )
                            break
                            
                        elif xbmcvfs.exists( path[0].replace( ".DATA.xml", ".shortcuts" ) ):
                            # An original .shortcuts file exists
                            
                            # Copy a default shortcuts file to the target path
                            xbmcvfs.copy( path[0].replace( ".DATA.xml", ".shortcuts" ), target.replace( ".DATA.xml", ".shortcuts" ) )
                            
                            # Upgrade the default shortcuts file
                            datafunctions.UpgradeFunctions().upgrade_file( target.replace( ".DATA.xml", ".shortcuts" ) )
                            datafunctions.UpgradeFunctions().upgrade_xmlfile( target.replace( ".DATA.xml", ".shortcuts" ) )
                            
                            # Delete the backup file
                            xbmcvfs.delete( target.replace( ".DATA.xml", ".shortcuts.backup" ) )
                            break
                        
                labelIDChanges.pop( 0 )
                
            # Save widgets, backgrounds and custom properties
            self._save_properties( properties, labelIDChangesDict )
            
            # Note that we've saved stuff
            xbmcgui.Window( 10000 ).setProperty( "skinshortcuts-reloadmainmenu", "True" )
                    
    def _save_properties( self, properties, labelIDChanges ):
        # Save all additional properties (widgets, backgrounds, custom)
        log( "Saving properties" )
        
        currentProperties = []
        
        # Get previously loaded properties
        path = os.path.join( __datapath__ , xbmc.getSkinDir().decode('utf-8') + ".properties" )
        if xbmcvfs.exists( path ):
            # The properties file exists, load from it
            listProperties = eval( xbmcvfs.File( path ).read() )
            for listProperty in listProperties:
                # listProperty[0] = groupname
                # listProperty[1] = labelID
                # listProperty[2] = property name
                # listProperty[3] = property value
                currentProperties.append( [listProperty[0], listProperty[1], listProperty[2], listProperty[3]] )
        
        # Copy any items not in the current group to the array we'll save, and
        # make any labelID changes whilst we're at it
        saveData = []
        for property in currentProperties:
            #[ groupname, itemLabelID, property, value ]
            if not property[0] == self.group:
                if property[0] in labelIDChanges.keys():
                    property[0] = labelIDChanges[property[0]]
                saveData.append( property )
        
        # Add all the properties we've been passed
        for property in properties:
            # property[0] = labelID
            for toSave in property[1]:
                # toSave[0] = property name
                # toSave[1] = property value
                
                saveData.append( [ self.group, property[0], toSave[0], toSave[1] ] )
        
        # Try to save the file
        try:
            f = xbmcvfs.File( os.path.join( __datapath__ , xbmc.getSkinDir().decode('utf-8') + ".properties" ), 'w' )
            f.write( repr( saveData ).replace( "],", "],\n" ) )
            f.close()
        except:
            print_exc()
            log( "### ERROR could not save file %s" % __datapath__ )                
    
    def _load_widgetsbackgrounds( self ):
        # Pre-load widget, background and thumbnail options provided by skin
        self.widgets = []
        self.widgetsPretty = {}
        self.backgrounds = []
        self.backgroundsPretty = {}
        self.thumbnails = []
        self.thumbnailsPretty = {}
        
        # Load skin overrides
        tree = DATA._get_overrides_skin()
        
        # Get widgets
        if tree is not None:
            elems = tree.getroot().findall('widget')
            for elem in elems:
                widgetType = None
                if "type" in elem.attrib:
                    widgetType = elem.attrib.get( "type" )
                if "condition" in elem.attrib:
                    if not xbmc.getCondVisibility( elem.attrib.get( "condition" ) ):
                        continue
                self.widgets.append( [elem.text, DATA.local( elem.attrib.get( 'label' ) )[2], widgetType ] )
                self.widgetsPretty[elem.text] = DATA.local( elem.attrib.get( 'label' ) )[2]
                
            # Should we allow the user to select a playlist as a widget...
            elem = tree.find('widgetPlaylists')
            if elem is not None and elem.text == "True":
                self.widgetPlaylists = True
                if "type" in elem.attrib:
                    self.widgetPlaylistsType = elem.attrib.get( "type" )
                
        # Get backgrounds
        if tree is not None:
            elems = tree.findall('background')
            for elem in elems:
                if "condition" in elem.attrib:
                    if not xbmc.getCondVisibility( elem.attrib.get( "condition" ) ):
                        continue

                self.backgrounds.append( [elem.text, DATA.local( elem.attrib.get( 'label' ) )[2] ] )
                self.backgroundsPretty[elem.text] = DATA.local( elem.attrib.get( 'label' ) )[2]
                
            # Should we allow the user to browse for background images...
            elem = tree.find('backgroundBrowse')
            if elem is not None and elem.text == "True":
                self.backgroundBrowse = True
                if "default" in elem.attrib:
                    self.backgroundBrowseDefault = elem.attrib.get( "default" )

        # Get thumbnails
        if tree is not None:
            elems = tree.findall('thumbnail')
            for elem in elems:
                self.thumbnails.append( [elem.text, DATA.local( elem.attrib.get( 'label' ) )[2] ] )
                self.thumbnailsPretty[elem.text] = DATA.local( elem.attrib.get( 'label' ) )[2]
                
    # ========================
    # === GUI INTERACTIONS ===
    # ========================

    
    def onClick(self, controlID):
        if controlID == 102:
            # Move to previous type of shortcuts
            self.shortcutgroup = self.shortcutgroup - 1
            if self.shortcutgroup == 0:
                self.shortcutgroup = LIBRARY.flatGroupingsCount()
            
            self._display_shortcuts()

        if controlID == 103:
            # Move to next type of shortcuts
            self.shortcutgroup = self.shortcutgroup + 1
            if self.shortcutgroup > LIBRARY.flatGroupingsCount():
                self.shortcutgroup = 1
            
            self._display_shortcuts()
            
        if controlID == 111:
            # User has selected an available shortcut they want in their menu
            log( "Select shortcut (111)" )
            listControl = self.getControl( 211 )
            itemIndex = listControl.getSelectedPosition()
            orderIndex = int( listControl.getListItem( itemIndex ).getProperty( "skinshortcuts-orderindex" ) )
            altAction = None
            
            if self.warnonremoval( listControl.getListItem( itemIndex ) ) == False:
                return
            
            # Copy the new shortcut
            selectedItem = self.getControl( 111 ).getSelectedItem()
            listitemCopy = self._duplicate_listitem( selectedItem, listControl.getListItem( itemIndex ) )
            
            path = listitemCopy.getProperty( "path" )
            if path.startswith( "||BROWSE||" ):
                # If this is a plugin, call our plugin browser
                returnVal = LIBRARY.explorer( ["plugin://" + path.replace( "||BROWSE||", "" )], "plugin://" + path.replace( "||BROWSE||", "" ), [self.getControl( 111 ).getSelectedItem().getLabel()], [self.getControl( 111 ).getSelectedItem().getProperty("thumbnail")], self.getControl( 111 ).getSelectedItem().getProperty("shortcutType")  )
                if returnVal is not None:
                    # Convert backslashes to double-backslashes (windows fix)
                    newAction = returnVal.getProperty( "Path" )
                    newAction = newAction.replace( "\\", "\\\\" )
                    returnVal.setProperty( "path", newAction )
                    returnVal.setProperty( "displayPath", newAction )
                    listitemCopy = self._duplicate_listitem( returnVal, listControl.getListItem( itemIndex ) )
                else:
                    listitemCopy = None
            elif path == "||UPNP||":
                returnVal = LIBRARY.explorer( ["upnp://"], "upnp://", [self.getControl( 111 ).getSelectedItem().getLabel()], [self.getControl( 111 ).getSelectedItem().getProperty("thumbnail")], self.getControl( 111 ).getSelectedItem().getProperty("shortcutType")  )
                if returnVal is not None:
                    listitemCopy = self._duplicate_listitem( returnVal, listControl.getListItem( itemIndex ) )
                else:
                    listitemCopy = None
            elif path.startswith( "||SOURCE||" ):
                returnVal = LIBRARY.explorer( [path.replace( "||SOURCE||", "" )], path.replace( "||SOURCE||", "" ), [self.getControl( 111 ).getSelectedItem().getLabel()], [self.getControl( 111 ).getSelectedItem().getProperty("thumbnail")], self.getControl( 111 ).getSelectedItem().getProperty("shortcutType")  )
                if returnVal is not None:
                    if "upnp://" in returnVal.getProperty( "Path" ):
                        listitemCopy = self._duplicate_listitem( returnVal, listControl.getListItem( itemIndex ) )
                    else:
                        returnVal = LIBRARY._sourcelink_choice( returnVal )
                        if returnVal is not None:
                            listitemCopy = self._duplicate_listitem( returnVal, listControl.getListItem( itemIndex ) )
                        else:
                            listitemCopy = None
                else:
                    listitemCopy = None
            elif path == "::PLAYLIST::":
                # Give the user the choice of playing or displaying the playlist
                dialog = xbmcgui.Dialog()
                userchoice = dialog.yesno( __language__( 32040 ), __language__( 32060 ), "", "", __language__( 32061 ), __language__( 32062 ) )
                # False: Display
                # True: Play
                if userchoice == False:
                    listitemCopy.setProperty( "path", selectedItem.getProperty( "action-show" ) )
                    listitemCopy.setProperty( "displayPath", selectedItem.getProperty( "action-show" ) )
                else:
                    listitemCopy.setProperty( "path", selectedItem.getProperty( "action-play" ) )
                    listitemCopy.setProperty( "displayPath", selectedItem.getProperty( "action-play" ) )
             
            if listitemCopy is None:
                # Nothing was selected in the explorer
                return
                
            self.changeMade = True
            
            # Replace the allListItems listitem with our new list item
            self.allListItems[ orderIndex ] = listitemCopy
            
            # Delete playlist (TO BE REMOVED!)
            LIBRARY._delete_playlist( listControl.getListItem( itemIndex ).getProperty( "path" ) )
            
            # Display list items
            self._display_listitems( focus = itemIndex )
        
        if controlID == 301:
            # Add a new item
            log( "Add item (301)" )
            self.changeMade = True
            listControl = self.getControl( 211 )
            
            listitem = xbmcgui.ListItem( __language__(32013) )
            listitem.setProperty( "Path", 'noop' )
            
            # Add new item to both displayed list and list kept in memory
            self.allListItems.append( listitem )
            self._display_listitems( listControl.size() )
            
            # Set focus
            listControl.selectItem( listControl.size() -1 )
        
        if controlID == 302:
            # Delete an item
            log( "Delete item (302)" )
            self.changeMade = True
            
            listControl = self.getControl( 211 )
            num = listControl.getSelectedPosition()
            orderIndex = int( listControl.getListItem( num ).getProperty( "skinshortcuts-orderindex" ) )
            
            if self.warnonremoval( listControl.getListItem( num ) ) == False:
                return
            
            LIBRARY._delete_playlist( listControl.getListItem( num ).getProperty( "path" ) )
            
            self.changeMade = True
            
            # Remove item from memory list, and reload all list items
            self.allListItems.pop( orderIndex )
            self._display_listitems( num )
            
        if controlID == 303:
            # Move item up in list
            log( "Move up (303)" )
            listControl = self.getControl( 211 )
            
            itemIndex = listControl.getSelectedPosition()
            orderIndex = int( listControl.getListItem( itemIndex ).getProperty( "skinshortcuts-orderindex" ) )
            if itemIndex == 0:
                # Top item, can't move it up
                return
                
            self.changeMade = True
                
            while True:
                # Move the item one up in the list
                self.allListItems[ orderIndex - 1 ], self.allListItems[ orderIndex ] = self.allListItems[ orderIndex ], self.allListItems[ orderIndex - 1 ]
                
                # If we've just moved to the top of the list, break
                if orderIndex == 1:
                    break
                    
                # Check if the item we've just swapped is visible
                shouldBreak = True
                if self.allListItems[ orderIndex ].getProperty( "visible-condition" ):
                    shouldBreak = xbmc.getCondVisibility( self.allListItems[ orderIndex ].getProperty( "visible-condition" ) )
                    
                if shouldBreak:
                    break
                    
                orderIndex -= 1
                    
            # Display the updated order
            self._display_listitems( itemIndex - 1 )
            
        if controlID == 304:
            # Move item down in list
            log( "Move down (304)" )
            listControl = self.getControl( 211 )
            
            itemIndex = listControl.getSelectedPosition()
            orderIndex = int( listControl.getListItem( itemIndex ).getProperty( "skinshortcuts-orderindex" ) )
            
            log( str( itemIndex ) + " : " + str( listControl.size() ) )
            
            if itemIndex == listControl.size() - 1:
                return
                
            self.changeMade = True
            
            while True:
                # Move the item one up in the list
                self.allListItems[ orderIndex + 1 ], self.allListItems[ orderIndex ] = self.allListItems[ orderIndex ], self.allListItems[ orderIndex + 1 ]
                
                # If we've just moved to the top of the list, break
                if orderIndex == len( self.allListItems ) - 1:
                    break
                    
                # Check if the item we've just swapped is visible
                shouldBreak = True
                if self.allListItems[ orderIndex ].getProperty( "visible-condition" ):
                    shouldBreak = xbmc.getCondVisibility( self.allListItems[ orderIndex ].getProperty( "visible-condition" ) )
                    
                if shouldBreak:
                    break
                    
                orderIndex += 1
                    
            # Display the updated order
            self._display_listitems( itemIndex + 1 )

        if controlID == 305:
            # Change label
            log( "Change label (305)" )
            listControl = self.getControl( 211 )
            listitem = listControl.getSelectedItem()
            
            # Retreive current label and labelID
            label = listitem.getLabel()
            oldlabelID = listitem.getProperty( "labelID" )
            
            # If the item is blank, set the current label to empty
            if label == __language__(32013):
                label = ""
                
            # Get new label from keyboard dialog
            keyboard = xbmc.Keyboard( label, xbmc.getLocalizedString(528), False )
            keyboard.doModal()
            if ( keyboard.isConfirmed() ):
                label = keyboard.getText()
                if label == "":
                    label = __language__(32013)
            else:
                return
                
            self.changeMade = True
            self._set_label( listitem, label )

        if controlID == 306:
            # Change thumbnail
            log( "Change thumbnail (306)" )
            listControl = self.getControl( 211 )
            listitem = listControl.getSelectedItem()
            
            # Get new thumbnail from browse dialog
            dialog = xbmcgui.Dialog()
            custom_thumbnail = dialog.browse( 2 , xbmc.getLocalizedString(1030), 'files')
            
            if custom_thumbnail:
                # Update the thumbnail
                self.changeMade = True
                listitem.setThumbnailImage( custom_thumbnail )
                listitem.setProperty( "thumbnail", custom_thumbnail )
            
        if controlID == 307:
            # Change Action
            log( "Change action (307)" )
            listControl = self.getControl( 211 )
            listitem = listControl.getSelectedItem()
            
            if self.warnonremoval( listitem ) == False:
                return
            
            # Retrieve current action
            action = listitem.getProperty( "path" )
            if action == "noop":
                action = ""
                
            # Get new action from keyboard dialog
            keyboard = xbmc.Keyboard( action, xbmc.getLocalizedString(528), False )
            keyboard.doModal()
            
            if ( keyboard.isConfirmed() ):
                try:
                    action = keyboard.getText().decode( "utf-8" )
                except:
                    action = keyboard.getText()
                if action == "":
                    action = "noop"
                    
                # Check that a change was really made
                if action == listitem.getProperty( "path" ):
                    return
            else:
                return
                
            self.changeMade = True
            LIBRARY._delete_playlist( listitem.getProperty( "path" ) )
            
            # Update the action
            listitem.setProperty( "path", action )
            listitem.setProperty( "displaypath", action )
            listitem.setLabel2( __language__(32024) )
            listitem.setProperty( "shortcutType", "32024" )
            
        if controlID == 308:
            # Reset shortcuts
            log( "Reset shortcuts (308)" )
            self.changeMade = True
            
            # Delete any auto-generated source playlists
            for x in range(0, self.getControl( 211 ).size()):
                LIBRARY._delete_playlist( self.getControl( 211 ).getListItem( x ).getProperty( "path" ) )

            self.getControl( 211 ).reset()
            
            self.allListItems = []
            
            # Call the load shortcuts function, but add that we don't want
            # previously saved user shortcuts
            self.load_shortcuts( False )
                
        if controlID == 309:
            # Choose widget
            log( "Choose widget (309)" )
            listControl = self.getControl( 211 )
            listitem = listControl.getSelectedItem()
            
            # Get the default widget for this item
            defaultWidget = self.find_default( "widget", listitem.getProperty( "labelID" ), listitem.getProperty( "defaultID" ) )
            
            # Generate list of widgets for select dialog
            widget = [""]
            widgetLabel = [__language__(32053)]
            widgetName = [""]
            widgetType = [ None ]
            for key in self.widgets:
                widget.append( key[0] )
                widgetName.append( "" )
                widgetType.append( key[2] )
                
                if key[0] == defaultWidget:
                    widgetLabel.append( key[1] + " (%s)" %( __language__(32050) ) )
                else:
                    widgetLabel.append( key[1] )
                
            # If playlists have been enabled for widgets, add them too
            if self.widgetPlaylists:
                for playlist in LIBRARY.widgetPlaylistsList:
                    widget.append( "::PLAYLIST::" + playlist[0] )
                    widgetLabel.append( playlist[1] )
                    widgetName.append( playlist[2] )
                    widgetType.append( self.widgetPlaylistsType )
                for playlist in LIBRARY.scriptPlaylists():
                    widget.append( "::PLAYLIST::" + playlist[0] )
                    widgetLabel.append( playlist[1] )
                    widgetName.append( playlist[2] )
                    widgetType.append( self.widgetPlaylistsType )
                    
            # Show the dialog
            selectedWidget = xbmcgui.Dialog().select( __language__(32044), widgetLabel )
            
            if selectedWidget == -1:
                # User cancelled
                return
            elif selectedWidget == 0:
                # User selected no widget
                self._remove_additionalproperty( listitem, "widget" )
                self._remove_additionalproperty( listitem, "widgetName" )
                self._remove_additionalproperty( listitem, "widgetType" )
                self._remove_additionalproperty( listitem, "widgetPlaylist" )
                
            else:
                currentWindow = xbmcgui.Window(xbmcgui.getCurrentWindowDialogId())
                
                if widget[selectedWidget].startswith( "::PLAYLIST::" ):
                    self._add_additionalproperty( listitem, "widget", "Playlist" )
                    self._add_additionalproperty( listitem, "widgetName", widgetName[selectedWidget] )
                    self._add_additionalproperty( listitem, "widgetPlaylist", widget[selectedWidget].strip( "::PLAYLIST::" ) )
                    if currentWindow.getProperty( "useWidgetNameAsLabel" ) == "true" :
                        self._set_label( listitem, widgetName[selectedWidget] )
                        currentWindow.clearProperty( "useWidgetNameAsLabel" )
                else:
                    self._add_additionalproperty( listitem, "widgetName", widgetLabel[selectedWidget].replace( " (%s)" %( __language__(32050) ), "" ) )
                    self._add_additionalproperty( listitem, "widget", widget[selectedWidget] )
                    self._remove_additionalproperty( listitem, "widgetPlaylist" )
                    if currentWindow.getProperty( "useWidgetNameAsLabel" ) == "true" :
                        self._set_label( listitem, widgetLabel[selectedWidget].replace( " (%s)" %( __language__(32050) ), "" ) )
                        currentWindow.clearProperty( "useWidgetNameAsLabel" )
                
                if widgetType[ selectedWidget] is not None:
                    self._add_additionalproperty( listitem, "widgetType", widgetType[ selectedWidget] )
                else:
                    self._remove_additionalproperty( listitem, "widgetType" )
                
            self.changeMade = True
                
        if controlID == 310:
            # Choose background
            log( "Choose background (310)" )
            listControl = self.getControl( 211 )
            listitem = listControl.getSelectedItem()
            
            usePrettyDialog = False
            
            # Create lists for the select dialog, with image browse buttons if enabled
            if self.backgroundBrowse:
                background = ["", "", ""]         
                backgroundLabel = [__language__(32053), __language__(32051), __language__(32052)]
                backgroundPretty = [ LIBRARY._create(["", __language__(32053), "", { "icon": "DefaultAddonNone.png" }] ), LIBRARY._create(["", __language__(32051), "", { "icon": "DefaultFile.png" }] ), LIBRARY._create(["", __language__(32052), "", { "icon": "DefaultFolder.png" }] ) ]
            else:
                background = [""]                         
                backgroundLabel = [__language__(32053)]
                backgroundPretty = [ LIBRARY._create(["", __language__(32053), "", { "icon": "DefaultAddonNone.png" }] ) ]

            # Get the default background for this item
            defaultBackground = self.find_default( "background", listitem.getProperty( "labelID" ), listitem.getProperty( "defaultID" ) )
            log( repr( defaultBackground ) )
            
            # Generate list of backgrounds for the dialog
            for key in self.backgrounds:
                if "::PLAYLIST::" in key[1]:
                    for playlist in LIBRARY.widgetPlaylistsList:
                        background.append( [ key[0], playlist[0], playlist[1] ] )
                        backgroundLabel.append( key[1].replace( "::PLAYLIST::", playlist[1] ) )
                        backgroundPretty.append( LIBRARY._create(["", key[1].replace( "::PLAYLIST::", playlist[1] ), "", {}] ) )
                    for playlist in LIBRARY.scriptPlaylists():
                        background.append( [ key[0], playlist[0], playlist[1] ] )
                        backgroundLabel.append( key[1].replace( "::PLAYLIST::", playlist[1] ) )
                        backgroundPretty.append( LIBRARY._create(["", key[1].replace( "::PLAYLIST::", playlist[1] ), "", {}] ) )
                else:
                    background.append( key[0] )     
                    log( repr( defaultBackground ) + " : " + repr( key[ 0 ] ) )
                    if defaultBackground == key[ 0 ]:
                        backgroundLabel.append( key[1] + " (%s)" %( __language__(32050) ) )
                        if xbmc.skinHasImage( key[ 0 ] ) == True:
                            usePrettyDialog = True
                            backgroundPretty.append( LIBRARY._create(["", key[ 1 ] + " (%s)" %( __language__(32050) ), "", { "icon": "DefaultFile.png", "thumb": key[ 0 ] } ] ) )
                        else:
                            backgroundPretty.append( LIBRARY._create(["", key[ 1 ] + " (%s)" %( __language__(32050) ), "", {} ] ) )
                    else:
                        backgroundLabel.append( key[1] )
                        if xbmc.skinHasImage( key[ 0 ] ) == True:
                            usePrettyDialog = True
                            backgroundPretty.append( LIBRARY._create(["", key[ 1 ], "", { "icon": "DefaultFile.png", "thumb": key[ 0 ] } ] ) )
                        else:
                            backgroundPretty.append( LIBRARY._create(["", key[ 1 ], "", {} ] ) )
            
            if usePrettyDialog:
                w = library.ShowDialog( "DialogSelect.xml", __cwd__, listing=backgroundPretty, windowtitle=__language__(32045) )
                w.doModal()
                selectedBackground = w.result
                del w
            else:
                # Show the dialog
                selectedBackground = xbmcgui.Dialog().select( __language__(32045), backgroundLabel )
            
            if selectedBackground == -1:
                # User cancelled
                return
            elif selectedBackground == 0:
                # User selected no background
                self._remove_additionalproperty( listitem, "background" )
                self._remove_additionalproperty( listitem, "backgroundName" )
                self._remove_additionalproperty( listitem, "backgroundPlaylist" )
                self._remove_additionalproperty( listitem, "backgroundPlaylistName" )

            elif self.backgroundBrowse == True and (selectedBackground == 1 or selectedBackground == 2):
                # User has chosen to browse for an image/folder
                imagedialog = xbmcgui.Dialog()
                if selectedBackground == 1: # Single image
                    custom_image = imagedialog.browse( 2 , xbmc.getLocalizedString(1030), 'files', '', True, False, self.backgroundBrowseDefault)
                else: # Multi-image
                    custom_image = imagedialog.browse( 0 , xbmc.getLocalizedString(1030), 'files', '', True, False, self.backgroundBrowseDefault)
                
                if custom_image and custom_image != self.backgroundBrowseDefault:
                    self._add_additionalproperty( listitem, "background", custom_image )
                    self._add_additionalproperty( listitem, "backgroundName", custom_image )
                    self._remove_additionalproperty( listitem, "backgroundPlaylist" )
                    self._remove_additionalproperty( listitem, "backgroundPlaylistName" )
                else:
                    # User cancelled
                    return

            else:
                if isinstance( background[selectedBackground], list ):
                    # User has selected a playlist backgrounds
                    self._add_additionalproperty( listitem, "background", background[selectedBackground][0] )
                    self._add_additionalproperty( listitem, "backgroundName", backgroundLabel[selectedBackground].replace("::PLAYLIST::", background[selectedBackground][1]) )
                    self._add_additionalproperty( listitem, "backgroundPlaylist", background[selectedBackground][1] )
                    self._add_additionalproperty( listitem, "backgroundPlaylistName", background[selectedBackground][2] )
                    
                else:
                    # User has selected a normal background
                    self._add_additionalproperty( listitem, "background", background[selectedBackground] )
                    self._add_additionalproperty( listitem, "backgroundName", backgroundLabel[selectedBackground].replace( " (%s)" %( __language__(32050) ), "" ) )
                    self._remove_additionalproperty( listitem, "backgroundPlaylist" )
                    self._remove_additionalproperty( listitem, "backgroundPlaylistName" )
            
            self.changeMade = True
        
        if controlID == 311:
            # Choose thumbnail
            log( "Choose thumbnail (311)" )
            listControl = self.getControl( 211 )
            listitem = listControl.getSelectedItem()
            
            # Create lists for the select dialog
            thumbnail = [""]                     
            thumbnailLabel = [LIBRARY._create(["", __language__(32096), "", {}] )]
            
            # Generate list of thumbnails for the dialog
            for key in self.thumbnails:
                log( repr( key[ 0 ] ) + " " + repr( key[ 1 ] ) )
                thumbnail.append( key[0] )            
                thumbnailLabel.append( LIBRARY._create(["", key[ 1 ], "", {"icon": key[ 0 ] }] ) )
            
            # Show the dialog
            w = library.ShowDialog( "DialogSelect.xml", __cwd__, listing=thumbnailLabel, windowtitle="Select thumbnail" )
            w.doModal()
            selectedThumbnail = w.result
            del w
            
            if selectedThumbnail == -1:
                # User cancelled
                return

            elif selectedThumbnail == 0:
                # User has chosen to browse for an image
                imagedialog = xbmcgui.Dialog()
                custom_image = imagedialog.browse( 2 , xbmc.getLocalizedString(1030), 'files', '', True, False, self.backgroundBrowseDefault)
                
                if custom_image:
                    listitem.setThumbnailImage( custom_image )
                    listitem.setProperty( "thumbnail", custom_image )
                else:
                    # User cancelled
                    return

            else:
                # User has selected a normal thumbnail
                listitem.setThumbnailImage( thumbnail[ selectedThumbnail ] )
                listitem.setProperty( "thumbnail", thumbnail[ selectedThumbnail ] )
            self.changeMade = True
        
        if controlID == 401:
            # Select shortcut
            log( "Select shortcut (401)" )
            
            num = self.getControl( 211 ).getSelectedPosition()
            orderIndex = int( self.getControl( 211 ).getListItem( num ).getProperty( "skinshortcuts-orderindex" ) )
            
            if self.warnonremoval( self.getControl( 211 ).getListItem( num ) ) == False:
                return
            
            selectedShortcut = LIBRARY.selectShortcut()
            if selectedShortcut is not None:
                listitemCopy = self._duplicate_listitem( selectedShortcut, self.getControl( 211 ).getListItem( num ) )
                if selectedShortcut.getProperty( "chosenPath" ):
                    listitemCopy.setProperty( "path", selectedShortcut.getProperty( "chosenPath" ) )
                    listitemCopy.setProperty( "displayPath", selectedShortcut.getProperty( "chosenPath" ) )
                LIBRARY._delete_playlist( self.getControl( 211 ).getListItem( num ).getProperty( "path" ) )
            
                self.changeMade = True
                
                self.allListItems[ orderIndex ] = listitemCopy
                self._display_listitems( num )
                    
        if controlID == 404:
            # Set custom property
            log( "Setting custom property (404)" )
            listControl = self.getControl( 211 )
            listitem = listControl.getSelectedItem()
            
            currentWindow = xbmcgui.Window(xbmcgui.getCurrentWindowDialogId())
            propertyName = ""
            propertyValue = ""
            
            # Retrieve the custom property
            if currentWindow.getProperty( "customProperty" ):
                propertyName = currentWindow.getProperty( "customProperty" )
                currentWindow.clearProperty( "customProperty" )
            
                # Retrieve the custom value
                if currentWindow.getProperty( "customValue" ):
                    propertyValue = currentWindow.getProperty( "customValue" )
                    currentWindow.clearProperty( "customValue" )
                    
                if propertyValue == "":
                    # No value set, so remove it from additionalListItemProperties
                    self._remove_additionalproperty( listitem, propertyName )
                    self.changeMade = True
                else:
                    # Set the property
                    self._add_additionalproperty( listitem, propertyName, propertyValue )
                    self.changeMade = True

            elif currentWindow.getProperty( "chooseProperty" ):
                propertyName = currentWindow.getProperty( "chooseProperty" )
                # Create lists for the select dialog
                property = [""]
                propertyLabel = ["None"]
                
                # Get all the skin-defined properties
                tree = DATA._get_overrides_skin()
                for elem in tree.findall( "property" ):
                    if "property" in elem.attrib and elem.attrib.get( "property" ) == propertyName:
                        foundProperty = elem.text
                        property.append( foundProperty )
                        propertyLabel.append( DATA.local( foundProperty )[2] )
                
                # Show the dialog
                selectedProperty = xbmcgui.Dialog().select( "Choose property", propertyLabel )
                
                if selectedProperty == -1:
                    # User cancelled
                    return
                elif selectedProperty == 0:
                    # User selected no property
                    self.changeMade = True
                    self._remove_additionalproperty( listitem, propertyName )
                else:
                    self.changeMade = True
                    self._add_additionalproperty( listitem, propertyName, property[ selectedProperty ] )
                
            else:
                # The customProperty value needs to be set, so return
                currentWindow.clearProperty( "customValue" )
                return
            
        if controlID == 405 or controlID == 406 or controlID == 407 or controlID == 408 or controlID == 409 or controlID == 410:
            # Launch management dialog for submenu
            log( "Launching management dialog for submenu/additional menu (" + str( controlID ) + ")" )

            currentWindow = xbmcgui.Window(xbmcgui.getCurrentWindowDialogId())
            
            # Get the group we're about to edit
            launchGroup = self.getControl( 211 ).getSelectedItem().getProperty( "labelID" )
            launchDefaultGroup = self.getControl( 211 ).getSelectedItem().getProperty( "defaultID" )
            groupName = self.getControl( 211 ).getSelectedItem().getLabel()
            
            if launchDefaultGroup == None:
                launchDefaultGroup = ""
                            
            # If the labelID property is empty, we need to generate one
            if launchGroup is None or launchGroup == "":
                DATA._clear_labelID()
                num = self.getControl( 211 ).getSelectedPosition()
                orderIndex = self.getControl( 211 ).getListItem( num )
                
                # Get the labelID's of all other menu items
                for listitem in self.allListItems:
                    if listitem != orderIndex:
                        DATA._get_labelID( listitem.getProperty( "labelID" ), listitem.getProperty( "path" ) )
                
                # Now generate labelID for this menu item, if it doesn't have one
                labelID = self.getControl( 211 ).getListItem( num ).getProperty( "localizedString" )
                if labelID is None or labelID == "":
                    launchGroup = DATA._get_labelID( self.getControl( 211 ).getListItem( num ).getLabel(), self.getControl( 211 ).getListItem( num ).getProperty( "path" ) )
                else:
                    launchGroup = DATA._get_labelID( labelID, self.getControl( 211 ).getListItem( num ).getProperty( "path" ) )
                self.getControl( 211 ).getListItem( num ).setProperty( "labelID", launchGroup )                                        
            
            # Check if we're launching a specific additional menu
            if controlID == 406:
                launchGroup = launchGroup + ".1"
            elif controlID == 407:
                launchGroup = launchGroup + ".2"
            elif controlID == 408:
                launchGroup = launchGroup + ".3"
            elif controlID == 409:
                launchGroup = launchGroup + ".4"
            elif controlID == 410:
                launchGroup = launchGroup + ".5"
            # Check if 'level' property has been set
            elif currentWindow.getProperty("level"):
                launchGroup = launchGroup + "." + currentWindow.getProperty("level")
                currentWindow.clearProperty("level")
                
            # Check if 'groupname' property has been set
            if currentWindow.getProperty( "overrideName" ):
                groupName = currentWindow.getProperty( "overrideName" )
                currentWindow.clearProperty( "overrideName" )
                
            # Execute the script
            currentWindow.setProperty( "additionalDialog", "True" )
            import gui
            ui= gui.GUI( "script-skinshortcuts.xml", __cwd__, "default", group=launchGroup, defaultGroup=launchDefaultGroup, nolabels=self.nolabels, groupname=groupName )
            ui.doModal()
            del ui
            currentWindow.clearProperty( "additionalDialog" )



    # ========================
    # === HELPER FUNCTIONS ===
    # ========================
    
            
    def _display_shortcuts( self ):
        # Load the currently selected shortcut group
        newGroup = LIBRARY.retrieveGroup( self.shortcutgroup )
        
        label = DATA.local( newGroup[0] )[2]
        
        self.getControl( 111 ).reset()
        for item in newGroup[1]:
            newItem = self._duplicate_listitem( item )
            if item.getProperty( "action-show" ):
                newItem.setProperty( "action-show", item.getProperty( "action-show" ) )
                newItem.setProperty( "action-play", item.getProperty( "action-play" ) )
            self.getControl( 111 ).addItem( newItem )
        self.getControl( 101 ).setLabel( label + " (%s)" %self.getControl( 111 ).size() )
        
    def _duplicate_listitem( self, listitem, originallistitem = None ):
        # Create a copy of an existing listitem
        listitemCopy = xbmcgui.ListItem(label=listitem.getLabel(), label2=listitem.getLabel2(), iconImage=listitem.getProperty("icon"), thumbnailImage=listitem.getProperty("thumbnail"))
        listitemCopy.setProperty( "path", listitem.getProperty("path") )
        listitemCopy.setProperty( "displaypath", listitem.getProperty("path") )
        listitemCopy.setProperty( "icon", listitem.getProperty("icon") )
        listitemCopy.setProperty( "thumbnail", listitem.getProperty("thumbnail") )
        listitemCopy.setProperty( "localizedString", listitem.getProperty("localizedString") )
        listitemCopy.setProperty( "shortcutType", listitem.getProperty("shortcutType") )
                
        if listitem.getProperty( "LOCKED" ):
            listitemCopy.setProperty( "LOCKED", listitem.getProperty( "LOCKED" ) )
            
        if listitem.getProperty( "defaultID" ):
            listitemCopy.setProperty( "defaultID", listitem.getProperty( "defaultID" ) )
        else:
            listitemCopy.setProperty( "defaultID", DATA._get_labelID( DATA.local( listitem.getProperty( "localizedString" ) )[3],  listitem.getProperty( "path" ), True ) )
            
        # If the item has an untranslated icon, set the icon image to it
        if listitem.getProperty( "untranslatedIcon" ):
            icon = listitem.getProperty( "untranslatedIcon" )
            listitemCopy.setIconImage( icon )
            listitemCopy.setProperty( "icon", icon )
            
        # Revert to original icon (because we'll override it again in a minute!)
        if listitem.getProperty( "original-icon" ):
            icon = listitem.getProperty( "original-icon" )
            if icon == "":
                icon = None
            listitemCopy.setIconImage( icon )
            listitemCopy.setProperty( "icon", icon )
        
        # If we've haven't been passed an originallistitem, set the following from the listitem we were passed
        if originallistitem is None:
            listitemCopy.setProperty( "labelID", listitem.getProperty("labelID") )
            if listitem.getProperty( "visible-condition" ):
                listitemCopy.setProperty( "visible-condition", listitem.getProperty( "visible-condition" ) )
            if listitem.getProperty( "additionalListItemProperties" ):
                listitemCopy.setProperty( "additionalListItemProperties", listitem.getProperty( "additionalListItemProperties" ) )
                listitemProperties = eval( listitem.getProperty( "additionalListItemProperties" ) )
                
                for listitemProperty in listitemProperties:
                    listitemCopy.setProperty( listitemProperty[0], DATA.local( listitemProperty[1] )[2] )
        else:
            # Set these from the original item we were passed (this will keep original labelID and additional properties
            # in tact)
            listitemCopy.setProperty( "labelID", originallistitem.getProperty( "labelID" ) )
            if originallistitem.getProperty( "visible-condition" ):
                listitemCopy.setProperty( "visible-condition", originallistitem.getProperty( "visible-condition" ) )
            if originallistitem.getProperty( "additionalListItemProperties" ):
                listitemCopy.setProperty( "additionalListItemProperties", originallistitem.getProperty( "additionalListItemProperties" ) )
                listitemProperties = eval( originallistitem.getProperty( "additionalListItemProperties" ) )
                
                for listitemProperty in listitemProperties:
                    listitemCopy.setProperty( listitemProperty[0], DATA.local(listitemProperty[1] )[2] )
                
        return listitemCopy
                
    def _add_additionalproperty( self, listitem, propertyName, propertyValue ):
        # Add an item to the additional properties of a user items
        properties = []
        if listitem.getProperty( "additionalListItemProperties" ):
            properties = eval( listitem.getProperty( "additionalListItemProperties" ) )
        
        foundProperty = False
        for property in properties:
            if property[0] == propertyName:
                foundProperty = True
                property[1] = DATA.local( propertyValue )[0]
                listitem.setProperty( propertyName, DATA.local( propertyValue )[2] )
                
        if foundProperty == False:
            properties.append( [propertyName, DATA.local( propertyValue )[0] ] )
            listitem.setProperty( propertyName, DATA.local( propertyValue )[2] )
            
        listitem.setProperty( "additionalListItemProperties", repr( properties ) )
        
    def _remove_additionalproperty( self, listitem, propertyName ):
        # Remove an item from the additional properties of a user item
        properties = []
        hasProperties = False
        if listitem.getProperty( "additionalListItemProperties" ):
            properties = eval( listitem.getProperty( "additionalListItemProperties" ) )
            hasProperties = True
        
        for property in properties:
            if property[0] == propertyName:
                properties.remove( property )
        
        listitem.setProperty( "additionalListItemProperties", repr( properties ) )
            
        listitem.setProperty( propertyName, None )
    
    def warnonremoval( self, item ):
        # This function will warn the user before they modify a settings link
        # (if the skin has enabled this function)
        tree = DATA._get_overrides_skin()
        if tree is None:
            return True
            
        for elem in tree.findall( "warn" ):
            if elem.text.lower() == item.getProperty( "displaypath" ).lower():
                # We want to show the message :)
                message = DATA.local( elem.attrib.get( "message" ) )[2]
                    
                heading = DATA.local( elem.attrib.get( "heading" ) )[2]
                
                dialog = xbmcgui.Dialog()
                return dialog.yesno( heading, message )
                
        return True
        
    def find_default( self, backgroundorwidget, labelID, defaultID ):
        # This function finds the id of an items default background or widget
        tree = DATA._get_overrides_skin()
        if tree is not None:
            if backgroundorwidget == "background":
                elems = tree.getroot().findall( "backgrounddefault" )
            else:
                elems = tree.getroot().findall( "widgetdefault" )
                
            if elems is not None:
                for elem in elems:
                    if elem.attrib.get( "labelID" ) == labelID or elem.attrib.get( "defaultID" ) == defaultID:
                        if "group" in elem.attrib:
                            if elem.attrib.get( "group" ) == self.group:
                                return elem.text
                            else:
                                continue
                        else:
                            return elem.text
                                        
            return None
        
    def _set_label( self, listitem, label ):
        # Update the label, local string and labelID
        listitem.setLabel( label )
        listitem.setProperty( "localizedString", "" )
            
        LIBRARY._rename_playlist( listitem.getProperty( "path" ), label )
            
        # If there's no label2, set it to custom shortcut
        if not listitem.getLabel2():
            listitem.setLabel2( __language__(32024) )
            listitem.setProperty( "shortcutType", "32024" )
            
    # ====================
    # === CLOSE WINDOW ===
    # ====================
    
    def onAction( self, action ):
        if action.getId() in ACTION_CANCEL_DIALOG:
            self._save_shortcuts()
            xbmcgui.Window(self.window_id).clearProperty('groupname')
            self._close()

    def _close( self ):
            self.close()
            