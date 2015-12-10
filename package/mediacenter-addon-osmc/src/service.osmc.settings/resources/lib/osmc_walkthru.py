

# XBMC modules
import xbmc
import xbmcaddon
import xbmcgui

# STANDARD library modules
import os
import random
import requests
import subprocess
import sys
import threading
import traceback
import xml.etree.ElementTree as ET

sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources','lib')))

# Custom Modules
import osmc_timezones
import LICENSE
import WARRANTY

EULA       = LICENSE.license
WARR       = WARRANTY.warranty
DIALOG     = xbmcgui.Dialog()

__addon__  = xbmcaddon.Addon()
scriptPath = __addon__.getAddonInfo('path')


def log(message):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )

	xbmc.log(str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


class mock_Networking_caller(object):

	def __init__(self, parent, net_call):

		self.ftr_running = False
		self.timeout     = 0
		self.parent = parent
		self.parent.internet_connected = True

	def start(self):

		pass

	def setDaemon(self, bool):

		pass


class Networking_caller(threading.Thread):

	def __init__(self, parent, net_call):

		super(Networking_caller, self).__init__()

		self.daemon      = True
		self.cancelled   = False
		self.parent      = parent
		self.net_call    = net_call
		self.ftr_running = True
		self.timeout     = 0
		# instantiate Barkers interface class
		# self.networking_interface = NETWORKING.Barkersinterface()


	def run(self):
		"""Calls Barkers method to check for network connection"""

		log('checking internet connection')

		while self.ftr_running and self.timeout < 12:

			self.ftr_running = self.net_call.is_ftr_running()

			# break early if ftr is not running
			if not self.ftr_running: break

			self.timeout += 1

			xbmc.sleep(10000)

		if not self.ftr_running:

			self.parent.internet_connected = self.net_call.check_network(False)

		else:
			# ftr_running has timed out, consider it ended and leave internet_connected as False
			self.ftr_running = False

		log('network connection is %s' % self.parent.internet_connected)
		log('internet connection is %s' % self.net_call.check_network(True))


class walkthru_gui(xbmcgui.WindowXMLDialog):

	def __init__(	self, 
					strXMLname, 
					strFallbackPath, 
					strDefaultName, 
					networking_instance, 
					lang_rerun, 
					selected_language,
					testing=False):

		# show timezone switch
		self.showtimezone = True

		# switch that identifies whether the internet is connected
		self.internet_connected = False

		#start a new thread that begins checking for an internet connection
		self.net_call = networking_instance

		self.internet_check   = False
		if not testing:
			self.internet_checker = Networking_caller(self, self.net_call)
		else:
			self.internet_checker = mock_Networking_caller(self, self.net_call)
		self.internet_checker.setDaemon(True)
		self.internet_checker.start()

		# this flag tells us whether the GUI has been reloaded due to language selection
		self.lang_rerun = lang_rerun

		# edit the timezone in /etc/timezone
		if self.showtimezone:
			self.timezones = osmc_timezones.get_timezones()

		# this attribute denotes the skin the user wants to have applied when the walkthru closes
		self.selected_skin = 'OSMC'

		# this is the default hostname for the device
		self.device_name = 'current name: osmc'

		# this holds the users desired SSH state (True for enabled)
		self.ssh_state = True

		# this holds the users ssh password
		self.ssh_pass = 'osmc'

		# newsletter email address
		self.email = ''

		# get the languages
		self.languages = self.get_languages()
		self.languages = list(set(self.languages))
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
		for hide_this in [1003, 10035, 10037, 1004, 1005, 1006, 1007, 1008, 1009]:

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
		for visibility_control in [93000,125000,127000,94000,95000, 96000, 97000, 98000, 99000]:

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

		# set the SSH toggle to ON
		self.getControl(370010).setSelected(True)


		# this will only be True, if the language has been selected and the GUI has reloaded
		if self.lang_rerun:
			# set the flag to False so the GUI doesnt reload on exit
			self.lang_rerun = False
			self.bypass_language()


	def get_languages(self):
		# try and find language files (Kodi 14.x)
		try:
			return [folder for folder in os.listdir('/usr/share/kodi/language/')]
		except:
			pass
		# if we have not found yet try looking for laonguage addons (Kodi 15.x)
		languages = ['English']
		languagedirs = ["/home/osmc/.kodi/addons", "/usr/share/kodi/addons" ]
		language_folder_contents = []

		try:
			log("Contents of %s: %s" % (languagedirs[0], os.listdir(languagedirs[0])))
			language_folder_contents = os.listdir(languagedirs[0])
		except:
			log('%s not found' % languagedirs[0])
		
		try:
			log("Contents of %s: %s" % (languagedirs[1], os.listdir(languagedirs[1])))
			language_folder_contents = os.listdir(languagedirs[1])
		except:
			log('%s not found' % languagedirs[1])

		for folder in language_folder_contents:
			if folder.startswith('resource.language.'):
				try:
					tree = ET.parse('/home/osmc/.kodi/addons/' + folder + os.sep + 'addon.xml')
				except:
					tree = ET.parse('/usr/share/kodi/addons/' + folder + os.sep + 'addon.xml')
				root = tree.getroot()
				log('Name from %s: %s' % (folder, root.attrib['name']))
				languages.append(root.attrib['name'])
		return languages
 

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

					if 'vero' in setting:

						log('Hardware is Vero')

						return True

		log('Hardware not Vero')

		return False


	def apply_SSH_state_password(self):

		# INTERFACE TO CHANGE THE SSH State and password


		# SSH PASSWORD CHANGE NOT ALLOWED AT THIS TIME
		# try:

		# 	if self.ssh_pass != 'osmc':

		# 		log('changing ssh password')
		# 		log('STILL NEED TO WRITE THE CODE TO CHANGE THE SSH PASSWORD!!!!!!!!!')

		# except:

		# 	log('ssh password change failed')
		# 	log(traceback.format_exc())

		try:

			if self.ssh_state != True:

				log('Disabling SSH service')

				os.system("sudo /bin/systemctl disable ssh-app-osmc")
				os.system("sudo /bin/systemctl stop ssh-app-osmc")

			else:

				log('Leaving SSH service enabled')

		except:

			log('ssh state change failed')
			log(traceback.format_exc())


	def apply_hostname_change(self):

		# INTERFACE TO CHANGE THE HOSTNAME
		try:

			log('changing hostname to %s' % self.device_name.replace('current name: ', ''))
			xbmc.sethostname(self.device_name.replace('current name: ', '')) 

		except:

			log('hostname change failed')
			log(traceback.format_exc())


	def exit_proceedure(self):

		self.apply_hostname_change()

		self.apply_SSH_state_password()

		if self.selected_country != None:

			# set timezone
			for reg, cnt in self.timezones.iteritems():

				if self.selected_country in cnt:
					self.selected_region = reg
					break

		if self.selected_country != None and self.selected_region != None:

			users_timezone = "%s/%s" % (self.selected_region, self.selected_country)

			log('users timezone: %s' % users_timezone)

			try:
				xbmc.settimezone(users_timezone)
			except:
				log('Failed to set users timezone: %s' % users_timezone)
				log(traceback.format_exc())
		
		# delete skin update block file
		subprocess.call(['sudo', 'rm', '/tmp/NO_UPDATE'])

		self.close()


	def networking_page_director(self, controlID):
		''' Controls the navigation to the Networking page of the Walkthru GUI '''

		control_id_pairs = {
							40010: 94000,
							70010: 97000
							}

		# check if internet is connected
		if self.internet_connected:
			log('internet is connected, jumping to exit')

			# skip the Networking setup menu item and go to the skin panel
			# -- SUPPRESSED WHILE THE SKIN CHANGE METHOD IS WORKED ON --
			self.getControl(94000).setVisible(False)
			self.getControl(98000).setVisible(True)
			self.getControl(1008).setVisible(True)
			self.setFocusId(80010)

			# display the sign-up panel
			# -- INCLUDED ONLY WHILE THE SKIN CHANGE METHOD IS WORKED ON --
			# self.getControl(control_id_pairs.get(controlID, 94000)).setVisible(False)
			# self.getControl(99000).setVisible(True)
			# self.getControl(1009).setVisible(True)
			# self.setFocusId(90010)

		else:

			if self.internet_checker.ftr_running == True:
				# only display Please Wait Progress Bar if the ftr is still running
				self.pDialog = xbmcgui.DialogProgress()
				self.pDialog.create(lang(32025), lang(32024))

			# the starting point of the progress bar is the current cycle on the ftr_running loop * 10
			# this will allow more frequent updates to the progress bar than using the timeout value would permit
			cnt = self.internet_checker.timeout * 10.0

			while self.internet_checker.ftr_running == True:
				# ftr is still running, tell the user to Please Wait, and try again
				# this will time out after 2 minutes

				cnt += 1
				
				prog = int(min(max(int(cnt/120.0*100),1),100))

				self.pDialog.update(percent=prog)

				# break early if the user instructs to
				if self.pDialog.iscanceled(): break

				xbmc.sleep(1000)

			try:
				# wrapped in a Try as pDialog is not always created
				self.pDialog.close()
			except:
				pass

			if not self.internet_connected:

				log('internet is not connected, jumping to networking')
				# display the Networking panel
				self.getControl(control_id_pairs.get(controlID, 94000)).setVisible(False)
				self.getControl(96000).setVisible(True)
				self.getControl(1006).setVisible(True)
				self.setFocusId(60010)

			else:	# internet is connected, jump to the next appropriate page
				self.getControl(control_id_pairs.get(controlID, 94000)).setVisible(False)
				self.getControl(99000).setVisible(True)
				self.getControl(1009).setVisible(True)
				self.setFocusId(90010)


	def enter_password(self, pass_store, confirm=True, hidden=True):

		mypass = None

		# show keyboard for the first password
		kb = xbmc.Keyboard(pass_store, 'Please enter your password', hidden=hidden)

		kb.doModal()

		# only move on if the device has been given a name
		if kb.isConfirmed():

			pass1 = kb.getText()

			if not confirm:

				# if we dont want password confirmation, then just return the first password entered
				mypass = pass1

			else:

				if pass1 == pass_store:
					passhint = pass_store
				else:
					passhint = ''

				# show keyboard
				kb = xbmc.Keyboard(passhint, 'Please confirm your password', hidden=hidden)

				kb.doModal()
				
				if not kb.isConfirmed():

					# if the user escapes the entry, then just return None

					mypass = None

				else:

					pass2 = kb.getText()

					# if the passwords dont match, then give the user the option of entering via hidden or plain text kayboards
					if pass1 != pass2 and hidden == True:

						plain_text_pass = DIALOG.yesno('Password mismatch', 'Would you like to enter your password in plain text?')

						if plain_text_pass:

							mypass = self.enter_password(pass_store, confirm=confirm, hidden=False)

						else:

							mypass = self.enter_password(pass_store, confirm=confirm, hidden=True)

					# if the passwords dont match and they aren't hidden then alert the user and reshow the entry dialog
					elif pass1 != pass2:

						ok = DIALOG.ok('Password mismatch', 'Your password entries did not match')

						mypass = self.enter_password(pass_store, confirm=confirm, hidden=hidden)

					else:

						mypass = pass1

		return mypass


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
			self.getControl(125000).setVisible(True)
			self.getControl(10035).setVisible(True)
			self.setFocusId(350010)

			# move on to choosing the hostname

		elif controlID == 350010:			# choosing the hostname

			# show keyboard
			kb = xbmc.Keyboard(self.device_name.replace('current name: ', ''), 'Name your device')

			kb.doModal()

			# only move on if the device has been given a name
			if kb.isConfirmed():

				self.device_name = kb.getText()

				self.getControl(350010).setLabel('current name: ' + self.device_name.replace('_',''))

				self.getControl(350012).setVisible(True)

		elif controlID == 350011:			# user has asked for a random name

			self.device_name = self.random_name()

			self.getControl(350010).setLabel('current name: ' + self.device_name)

			self.getControl(350012).setVisible(True)

		elif controlID == 350012: 			# user has accepted the hostname

			# moved the changes to the shutdown process

			# user has chosen a hostname
			self.getControl(125000).setVisible(False)
			self.getControl(127000).setVisible(True)
			self.getControl(10037).setVisible(True)
			self.setFocusId(370010)

		elif controlID == 370010: 			# user wants to disable SSH service

			ctl = self.getControl(370010)

			if ctl.isSelected():
				# change to ENABLED
				self.ssh_state = True
				ctl.setLabel(lang(32036))
				self.setFocusId(370012)

			else:
				# change to DISABLED
				self.ssh_state = False
				ctl.setLabel(lang(32037))
				self.setFocusId(370012)

		elif controlID == 370011: 			# user wants to change the SSH password

			pass

			# SSH PASSWORD CHANGE SUPPRESSED AT THIS TIME

			# show keyboard for the first password
			# user_pass = self.enter_password(self.ssh_pass, confirm=True, hidden=True)

			# if user_pass is not None:

			# 	self.ssh_pass = user_pass

			# 	self.getControl(370011).setLabel('Click here to change/confirm password:    ' + self.ssh_pass.replace('_',''))

			# 	self.getControl(370012).setVisible(True)			

		elif controlID == 370012: 			# user has accepted the SSH settings

			# this Accept button does not actually DO anything other than show the next menu item,
			# but I think it is worthwhile having the user specifically consent to the settings

			# user has chosen a hostname
			self.getControl(127000).setVisible(False)
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

				self.networking_page_director(controlID)
	
		elif controlID == 70010:			# warranty I Agree button
			

			if self.vero:

				self.networking_page_director(controlID)

				self.getControl(97000).setVisible(False)
				self.getControl(98000).setVisible(True)
				self.getControl(1008).setVisible(True)
				self.setFocusId(80010)

			else:
				pass							

		elif controlID == 40020:			# unused scroll bar for TandC		

			self.getControl(555).scroll(10)

		elif controlID == 40030:			# unused scroll bar for TandC
			
			self.getControl(555).scroll(-10)

		elif controlID == 60090:			# skip networking button

			# display the skin panel
			# -- SUPPRESSED WHILE THE SKIN CHANGE METHOD IS WORKED ON --
			self.getControl(96000).setVisible(False)
			self.getControl(98000).setVisible(True)
			self.getControl(1008).setVisible(True)
			self.setFocusId(80010)	

			# display the sign-up panel
			# -- INCLUDED ONLY WHILE THE SKIN CHANGE METHOD IS WORKED ON --
			# self.getControl(96000).setVisible(False)
			# self.getControl(99000).setVisible(True)
			# self.getControl(1009).setVisible(True)
			# self.setFocusId(90010)

		elif controlID == 60010:			# open networking gui
			
			self.net_call.run(False)

			# display the skin panel  
			# -- SUPPRESSED WHILE THE SKIN CHANGE METHOD IS WORKED ON --
			self.getControl(96000).setVisible(False)
			self.getControl(98000).setVisible(True)
			self.getControl(1008).setVisible(True)
			self.setFocusId(80010)	

			# display the sign-up panel
			# -- INCLUDED ONLY WHILE THE SKIN CHANGE METHOD IS WORKED ON --
			# self.getControl(96000).setVisible(False)
			# self.getControl(99000).setVisible(True)
			# self.getControl(1009).setVisible(True)
			# self.setFocusId(90010)

		elif controlID in [80010, 80020]:	# user has selected a skin

			# -- THIS SECTION IS SUPPRESSED WHILE THE SKIN CHANGE METHOD IS WORKED ON  --
			# -- IT CANNOT BE REACHED AS THE PANEL WITH THE BUTTONS IS NEVER DISPLAYED --
			
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


	def random_name(self):

		names = [	
				"Alfonse", "Barnaby", "Aloyisius", "Archibald", "Algernon", "Basil", "Bertram", "Carston", "Cavendish", "Cecil", 
				"Cyril", "Danforth", "Cuthbert", "Alastair", "Preston", "Giles", "Cortland", "Atticus",
				"Edmund", "Gilbert", "Ethelbert", "Frederick", "Geoffrey", "Gideon", "Giggleswick", "Grumbole", "Hamilton",
				"Ignatius", "Ebenezer", "Herbert", "Clement", "Humphrey", "Ian", "Ichabod", "Jonathan", "Malcolm",
				"Mervyn", "Mortimer", "Nigel", "Percy", "Prentis", "Reginald", "Ridgewell", "Royston",
				"Theophilus", "Tobias", "Tristram", "Ulysses", "Ulrich", "Virgil", "Vivian", "Waldo",
				"Wesley", "Wilbur", "Wilfred", "Willard", "Willoughby",
				]

		return "osmc-" + random.choice(names)


	def onAction(self, action):

		if self.prevent_escape:
			return

		if action == 10 or action == 92:

			self.close()


	def onFocus(self, controlID):

		main_controls 	= [1002, 1003, 10035, 10037, 1004, 1005, 1006, 1007, 1008, 1009]

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

			# -- THIS SECTION IS SUPPRESSED WHILE THE SKIN CHANGE METHOD IS WORKED ON  --
			# -- IT CANNOT BE REACHED AS THE PANEL WITH THE SKIN CONTROLS IS NEVER MADE VISIBLE --


			if controlID == 80010:

				# display the OSMC skin image
				self.set_skin_image('OSMC')

			elif controlID == 80020:

				# display the confluence skin image
				self.set_skin_image('CONF')


def open_gui(networking_instance, testing=False):

	__addon__        = xbmcaddon.Addon()
	scriptPath       = __addon__.getAddonInfo('path')

	xml = "walkthru_720.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "walkthru.xml"

	lang_rerun 			= False
	first_run 			= True

	selected_language 	= None

	while first_run or lang_rerun:

		first_run = False
		
		GUI = walkthru_gui(xml, scriptPath, 'Default', networking_instance=networking_instance, lang_rerun=lang_rerun, selected_language=selected_language, testing=testing)
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
		
	# -- THIS SECTION SHOULD BE SUPPRESSED WHILE THE SKIN CHANGE METHOD IS WORKED ON  --
	if skin_choice != 'OSMC':

		log('Loading Confluence')
		try:
			xbmc.setskin('skin.confluence')
		except:
			log('Loading Confluence failed.')

	log('Exiting GUI')

