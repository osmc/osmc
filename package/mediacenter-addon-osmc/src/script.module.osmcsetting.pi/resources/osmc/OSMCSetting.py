'''

	The settings for OSMC are handled by the OSMC Settings Addon (OSA).

	In order to more easily accomodate future changes and enhancements, each OSMC settings bundle (module) is a separate addon.
	The module can take the form of an xbmc service, an xbmc script, or an xbmc module, but it must be installed into the users'
	/usr/share/kodi/addons folder.

	The OSA collects the modules it can find, loads their icons, and launches them individually when the user clicks on an icon.

	The modules can either have their own GUI, or they can leverage the settings interface provided by XBMC. If the OSG uses the XBMC 
	settings interface, then all of their settings must be stored in the addons settings.xml. This is true even if the source of record
	is a separate config file.

	An example of this type is the Pi settings module; the actual settings are read from the config.txt, then written to the 
	settings.xml for display in kodi, then finally all changes are written back to the config.txt. The Pi module detects user 
	changes to the settings by identifying the differences between a newly read settings.xml and the values from a previously 
	read settings.xml.

	The values of the settings displayed by this module are only ever populated by the items in the settings.xml. [Note: meaning that 
	if the settings data is retrieved from a different source, it will need to be populated in the module before it is displayed
	to the user.]

	Each module must have in its folder, a sub-folder called 'resources/osmc'. Within that folder must reside this script (OSMCSetting.py), 
	and the icons to be used in the OSG to represent the module (FX_Icon.png and FO_Icon.png for unfocused and focused images
	respectively).

	When the OSA creates the OSMC Settings GUI (OSG), these modules are identified and the OSMCSetting.py script in each of them 
	is imported. This script provides the mechanism for the OSG to apply the changes required from a change in a setting.

	The OSMCSetting.py file must have a class called OSMCSettingClass as shown below.

	The key variables in this class are:

		addonid							: The id for the addon. This must be the id declared in the addons addon.xml.

		description 					: The description for the module, shown in the OSA

		reboot_required					: A boolean to declare if the OS needs to be rebooted. If a change in a specific setting 
									 	  requires an OS reboot to take affect, this is flag that will let the OSG know.

		setting_data_method 			: This dictionary contains:
												- the name of all settings in the module
												- the current value of those settings
												- [optional] apply - a method to call for each setting when the value changes
												- [optional] translate - a method to call to translate the data before adding it to the 
												  setting_data_method dict. The translate method must have a 'reverse' argument which 
												  when set to True, reverses the transformation.  



	The key methods of this class are:

		open_settings_window			: This is called by the OSG when the icon is clicked. This will open the settings window.
										  Usually this would be __addon__.OpenSettings(), but it could be any other script.
										  This allows the creation of action buttons in the GUI, as well as allowing developers 
										  to script and skin their own user interfaces.

		[optional] first_method			: called before any individual settings changes are applied.

		[optional] final_method			: called after all the individual settings changes are done.

		[optional] boot_method			: called when the OSA is first started.

		apply_settings					: This is called by the OSG to apply the changes to any settings that have changed.
										  It calls the first setting method, if it exists. 
										  Then it calls the method listed in setting_data_method for each setting. Then it 
										  calls the final method, again, if it exists.

		populate_setting_data_method	: This method is used to populate the setting_data_method with the current settings data.
										  Usually this will be from the addons setting data stored in settings.xml and retrieved
										  using the settings_retriever_xml method.

										  Sometimes the user is able to edit external setting files (such as the Pi's config.txt).
										  If the developer wants to use this source in place of the data stored in the
										  settings.xml, then they should edit this method to include a mechanism to retrieve and 
										  parse that external data. As the window shown in the OSG populates only with data from 
										  the settings.xml, the developer should ensure that the external data is loaded into that
										  xml before the settings window is opened.

		settings_retriever_xml			: This method is used to retrieve all the data for the settings listed in the 
										  setting_data_method from the addons settings.xml.

	The developer is free to create any methods they see fit, but the ones listed above are specifically used by the OSA. 
	Specifically, the apply_settings method is called when the OSA closes. 

	Settings changes are applied when the OSG is called to close. But this behaviour can be changed to occur when the addon
	settings window closes by editing the open_settings_window. The method apply_settings will still be called by OSA, so 
	keep that in mind.

'''


# XBMC Modules
import xbmc
import xbmcaddon
import xbmcgui

# STANDARD Modules
import subprocess
import sys
import os
import threading
import traceback

addonid = "script.module.osmcsetting.pi"
__addon__  = xbmcaddon.Addon(addonid)
DIALOG     = xbmcgui.Dialog()

# Custom modules
sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon(addonid).getAddonInfo('path'), 'resources','lib')))

# OSMC SETTING Modules
import OSMC_REparser as parser


def lang(id):
    san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
    return san 


def log(message):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )
		
	xbmc.log('OSMC PI ' + str(message), level=xbmc.LOGDEBUG)


class OSMCSettingClass(threading.Thread):

	''' 
		A OSMCSettingClass is way to substantiate the settings of an OSMC settings module, and make them available to the 
		OSMC Settings Addon (OSA).

	'''

	def __init__(self):

		''' 
			The MASTER_SETTINGS contains all the settings in the settings group, as well as the methods to call when a
			setting_value has changed and the existing setting_value. 
		'''

		super(OSMCSettingClass, self).__init__()

		self.addonid = addonid
		self.me = xbmcaddon.Addon(self.addonid)

		# this is what is displayed in the main settings gui
		self.shortname = 'Pi Config'

		self.description = 	"""This is the text that is shown on the OSG. [CR][CR]It should describe:[CR]   - what the settings module is for,[CR]   - the settings it controls,[CR]   - and anything else you want, I suppose."""

		self.description = 	"""The Raspberry Pi doesn't have a conventional BIOS. System configuration parameters are stored in a "config.txt" file. For more detail, visit http://elinux.org/RPiconfig[CR]
This settings module allows you to edit your config.txt from within OSMC using a graphical interface.

The module includes:
- display rotation
- hdmi_safe & hdmi_boost
- hdmi_group & hdmi_mode
- function to save edid to file
- sdtv_mode & sdtv_aspect
- GPU memory split
- MPG2 & WVC1 licences (including status)
- your Pi's serial number

Finally, there is a Config Editor that will allow you to quickly add, edit, or delete lines in your config.txt.

Overclock settings are set using the Pi Overclock module."""


		# the location of the config file FOR TESTING ONLY
		try:								
			self.config_location = '/boot/config.txt'

			self.populate_misc_info()

		except:

			# if anything fails above, assume we are testing and look for the config
			# in the testing location
			self.config_location = '/home/plaskev/Documents/config.txt'

		try:
			self.clean_user_config()
		except Exception:

			log('Error cleaning users config')
			log(traceback.format_exc())


	def run(self):

		'''
			The method determines what happens when the item is clicked in the settings GUI.
			Usually this would be __addon__.OpenSettings(), but it could be any other script.
			This allows the creation of action buttons in the GUI, as well as allowing developers to script and skin their 
			own user interfaces.
		'''

		# read the config.txt file everytime the settings are opened. This is unavoidable because it is possible for
		# the user to have made manual changes to the config.txt while OSG is active.
		config = parser.read_config_file(self.config_location)

		extracted_settings = parser.config_to_kodi(parser.MASTER_SETTINGS, config)

		# load the settings into kodi
		log('Settings extracted from the config.txt')
		for k, v in extracted_settings.iteritems():

			log("%s : %s" % (k, v))
			self.me.setSetting(k, str(v))

		# open the settings GUI and let the user monkey about with the controls
		self.me.openSettings()

		# retrieve the new settings from kodi 
		new_settings = self.settings_retriever_xml()

		log('New settings applied to the config.txt')
		for k, v in new_settings.iteritems():
			log("%s : %s" % (k, v))

		# read the config into a list of lines again
		config = parser.read_config_file(self.config_location)

		# construct the new set of config lines using the protocols and the new settings
		new_settings = parser.kodi_to_config(parser.MASTER_SETTINGS, config, new_settings)

		# write the new lines to the temporary config file
		parser.write_config_file('/var/tmp/config.txt', new_settings)

		# copy over the temp config.txt to /boot/ as superuser
		subprocess.call(["sudo", "mv",  '/var/tmp/config.txt', self.config_location])

		ok = DIALOG.notification(lang(32095), lang(32096))


	def apply_settings(self):

		pass 


	def settings_retriever_xml(self):

		''' 
			Reads the stored settings (in settings.xml) and returns a dictionary with the setting_name: setting_value. This 
			method cannot be overwritten.
		'''

		latest_settings = {}

		addon = xbmcaddon.Addon(self.addonid)

		for key in parser.MASTER_SETTINGS.keys():

			latest_settings[key] = addon.getSetting(key)

		return latest_settings


	def populate_misc_info(self):

		# grab the Pi serial number and check to see whether the codec licences are enabled
		mpg = subprocess.check_output(["/opt/vc/bin/vcgencmd", "codec_enabled", "MPG2"])
		wvc = subprocess.check_output(["/opt/vc/bin/vcgencmd", "codec_enabled", "WVC1"])
		serial_raw = subprocess.check_output(["cat", "/proc/cpuinfo"])

		# grab just the serial number
		serial = serial_raw[serial_raw.index('Serial') + len('Serial'):].replace('\n','').replace(':','').replace(' ','').replace('\t','')

		# load the values into the settings gui
		__addon__.setSetting('codec_check', mpg.replace('\n','') + ', ' + wvc.replace('\n',''))
		__addon__.setSetting('serial', serial)


	def clean_user_config(self):
		''' Comment out problematic lines in the users config.txt '''

		patterns = [

			r".*=.*\[remove\].*", 
			r".*=remove",
		]

		config = parser.read_config_file(self.config_location)

		new_config = parser.clean_config(config, patterns)

		# write the new lines to the temporary config file
		parser.write_config_file('/var/tmp/config.txt', new_config)

		# copy over the temp config.txt to /boot/ as superuser
		subprocess.call(["sudo", "mv",  '/var/tmp/config.txt', self.config_location])


if __name__ == "__main__":
	pass

