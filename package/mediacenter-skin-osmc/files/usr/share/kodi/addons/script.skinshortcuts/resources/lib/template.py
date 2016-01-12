# coding=utf-8
import os
import xbmc, xbmcaddon, xbmcvfs
import xml.etree.ElementTree as xmltree
import hashlib, hashlist
import copy
from traceback import print_exc

__addon__        = xbmcaddon.Addon()
__addonid__      = __addon__.getAddonInfo('id').decode( 'utf-8' )
__xbmcversion__  = xbmc.getInfoLabel( "System.BuildVersion" ).split(".")[0]
__skinpath__     = xbmc.translatePath( "special://skin/shortcuts/" ).decode('utf-8')

def log(txt):
    if __xbmcversion__ == "13" or __addon__.getSetting( "enable_logging" ) == "true":
        try:
            if isinstance (txt,str):
                txt = txt.decode('utf-8')
            message = u'%s: %s' % (__addonid__, txt)
            xbmc.log(msg=message.encode('utf-8'), level=xbmc.LOGDEBUG)
        except:
            pass

class Template():
    def __init__( self ):
        # Load the skins template.xml file
        templatepath = os.path.join( __skinpath__ , "template.xml" )
        try:
            self.tree = xmltree.parse( templatepath )

            log( "Loaded template.xml file")
            
            # Add the template.xml to the hash file
            self._save_hash( templatepath, xbmcvfs.File( templatepath ).read() )
        except:
            # We couldn't load the template.xml file
            if xbmcvfs.exists( templatepath ):
                # Unable to parse template.xml
                log( "Unable to parse template.xml. Invalid xml?" )
                self._save_hash( templatepath, xbmcvfs.File( templatepath ).read() )
            else:
                # No template.xml            
                self.tree = None
                self._save_hash( templatepath, None )
            
        # Empty variable which will contain our base elementree (passed from buildxml)
        self.includes = None
        
        # List which will contain 'other' elements we will need to finalize (we won't have all the
        # visibility conditions until the end)
        self.finalize = []
            
    def parseItems( self, menuType, level, items, profile, profileVisibility, visibilityCondition, menuName, mainmenuID = None ):
        # This will build an item in our includes for a menu
        if self.includes is None or self.tree is None:
            return
            
        if profile == "test":
            pass
            
        # Get the template for this menu
        if menuType == "mainmenu":
            template = copy.deepcopy( self.tree.find( "mainmenu" ) )
        else:
            template = self.findSubmenu( menuName, level )
            
        if template is not None:
            # Found a template - let's build it
            if menuType == "mainmenu":
                log( "Main menu template found" )
            else:
                log( " - Submenu template found" )
        
            # We need to check that the relevant includes existing
            # First, the overarching include
            includeName = "skinshortcuts-template"
            if "include" in template.attrib:
                includeName += "-%s" %( template.attrib.get( "include" ) )
            
            treeRoot = self.getInclude( self.includes, includeName, profileVisibility, profile )
            includeTree = self.getInclude( self.includes, includeName + "-%s" %( profile ), None, None )
            
            # Now replace all <skinshortcuts> elements with correct data
            self.replaceElements( template, visibilityCondition, profileVisibility, items )
            
            # Add the template to the includes
            for child in template.find( "controls" ):
                includeTree.append( child )
            
        # Now we want to see if any of the main menu items match a template
        if menuType != "mainmenu":
            return
        for item in items:
            # First we need to build the visibilityCondition, based on the items
            # submenuVisibility element, and the mainmenuID
            visibilityName = ""
            for element in item.findall( "property" ):
                if "name" in element.attrib and element.attrib.get( "name" ) == "submenuVisibility":
                    visibilityName = element.text
                    break
            
            visibilityCondition = "StringCompare(Container(" + mainmenuID + ").ListItem.Property(submenuVisibility)," + visibilityName + ")"
            
            # Now find a matching template - if one matches, it will be saved to be processed
            # at the end (when we have all visibility conditions)
            self.findOther( item, profile, profileVisibility, visibilityCondition )
                    
    def writeOthers( self ):
        # This will write any 'other' elements we have into the includes file
        # (now we have all the visibility conditions for them)
        if self.includes is None or self.tree is None:
            return
        
        if len( self.finalize ) == 0:
            return
            
        for template in self.finalize:
            # Get the group name
            name = "skinshortcuts-template"
            if "include" in template.attrib:
                name += "-%s" %( template.attrib.get( "include" ) )
            
            # Loop through any profiles we have
            for profile in template.findall( "skinshortcuts-profile" ):
                visibilityCondition = None
                # Build the visibility condition
                for condition in profile.findall( "visible" ):
                    if visibilityCondition is None:
                        visibilityCondition = condition.text
                    elif condition.text != "":
                        visibilityCondition += " | " + condition.text
                
                # Get the include this will be done under
                root = self.getInclude( self.includes, name, profile.attrib.get( "visible" ), profile.attrib.get( "profile" ) )
                include = self.getInclude( self.includes, "%s-%s" %( name, profile.attrib.get( "profile" ) ), None, None ) #profile.attrib.get( "visible" ) )
                
                # Create a copy of the node with any changes within (this time it'll be visibility)
                final = copy.deepcopy( template )
                self.replaceElements( final, visibilityCondition, profile.attrib.get( "visible" ), [] )
                
                # Add the template to the includes
                for child in final.find( "controls" ):
                    include.append( child )
            
    def getInclude( self, tree, name, condition, profile ):
        # This function gets an existing <include/>, or creates it
        for include in tree.findall( "include" ):
            if include.attrib.get( "name" ) == name:
                if condition is None:
                    return include
                    
                # We've been passed a condition, check there's an include with that
                # as condition and name as text
                for visInclude in include.findall( "include" ):
                    if visInclude.attrib.get( "condition" ) == condition:
                        return include
                        
                # We didn't find condition,so create it
                visInclude = xmltree.SubElement( include, "include" )
                visInclude.set( "condition", condition )
                visInclude.text = name + "-" + profile
                
                return include
        
        # We didn't find the node, so create it
        newInclude = xmltree.SubElement( tree, "include" )
        newInclude.set( "name", name )
        
        # If we've been passed a condition, create an include with that as condition
        # and name as text
        if condition is not None:
            visInclude = xmltree.SubElement( newInclude, "include" )
            visInclude.set( "condition", condition )
            visInclude.text = name + "-" + profile
        
        return newInclude
        
        
    def findSubmenu( self, name, level ):
        # Find the correct submenu template
        returnElem = None
        for elem in self.tree.findall( "submenu" ):
            # Check if the level matched
            if level == 0:
                # No level, so there shouldn't be a level attrib
                if "level" in elem.attrib:
                    continue
            else:
                # There is a level, so make sure there's a level attrib
                if "level" not in elem.attrib:
                    continue
                # Make sure the level values match
                if elem.attrib.get( "level" ) != str( level ):
                    continue
            # If there's a name attrib, check if it matches
            if "name" in elem.attrib:
                if elem.attrib.get( "name" ) == name:
                    # This is the one we want :)
                    return copy.deepcopy( elem )
                else:
                    continue
            # Save this, in case we don't find a better match
            returnElem = elem
            
        return copy.deepcopy( returnElem )
        
    def findOther( self, item, profile, profileVisibility, visibilityCondition ):
        # Find a template matching the item we have been passed
        foundTemplateIncludes = []
        for elem in self.tree.findall( "other" ):
            # Check that we don't already have a template for this include
            includeName = None
            if "include" in elem.attrib:
                includeName = elem.attrib.get( "include" )
            if includeName in foundTemplateIncludes:
                continue

            template = copy.deepcopy( elem )
            matched = True

            # Check whether the skinner has set the match type (whether all conditions need to match, or any)
            matchType = "all"
            matchElem = template.find( "match" )
            if matchElem is not None:
                matchType = matchElem.text.lower()
                if matchType not in [ "any", "all" ]:
                    log( "Invalid <match /> element in template" )
                    matchType = "all"
                elif matchType == "any":
                    matched = False
            
            # Check the conditions
            for condition in template.findall( "condition" ):
                if matchType == "all":
                    if matched == False:
                        break
                    if self.checkCondition( condition, item ) == False:
                        matched = False
                        break
                else:
                    if matched == True:
                        break
                    if self.checkCondition( condition, item ) == True:
                        matched = True
                        break

                
            # If the conditions didn't match, we're done here
            if matched == False:
                continue
                
            # All the rules matched, so next we'll get any properties
            properties = self.getProperties( template, item )
            
            # Next up, we do any replacements - EXCEPT for visibility, which
            # we'll store for later (in case multiple items would have an
            # identical template
            self.replaceElements( template.find( "controls" ), None, None, [], properties )
            
            # Now we need to check if we've already got a template identical to this
            textVersion = None
            foundInPrevious = False
            for previous in self.finalize:
                # Check that the previous template uses the same include
                includeNameCheck = includeName
                if includeName is None:
                    includeNameCheck = "NONE"
                if previous.find( "skinshortcuts-includeName" ).text != includeNameCheck:
                    continue
                    
                # If we haven't already, convert our new template to a string
                if textVersion is None:
                    textVersion = xmltree.tostring( template.find( "controls" ), encoding='utf8' )
                    
                # Compare string representations
                if textVersion == xmltree.tostring( previous.find( "controls" ), encoding='utf8' ):
                    # They are the same
                    
                    # Add our details to the previous version, so we can build it
                    # with full visibility details later
                    for profileMatch in previous.findall( "skinshortcuts-profile" ):
                        if profileMatch.attrib.get( "profile" ) == profile:
                            # Check if we've already added this visibilityCondition
                            for visible in profileMatch.findall( "visible" ):
                                if visible.text == visibilityCondition:
                                    # The condition is already there
                                    return previous
                            
                            # We didn't find it, so add it
                            xmltree.SubElement( profileMatch, "visible" ).text = visibilityCondition
                            return previous
                            
                    # We didn't find this profile, so add it
                    newElement = xmltree.SubElement( previous, "skinshortcuts-profile" )
                    newElement.set( "profile", profile )
                    newElement.set( "visible", profileVisibility )
                    
                    # And save the visibility condition
                    xmltree.SubElement( newElement, "visible" ).text = visibilityCondition
                    
                    # And we're done
                    foundTemplateIncludes.append( includeName )
                    foundInPrevious = True

            if foundInPrevious == False:
                # We don't have this template saved, so add our profile details to it
                newElement = xmltree.SubElement( template, "skinshortcuts-profile" )
                newElement.set( "profile", profile )
                newElement.set( "visible", profileVisibility )

                # Save the visibility condition
                xmltree.SubElement( newElement, "visible" ).text = visibilityCondition

                newElement = xmltree.SubElement( template, "skinshortcuts-includeName" )
                if includeName is None:
                    newElement.text = "NONE"
                else:
                    newElement.text = includeName
                
                # Add it to our finalize list
                self.finalize.append( template )

                # Add that we've found a template for this include
                foundTemplateIncludes.append( includeName )

                log( " - Other template found" )
            
    def checkCondition( self, condition, items ):
        # Check if a particular condition is matched for an 'other' template
        if "tag" not in condition.attrib:
            # Tag attrib is required
            return False
        else:
            tag = condition.attrib.get( "tag" )
            
        attrib = None
        if "attribute" in condition.attrib:
            attrib = condition.attrib.get( "attribute" ).split( "|" )
            
        # Find all elements with matching tag
        matchedRule = False
        for item in items.findall( tag ):
            if attrib is not None:
                if attrib[ 0 ] not in item.attrib:
                    # Doesn't have the attribute we're looking for
                    continue
                if attrib[ 1 ] != item.attrib.get( attrib[ 0 ] ):
                    # This property doesn't match
                    continue
                    
            if condition.text is not None and item.text != condition.text:
                # This property doesn't match
                continue
            
            # The rule has been matched :)
            return True
            
        return False
        
    def getProperties( self, elem, items ):
        # Get any properties specified in an 'other' template
        properties = {}
        for property in elem.findall( "property" ):
            value = None
            if "name" not in property.attrib or property.attrib.get( "name" ) in properties:
                # Name attrib required, or we've already got a property with this name
                continue
            name = property.attrib.get( "name" )
            if "tag" in property.attrib:
                tag = property.attrib.get( "tag" )
            else:
                # No tag property, so this will always match (so let's just use it!)
                if property.text:
                    properties[ name ] = property.text
                else:
                    properties[ name ] = ""
                continue

            if "attribute" in property.attrib and "value" not in property.attrib:
                attrib = property.attrib.get( "attribute" ).split( "|" )
            else:
                attrib = property.attrib.get( "attribute" ).split( "|" )
                value = property.attrib.get( "value" )
                
            # Let's get looking for any items that match
            for item in items.findall( tag ):
                if attrib is not None:
                    if attrib[ 0 ] not in item.attrib:
                        # Doesn't have the attribute we're looking for
                        continue
                    if attrib[ 1 ] != item.attrib.get( attrib[ 0 ] ):
                        # The attributes value doesn't match
                        continue

                if not item.text:
                    # The item doesn't have a value to match
                    continue
                        
                if value is not None and item.text != value:
                    # The value doesn't match
                    continue
                    
                # We've matched a property :)
                if property.text:
                    properties[ name ] = property.text
                else:
                    properties[ name ] = item.text
                break
        
        return properties
    
    def replaceElements( self, tree, visibilityCondition, profileVisibility, items, properties = {} ):
        for elem in tree:
            # <tag skinshortcuts="visible" /> -> <tag condition="[condition]" />
            if "skinshortcuts" in elem.attrib:
                # Get index of the element
                index = list( tree ).index( elem )
                
                # Get existing attributes, text and tag
                attribs = []
                for singleAttrib in elem.attrib:
                    if singleAttrib == "skinshortcuts":
                        type = elem.attrib.get( "skinshortcuts" )
                    else:
                        attribs.append( ( singleAttrib, elem.attrib.get( singleAttrib ) ) )
                text = elem.text
                tag = elem.tag
                
                # Don't continue is type = visibility, and no visibilityCondition
                if type == "visibility" and visibilityCondition is None:
                    continue
                
                # Remove the existing element
                tree.remove( elem )
                
                # Make replacement element
                newElement = xmltree.Element( tag )
                if text is not None:
                    newElement.text = text
                for singleAttrib in attribs:
                    newElement.set( singleAttrib[ 0 ], singleAttrib[ 1 ] )
                    
                # Make replacements
                if type == "visibility" and visibilityCondition is not None:
                    newElement.set( "condition", visibilityCondition )
                    
                # Insert it
                tree.insert( index, newElement )
            
            # <tag>$skinshortcuts[var]</tag> -> <tag>[value]</tag>
            # <tag>$skinshortcuts[var]</tag> -> <tag><include>[includeName]</include></tag> (property = $INCLUDE[includeName])
            if elem.text is not None and elem.text.startswith( "$SKINSHORTCUTS[" ):
                newValue = ""
                if elem.text[ 15:-1 ] in properties:
                    newValue = properties[ elem.text[ 15:-1 ] ]
                if newValue.startswith( "$INCLUDE[" ):
                    # Remove text property
                    elem.text = ""
                    # Add include element
                    includeElement = xmltree.SubElement( elem, "include" )
                    includeElement.text = newValue[ 9:-1 ]
                else:
                    # Replace the text with the property value
                    elem.text = newValue
            
            # <tag attrib="$skinshortcuts[var]" /> -> <tag attrib="[value]" />
            for attrib in elem.attrib:
                value = elem.attrib.get( attrib )
                if value.startswith( "$SKINSHORTCUTS[" ) and value[ 15:-1 ] in properties:
                    newValue = ""
                    if value[ 15:-1 ] in properties:
                        newValue = properties[ value[ 15:-1 ] ]
                    elem.set( attrib, newValue )
            
            # <skinshortcuts>visible</skinshortcuts> -> <visible>[condition]</visible>
            # <skinshortcuts>items</skinshortcuts> -> <item/><item/>...
            if elem.tag == "skinshortcuts":
                # Get index of the element
                index = list( tree ).index( elem )
                
                # Get the type of replacement
                type = elem.text
                
                # Don't continue is type = visibility, and no visibilityCondition
                if type == "visibility" and visibilityCondition is None:
                    continue
                
                # Remove the existing element
                tree.remove( elem )
                
                # Make replacements
                if type == "visibility" and visibilityCondition is not None:
                    # Create a new visible element
                    newelement = xmltree.Element( "visible" )
                    newelement.text = visibilityCondition
                    # Insert it
                    tree.insert( index, newelement )
                elif type == "items":
                    # Firstly, go through and create an array of all items in reverse order, without
                    # their existing visible element, if it matches our visibilityCondition
                    newelements = []
                    if items == []:
                        break
                    for item in items.findall( "item" ):
                        newitem = copy.deepcopy( item )

                        # Remove the existing visible elem from this
                        for visibility in newitem.findall( "visible" ):
                            if visibility.text != profileVisibility:
                                continue
                            newitem.remove( visibility )
                        
                        # Add a copy to the array
                        newelements.insert( 0, newitem )
                    if len( newelements ) != 0:
                        for elem in newelements:
                            # Insert them into the template
                            tree.insert( index, elem )
            else:
                # Iterate through tree
                self.replaceElements( elem, visibilityCondition, profileVisibility, items, properties )
            
    def _save_hash( self, filename, file ):
        if file is not None:
            hasher = hashlib.md5()
            hasher.update( file )
            hashlist.list.append( [filename, hasher.hexdigest()] )
        else:
            hashlist.list.append( [filename, None] )            