'''

	The settings for OSMC are handled by the OSMC Settings Addon (OSA).

	In order to more easily accomodate future changes and enhancements, each OSMC settings bundle (module) is a separate addon.
	The module can take the form of an xbmc service, an xbmc script, or an xbmc module, but it must be installed into the users'
	userdata/addons folder.

	The OSA leverages the settings interface provided by XBMC. Each addon has its own individual settings defined in a
	settings.xml file located in the addon's resources/ folder.

	The OSG detects changes to the settings by identifying the differences between a newly read settings.xml and the values from 
	a previously read settings.xml.

	The values of the settings displayed by the OSG are only ever populated by the items in the settings.xml. [Note: meaning that 
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

		reboot_required					: A boolean to declare if the OS needs to be rebooted. If a change in a specific setting 
										  requires an OS reboot to take affect, this is flag that will let the OSG know.

		setting_data_method 			: This dictionary contains:
												- the name of all settings in the module
												- the current value of those settings
												- [optional] a method to call for each setting when the value changes



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

	Settings changes are applied when the OSG is called to close. But this behaviour can be changed to occur when the addon
	settings window closes by editing the open_settings_window. The method apply_settings will still be called by OSG, so 
	keep that in mind.

'''
# Standard Modules
from collections import namedtuple
import sys
import os
import threading

# XBMC Modules
import xbmc
import xbmcaddon
import xbmcgui

addonid = "script.module.osmcsetting.pioverclock"
__addon__  = xbmcaddon.Addon(addonid)
DIALOG     = xbmcgui.Dialog()

# Custom modules
sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon(addonid).getAddonInfo('path'), 'resources','lib')))

# OSMC SETTING Modules
import config_tools as ct
from gui import overclock_gui


def log(message):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )
		
	xbmc.log('OSMC PI OVERCLOCK ' + str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


def is_pi_zero():

	try:
		with open('/proc/cpuinfo','r') as f:
			lines = f.readlines()
			for line in lines:
				if line[0:8]=='Revision':
					myrevision = line[11:len(line)-1]
			else:
				raise
	except:
		myrevision = "0000"

	try: # Pi2 revision starts with 'a'
		revisionInt = int(myrevision, 16)

	except ValueError:
		return False

	if revisionInt >> 23 & 1 == True:
		if (revisionInt >> 4) & 0X7FF == 9:
			raise

	return False


class OSMCSettingClass(threading.Thread):

	''' 
		A OSMCSettingClass is way to substantiate the settings of an OSMC settings module, and make them available to the 
		OSMC Settings Addon (OSA).

	'''

	def __init__(self):

		''' 
			The setting_data_method contains all the settings in the settings group, as well as the methods to call when a
			setting_value has changed and the existing setting_value. 
		'''

		is_pi_zero()

		super(OSMCSettingClass, self).__init__()

		self.addonid = "script.module.osmcsetting.pioverclock"

		# this is what is displayed in the main settings gui
		self.shortname = 'Overclock'
		
		self.description = 	"""The Raspberry Pi can often be overclocked to improve performance. Overclock configuration parameters are stored in the "config.txt" file. For more detail, visit http://elinux.org/RPiconfig[CR]
This module provides three recommended overclocking profiles but also allows you to edit the overclock settings in your config.txt.

The module allows you to manually adjust:
- arm_freq
- sdram_freq
- core_freq
- initial_turbo
- over_voltage
- over_voltage_sdram
- force_turbo
"""

		# a flag to determine whether a setting change requires a reboot to take effect
		self.reboot_required = False

		with open('/proc/cpuinfo', 'r') as f:
			lines = f.readlines()

		cpu_count = sum([1 for x in lines if x.startswith('processor')])

		if cpu_count == 1:
			self.pimodel = 'PiB'
		else:
			self.pimodel = 'Pi2'

		log('Model = %s' % self.pimodel)
		
		log('START')


	def run(self):

		'''
			The method that determines what happens when the item is clicked in the settings GUI.
			Usually this would be __addon__.OpenSettings(), but it could be any other script.
			This allows the creation of action buttons in the GUI, as well as allowing developers to script and skin their 
			own user interfaces.
		'''

		log(xbmcaddon.Addon("script.module.osmcsetting.pioverclock").getAddonInfo('id'))

		me = xbmcaddon.Addon(self.addonid)
		scriptPath = me.getAddonInfo('path')

		# read config file, take the parts you need

		# the location of the config file FOR TESTING ONLY
		try:								
			self.test_config = '/boot/config.txt'
			self.config_settings = ct.read_config(self.test_config)

		except:
			self.test_config = '/home/kubkev/Documents/config.txt'
			self.config_settings = ct.read_config(self.test_config)

		oc_keys = ['arm_freq', 'sdram_freq', 'core_freq', 'initial_turbo', 'over_voltage', 'over_voltage_sdram', 'force_turbo']

		self.setting_values = {}
		for key in oc_keys:
			if key in self.config_settings:
				self.setting_values[key] = self.config_settings[key]

		# setting_values = {'core_freq': 500, 'arm_freq': 800, 'sdram_freq': 700, 'initial_turbo': 60, 'over_voltage': 2, 'over_voltage_sdram': 6, 'force_turbo' : 0}

		xml = "new_gui_720.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "new_gui.xml"

		self.GUI = overclock_gui(xml, scriptPath, 'Default', setting_values=self.setting_values, model=self.pimodel)

		self.GUI.doModal()

		self.new_settings = self.GUI.snapshot()
		log('self.new_settings')
		log(self.new_settings)
		log('self.setting_values')
		log(self.setting_values)

		ct.write_config(self.test_config, self.new_settings)

		del self.GUI

		for k, s in self.new_settings.iteritems():
			if s != self.setting_values.get(k, 'no setting available'):
				self.reboot_required

		# dialog to notify the user they should restart for changes to take effect
		ok = DIALOG.notification(lang(32107), lang(32108))

		log('END')


	def apply_settings(self):

		'''
			This method will apply all of the settings. It calls the first_method, if it exists. 
			Then it calls the method listed in setting_data_method for each setting. Then it calls the
			final_method, again, if it exists.
		'''

		pass


if __name__ == "__main__":
	pass
