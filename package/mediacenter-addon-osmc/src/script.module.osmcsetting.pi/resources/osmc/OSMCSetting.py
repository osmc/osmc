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
import subprocess
import sys
import os
import threading

addonid = "script.module.osmcsetting.pi"
__addon__  = xbmcaddon.Addon(addonid)
DIALOG     = xbmcgui.Dialog()

# Custom modules
sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon(addonid).getAddonInfo('path'), 'resources','lib')))

# OSMC SETTING Modules
import OSMC_ConfigParser as ct
from CompLogger import comprehensive_logger as clog


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
			The pi_settings_dict contains all the settings in the settings group, as well as the methods to call when a
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



		self.not_going_to_config = [	'store_hdmi_to_file',
										'gpu_mem',
										]

		self.values_set_elsewhere = [	'hdmi_edid_file',
										'hdmi_force_hotplug',
										]

		# The setting_value in this dict is what is used in the settings.xml. The need to be translated from any external source,
		# line the config.txt, and then translated again for writing back.
		# I have added a translate method to translate the data recieved from an external source before adding it to the setting dict
		# I have also added a default setting here, because the settings stored in the settings.xml cannot be relied upon,
		# because if the user adds a setting, then deletes it offline, the settings.xml will add it back in when the addon exits.
		# A default value of configignore means that the setting should never be passed to the config parser.
		self.pi_settings_dict = 	{
									'hdmi_safe': 				{'setting_value' : '',
																	'default': 'false',
																		'translate': self.translate_bool
																		},
									'hdmi_ignore_edid': 		{'setting_value' : '',
																	'default': 'false',
																		'translate': self.translate_bool
																		},
									'store_hdmi_to_file':		{'setting_value' : '',
																	'default': 'false',
																		'translate': self.translate_store_hdmi,
																			},
									'hdmi_edid_file': 			{'setting_value' : '',
																	'default': 'false',
																		'translate': self.translate_bool
																		},
									'hdmi_force_hotplug': 		{'setting_value' : '',
																	'default': 'false',
																		'translate': self.translate_bool,
																		},
									'hdmi_ignore_cec': 			{'setting_value' : '',
																	'default': 'false',
																		'translate': self.translate_bool
																		},
									'hdmi_ignore_cec_init ': 	{'setting_value' : '',
																	'default': 'true',
																		'translate': self.translate_bool
																		},																		
									'hdmi_boost': 				{'setting_value' : '',
																	'default': '0',
																	},
									'hdmi_group': 				{'setting_value' : '',
																	'default': '0',
																	},
									'hdmi_mode': 				{'setting_value' : '',
																	'default': '0',
																	},
									'hdmi_pixel_encoding': 		{'setting_value' : '',
																	'default': '0',
																	},																	
									'display_rotate': 			{'setting_value' : '',
																	'default': '0',
																	},
									'sdtv_mode': 				{'setting_value' : '',
																	'default': '0',
																	},
									'sdtv_aspect': 				{'setting_value' : '',
																	'default': '0',
																		'translate': self.translate_sdtv_aspect
																	},																																		
									'gpu_mem':					{'setting_value' : '',
																	'default': 'false',
																		'translate': self.translate_gpu_mem
																	},
									'gpu_mem_256': 				{'setting_value' : '',
																	'default': '112',
																	},
									'gpu_mem_512': 				{'setting_value' : '',
																	'default': '144',
																	},
									'gpu_mem_1024': 			{'setting_value' : '',
																	'default': '256',
																	},																	
									'decode_MPG2': 				{'setting_value' : '',
																	'default': '',
																	},
									'decode_WVC1': 				{'setting_value' : '',
																	'default': '',
																	},
									'max_usb_current':			{'setting_value' : '',
																	'default': 'false',
																		'translate': self.translate_bool,
																	},
									'dtoverlay':				{'setting_value' : '',
																	'default': '',
																		'translate': self.translate_dtoverlay,
																	},
									'device_tree':				{'setting_value' : '',
																	'default': '',
																		'translate': self.translate_device_tree,
																	},	
									'orphanedparams':			{'setting_value' : '',
																	'default': 'false',
																		'translate': self.translate_orphanedparams,
																	},																																																				
									# 'other_settings_string': 	{'setting_value' : '',
									# 								'default': '',
									# 									'translate': self.translate_other_string
																		# },								
									}

		# list to hold the keys for the other string settings
		self.unknown_setting_keys = []

		# list to hold the keys for the settings that need to be removed from the config.txt
		self.remove_list = []

		# the location of the config file FOR TESTING ONLY
		try:								
			self.test_config = '/boot/config.txt'

			# populate the settings data in the pi_settings_dict
			# self.populate_pi_settings_dict()

			# a flag to determine whether a setting change requires a reboot to take effect
			self.reboot_required = False

			# grab the Pi serial number and check to see whether the codec licences are enabled
			mpg = subprocess.check_output(["/opt/vc/bin/vcgencmd", "codec_enabled", "MPG2"])
			wvc = subprocess.check_output(["/opt/vc/bin/vcgencmd", "codec_enabled", "WVC1"])
			serial_raw = subprocess.check_output(["cat", "/proc/cpuinfo"])

			# grab just the serial number
			serial = serial_raw[serial_raw.index('Serial') + len('Serial'):].replace('\n','').replace(':','').replace(' ','').replace('\t','')

			# load the values into the settings gui
			__addon__.setSetting('codec_check', mpg.replace('\n','') + ', ' + wvc.replace('\n',''))
			__addon__.setSetting('serial', serial)

		except:
			self.test_config = '/home/plaskev/Documents/config.txt'

		log('START')
		for x, k in self.pi_settings_dict.iteritems():
			log("%s = %s" % (x, k.get('setting_value','no setting value')))

	@clog(log)
	def populate_pi_settings_dict(self):

		'''
			Populates the setting_value in the pi_settings_dict.
		'''

		# # this is the method to use if you are populating the dict from the settings.xml
		# latest_settings = self.settings_retriever_xml()

		# but I am going to set up my own process in addition to the xml one, I will be reading some
		# settings from the config.txt, and getting the rest from the settings.xml
		self.config_settings = ct.retrieve_settings_from_configtxt(self.test_config)

		log('Config settings received from the parser: %s' % self.config_settings)

		# cycle through the pi_settings_dict dict, and populate with the settings values
		for key in self.pi_settings_dict.keys():

			# if the value of the setting is to be assigned by another setting, then just ignore it here
			# note: this will mean that the other setting will have to populate both the settings_dict and the settings.xml
			if key in self.values_set_elsewhere:
				continue

			# grab the translate method (if there is one)
			translate_method = self.pi_settings_dict.get(key,{}).get('translate',{})

			# if the key is in the config.txt
			if key in self.config_settings:

				setting_value = self.config_settings[key]

			else:
				# if the key ISNT in the config.txt then set the value from the default stored in 
				# the pi_settings_dict dict

				setting_value = self.pi_settings_dict[key].get('default','')

			# get the setting value, translate it if needed
			if translate_method:
				setting_value = translate_method(setting_value)

			# if default is setting_value, then the setting has been set in the translation so ignore it
			if setting_value not in self.not_going_to_config:
				self.pi_settings_dict[key]['setting_value'] = setting_value

			# also set the value in the settings.xml
			self.me.setSetting(key, str(setting_value))

	@clog(log, nowait=True)
	def run(self):

		'''
			The method determines what happens when the item is clicked in the settings GUI.
			Usually this would be __addon__.OpenSettings(), but it could be any other script.
			This allows the creation of action buttons in the GUI, as well as allowing developers to script and skin their 
			own user interfaces.
		'''

		# read the config.txt file everytime the settings are opened. This is unavoidable because it is possible for
		# the user to have made manual changes to the config.txt while OSG is active.
		self.populate_pi_settings_dict()

		for x, k in self.pi_settings_dict.iteritems():
			log("%s = %s" % (x, k.get('setting_value','no setting value')))

		self.me.openSettings()

		# code placed here will run when the modules settings window is closed
		self.apply_permitted = True
		
		self.apply_settings()

		self.apply_permitted = False

		# apply_permitted will prevent the apply function being called by anything other than this method.
		# This stops it from being called twice, once when the settings are closed and another when the OSG is closed

		''' FOR TESTING ONLY '''
		log('END')
		for x, k in self.pi_settings_dict.iteritems():
			log("%s = %s" % (x, k.get('setting_value','no setting value')))

	@clog(log)
	def apply_settings(self):

		'''
			This method will apply all of the settings. It calls the first_method, if it exists. 
			Then it calls the method listed in pi_settings_dict for each setting. Then it calls the
			final_method, again, if it exists.
		'''

		# this prevents the method running when called by the OSG. Rather, the method is only being run when the settings
		# window is closed.
		if not self.apply_permitted:
			return 'apply not permitted'

		# retrieve the current settings from the settings.xml (this is where the user has made changes)
		new_settings = self.settings_retriever_xml()

		# dict to hold the keys of the changed settings
		self.changed_settings = {}

		# call the first method, if there is one
		self.first_method()

		# apply the individual settings changes
		for k, v in self.pi_settings_dict.iteritems():

			# if the value of the setting is set elsewhere, then the adding of the settings to changed settings will also
			# have to be handled by the apply method of that other setting.
			if k in self.values_set_elsewhere:
				continue

			# get the application method and stored setting value from the dictionary
			method = v.get('apply', False)
			value  = v.get('setting_value', '')

			# if the new setting is different to the stored setting then change the dict and run the 'apply' method
			if new_settings[k] != value:

				# change stored setting_value to the new value
				self.pi_settings_dict[k]['setting_value'] = new_settings[k]
		
				# add it to the changed settings dict
				self.changed_settings[k] = new_settings[k]

				# if a specific apply method exists for the setting, then call that
				try:
					method(new_settings[k])
				except:
					pass

		# call the final method if there is one
		self.final_method()

		ok = DIALOG.notification(lang(32095), lang(32096))


	def settings_retriever_xml(self):

		''' 
			Reads the stored settings (in settings.xml) and returns a dictionary with the setting_name: setting_value. This 
			method cannot be overwritten.
		'''

		latest_settings = {}

		addon = xbmcaddon.Addon(self.addonid)

		for key in self.pi_settings_dict.keys():

			latest_settings[key] = addon.getSetting(key)

		return latest_settings


	##############################################################################################################################
	#																															 #
	def first_method(self):

		''' 
			The method to call before all the other setting methods are called.

			For example, this could be a call to stop a service. The final method could then restart the service again. 
			This can be used to apply the setting changes.

		'''	

	@clog(log)
	def final_method(self):

		''' 
			The method to call after all the other setting methods have been called.

			For example, in the case of the Raspberry Pi's settings module, the final writing to the config.txt can be delayed
			until all the settings have been updated in the pi_settings_dict. 

		'''

		''' This method will write the changed settings to the config.txt file. '''

		# translate the changed settings into values that can be used in the config.txt
		self.translated_changed_settings = {}
		for k, v in self.changed_settings.iteritems():

			# translate the setting if needed
			# in some cases this translation can be used to set the values for other settings and have them added
			# to the translated_changed_settings dict
			translate_method = self.pi_settings_dict.get(k,{}).get('translate', False)

			if translate_method:
				value = translate_method(v, reverse=True)
			else:
				value = v #.get('setting_value','')

			# if the setting is not to be added to the config.txt, then dont add it to the self.translated_changed_settings dict
			if k in self.not_going_to_config:
				continue

			# # if this is the other_settings_string then break up into the individual settings
			# elif k == 'other_settings_string':
			# 	for key, svalue in value.iteritems():
			# 		self.translated_changed_settings[key] = svalue

			# add the setting to the translated settings dict, this is now ready to send to the config.txt writer	
			else:
				self.translated_changed_settings[k] = value

		# transfer the remove list into the changes dict
		# this will make sure that existing settings in the config.txt that need to be removed, will be removed
		for remove_key in self.remove_list:
			self.translated_changed_settings[remove_key] = 'remove'

		# reset the remove list
		self.remove_list = []

		# start_x=1 added by default to every config.txt
		# popcornmix: I would remove start_file=start_x.elf and fixup_file=fixup_x.dat and use the shortcut start_x=1
		self.translated_changed_settings['start_x'] = 1

		# write the settings to the config.txt
		ct.apply_changes_to_configtxt(self.translated_changed_settings, self.test_config)


	def boot_method(self):

		''' 
			The method to call when the OSA is first activated (on reboot)

		'''

		pass

	#																															 #
	##############################################################################################################################


	##############################################################################################################################
	#																															 #

	''' 
		Methods beyond this point are for specific settings. 
	'''

	def translate_sdtv_aspect(self, data, reverse=False):

		''' Method to translate the sdtv_aspect from 0 based index to 1 based '''

		if not reverse:

			if data:
	
				return int(data) - 1

			else:

				return 0

		else:

			return int(data) + 1


	def translate_bool(self, data, reverse=False):

		''' method to convert number or text into boolean '''

		if not reverse:
			if data in [1, '1']:
				return 'true'
			else:
				return 'false'

		else:
			if data in [1, '1', 'true']:
				return '1'
			else:
				return '0'


	# def translate_other_string(self, data='', reverse=False):

	# 	''' 
	# 		Method to collate all the unknown settings from the config.txt into a single string, delimited by |:-:|.
	# 		The reverse function returns a dictionary with {setting_name: setting_value, ... }
	# 	'''

	# 	if not reverse:
	# 		config_keys = set(self.config_settings.keys())
	# 		xml_keys    = set(self.pi_settings_dict.keys())

	# 		self.unknown_setting_keys = list(config_keys.difference(xml_keys))

	# 		unknown_settings = [str(x) + '=' + str(self.config_settings[x]) for x in self.unknown_setting_keys]

	# 		return "|:-:|".join(unknown_settings)

	# 	else:

	# 		no_space_data = data.replace(" ",'')
	# 		setting_pairs = no_space_data.split("|:-:|")

	# 		other_settings = []

	# 		for setting in setting_pairs:
	# 			set_list = setting.split('=')

	# 			if len(set_list) == 2:
	# 				other_settings.append(tuple(set_list))

	# 		new_unknown_settings = dict(other_settings)

	# 		# construct a list of keys that are in self.unknown_setting_keys but not in new_unknown_settings_keys
	# 		new_unknown_settings_keys = set(new_unknown_settings.keys())
	# 		unknown_settings_keys = set(self.unknown_setting_keys)

	# 		removals = list(unknown_settings_keys.difference(new_unknown_settings_keys))

	# 		# setup the removed unknown settings to be removed from the config.txt
	# 		for rem in removals:
	# 			new_unknown_settings[rem] = 'remove'

	# 		# change the self.unknown_setting_keys list to the current list of unknown keys
	# 		self.unknown_setting_keys = list(new_unknown_settings_keys)

	# 		return new_unknown_settings

	@clog(log)
	def translate_device_tree(self, data, reverse=False):
		'''
			Checks for the presence of an empty device_tree setting, which disables device tree overlays.
		'''

		datalist = data.split('\n')

		if not reverse:

			if 'device_tree' in self.config_settings and '' in datalist:
				self.me.setSetting('suppress_dtoverlay', 'true')
			else:
				self.me.setSetting('suppress_dtoverlay', 'false')

		else:
			if self.me.getSetting('suppress_dtoverlay') == 'true':
				return ['']
			else:
				return ['[remove]']

	@clog(log)
	def translate_dtoverlay(self, data, reverse=False):
		'''
			Parses the dtoverlay list. There can be multiple dtoverlays, so the config_tool puts them all into 
			a single list.
		'''

		# setting: the set of settings in the group
		# value: the value to assign to the kodi displayed settings if the overlay is active
		overlay_settings 		= 	{
		'hifiberry-dac-overlay'		: {'setting': 'soundcard_dac', 'value': '1'},
		'hifiberry-dacplus-overlay'	: {'setting': 'soundcard_dac', 'value': '2'},
		'hifiberry-digi-overlay'	: {'setting': 'soundcard_dac', 'value': '3'},
		'iqaudio-dac-overlay'		: {'setting': 'soundcard_dac', 'value': '4'},
		'iqaudio-dacplus-overlay'	: {'setting': 'soundcard_dac', 'value': '5'},
		'w1-gpio-overlay'			: {'setting': 'w1gpio', 'value': '1'},
		'w1-gpio-pullup-overlay'	: {'setting': 'w1gpio', 'value': '2'},
		'lirc-rpi-overlay'			: {'setting': 'lirc-rpi-overlay', 'value': 'true'},
		'spi-bcm2835-overlay'		: {'setting': 'spi-bcm2835-overlay', 'value': 'true'},
									}

		dac_all = ['hifiberry-dac-overlay', 'hifiberry-dacplus-overlay','hifiberry-digi-overlay', 'iqaudio-dac-overlay','iqaudio-dacplus-overlay']
		w1gpio  = ['w1-gpio-overlay', 'w1-gpio-pullup-overlay']

		datalist = data.split('\n')

		log('datalist = %s' % datalist)

		if not reverse:

			# do this when reading the items into Kodi
				
			self.me.setSetting('lirc-rpi-overlay', 'false')
			self.me.setSetting('spi-bcm2835-overlay', 'false')
			self.me.setSetting('soundcard_dac', '0')
			self.me.setSetting('w1gpio', '0')

			# dtoverlay=lirc-rpi:gpio_out_pin=19,gpio_in_pin=23,gpio_in_pull=down

			for overlay in datalist:

				log('individual overlay=%s' % overlay)

				# lirc has to be handled individually as it may include extra parameters
				if 'lirc-rpi' in overlay:
					self.me.setSetting('lirc-rpi-overlay', 'true')

					sub_params = ['gpio_out_pin', 'gpio_in_pin', 'gpio_in_pull']

					if ':' in overlay:
						params = [x.split('=') for x in overlay[overlay.index(':')+1:].split(',')]

						log('lirc-rpi params=%s' % params)
						
						for param in params:
							for sub in sub_params:
								if param[0] == sub:
									self.me.setSetting(sub, param[1].strip())
					continue

				if overlay not in overlay_settings:

					log('%s not in overlay_settings' % overlay)
					
					continue

				else:

					ovl = overlay_settings[overlay]

					self.me.setSetting(ovl['setting'], ovl['value'])

					log('overlay: %s,   setting: %s,   value: %s' % (overlay, ovl['setting'], ovl['value']))

		else:

			# do this when writing the Kodi settings back to config.txt

			new_dtoverlay = []

			pos = self.me.getSetting('soundcard_dac')

			if pos == '0':

				new_dtoverlay.extend(['dtoverlay_||_' + x + '[remove]' for x in dac_all])
			
			else:
				
				soundcard = dac_all[int(pos)-1]

				# add the soundcard overlay
				new_dtoverlay.append(soundcard)

				#remove the unneeded entries
				new_dtoverlay.extend(['dtoverlay_||_' + x + '[remove]' for x in dac_all if x != soundcard])

			wgp = self.me.getSetting('w1gpio')

			if wgp != '0':
				new_dtoverlay.append('dtoverlay_||_' + w1gpio[int(wgp)-1])
			else:
				new_dtoverlay.extend(['dtoverlay_||_' + x + '[remove]' for x in w1gpio])

			rpi = self.me.getSetting('lirc-rpi-overlay')

			if rpi == 'true':

				# dtoverlay=lirc-rpi:gpio_out_pin=19,gpio_in_pin=23,gpio_in_pull=down

				out_pin  = self.me.getSetting('gpio_out_pin')
				in_pin   = self.me.getSetting('gpio_in_pin')
				pull_pin = self.me.getSetting('gpio_in_pull')

				lirc = 'dtoverlay_||_lirc-rpi=' + 'gpio_out_pin=' + str(out_pin) + ',gpio_in_pin=' + str(in_pin)

				if pull_pin != 'off':
					lirc = lirc + ',gpio_in_pull=' + pull_pin

				new_dtoverlay.append(lirc)

			else:
				new_dtoverlay.append('dtoverlay_||_lirc-rpi-overlay' + '[remove]')

			spi = self.me.getSetting('spi-bcm2835-overlay')

			if spi == 'true':
				new_dtoverlay.append('dtoverlay_||_spi-bcm2835-overlay')
			else:
				new_dtoverlay.append('dtoverlay_||_spi-bcm2835-overlay' + '[remove]')

			log("NEW DT OVERLAY = %s" % new_dtoverlay)
			return new_dtoverlay

	@clog(log)
	def translate_store_hdmi(self, data, reverse=False):

		''' 
			Sets the settings_dict and settings.xml values for hdmi_edid_file and hdmi_force_hotplug.
		'''

		if not reverse:

			# set the pi_settings_dict and settings.xml values for these two settings
			hdmi_edid_file     = self.translate_bool(self.config_settings.get('hdmi_edid_file', 0))
			hdmi_force_hotplug = self.translate_bool(self.config_settings.get('hdmi_force_hotplug', 0))

			# popcornmix says that if either of these settings are active, then both should be active
			tethered_settings = all([hdmi_edid_file=='true' , hdmi_force_hotplug=='true'])

			self.pi_settings_dict['hdmi_edid_file']['setting_value']     = tethered_settings
			self.pi_settings_dict['hdmi_force_hotplug']['setting_value'] = tethered_settings

			# return the appropriate value for the parent setting
			if tethered_settings: return 'true'

			return 'false'


		else:

			# if the parent setting is true, then the child settings should be set to one
			# if it isnt true, then both settings should be removed from the config.txt
			if data == 'true':

				self.translated_changed_settings['hdmi_edid_file'] = '1'
				self.translated_changed_settings['hdmi_force_hotplug'] = '1'

				# run the sub_process : "tvservice -d /boot/edid.dat"
				subprocess.call(["sudo", "/opt/vc/bin/tvservice", "-d", "/boot/edid.dat"])

			else:

				# if the parent setting is false, then remove these two child settings from the config.xml			
				self.remove_list.append('hdmi_edid_file')
				self.remove_list.append('hdmi_force_hotplug')

				return 'remove'
			
	@clog(log)
	def translate_gpu_mem(self, data, reverse=False):

		''' 
			If gpu_mem is present in the config.txt, then apply it to both gpu_mem_256 and gpu_mem_512.
			Any new config.txt should be missing the gpu_mem setting.
		'''

		if not reverse:

			memgpu = self.config_settings.get('gpu_mem', False)

			# if gpu_mem is not in the config.txt then just return
			if not memgpu:

				return 'remove'

			# set gpu_mem for removal from the config.txt
			self.remove_list.append('gpu_mem')

			# get the values for the other memory setting variants
			mem1024 = self.config_settings.get('gpu_mem_1024', False)
			mem512  = self.config_settings.get('gpu_mem_512',  False)
			mem256  = self.config_settings.get('gpu_mem_256',  False)

			if mem1024:

				return 'remove'

			elif memgpu:

				# set the value in the pi_settings_dict and the settings.xml for display
				val1024 = min(768, int(memgpu))
				self.me.setSetting('gpu_mem_1024', str(val1024))
				self.pi_settings_dict['gpu_mem_1024']['setting_value'] = val1024

			# if gpu_mem_512 is in the config, then use that, otherwise use gpu_mem, otherwise use default
			if mem512:

				return 'remove'

			elif memgpu:

				# set the value in the pi_settings_dict and the settings.xml for display
				val512 = min(448, int(memgpu))
				self.me.setSetting('gpu_mem_512', str(val512))
				self.pi_settings_dict['gpu_mem_512']['setting_value'] = val512

			# if gpu_mem_256 is in the config, then use that, otherwise use gpu_mem, otherwise use default
			if mem256:

				return 'remove'

			elif memgpu:

				# set the value in the pi_settings_dict and the settings.xml for display
				val256 = min(192, int(memgpu))
				self.me.setSetting('gpu_mem_256', str(val256))
				self.pi_settings_dict['gpu_mem_256']['setting_value'] = val256

			return 'remove'

		else:

			return 'remove'
			

	def translate_orphanedparams(self, data, reverse=False):

		''' Translates the orphaned dtparam for audio from either on, or remove '''

		if not reverse:

			datalist = data.split('\n')

			for param in datalist:

				k, v = param.split('|__|')

				if v == 'on':
					self.me.setSetting(k, 'true')
				else:
					self.me.setSetting(k, 'false')

		else:

			new_dtparams = []

			if self.me.getSetting('audio') == 'true' or self.me.getSetting('spi-bcm2835-overlay') == 'true':
				new_dtparams.append('audio|__|on')
			else:
				new_dtparams.append('audio[remove]')

			return new_dtparams

	#																															 #
	##############################################################################################################################

if __name__ == "__main__":
	pass

