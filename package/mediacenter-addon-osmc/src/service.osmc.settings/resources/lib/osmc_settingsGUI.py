# XBMC modules
import xbmc
import xbmcaddon
import xbmcgui

# STANDARD library modules
import ast
import datetime
import imp
import json
import os
import pickle
import Queue
import select
import socket
import sys
import threading
import time
import traceback
from CompLogger import comprehensive_logger as clog

path       = xbmcaddon.Addon().getAddonInfo('path')
lib        = os.path.join(path, 'resources','lib')
media      = os.path.join(path, 'resources','skins','Default','media')

sys.path.append(xbmc.translatePath(lib))

__addon__  = xbmcaddon.Addon()
scriptPath = __addon__.getAddonInfo('path')
WINDOW     = xbmcgui.Window(10000)

def log(message):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )
		
	xbmc.log('osmc_settings: ' + str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


class OSMC_gui(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):

		self.live_modules   = kwargs.get('live_modules' , [])

		self.module_holder = {}

		log(kwargs)

		log(len(self.live_modules))

		self.first_run = True


	def onInit(self):

		if self.first_run:

			self.first_run = False

			# add the exit button
			self.getControl(555).addItem(xbmcgui.ListItem(label='Exit', label2=''))

			# place the items into the gui
			for i, module in enumerate(self.live_modules):

				try:
					shortname = module['SET'].shortname
				except:
					shortname = ''

				# set the icon (texturefocus, texturenofocus)
				list_item = xbmcgui.ListItem(label=shortname, label2='', thumbnailImage = module['FX_Icon'])
				list_item.setProperty('FO_ICON', module['FO_Icon'])

				controlID = 555

				self.module_holder[i + 1] = module

				self.getControl(controlID).addItem(list_item)

			self.setFocusId(555)


	def onAction(self, action):

		actionID = action.getId()
		focused_control = self.getFocusId()

		if (actionID in (10, 92)):
			self.close()


	def onClick(self, controlID):

		if controlID == 909:
			# open the advanced settings beta addon
			xbmc.executebuiltin("RunAddon(script.advancedsettingsetter)")

		else:

			pos = self.getControl(555).getSelectedPosition()

			if pos == 0:
				self.close()

			else:

				module = self.module_holder.get(pos, {})
				instance = module.get('SET', False)

				log(instance)
				log(instance.isAlive())

				# try:
				if instance.isAlive():
					instance.run()
				else:

					setting_instance = module['OSMCSetting'].OSMCSettingClass()
					setting_instance.setDaemon(True)

					module['SET'] = setting_instance

					# instance = imp.load_source(new_module_name, osmc_setting_file)
					# module_holder['SET'] = instance
					# instance.setDaemon(True)
					setting_instance.start()
				# except:
				# log('Settings window for __ %s __ failed to open' % module.get('id', "Unknown"))


	def onFocus(self, controlID):

		pass



class OSMCGui(threading.Thread):

	def __init__(self, **kwargs):

		self.queue = kwargs['queue']

		super(OSMCGui, self).__init__()

		self.create_gui()

	@clog(log)
	def create_gui(self):
		# known modules is a list of tuples detailing all the known and permissable modules and services
		# (module name, order): the order is the hierarchy of addons (which is used to 
		# determine the positions of addon in the gui)
		self.known_modules_order = 	{
									"script.module.osmcsetting.pi":						0,
									"script.module.osmcsetting.pioverclock":			1,
									"script.module.osmcsetting.updates":				2,
									"script.module.osmcsetting.networking":				3,
									"script.module.osmcsetting.logging":				4, 
									"script.module.osmcsetting.apfstore":				5,
									"script.module.osmcsetting.services":				6,
									"script.module.osmcsetting.remotes":				7,

									}

		# order of addon hierarchy
		# 105 is Apply
		self.item_order    = [104, 106, 102, 108, 101, 109, 103, 107]
		self.apply_button  = [105]

		# window xml to use
		xml = "settings_gui_720.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "settings_gui.xml"

		# check if modules and services exist, add the ones that exist to the live_modules list
		self.ordered_live_modules = self.retrieve_modules()
		self.ordered_live_modules.sort()
		self.live_modules = [x[1] for x in self.ordered_live_modules]

		# load the modules as widget entries
		self.load_widget_info()

		# instantiate the window
		self.GUI = OSMC_gui(xml, scriptPath, 'Default', live_modules=self.live_modules)


	def load_widget_info(self):
		''' Takes each live_module and loads the information required for it to be included in the MyOSMC widget into the Home window.
		'''

		script_location = os.path.join(scriptPath, 'resources', 'lib', 'call_osmc_parent.py')
		
		WINDOW.setProperty('MyOSMC.Module.Script', script_location)

		for i, module in enumerate(self.live_modules):

			WINDOW.setProperty('MyOSMC.Module.%s.name' 		% i, module['SET'].shortname)
			WINDOW.setProperty('MyOSMC.Module.%s.fo_icon' 	% i, module['FO_Icon_Widget'])
			WINDOW.setProperty('MyOSMC.Module.%s.fx_icon' 	% i, module['FX_Icon_Widget'])
			WINDOW.setProperty('MyOSMC.Module.%s.id' 		% i, module['id'])

	
	def close(self):
		'''
			Closes the gui
		'''

		self.GUI.close()



	def run(self):
		'''
			Opens the gui window
		'''
		
		log('Opening GUI')
		# display the window
		self.GUI.doModal()

		# run the apply_settings method on all modules
		for module in self.live_modules:
			m = module.get('SET', False)
			try:
				m.apply_settings()
			except:
				log('apply_settings failed for %s' % m.addonid)


		# check is a reboot is required
		reboot = False
		for module in self.live_modules:
			m = module.get('SET', False)
			try:
				if m.reboot_required:
					reboot = True
					break
			except:
				pass

		if reboot:
			self.queue.put('reboot')


		log('Exiting GUI')


	@clog(log)
	def retrieve_modules(self):
		'''
			Checks to see whether the module exists and is active. If it doesnt exist (or is set to inactive)
			then return False, otherwise import the module (or the setting_module.py in the service or addons
			resources/lib/) and create then return the instance of the SettingGroup in that module.

		'''

		self.module_tally = 1000

		if os.path.isdir('/usr/share/kodi/addons/service.osmc.settings'):
			addon_folder  = '/usr/share/kodi/addons'
		else:
			addon_folder  = os.path.join(xbmc.translatePath("special://home"), "addons/")  # FOR TESTING	

		folders       = [item for item in os.listdir(addon_folder) if os.path.isdir(os.path.join(addon_folder, item))]

		osmc_modules   = [x for x in [self.inspect_folder(addon_folder, folder) for folder in folders] if x]

		return osmc_modules


	def inspect_folder(self, addon_folder, sub_folder):
		'''
			Checks the provided folder to see if it is a genuine OSMC module.
			Returns a tuple.
				(preferred order of module, module name: {unfocussed icon, focussed icon, instance of OSMCSetting class})
		'''

		# check for osmc subfolder, return nothing is it doesnt exist
		osmc_subfolder = os.path.join(addon_folder, sub_folder, "resources", "osmc")
		if not os.path.isdir(osmc_subfolder): return

		# check for OSMCSetting.py, return nothing is it doesnt exist
		osmc_setting_file = os.path.join(osmc_subfolder, "OSMCSetting.py")
		if not os.path.isfile(osmc_setting_file): return

		# check for the unfocussed icon.png
		osmc_setting_FX_icon = os.path.join(osmc_subfolder, "FX_Icon.png")
		if not os.path.isfile(osmc_setting_FX_icon): return

		# check for the focussed icon.png
		osmc_setting_FO_icon = os.path.join(osmc_subfolder, "FO_Icon.png")
		if not os.path.isfile(osmc_setting_FO_icon): return

		# check for the unfocussed widget icon.png
		osmc_setting_FX_icon_Widget = os.path.join(osmc_subfolder, "FX_Icon_Widget.png")
		if not os.path.isfile(osmc_setting_FX_icon): 
			# if not found, use the ordinary icon instead
			osmc_setting_FX_icon_Widget = osmc_setting_FX_icon

		# check for the focussed widget icon.png
		osmc_setting_FO_icon_Widget = os.path.join(osmc_subfolder, "FO_Icon_Widget.png")
		if not os.path.isfile(osmc_setting_FO_icon):
			# if not found, use the ordinary icon instead
			osmc_setting_FO_icon_Widget = osmc_setting_FO_icon

		# if you got this far then this is almost certainly an OSMC setting
		try:
			new_module_name = sub_folder.replace('.','')
			log(new_module_name)
			OSMCSetting = imp.load_source(new_module_name, osmc_setting_file)
			log(dir(OSMCSetting))
			setting_instance = OSMCSetting.OSMCSettingClass()
			setting_instance.setDaemon(True)
		except:
			exc_type, exc_value, exc_traceback = sys.exc_info()
			log('OSMCSetting __ %s __ failed to import' % sub_folder)
			log(exc_type)
			log(exc_value)
			log(traceback.format_exc())
			return

		# success!
		log('OSMC Setting Module __ %s __ found and imported' % sub_folder)

		# DETERMINE ORDER OF ADDONS, THIS CAN BE HARDCODED FOR SOME OR THE USER SHOULD BE ABLE TO CHOOSE THEIR OWN ORDER
		if sub_folder in self.known_modules_order.keys():
			order = self.known_modules_order[sub_folder]
		else:
			order = self.module_tally
			self.module_tally += 1

		return (order, {
						'id': sub_folder, 
						'FX_Icon': osmc_setting_FX_icon, 
						'FO_Icon': osmc_setting_FO_icon, 
						'FX_Icon_Widget': osmc_setting_FX_icon_Widget, 
						'FO_Icon_Widget': osmc_setting_FO_icon_Widget, 
						'SET':setting_instance, 
						'OSMCSetting':OSMCSetting
						}
				)


