script.skinshortcuts was written with the intention of making user customizable shortcuts on the home page easier for skinners.


What's New for Skinners (version 0.5.2 - beta release)
-----------------------

 - Add additional actions on a per-menu basis - see "Advanced Usage.txt", section "Overrides.xml", part 16
 - Ability to define default folder for thumbnail selection - see "Advanced Usage.txt", section "Overrides.xml", part 13
 - Custom properties based on patterns - see "Advanced Usage.txt", section "Overrides.xml", part 14
 - Ability to build skin xml elements for menu or other items based off of menu items from templates - see "Templates.txt"
 - New widget selection with option to choose from library nodes, add-on nodes, playlists and sources - see "Advanced Usage.txt", section "Overrides.xml", part 4
 - Ability to specify which grouping of available shortcuts will be able to be selected by user - see "Advanced Usage.txt", section "Overrides.xml", part 15
 - Ability to provide a global override - see "Advanced Usage.txt", section "Overrides.xml", part 1.1
 
 
With Thanks - Because their names don't deserve to be at the bottom :)
-----------

Huge thanks to Ronie, whose code for listing plugins is used in this script
Equally huge thanks to Ronie and `Black, for their favourites code used in this script
More huge thanks to BigNoid, for the ability to edit shortcuts, and Jeroen, for so many suggestions each of which just made the script better.
The thanks remain absolutely huge to the translaters on Transifex for localising the script
And the biggest thanks of all to Annie and my family, for feature suggestions, testing and shouting at me when I broke things

 
Where To Get Help
-----------------

Though hopefully Skin Shortcuts will save you a lot of code in the long-run, it is a relatively complex script and so may take some time to get your head around.

To help, it includes several documentation files:
	readme.txt - This file, containing basics of integrating the script
	
	resources/Advanced Usage.txt - Contains details of more advanced configuration and customisation
	
	resources/Management Dialog.txt - Contains details of how to skin the management dialog the user will interact with to manage their shortcuts
	
	resources/labelID.txt - Contains details of the labelID system used to identify shortcuts.

It's highly recommended to take time to read through all of these documents before you begin.

If you require any assistance post on the XBMC forum and I'll do my best to assist:

	http://forum.xbmc.org/showthread.php?tid=178294

 
Before you Begin
----------------

Before you can add the script to your skin, you have a decisions to make:
			
How much of the menu system should the script manage?
	- Main menu and sub-menus
	- Main menu and sub-menus, with additional menus unlinked to main menu
	- Sub-menus only
			
Whichever menu system you choose, details of how to implement are below. Below that, there is information common to all methods which you should also read.

You may also choose to manage menu's yourself, and just make use of the scripts list of available shortcuts. For more details see "Just Select Shortcuts" in Advanced Usage.txt


Main menu and sub-menus
-----------------------

1. Include the necessary file, and run the script

This method runs the script when the user enters the main menu or has left the skin settings. If a change to the menu has been made, it writes out the new menu into a file that you can <include> in your skin.

*IMPORTANT*

Using this method WILL WRITE AN ADDITIONAL FILE TO YOUR SKINS DIRECTORY called script-skinshortcut-includes.xml!

First the file must be imported - in your includes.xml add the line:

	<include file="script-skinshortcuts-includes.xml"/>
	
In home.xml, add the line:

	<onload>RunScript(script.skinshortcuts,type=buildxml&amp;mainmenuID=9000&amp;levels=0)</onload>

And in skinsettings.xml, the line:

	<onunload>RunScript(script.skinshortcuts,type=buildxml&amp;mainmenuID=9000&amp;levels=0)</onunload>
	
Replace 9000 with the ID of the list you are using for the mainmenu. If you are providing more than one sub-menu per main-menu item, increase the levels accordingly.


2. Let users manage their main menu and sub-menu shortcuts
 
Create a button with the following onclick method:

	RunScript(script.skinshortcuts,type=manage&amp;group=mainmenu)
	
Then, within the management dialog, provide a button with the id 405 to let the user manage sub-menus.

You may also want to provide a reset button, with the following in the onclick method:

	RunScript(script.skinshortcuts,type=resetall)
 
 
3. Display main menu and shortcuts
 
This details the simplest method of displaying main menu and sub-menus, using two lists. When the user focuses on an item in the main menu list, the sub-menu list will update with the shortcuts for that item.
  
In the list where you want the main menu to appear, put the following in the <content> tag:
 
	<include>skinshortcuts-mainmenu</include>
   
In the list where you want the sub-menu to appear, put the following in the <content> tag:
 
	<include>skinshortcuts-submenu</include>
   
Remember to replace 9000 with the id of the list you are using for the main menu.

To provide additional sub-menus, use the following in the <content> tag:

	<include>skinshortcuts-submenu-[level]</include>
	
Where level indicates the additional submenu level, starting from 1.


Main menu and sub-menus, with additional menus unlinked to main menu
-----------------------

Same as the method above, except you need to include the &amp;group= parameter to the build line, with mainmenu as the first group, and any additional groups you wish built separated by a pipe - | - symbol.

	<onload>RunScript(script.skinshortcuts,type=buildxml&amp;mainmenuID=9000&amp;group=mainmenu|[groupname]|[groupname]|[groupname])</onload>
	
To the let user manage shortcuts in these additional groups

	RunScript(script.skinshortcuts,type=manage&amp;group=[groupname])
	
And to display them:

	<include>skinshortcuts-group-[groupname]</include>


Sub-menu Only
-------------

1. Include the necessary file, and run the script

This method runs the script when the user enters the main menu or has left the skin settings. If a change to the menu has been made, it writes out the new menu into a file that you can <include> in your skin.

*IMPORTANT*

Using this method WILL WRITE AN ADDITIONAL FILE TO YOUR SKINS DIRECTORY called script-skinshortcut-includes.xml!

First the file must be imported - in your includes.xml add the line:

	<include file="script-skinshortcuts-includes.xml"/>
	
In home.xml, add the line:

	<onload>RunScript(script.skinshortcuts,type=buildxml&amp;mainmenuID=9000&amp;group=[groupname]|[groupname]|[groupname])</onload>

And in skinsettings.xml, the line:

	<onunload>RunScript(script.skinshortcuts,type=buildxml&amp;mainmenuID=9000&amp;group=[groupname]|[groupname]|[groupname])</onunload>
	
Replace 9000 with the ID of the list you are using for the mainmenu. You should include all [groupname]'s that your skin supports, separated by a pipe. The script will then load all of the submenus, and set visibility conditions on each one.

If you are not using a main menu to choose which display group to show, you must still include a mainmenuID value, but can set this to any random value. If you want to build the mainmenu and additional standalone groups, include the group parameter with "mainmenu" as the first option.


2. Let the user manage their shortcuts

In your skinsettings.xml file, you need to create a button for each [groupname] that you want to support, with the following in the <onclick> tag
 
	RunScript(script.skinshortcuts,type=manage&amp;group=[groupname])

	
3. Display user shortcuts (single group)
 
In the list where you want the submenu to appear, put the following in the <content> tag:
 
	<include>skinshortcuts-group-[groupname]</include>
	
	
4. Display user shortcuts based on another list

If your skin provides a main menu you want to display shortcuts for using the script, then you need to add an additional property to each main menu item, called submenuVisibility, containing the [groupname] of the submenu to associate with that main menu.

	<property name="submenuVisibility">[groupname]</property>
	
Then, in the list where you want the submenu to appear, put the following in the <content> tag:

	<include>skinshortcuts-submenu</include>
	
	
Resetting All Shortcuts
-----------------------

It is recommended you give users the option to reset all shortcuts to defaults within your skinsettings.xml, by including a button with the following onclick:

	<onclick>RunScript(script.skinshortcuts,type=resetall)</onclick>
	
You can also include &amp;warning=false if your skin provides a warning to the user before this action is carried out.


Recommended [groupname]'s
-------------------------

If providing only the sub-menu, the script uses the [groupname] property to determind which set of shortcuts to show. In order to share users customized shortcuts across different skins using this script, there are a few recommended [groupname]'s to use:

	videos
	movies
	tvshows
	livetv
	radio		(Helix)
	music
	musicvideos
	pictures
	weather
	programs
	dvd
	settings
	
	
Providing default shortcuts
---------------------------

If the user has not already selected any shortcuts or if the user resets shortcuts, the script will first attempt to load defaults from a file provided by the skin before trying to load its own.

To provide this optional file, create a new sub-directory in your skin called 'shortcuts', and drop the relevant [groupname].DATA.xml file into it, or create the file manually. See "Recommended [groupname]'s" for ideas of some of the default files you may wish to provide, along with mainmenu.DATA.xml if you are using the script to manage the main menu.

The script provides defaults equivalent to Confluence's main menu and sub-menus.

The file format is simple enough:

<?xml version='1.0' encoding='UTF-8'?>
<shortcuts>
	<shortcut>
		<label>Label of shortcut, can be localised string</label>
		<label2>Type of shortcut, can be localised string</label2>
		<icon>The default icon to use for the shortcut</icon>
		<thumb>The default thumbnail to use for the shortcut (optional)</thumb>
		<action>The default action of the shortcut</action>
		<visible>Visibility condition for the shortcut (optional)</visible>
		<defaultID>The defaultID of the menu item</defaultID>
	</shortcut>
</shortcuts

If you want to provide a default which links to a playlist you include with your skin, then make sure the .DATA.xml file uses the special protocol (e.g. special://skin/) as the URI to it. The script will replace this with a localised version, so that the playlist link will continue to work even if the user switches to another skin supporting skin shortcuts.

Particularly when setting defaults for the main menu, you may wish to include the <defaultID> element. This will set the [groupname] of the submenu's .DATA.xml file. (e.g. defaultID of tvshows -> tvshows.DATA.xml). Please note, any defaultID must follow the standard rules - all lowercase, all english characters, no spaces.

As Skin Shortcuts will integrate with a Video Node Editor script (for skins where the script manages the full menu), it is strongly suggested that default shortcuts use video nodes rather than library shortcuts where appropriate.

For example, a link to movie titles should be:

	<action>ActivateWindow(10025,videodb://movies/titles/,return)</action>
	
rather than

	<action>ActivateWindow(10025,MovieTitles,return)</action>
	
This will ensure that any users modifying the default Movies node (for example) will access the content they expect rather than, as video library links provide, all content.
	
	
Properties returned
-------------------

Regardless of which method you use, the script will always return a list with a few standard properties:

	Label		Label of the item (localized where possible)
	Label2		Type of shortcut
	Icon		Icon image
	Thumbnail	Thumbnail image
	Property(labelID)	Unlocalized string used for sub-menu and for displaying more controls depending on the main menu item
	property(defaultID)	Permanent form of the labelID
	Property(action)	The action that will be run when the shortcut is selected
	Property(group)		The [groupname] that this shortcut is listed from
	Property(widget)	If your skin uses Skin Shortcuts to manage widgets, the [widgetID] will appear here (mainmenu only)
	Property(widgetName)        - The display name of the widget will appear here
	Property(background)If your skin uses Skin Shortcuts to manage background, the [backgroundID] will appear here (mainmenu only)
	Property(backgroundName)    - The display name of the widget will appear here

You can also use the script to manage any additional properties you would like. See "resources/Management Dialog.txt" and "resources/Advanced Usage.txt" - "Overrides.xml" - section 5 (Custom shortcut properties)

Note, there is an additional property - Property(hasSubmenu) - which is used by the methods used for building all menus in a single list. It should not be relied upon to determine whether there are any visible items in a submenu, as it will still return True if all items within the submenu are invisible. Suggestion is to use Container.NumItems to determine this information instead.
	
	
Display more controls depending on the mainmenu item
----------------------------------------------------

If your skin provides widgets or has custom backgrounds, you can use Skin Shortcuts to manage these. See resources/Advanced Usage.txt

Otherwise, you can set visibility of any additional controls based on the labelID of the main menu listitems property 'labelID' or 'defaultID'. For common main menu items, it will contain one of the following strings:
	videos
	movies
	tvshows
	livetv
	radio		(Helix)
	music
	musicvideos
	pictures
	weather
	programs
	dvd
	settings
   
A full list of labelID/defaultID's can be found in the Resources folder.


Providing alternative access to settings
----------------------------------------

One of the side effects of using skinshortcuts to provide the whole main menu is that users have the ability to delete any shortcut, including those that they will later turn out to actually want. Generally, this isn't a problem as they can add them back at any time. However if they delete all links to settings, they will have no way to add it back unless your skin offers an alternative access.

Therefore, you should either include an alternative link to settings such as in the skins shutdown menu, or you can force settings to always be included in the menu - see "Advanced Usage", section "Overrides.xml" part 11

It's possible to warn the user before they remove a link to shortcuts - see "Advanced Usage", section "Overrides.xml" part 10


Skinning the management dialog
------------------------------

For details on skinning the management dialog, see resources/Management Dialog.txt.


Targeting Helix (Kodi 14)
-------------------------

There are no required changes to your implementation of Skin Shortcuts to target the upcoming release of Kodi. However, you may wish to update your PVR defaults to use the new implementation (full details on the skinning forum).

It is possible to include defaults for both Gotham and Helix in your defaults if you use the new XML format. Include a <version/> tag, with the value of either 13 or 14 for the version of Kobi for which it applies:

	<version>13</version>  - A default for Gotham
	<version>14</version>  - A default for Helix

If you have customised the order of available shortcuts in your overrides.xml, it is also possible to target both Gotham and Helix by displaying different nodes depending on the version by including the version in the node tag:

	<node label="32017" condition="System.GetBool(pvrmanager.enabled)" version="13">  - A node for Gotham
	<node label="32017" condition="PVR.HasTVChannels" version="14">  - A node for Helix


Advanced Usage
--------------

The script includes a number of options and features to try to make a skinners life easier, and to help customise the script for your skin.

* Manage backgrounds
* Manage widgets
* Overrides.xml
* Localisation

For details, see "resources/Advanced Usage.txt".
