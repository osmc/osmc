

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


PANEL_MAP = {
	'language': {
				'order'					:			1,
				'panel_menu_item_id'	:			1002,
				'visibility_controller' :			92000,
				'default_control'		:			20010,
				},
	'timezone': {
				'order'					:			2,
				'panel_menu_item_id'	:			1003,
				'visibility_controller' :			93000,
				'default_control'		:			3001,
				},	
	'hostname': {
				'order'					:			3,
				'panel_menu_item_id'	:			10035,
				'visibility_controller' :			125000,
				'default_control'		:			350010,
				},
	'SSH': 		{
				'order'					:			4,
				'panel_menu_item_id'	:			10037,
				'visibility_controller' :			127000,
				'default_control'		:			370010,
				},		
	'TandC': 	{
				'order'					:			5,
				'panel_menu_item_id'	:			1004,
				'visibility_controller' :			94000,
				'default_control'		:			40010,
				},
	'warranty': {
				'order'					:			6,
				'panel_menu_item_id'	:			1007,
				'visibility_controller' :			97000,
				'default_control'		:			70010,
				},	
	'networking':{
				'order'					:			7,
				'panel_menu_item_id'	:			1006,
				'visibility_controller' :			96000,
				'default_control'		:			60010,
				},
	'skin': 	{
				'order'					:			8,
				'panel_menu_item_id'	:			1008,
				'visibility_controller' :			98000,
				'default_control'		:			80010,
				},		
	'newsletter':{				
				'order'					:			9,
				'panel_menu_item_id'	:			1009,
				'visibility_controller' :			99000,
				'default_control'		:			90010,
				},	
	'exit': 	{
				'order'					:			10,
				'panel_menu_item_id'	:			1005,
				'visibility_controller' :			95000,
				'default_control'		:			1005,
				},	
}


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

		self.testing = testing

		# the order of the panels, this list can be changed depending on the specific need
		self.panel_order 	= sorted(PANEL_MAP.keys(), key=lambda x: PANEL_MAP[x]['order'])

		# the specific controlIDs for the main menu items and others, this is used by onFocus and saves recreating the list every time
		self.menu_controls 	= [v['panel_menu_item_id'] for v in PANEL_MAP.values()]
		self.tz_controls 	= [3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009]
		self.skin_controls 	= [80010, 80020]

		# switch that identifies whether the internet is connected, there is also a flag to determine if the
		# networking panel has already been revealed.
		self.internet_connected = False
		self.networking_panel_revealed = False

		#start a new thread that begins checking for an internet connection
		self.start_checking_for_internet(networking_instance)

		# this flag tells us whether the GUI has been reloaded due to language selection
		self.lang_rerun = lang_rerun

		# get the list of timezones in /etc/timezone
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

		# if the device is not recognised as a vero, then remove the warranty panel from the walkthrough
		if not self.is_Vero():
			self.panel_order.remove('warranty')

		# this attribute is used to determine when the user is allowed to exit the walkthru using the Esc or Back buttons
		self.prevent_escape = True


	def onInit(self):

		global EULA
		global WARR

		self.hide_controls_on_init()

		self.populate_language_controls()
		self.populate_timezone_controls()

		# populate the terms and conditions and the warranty
		self.getControl(555).setText(EULA)
		self.getControl(777).setText(WARR)		

		# set the image for the skin preview control
		self.set_skin_image('OSMC')

		# set the SSH toggle to ON
		self.getControl(370010).setSelected(True)

		self.remove_coversheet()

		# this will only be True, if the language has been selected and the GUI has reloaded
		if self.lang_rerun:
			# set the flag to False so the GUI doesnt reload on exit
			self.lang_rerun = False
			self.reveal_next_panel('language')


	def start_checking_for_internet(self, networking_instance):
		self.net_call = networking_instance

		self.internet_check = False
		if not self.testing:
			self.internet_checker = Networking_caller(self, self.net_call)
		else:
			self.internet_checker = mock_Networking_caller(self, self.net_call)
		self.internet_checker.setDaemon(True)
		self.internet_checker.start()


	def remove_coversheet(self):
		''' When the window is first loaded, this coversheet (repeated of the background) covers everything.
			This allows the controls in the background to be hidden discretely. This method simply removes the cover
			sheet as the final step of the Init. '''

		self.getControl(123456789).setVisible(False)


	def hide_controls_on_init(self):

		#hide all timezone, TandC and Apply buttons
		for hide_this in [1003, 10035, 10037, 1004, 1005, 1006, 1007, 1008, 1009]:

			self.getControl(hide_this).setVisible(False)

		# hide the controls that determine panel visibility
		for visibility_control in [93000,125000,127000,94000,95000, 96000, 97000, 98000, 99000]:

			self.getControl(visibility_control).setVisible(False)

		# hide the language sub menus
		for tz in [3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009]:

			self.getControl(tz*10).setVisible(False)


	def populate_language_controls(self):

		# populate the language control
		for language in self.languages:

			self.tmp = xbmcgui.ListItem(label=language, label2='', thumbnailImage='')

			self.getControl(20010).addItem(self.tmp)


	def populate_timezone_controls(self):

		# populate the timezone controls
		for region, countries in self.timezones.iteritems():

			for country in countries:

				ctl_id = self.tz_control_map.get(region, False)

				if not ctl_id: continue

				self.tmp = xbmcgui.ListItem(label=country, label2='', thumbnailImage='')

				self.getControl(ctl_id).addItem(self.tmp)		


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
 

	def set_skin_image(self, skin):

		''' Sets the image for the skin preview '''

		if skin == 'CONF':
			self.getControl(88888).setImage(self.conf_skin_image)
		else:
			self.getControl(88888).setImage(self.osmc_skin_image)


	def is_Vero(self):
		'''
			Checks whether this is a Vero and whether the warranty info should be shown 
		'''

		# generate the URL
		with open('/proc/cmdline', 'r') as f:

			line = f.readline()

			settings = line.split(' ')

			for setting in settings:

				if setting.startswith('osmcdev='):

					if 'vero' in setting or 'vero2' in setting:

						log('Hardware is Vero')

						return True

		log('Hardware not Vero')

		return False


	def apply_SSH_state_password(self):


		try:

			if self.ssh_state != True:

				log('Disabling SSH service')

				os.system("/bin/sudo /bin/systemctl disable ssh")
				os.system("/bin/sudo /bin/systemctl stop ssh")

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


	def still_checking_for_network(self, controlID):
		''' Checks to see if the internet checker has finished, and if not, shows a progress bar until complete or exit. 
			The internet checker will toggle the internet_connected bool if a connection is made.'''

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

			# break early if the user instructs to do so
			if self.pDialog.iscanceled(): 
				self.internet_checker.ftr_running = False
				break

			xbmc.sleep(1000)

		try:
			# wrapped in a Try as pDialog is not always created
			self.pDialog.close()
		except:
			pass


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


	def language_clicked(self, controlID):

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
				pass
		else:
			# if not, then revert to the previous_language
			self.selected_language = self.previous_language


	def choose_hostname(self):

		# show keyboard
		kb = xbmc.Keyboard(self.device_name.replace('current name: ', ''), 'Name your device')

		kb.doModal()

		# only move on if the device has been given a name
		if kb.isConfirmed():

			self.device_name = kb.getText()

			self.getControl(350010).setLabel('current name: ' + self.device_name.replace('_',''))

			self.getControl(350012).setVisible(True)


	def requesting_random_name(self):

		self.device_name = self.random_name()

		self.getControl(350010).setLabel('current name: ' + self.device_name)

		self.getControl(350012).setVisible(True)		


	def toggle_ssh(self):

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


	def enter_ssh_password(self):

		# show keyboard for the first password
		user_pass = self.enter_password(self.ssh_pass, confirm=True, hidden=True)

		if user_pass is not None:

			self.ssh_pass = user_pass

			self.getControl(370011).setLabel('Click here to change/confirm password:    ' + self.ssh_pass.replace('_',''))

			self.getControl(370012).setVisible(True)	


	def newsletter_signup(self, controlID):

		if controlID == 90010:

			# show keyboard
			kb = xbmc.Keyboard(self.email, 'Please enter your email')
			kb.doModal()
			if kb.isConfirmed():
				self.email = kb.getText()
				requests.post('https://osmc.tv/wp-content/plugins/newsletter/do/subscribe.php', data={'ne': self.email})	


	def get_selected_country(self, controlID):
		
		self.selected_country = self.getControl(controlID).getSelectedItem().getLabel()


	def onClick(self, controlID):

		if controlID == 1005:				# Exit control

			self.exit_proceedure()

		elif controlID == 20010:			# language container

			self.language_clicked(controlID)

			self.reveal_next_panel(current_panel='language')

		elif controlID in [30010, 30020, 30030,	30040, 30050, 30060, 30070, 30080, 30090]: # timezone containers
			
			self.get_selected_country(controlID)

			self.reveal_next_panel(current_panel='timezone')

		elif controlID == 350010:			# choosing the hostname

			self.choose_hostname()

		elif controlID == 350011:			# user has asked for a random name

			self.requesting_random_name()

		elif controlID == 350012: 			# user has accepted the hostname

			self.reveal_next_panel(current_panel='hostname')

		elif controlID == 370010: 			# user wants to disable SSH service

			self.toggle_ssh()

		elif controlID == 370011: 			# user wants to change the SSH password DEPRECATED

			# self.enter_ssh_password()
			pass

		elif controlID == 370012: 			# user has accepted the SSH settings

			self.reveal_next_panel(current_panel='SSH')	

		elif controlID == 40010:			# terms and conditions I Agree button	

			self.reveal_next_panel(current_panel='TandC')

		elif controlID == 70010:			# warranty I Agree button
			
			self.reveal_next_panel(current_panel='warranty')

		elif controlID == 40020:			# unused scroll bar for TandC		

			self.getControl(555).scroll(10)

		elif controlID == 40030:			# unused scroll bar for TandC
			
			self.getControl(555).scroll(-10)

		elif controlID == 60090:			# skip networking button

			self.reveal_next_panel(current_panel='networking')

		elif controlID == 60010:			# open networking gui
			
			self.net_call.run(False)

			self.reveal_next_panel(current_panel='networking')

		elif controlID in [80010, 80020]:	# user has selected a skin

			self.selected_skin = 'OSMC' if controlID == 80010 else 'Confluence'

			self.reveal_next_panel(current_panel='skin')

		elif controlID in [90010, 90020]:	# newsletter sign up

			self.newsletter_signup(controlID)

			self.reveal_next_panel(current_panel='newsletter')

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


	def networking_special_handling(self, next_panel):

		if not self.networking_panel_revealed:

			if self.internet_connected:

				# if the internet is connected and the networking panel has not already been revealed, then just jump to the
				# next panel after networking
				next_panel = self.panel_order[self.panel_order.index('networking') + 1]
				self.panel_order.remove('networking')

			else:
				# if the internet is NOT connected, and the networking panel has NOT been revealed then show the internet checker progress bar. 
				# Once that is complete (or cancelled) then check for the internet connection again
				# and show the networking panel if negative, or jump to the next panel if positive.
				self.still_checking_for_network()

				if self.internet_connected:
					next_panel = self.panel_order[self.panel_order.index('networking') + 1]
					self.panel_order.remove('networking')

				self.networking_panel_revealed = True

		return next_panel


	def reveal_next_panel(self, current_panel):
		''' Builds the menu as we proceed.
			Current_panel is the string name of the panel we are moving from. '''

		try:
			next_panel = self.panel_order[self.panel_order.index(current_panel) + 1]
		except IndexError:
			next_panel = 'exit'

		log('Changing from %s to %s' % (current_panel, next_panel))

		# special workaround to only show the networking panel if we havent been able to connect to the internet
		if next_panel == 'networking':
			next_panel = self.networking_special_handling(next_panel)

		# show the next panels menu item
		self.getControl(PANEL_MAP[next_panel]['panel_menu_item_id']).setVisible(True)

		# display the next panel
		self.show_panel(next_panel)

		# focus on the default control on that new panel
		self.setFocusId(PANEL_MAP[next_panel]['default_control'])

		# set the up/down controls for the menu item
		old_menuitem_id = PANEL_MAP[current_panel]['panel_menu_item_id']
		new_menuitem_id = PANEL_MAP[next_panel]['panel_menu_item_id']

		self.getControl(new_menuitem_id).controlUp(self.getControl(old_menuitem_id))
		self.getControl(old_menuitem_id).controlDown(self.getControl(new_menuitem_id))


	def show_panel(self, controlID):

		for panel_name, control_dict in PANEL_MAP.iteritems():

			if controlID == panel_name or controlID == control_dict['panel_menu_item_id']:

				self.getControl(control_dict['visibility_controller']).setVisible(True)

			else:

				self.getControl(control_dict['visibility_controller']).setVisible(False)


	def onFocus(self, controlID):


		if controlID in self.menu_controls:

			self.show_panel(controlID)

		elif controlID in self.tz_controls:

			for tz in self.tz_controls:

				ctl = self.getControl(tz * 10)

				if tz == controlID:

					ctl.setVisible(True)

				else:

					ctl.setVisible(False)

		elif controlID in self.skin_controls:

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

