# XBMC modules
import xbmc
import xbmcgui
import xbmcaddon

# STANDARD library modules
import os
import sys
import requests
import subprocess
from threading import Thread
sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources','lib')))

# Custom Modules
import timezones
import LICENSE

EULA = LICENSE.license

def log(message):
	xbmc.log(str(message), level=xbmc.LOGDEBUG)


class Networking_caller(Thread):
	def __init__(self, parent, net_call):
		super(Networking_caller, self).__init__()
		self.daemon = True
		self.cancelled = False
		self.parent = parent
		self.net_call = net_call
		# instantiate Barkers interface class
		# self.networking_interface = NETWORKING.Barkersinterface()

	def run(self):
		"""Calls Barkers method to check for internet connection"""

		log('checking internet connection')

		self.parent.internet_connected = self.net_call.check_internet()



class walkthru_gui(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, networking_instance):


		# show timezone switch
		self.showtimezone = False

		# switch that identifies whether the internet is connected
		self.internet_connected = False

		#start a new thread that begins checking for an internet connection
		self.net_call = networking_instance

		self.internet_check = False
		self.internet_checker = Networking_caller(self, self.net_call)
		self.internet_checker.start()

		# edit the timezone in /etc/timezone
		if self.showtimezone:
			self.timezones = timezones.get_timezones()

		# newsletter email address
		self.email = ''

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

		self.selected_language = None
		self.selected_region   = None
		self.selected_country  = None


	def onInit(self):

		global EULA

		#hide all timezone, TandC and Apply buttons
		for hide_this in [1003, 1004, 1005, 1006]:

			self.getControl(hide_this).setVisible(False)

		if self.showtimezone:
			# populate the timezone controls
			for region, countries in self.timezones.iteritems():

				for country in countries:

					ctl_id = self.tz_control_map.get(region, False)

					if not ctl_id: continue

					self.tmp = xbmcgui.ListItem(label=country, label2='', thumbnailImage='')

					self.getControl(ctl_id).addItem(self.tmp)

		# hide the controls that determine panel visibility
		for visibility_control in [93000,94000,95000, 96000]:

			self.getControl(visibility_control).setVisible(False)

		# hide the language sub menus
		for tz in [3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009]:

			self.getControl(tz*10).setVisible(False)

		# populate the language control
		for language in self.languages:

			self.tmp = xbmcgui.ListItem(label=language, label2='', thumbnailImage='')

			self.getControl(20010).addItem(self.tmp)

		# populate the terms and conditions
		self.getControl(555).setText(EULA)


	def onClick(self, controlID):

		if controlID == 1005:
			# Exit control

			# delete skin update block file
			subprocess.call(['sudo', 'rm', '/tmp/NO_UPDATE'])

			if self.getControl(50001).isSelected():
				# sign up for newsletter button

				# show keyboard
				kb = xbmc.Keyboard(self.email, 'Please enter your email')
				kb.doModal()
				if kb.isConfirmed():
					self.email = kb.getText()
					requests.post('https://osmc.tv/wp-content/plugins/newsletter/do/subscribe.php', data={'ne': self.email})
					self.setFocusId(1005)

			if self.selected_language != None:

				log('users language: %s' % self.selected_language)

				# set language
				xbmc.executebuiltin('xbmc.SetGUILanguage(%s)' % self.selected_language)

			if self.selected_country != None:

				# set timezone
				for reg, cnt in self.timezones.iteritems():

					if self.selected_country in cnt:
						self.selected_region = reg
						break

			if self.selected_country != None and self.selected_region != None:

				users_timezone = "%s/%s" % (self.selected_region, self.selected_country)

				log('users timezone: %s' % users_timezone)

				os.system('echo %s | sudo tee /etc/timezone' % users_timezone)

			self.close()

		elif controlID == 20010:
			# language container

			self.selected_language = self.getControl(controlID).getSelectedItem().getLabel()

			if self.showtimezone:

				self.getControl(92000).setVisible(False)
				self.getControl(93000).setVisible(True)
				self.getControl(1003).setVisible(True)
				self.setFocusId(1003)

			else:

				self.getControl(92000).setVisible(False)
				self.getControl(94000).setVisible(True)
				self.getControl(1004).setVisible(True)
				self.setFocusId(1004)
				self.getControl(1004).controlUp(self.getControl(1002))
				self.getControl(1002).controlDown(self.getControl(1004))

		elif controlID in [30010, 30020, 30030,	30040, 30050, 30060, 30070, 30080, 30090]:
			# timezone containers

			# user has clicked on a timezone
			self.selected_country = self.getControl(controlID).getSelectedItem().getLabel()

			self.getControl(93000).setVisible(False)
			self.getControl(94000).setVisible(True)
			self.getControl(1004).setVisible(True)
			self.setFocusId(1004)

		elif controlID == 40010:
			# terms and conditions I Agree button

			# check if internet is connected
			if self.internet_connected:
				# skip the Networking setup menu item
				self.getControl(94000).setVisible(False)
				self.getControl(95000).setVisible(True)
				self.getControl(1005).setVisible(True)
				self.setFocusId(1005)
			else:
				# display the Networking panel
				self.getControl(94000).setVisible(False)
				self.getControl(96000).setVisible(True)
				self.getControl(1006).setVisible(True)
				self.setFocusId(1006)				

		elif controlID == 40020:
			# unused scroll bar for TandC

			self.getControl(555).scroll(10)

		elif controlID == 40030:
			#unused scroll bar for TandC

			self.getControl(555).scroll(-10)

		elif controlID == 60090:
			# skip networking button

			# display the Exit panel
			self.getControl(96000).setVisible(False)
			self.getControl(95000).setVisible(True)
			self.getControl(1005).setVisible(True)
			self.setFocusId(1005)	

		elif controlID == 60010:
			# open networking gui
			self.net_call.open_settings_window(True)

			# display the Exit panel
			self.getControl(96000).setVisible(False)
			self.getControl(95000).setVisible(True)
			self.getControl(1005).setVisible(True)
			self.setFocusId(1005)	


	def onAction(self, action):

		if action == 10 or action == 92:
			# *** THIS IS FOR TESTING ONLY ***

			# delete skin update block file
			subprocess.call(['sudo', 'rm', '/tmp/NO_UPDATE'])


	def onFocus(self, controlID):

		main_controls = [1002, 1003, 1004, 1005, 1006]

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

			for tz in tz_controls:

				ctl = self.getControl(tz * 10)

				if tz == controlID:

					ctl.setVisible(True)

				else:

					ctl.setVisible(False)




def open_gui(networking_instance):

	__addon__        = xbmcaddon.Addon()
	scriptPath       = __addon__.getAddonInfo('path')
	xmlfile = 'walkthru.xml'

	GUI = walkthru_gui(xmlfile, scriptPath, 'Default', networking_instance=networking_instance)

	GUI.doModal()

	log('Exiting GUI')

	del GUI

