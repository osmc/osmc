# XBMC modules
import xbmc
import xbmcgui
import xbmcaddon

# STANDARD library modules
import ast
import datetime
import json
import os
import pickle
import Queue
import select
import socket
import threading
import time
import sys
sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources','lib')))

# Custom Modules
import timezones



def log(message):
	xbmc.log(str(message), level=xbmc.LOGDEBUG)


class walkthru_gui(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName):

		# edit the timezone in /etc/timezone
		self.timezones = timezones.get_timezones()

		# get the languages
		self.languages = [folder for folder in os.listdir('/usr/share/kodi/language/')]
		self.languages.sort()

		self.tz_control_map = {
						'Africa'	: 30010,
						'America'	: 30020,
						'Asia'		: 30030,		
						'Atlantic'	: 30040,
						'Australia'	: 30050,
						'Europe'	: 30060,
						'Indian'	: 30070,
						'Pacific'	: 30080,
						'UTC'		: 30090,
						}

		with open('/usr/share/kodi/language/Afrikaans/strings.po', 'r') as f:

			lines = f.readlines()

			self.licence = ''.join(lines)


	def onInit(self):

		for region, countries in self.timezones.iteritems():

			for country in countries:

				ctl_id = self.tz_control_map.get(region, False)

				if not ctl_id: continue

				self.tmp = xbmcgui.ListItem(label=country, label2='', thumbnailImage='')

				self.getControl(ctl_id).addItem(self.tmp)

		for visibility_control in [93000,94000,95000]:

			self.getControl(visibility_control).setVisible(False)

		for tz in [3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009]:

			self.getControl(tz*10).setVisible(False)

		for language in self.languages:

			self.tmp = xbmcgui.ListItem(label=language, label2='', thumbnailImage='')

			self.getControl(20010).addItem(self.tmp)


		self.getControl(555).setText(self.licence)


	def onClick(self, controlID):

		if controlID == 110:
			self.close()

	def onFocus(self, controlID):

		main_controls = [1002, 1003, 1004]

		tz_controls = [3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009]

		if controlID in main_controls:

			for main in main_controls:

				sub_id = main % 1000

				ctl = self.getControl((sub_id * 1000) + 90000)

				if main == controlID:

					ctl.setVisible(True)

				else:

					ctl.setVisible(False)

		elif controlID in tz_controls:

			log(controlID)

			for tz in tz_controls:

				ctl = self.getControl(tz * 10)

				if tz == controlID:

					ctl.setVisible(True)

				else:

					ctl.setVisible(False)




	def set_language(self, language):

		xbmc.executebuiltin('xbmc.SetGUILanguage(%s)' % language)


def open_gui():

	__addon__        = xbmcaddon.Addon()
	scriptPath       = __addon__.getAddonInfo('path')
	xmlfile = 'walkthru.xml'

	GUI = walkthru_gui(xmlfile, scriptPath, 'Default')

	GUI.doModal()

	log('Exiting GUI')

	del GUI

