

# XBMC modules
import xbmc
import xbmcgui
import xbmcaddon

# STANDARD library modules
import os
import sys
import requests
import subprocess
import threading
sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources','lib')))

# Custom Modules
import timezones
import LICENSE
import WARRANTY

EULA   = LICENSE.license
WARR   = WARRANTY.warranty
DIALOG = xbmcgui.Dialog()



def log(message):
	xbmc.log(str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


class Networking_caller(threading.Thread):
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

		log('internet connection is %s' % self.parent.internet_connected)



class walkthru_gui(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, networking_instance, lang_rerun, selected_language):


		# show timezone switch
		self.showtimezone = False

		# switch that identifies whether the internet is connected
		self.internet_connected = False

		#start a new thread that begins checking for an internet connection
		self.net_call = networking_instance

		self.internet_check = False
		self.internet_checker = Networking_caller(self, self.net_call)
		self.internet_checker.setDaemon(True)
		self.internet_checker.start()

		# this flag tells us whether the GUI has been reloaded due to language selection
		self.lang_rerun = lang_rerun

		# edit the timezone in /etc/timezone
		if self.showtimezone:
			self.timezones = timezones.get_timezones()

		# this attribute denotes the skin the user wants to have applied when the walkthru closes
		self.selected_skin = 'OSMC'

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

		self.selected_language = selected_language
		self.selected_region   = None
		self.selected_country  = None

		# textures for the skin image
		media_path = xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources', 'skins', 'Default', 'media'))
		self.osmc_skin_image = os.path.join(media_path, 'osmc_preview.png')
		self.conf_skin_image = os.path.join(media_path, 'conf_preview.jpg')

		self.vero = self.check_hardware()

		# this attribute is used to determine when the user is allowed to exit the walkthru using the Esc or Back buttons
		self.prevent_escape = True


	def onInit(self):

		global EULA
		global WARR

		#hide all timezone, TandC and Apply buttons
		for hide_this in [1003, 1004, 1005, 1006, 1007, 1008, 1009]:

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
		for visibility_control in [93000,94000,95000, 96000, 97000, 98000, 99000]:

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

		# populate the warranty
		self.getControl(777).setText(WARR)		

		# set the image for the skin preview control
		self.set_skin_image('OSMC')

		# this will only be True, if the language has been selected and the GUI has reloaded
		if self.lang_rerun:
			# set the flag to False so the GUI doesnt reload on exit
			self.lang_rerun = False
			self.bypass_language()


	def bypass_language(self):

		''' Bypasses the language setting, sets the language as selected so the window doesnt reopen '''

		if self.showtimezone:
			# this part is being kept just in case we want to reimplement the timezone selection

			self.getControl(92000).setVisible(False)
			self.getControl(93000).setVisible(True)
			self.getControl(1003).setVisible(True)
			self.setFocusId(1003)

		else:

			# make the language panel invisible
			self.getControl(92000).setVisible(False)
			# make the terms panel visible
			self.getControl(94000).setVisible(True)
			# make the Term menu item visible
			self.getControl(1004).setVisible(True)
			# jump to the Terms menu item
			self.setFocusId(40010)
			# change the up and down controls for the language and terms menu items
			self.getControl(1004).controlUp(self.getControl(1002))
			self.getControl(1002).controlDown(self.getControl(1004))


	def set_skin_image(self, skin):

		''' Sets the image for the skin preview '''

		if skin == 'CONF':
			self.getControl(88888).setImage(self.conf_skin_image)
		else:
			self.getControl(88888).setImage(self.osmc_skin_image)



	def check_hardware(self):
		'''
			Checks whether this is a Vero and whether the warranty info should be shown 
		'''

		# generate the URL
		with open('/proc/cmdline', 'r') as f:

			line = f.readline()

			settings = line.split(' ')

			for setting in settings:

				if setting.startswith('osmcdev='):

					if setting[len('osmcdev='):] == 'vero':

						log('Hardware is Vero')

						return True

		log('Hardware not Vero')

		return False


	def exit_proceedure(self):

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


	def onClick(self, controlID):

		if   controlID == 1005:				# Exit control

			self.exit_proceedure()

		elif controlID == 20010:			# language container

			self.previous_language = self.selected_language

			self.selected_language = self.getControl(controlID).getSelectedItem().getLabel()

			# display confirmation dialog
			user_confirmation = DIALOG.yesno('Confirm', self.selected_language, autoclose=10000)

			if user_confirmation == True:
				# if user CONFIRMS, check whether a skin reload is required

				if 'english' not in self.selected_language.lower():
					# if skin reload required, then close the window, change system language, reload the window and jump to TERMS
					
					# when lang_rerun set is True, the walkthru is reloaded and skips to the setting after language
					self.lang_rerun = True
					self.close()

				else:
					# jump to the setting AFTER language
					self.bypass_language()

			else:
				# if not, then revert to the previous_language
				self.selected_language = self.previous_language

		elif controlID in [30010, 30020, 30030,	30040, 30050, 30060, 30070, 30080, 30090]: # timezone containers
			

			# user has clicked on a timezone
			self.selected_country = self.getControl(controlID).getSelectedItem().getLabel()

			self.getControl(93000).setVisible(False)
			self.getControl(94000).setVisible(True)
			self.getControl(1004).setVisible(True)
			self.setFocusId(40010)

		elif controlID == 40010:			# terms and conditions I Agree button
			

			if self.vero:		# show the warranty panel

				self.getControl(94000).setVisible(False)
				self.getControl(97000).setVisible(True)
				self.getControl(1007).setVisible(True)
				self.setFocusId(70010)

			else:


				# check if internet is connected
				if self.internet_connected:
					log('internet is connected, jumping to exit')
					# skip the Networking setup menu item and go to the skin pabel
					self.getControl(94000).setVisible(False)
					self.getControl(98000).setVisible(True)
					self.getControl(1008).setVisible(True)
					self.setFocusId(80010)
				else:
					log('internet is not connected, jumping to networking')
					# display the Networking panel
					self.getControl(94000).setVisible(False)
					self.getControl(96000).setVisible(True)
					self.getControl(1006).setVisible(True)
					self.setFocusId(60010)		
	
		elif controlID == 70010:			# warranty I Agree button
			

			if self.vero:

				# check if internet is connected
				if self.internet_connected:
					log('internet is connected, jumping to exit')
					# skip the Networking setup menu item and go to skin selection
					self.getControl(97000).setVisible(False)
					self.getControl(98000).setVisible(True)
					self.getControl(1008).setVisible(True)
					self.setFocusId(80010)
				else:
					log('internet is not connected, jumping to networking')
					# display the Networking panel
					self.getControl(97000).setVisible(False)
					self.getControl(96000).setVisible(True)
					self.getControl(1006).setVisible(True)
					self.setFocusId(60010)	

			else:
				pass							

		elif controlID == 40020:			# unused scroll bar for TandC
			

			self.getControl(555).scroll(10)

		elif controlID == 40030:			# unused scroll bar for TandC
			

			self.getControl(555).scroll(-10)

		elif controlID == 60090:			# skip networking button

			# display the skin panel
			self.getControl(96000).setVisible(False)
			self.getControl(98000).setVisible(True)
			self.getControl(1008).setVisible(True)
			self.setFocusId(80010)	

		elif controlID == 60010:			# open networking gui
			
			self.net_call.run(False)

			# display the skin panel
			self.getControl(96000).setVisible(False)
			self.getControl(98000).setVisible(True)
			self.getControl(1008).setVisible(True)
			self.setFocusId(80010)	

		elif controlID in [80010, 80020]:	# user has selected a skin
			
			if controlID == 80010:

				self.selected_skin = 'OSMC'

			else:

				self.selected_skin = 'Confluence'

			# display the sign-up panel
			self.getControl(98000).setVisible(False)
			self.getControl(99000).setVisible(True)
			self.getControl(1009).setVisible(True)
			self.setFocusId(90010)

		elif controlID in [90010, 90020]:	# newsletter sign up

			if controlID == 90010:

				# show keyboard
				kb = xbmc.Keyboard(self.email, 'Please enter your email')
				kb.doModal()
				if kb.isConfirmed():
					self.email = kb.getText()
					requests.post('https://osmc.tv/wp-content/plugins/newsletter/do/subscribe.php', data={'ne': self.email})

			# display the sign-up panel
			self.getControl(99000).setVisible(False)
			self.getControl(95000).setVisible(True)
			self.getControl(1005).setVisible(True)
			self.setFocusId(1005)

			# allow the user to exit
			self.prevent_escape = False


	def onAction(self, action):

		if self.prevent_escape:
			return

		if action == 10 or action == 92:

			# delete skin update block file
			subprocess.call(['sudo', 'rm', '/tmp/NO_UPDATE'])

			self.close()


	def onFocus(self, controlID):

		main_controls 	= [1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009]

		tz_controls 	= [3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009]

		skin_controls 	= [80010, 80020]

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

		elif controlID in skin_controls:

			if controlID == 80010:

				# display the OSMC skin image
				self.set_skin_image('OSMC')

			elif controlID == 80020:

				# display the confluence skin image
				self.set_skin_image('CONF')



def open_gui(networking_instance):

	__addon__        = xbmcaddon.Addon()
	scriptPath       = __addon__.getAddonInfo('path')

	xml = "walkthru_720.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "walkthru.xml"

	lang_rerun 			= False
	first_run 			= True

	selected_language 	= None

	while first_run or lang_rerun:

		first_run = False
		
		GUI = walkthru_gui(xml, scriptPath, 'Default', networking_instance=networking_instance, lang_rerun=lang_rerun, selected_language=selected_language)
		GUI.doModal()

		selected_language 	= GUI.selected_language
		skin_choice 		= GUI.selected_skin
		lang_rerun 			= GUI.lang_rerun

		# set language
		xbmc.executebuiltin('xbmc.SetGUILanguage(%s)' % selected_language)
		
		xbmc.sleep(1000)

		log('users language: %s' % selected_language)
		log('lang_rerun: %s' % lang_rerun)
		log('skin_choice: %s' % skin_choice)
		
	
	if skin_choice != 'OSMC':

		log('Loading Confluence')
		try:
			xbmc.setskin('skin.confluence')
		except:
			log('Loading Confluence failed.')

	log('Exiting GUI')

