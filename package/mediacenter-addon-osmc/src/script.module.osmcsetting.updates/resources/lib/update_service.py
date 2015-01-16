# Standard Modules
import apt
from datetime import datetime
import os
import sys
import subprocess
import Queue
import random
import json

# Kodi Modules
import xbmc
import xbmcaddon
import xbmcgui

# Custom modules
__libpath__ = xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources','lib'))
sys.path.append(__libpath__)
import comms
import simple_scheduler as sched


__addon__              	= xbmcaddon.Addon()
__addonid__            	= __addon__.getAddonInfo('id')
__scriptPath__         	= __addon__.getAddonInfo('path')
__setting__            	= __addon__.getSetting
__image_file__         	= os.path.join(__scriptPath__,'resources','media','update_available.png')
lang 					= __addon__.getLocalizedString

DIALOG  = xbmcgui.Dialog()


def log(message, label = ''):
	logmsg       = '%s : %s - %s ' % (__addonid__ , str(label), str(message))
	xbmc.log(msg = logmsg)


class Monitah(xbmc.Monitor):

	def __init__(self, **kwargs):
		super(Monitah, self).__init__()

		self.parent_queue = kwargs['parent_queue']


	def onAbortRequested(self):

		log('killing self')

		msg = json.dumps(('kill_yourself', {}))

		self.parent_queue.put(msg)


	def onSettingsChanged(self):
		log('settings changed!!!!!!!!!!!!!!!!!!!!!!!')

		msg = json.dumps(('update_settings', {}))

		self.parent_queue.put(msg)

		log(self.parent_queue, 'self.parent_queue')



class Main(object):

	''' This service allows for the checking for new updates, then:
			- posts a notification on the home screen to say there is an update available, or
			- calls for the download of the updates
			- calls for the installation of the updates 
			- restarts Kodi to implement changes
		The check for updates is done using the python-apt module. This module must be run as root, so is being called in
		external scripts from the command line using sudo. The other script communicates with the update service using a socket file.
	'''
	
	# MAIN METHOD
	def __init__(self):

		self.first_run = True

		# the time that the service started
		self.service_start = datetime.now()

		# dictionary containing the permissable actions (communicated from the child apt scripts) 
		# and the corresponding methods in the parent
		self.action_dict = 	{
								'apt_cache update complete' : self.apt_update_complete,
								'apt_cache commit complete' : self.apt_commit_complete,
								'apt_cache fetch complete'  : self.apt_fetch_complete,
								'progress_bar'				: self.progress_bar,
								'update_settings'			: self.update_settings,
								'update_now'				: self.update_now,
								'user_update_now'			: self.user_update_now,
								'kill_yourself'				: self.kill_yourself,
								'settings_command'			: self.settings_command,

							}

		# queue for communication with the comm and Main
		self.parent_queue = Queue.Queue()

		self.randomid = random.randint(0,1000)

		self.REBOOT_REQUIRED = 0

		# create socket, listen for comms
		self.listener = comms.communicator(self.parent_queue, socket_file='/var/tmp/osmc.settings.update.sockfile')
		self.listener.start()

		# grab the settings, saves them into a dict called seld.s
		self.update_settings()

		# a class to handle scheduling update checks
		self.scheduler = sched.SimpleScheduler(self.s)
		log(self.scheduler.trigger_time, 'trigger_time')

		# this holding pattern holds a function that represents the completion of a process that was put on hold
		# while the user was watching media or the system was active
		self.function_holding_pattern = False

		# monitor for identifying addon settings updates and kodi abort requests
		self.monitor = Monitah(parent_queue = self.parent_queue)

		# window onto which to paste the update notification
		self.window = xbmcgui.Window(10000)

		# property which determines whether the notification should be pasted to the window
		self.window.setProperty('OSMC_notification','false')

		# ControlImage(x, y, width, height, filename[, aspectRatio, colorDiffuse])
		self.update_image = xbmcgui.ControlImage(15, 55, 175, 75, __image_file__)
		self.position_icon()
		self.window.addControl(self.update_image)
		self.update_image.setVisibleCondition('[SubString(Window(Home).Property(OSMC_notification), true, left)]')

		# this flag is present when updates have been downloaded but the user wants to choose when to install using
		# the manual control in the settings
		self.block_update_file = '/var/tmp/.suppress_osmc_update_checks'
		
		# if the file is present, then suppress further update checks and show the notification
		if os.path.isfile(self.block_update_file):
			self.skip_update_check = True 

			# if the user has suppressed icon notification of updates and has chosen not to install the updates
			# its their own damned fault if osmc never get updated
			if not self.s['suppress_icon']:
				self.window.setProperty('OSMC_notification', 'true')

		else:

			self.skip_update_check =  False

		# a preliminary check for updates (for testing only)
		if self.s['check_onboot']:
			if not self.skip_update_check:
				self.boot_delay_passed = False
				self.function_holding_pattern = self.holding_pattern_boot_update
				log('boot update check put into holding pattern')

		# keep alive method
		self._daemon()


	# MAIN METHOD
	def _daemon(self):

		log('_daemon started')

		self.keep_alive = True

		count = 0 	# FOR TESTING ONLY

		while self.keep_alive:

			# periodic announcement to confirm the service is alive
			# FOR TESTING ONLY
			if not count % 100:									# FOR TESTING ONLY
				xml = xbmc.getInfoLabel('Window.Property(xmlfile)')
				log('blurp %s - %s' % (self.randomid, xml))					# FOR TESTING ONLY
			count += 1 											# FOR TESTING ONLY
			# FOR TESTING ONLY

			# check the aciton queue
			self.check_action_queue()

			# check the holding pattern, call item in holding pattern
			if self.function_holding_pattern:

				self.function_holding_pattern()

			# check for an early exit
			if not self.keep_alive: break

			# this controls the frequency of the instruction processing
			xbmc.sleep(500)


		self.exit_procedure()


	# HOLDING PATTERN METHOD
	def holding_pattern_update(self):

		if self.check_update_conditions():
			self.function_holding_pattern = False
			self.user_update_now()


	# HOLDING PATTERN METHOD
	def holding_pattern_boot_update(self):

		if (datetime.now() - self.service_start).total_seconds() > (self.s['check_boot_delay'] * 60):
			self.boot_delay_passed = True

		if self.boot_delay_passed:
			if self.check_update_conditions(media_only=True):
				self.function_holding_pattern = False
				self.call_child_script('update')


	# HOLDING PATTERN METHOD
	def holding_pattern_fetched(self, bypass=False):

		# stay in the holding pattern until the user returns to the Home screen
		if 'Home.xml' in xbmc.getInfoLabel('Window.Property(xmlfile)') or bypass:

			self.function_holding_pattern = False

			if not self.REBOOT_REQUIRED:

				install_now = DIALOG.yesno('OSMC Update Available', 'Updates have been downloaded and can be installed immediately.', 'Would you like to exit and install the updates now?')

				if install_now:

					self.call_child_script('commit')

					return

			else:

				exit_install = DIALOG.yesno('OSMC Update Available', 'Updates have been downloaded, but Kodi will need to exit to install them.', 'Would you like to exit and install the updates now?')

				if exit_install:

					subprocess.Popen(['sudo', 'systemctl', 'start', 'manual-update'])

					return


			# if the code reaches this far, the user has elected not to install right away
			# so we will need to suppress further update checks until the update occurs
			# we put a file there to make sure the suppression carries over after a reboot
				
			# create the file that will prevent further update checks until the updates have been installed
			with open(self.block_update_file, 'w') as f:
				f.write('d')

			# trigger the flag to skip update checks
			self.skip_update_check = True

			if not self.s['suppress_icon']:
				self.window.setProperty('OSMC_notification', 'true')


	# MAIN METHOD
	def exit_procedure(self):

		# stop the listener
		self.listener.stop()
		# del self.listener
		# log('listener cleaned up')

		del self.monitor
		log('del self.monitor')
		del self.update_image
		log('del self.update_image')

		del self.window 
		log('del self.window')

		# self.takedown_notification()
		# log('notification control removed from window(10000)')

		log('XBMC Aborting')


	# MAIN METHOD
	def check_action_queue(self):
		''' Checks the queue for data, if present it calls the appropriate method and supplies the data ''' 
		
		try:
			# the only thing the script should be sent is a tuple ('instruction as string', data as dict),
			# everything else is ignored
			raw_comm_from_script = self.parent_queue.get(False)
			
			# tell the queue that we are done with the task at hand
			self.parent_queue.task_done()

			# de-serialise the message into its original tuple
			comm_from_script = json.loads(raw_comm_from_script)

			log(comm_from_script, 'comm_from_script')

			# process the information from the child scripts
			if comm_from_script:

				# retrieve the relevant method
				method = self.action_dict.get(comm_from_script[0], False)
				if method: 

					# call the appropriate method with the data
					method(**comm_from_script[1])

				else:

					log(comm_from_script, 'instruction has no assigned method')

		except Queue.Empty:
			# the only exception that should be handled is when the queue is empty
			pass


	# MAIN METHOD
	def check_update_conditions(self, media_only=False):
		''' Checks the users update conditions are met. The media-only flag restricts the condition check to
			only the media playing condition. '''

		if self.s['ban_update_media']:
			result_raw = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "method": "Player.GetActivePlayers", "id": 1 }')
			result = json.loads(result_raw)
			log(result, 'result of Player.GetActivePlayers')
			players = result.get('result', False)
			if players:
				log('Update CONDITION : player playing')
				return False

		idle = xbmc.getGlobalIdleTime()
		if self.s['update_on_idle'] and idle < 60 and not media_only:
			log('Update CONDITION : idle time = %s' % idle)
			return False

		return True


	# MAIN METHOD
	def takedown_notification(self):
		log('taking down notification')

		try:
			self.window.removeControl(self.update_image)
		except Exception as e:
			log(e, 'an EXCEPTION occurred')


	# MAIN METHOD
	def call_child_script(self, action):
		
		log(action, 'calling child, action ')
		subprocess.Popen(['sudo', 'python','%s/apt_cache_action.py' % __libpath__, action])


	# MAIN METHOD
	def position_icon(self):
		''' sets the position of the icon '''

		return
		# this is suppressed for the time-being while we try and find a way to detect the skin resolution

		w = 1920
		h = 1080

		x_pct = int(self.s['pos_x'] / 100.0 * w )
		y_pct = int(self.s['pos_y'] / 100.0 * h )

		self.update_image.setPosition(x_pct, y_pct)


	# MAIN METHOD
	def update_settings(self):

		''' Updates the settings for the service while the service is still running '''

		log('Updating Settings...')

		if self.first_run:

			''' Construct the settings dicionary '''

			self.first_run = False

			self.scheduler_settings = ['check_freq', 'check_weekday', 'check_day', 'check_time', 'check_hour', 'check_minute']
			# self.icon_settings		= ['pos_x', 'pos_y']

			self.on_upd = [lang(x) for x in [32057,32058,32059,32060,32061,32062]]
			
			self.s = {}

			log(__setting__('on_upd_detected'))

			self.s['on_upd_detected']	= int(			__setting__('on_upd_detected')		)
			self.s['check_freq'] 		= int(			__setting__('check_freq')			)
			self.s['check_weekday'] 	= int(float(	__setting__('check_weekday')		))
			self.s['check_day'] 		= int(float(	__setting__('check_day')			))
			self.s['check_time'] 		= int(float(	__setting__('check_time')			))
			self.s['check_hour'] 		= int(float(	__setting__('check_hour')			))
			self.s['check_minute'] 		= int(float(	__setting__('check_minute')			))
			self.s['check_boot_delay']	= int(float(	__setting__('check_boot_delay')		))
			# self.s['pos_x']				= int(float(	__setting__('pos_x')				))
			# self.s['pos_y']				= int(float(	__setting__('pos_y')				))
			self.s['check_onboot']		= True if 		__setting__('check_onboot') 		== 'true' else False
			self.s['suppress_progress']	= True if 		__setting__('suppress_progress') 	== 'true' else False
			self.s['suppress_icon']		= True if 		__setting__('suppress_icon') 		== 'true' else False
			self.s['ban_update_media']	= True if 		__setting__('ban_update_media') 	== 'true' else False
			self.s['update_on_idle']	= True if 		__setting__('update_on_idle') 		== 'true' else False
			self.s['home_prompts_only']	= True if 		__setting__('home_prompts_only') 	== 'true' else False

			log(self.s, 'Initial Settings')

			return

		else:

			''' Construct a temporary dictionary for comparison with the existing settings dict '''

			tmp_s = {}

			tmp_s['on_upd_detected']	= int(			__setting__('on_upd_detected')		)
			tmp_s['check_freq'] 		= int(			__setting__('check_freq')			)
			tmp_s['check_weekday'] 		= int(float(	__setting__('check_weekday')		))
			tmp_s['check_day'] 			= int(float(	__setting__('check_day')			))
			tmp_s['check_time'] 		= int(float(	__setting__('check_time')			))
			tmp_s['check_hour'] 		= int(float(	__setting__('check_hour')			))
			tmp_s['check_minute'] 		= int(float(	__setting__('check_minute')			))
			tmp_s['check_boot_delay']	= int(float(	__setting__('check_boot_delay')		))
			# tmp_s['pos_x']				= int(float(	__setting__('pos_x')				))
			# tmp_s['pos_y']				= int(float(	__setting__('pos_y')				))			
			tmp_s['check_onboot']		= True if 		__setting__('check_onboot') 		== 'true' else False
			tmp_s['suppress_progress']	= True if 		__setting__('suppress_progress') 	== 'true' else False
			tmp_s['suppress_icon']		= True if 		__setting__('suppress_icon') 		== 'true' else False
			tmp_s['ban_update_media']	= True if 		__setting__('ban_update_media') 	== 'true' else False
			tmp_s['update_on_idle']		= True if 		__setting__('update_on_idle') 		== 'true' else False
			tmp_s['home_prompts_only']	= True if 		__setting__('home_prompts_only') 	== 'true' else False

		# flags to determine whether the update scheduler needs to be reconstructed or icon repositioned
		update_scheduler = False
		reposition_icon  = False

		# check the items in the temp dict and if they are differenct from the current settings, change the current settings,
		# prompt action if certain settings are changed (like the scheduler settings)
		for k, v in tmp_s.iteritems():

			if v == self.s[k]:
				continue
			else:
				self.s[k] = v
				if k in self.scheduler_settings:
					update_scheduler = True
				# elif k in self.icon_settings:
					# reposition_icon = True

		# reconstruct the scheduler if needed
		if update_scheduler:
			self.scheduler = sched.SimpleScheduler(self.s)

		# reposition the icon on the home screen
		if reposition_icon:
			self.position_icon()

		log(self.scheduler.trigger_time, 'trigger_time')


	# ACTION METHOD
	def progress_bar(self, **kwargs):

		''' Controls the creation and updating of the background prgress bar in kodi.
			The data gets sent from the apt_cache_action script via the socket
			percent, 	must be an integer
			heading,	string containing the running total of items, bytes and speed
			message, 	string containing the name of the package or the active process.
		 '''

		# return immediately if the user has suppressed on-screen progress updates or kwargs is empty
		if self.s['suppress_progress'] or not kwargs: return

		log(kwargs, 'kwargs')

		# check for kill order in kwargs
		kill = kwargs.get('kill', False)

		if kill:
			# if it is present, kill the dialog and delete it
			
			try:
				self.pDialog.close()
				del self.pDialog
			except:
				pass

			return

		# retrieve the necessary data for the progress dialog, if the data isnt supplied, then use 'nix' in its place
		# the progress dialog update has 3 optional arguments
		percent = kwargs.get('percent','nix')
		heading = kwargs.get('heading','nix')
		message = kwargs.get('message', 'nix')

		# create a dict of the actionable arguments
		keys = ['percent', 'heading', 'message']
		args = [percent, heading, message]
		update_args = {k:v for k, v in zip(keys, args) if v != 'nix'}

		# try to update the progress dialog
		try:
			log(update_args, 'update_args')
			self.pDialog.update(**update_args)

		except AttributeError:

			# on an AttributeError create the dialog and start showing it, the AttributeError will be raised if pDialog doesnt exist

			self.pDialog = xbmcgui.DialogProgressBG()
			self.pDialog.create('OSMC Update', 'Update Running.')

			self.pDialog.update(**update_args)

		except Exception as e:

			# on any other error, just log it and try to remove the dialog from the screen 

			log(e, 'pDialog has encountered and error')

			try:
				self.pDialog.close()
				del self.pDialog
			except:
				# a name error here is not interesting
				pass


	# ACTION METHOD
	def kill_yourself(self):

		self.keep_alive = False 


	# ACTION METHOD
	def update_now(self):
		''' Calls for an update check via the external script. This method checks if media is playing or whether the system has 
			been idle for two minutes before allowing the update. If an update is requested, but media is playing or the system
			isnt idle, then the update request is put into a loop, with the daemon checking periodically to see if the situation 
			has changed. '''

		# do not do anything while there is something in the holding pattern
		if self.function_holding_pattern: return

		if self.check_update_conditions():

			self.call_child_script('update')
		
		else:

			self.function_holding_pattern = self.holding_pattern_update


	# ACTION METHOD
	def user_update_now(self):
		''' Similar to update_now, but as this is a users request, forego all the player and idle checks. '''

		self.call_child_script('update')


	# ACTION METHOD
	def apt_commit_complete(self):

		# on commit complete, remove the notification from the Home window

		self.window.setProperty('OSMC_notification', 'false')

		# remove the file that blocks further update checks
		try:
			os.remove(self.block_update_file)
		except:
			pass

		if self.check_if_reboot_required():
			# the files flagging that an installed package needs a reboot are present

			if self.s['on_upd_detected'] == 5:
				# Download, install, auto-restart if needed

				xbmc.executebuiltin('Reboot')

			else:

				# 0 "Prompt for all actions" -- PROMPT
				# 1 "Display icon on home screen only" -- PROMPT
				# 2 "Download updates, then prompt" -- PROMPT
				# 3 "Download and display icon" -- PROMPT
				# 4 "Download, install, prompt if restart needed" -- PROMPT

				# display dialogue saying that osmc needs to reboot
				reboot = DIALOG.yesno('OSMC Update', 'Packages have been installed that require a reboot.', 'Would you like to reboot now?', yeslabel='Reboot Now', nolabel='Reboot Later')

				if reboot:

					xbmc.executebuiltin('Reboot')

				else:
					# skip further update checks until osmc has rebooted
					self.skip_update_check = True 

			

	# ACTION METHOD
	def apt_fetch_complete(self):

		log('apt_fetch_complete called')

		# Download and display icon
		if self.s['on_upd_detected'] == 3:
			
			log('Download complete, leaving icon displayed')

			# create the file that will prevent further update checks until the updates have been installed
			with open(self.block_update_file, 'w') as f:
				f.write('d')

		elif self.s['on_upd_detected'] == 5:
			# Download, install, auto-restart if needed

			subprocess.Popen(['sudo', 'systemctl', 'start', 'manual-update'])	

		else:
			# Download updates, then prompt
			# Download, install, prompt if restart needed (restart is needed)
			# Prompt for all actions


			if self.s['home_prompts_only']:
				log('Download complete, putting into holding pattern')
				self.function_holding_pattern = self.holding_pattern_fetched
			else:
				log('Download complete, prompting user')
				self.holding_pattern_fetched(bypass=True)


	# ACTION METHOD
	def settings_command(self, action):

		if action == 'update':

			self.call_child_script('update')

		elif action == 'install':

			if not self.REBOOT_REQUIRED:

				self.call_child_script('commit')

			else:

				subprocess.Popen(['sudo', 'systemctl', 'start', 'manual-update'])			


	# ACTION METHOD
	def apt_update_complete(self):

		self.cache = apt.Cache()

		self.REBOOT_REQUIRED = 0

		log('apt_update_complete called')

		self.cache.open(None)
		self.cache.upgrade()

		available_updates = self.cache.get_changes()

		del self.cache

		# if 'osmc' isnt in the name of any available updates, then return without doing anything
		# SUPPRESSED FOR TESTING
		# if not any(['osmc' in x.shortname.lower() for x in available_updates]):
		# 	log('There are no osmc packages')
		# 	return
		if not available_updates: return 		# dont bother doing anything else if there are no updates FOR TESTING ONLY

		log('The following packages have newer versions and are upgradable: ')

		for pkg in available_updates:
			if pkg.is_upgradable:

				log('is upgradeable', pkg.shortname)

				if "mediacenter" in pkg.shortname:
					self.REBOOT_REQUIRED = 1

		# display update available notification
		if not self.s['suppress_icon']:
			self.window.setProperty('OSMC_notification', 'true')

		# The following section implements the procedure that the user has chosen to take place when updates are detected

		if self.s['on_upd_detected'] == 1: 
			# Display icon on home screen only
			return

		elif (self.s['on_upd_detected'] in [2, 3, 5]) or (self.s['on_upd_detected'] == 4 and self.REBOOT_REQUIRED):
			# Download updates, then prompt
			# Download and display icon
			# Download, install, prompt if restart needed (restart is needed)
			# Download, install, auto-restart if needed
			self.call_child_script('fetch')
			return

		elif self.s['on_upd_detected'] == 4 and not self.REBOOT_REQUIRED:
			# Download, install, prompt if restart needed (restart is not needed)
			self.call_child_script('commit')
			return

		elif self.s['on_upd_detected'] == 0:
			# show all prompts (default)

			if self.REBOOT_REQUIRED == 1:

				log("We can't upgrade from within Kodi as it needs updating itself")

				# Downloading all the debs at once require su access. So we call an external script to download the updates 
				# to the default apt_cache. That other script provides a progress update to this parent script, 
				# which is displayed as a background progress bar
				self.call_child_script('fetch')

			else:

				log("Updates are available, no reboot is required")			

				install = DIALOG.yesno('OSMC Update Available', 'There are updates that are available for install.', 'Would you like to install them now?')

				if install:

					self.call_child_script('commit') # Actually installs

					self.window.setProperty('OSMC_notification', 'false')

				else:

					okey_dokey = DIALOG.ok('OSMC Update Available', 'Fair enough, then.', 'You can install them from within the OSMC settings later.')

					# create the file that will prevent further update checks until the updates have been installed
					with open(self.block_update_file, 'w') as f:
						f.write('d')

					# trigger the flag to skip update checks
					self.skip_update_check = True


	def check_if_reboot_required(self):
		''' Checks for the existence of two specific files that indicate an installed package mandates a reboot. '''

		flag_files = ['/tmp/reboot-needed', '/var/run/reboot-required']

		if any([os.path.isfile(x) for x in flag_files]):
			return True
		else:
			return False
