# Standard Modules
import apt
from datetime import datetime
import os
import sys
import subprocess
import traceback
import Queue
import random
import json
import socket

# Kodi Modules
import xbmc
import xbmcaddon
import xbmcgui

# Custom modules
__libpath__ = xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources','lib'))
sys.path.append(__libpath__)
import comms
import simple_scheduler as sched
import OSMC_Backups
from CompLogger import comprehensive_logger as clog

__addon__              	= xbmcaddon.Addon()
__addonid__            	= __addon__.getAddonInfo('id')
__scriptPath__         	= __addon__.getAddonInfo('path')
__setting__            	= __addon__.getSetting
__image_file__         	= os.path.join(__scriptPath__,'resources','media','update_available.png')
DIALOG = xbmcgui.Dialog()


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


def log(message, label = ''):
	logmsg       = '%s : %s - %s ' % (__addonid__ , str(label), str(message))
	xbmc.log(msg = logmsg, level=xbmc.LOGDEBUG)

@clog(log)
def exit_osmc_settings_addon():
	address = '/var/tmp/osmc.settings.sockfile'
	sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
	sock.connect(address)
	sock.sendall('exit')
	sock.close()

	return 'OSMC Settings addon called to exit'


class Monitah(xbmc.Monitor):

	def __init__(self, **kwargs):
		super(Monitah, self).__init__()

		self.parent_queue = kwargs['parent_queue']

	@clog(log)
	def onAbortRequested(self):

		msg = json.dumps(('kill_yourself', {}))

		self.parent_queue.put(msg)

	@clog(log)
	def onSettingsChanged(self):

		msg = json.dumps(('update_settings', {}))

		self.parent_queue.put(msg)


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
								'apt_cache update complete' 		: self.apt_update_complete,
								'apt_cache update_manual complete'	: self.apt_update_manual_complete,
								'apt_cache commit complete'			: self.apt_commit_complete,
								'apt_cache fetch complete'			: self.apt_fetch_complete,
								'progress_bar'						: self.progress_bar,
								'update_settings'					: self.update_settings,
								'update_now'						: self.update_now,
								'user_update_now'					: self.user_update_now,
								'kill_yourself'						: self.kill_yourself,
								'settings_command'					: self.settings_command,
								'apt_error'							: self.apt_error,
								'action_list'						: self.action_list,
								'apt_cache action_list complete'	: self.action_list_complete,
								'pre_backup_complete'				: self.pre_backup_complete, 

							}

		# queue for communication with the comm and Main
		self.parent_queue = Queue.Queue()

		self.randomid = random.randint(0,1000)

		self.EXTERNAL_UPDATE_REQUIRED = 0

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

		# check for the external update failed
		fail_check_file = '/var/tmp/.osmc_failed_update'
		if os.path.isfile(fail_check_file):
			with open(fail_check_file, 'r') as f:
				package = f.readline()

			ok = DIALOG.ok(lang(32087), lang(32088) % package, '', lang(32089))

			try:
				os.remove(fail_check_file)
			except:
				pass

		# keep alive method
		self._daemon()


	# MAIN METHOD
	@clog(log, nowait=True)
	def _daemon(self):

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

			# check the scheduler for the update trigger
			if self.scheduler.check_trigger():
				self.update_now()
				log(self.scheduler.trigger_time, 'trigger_time')

			# check the action queue
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
	@clog(log, nowait=True)
	def holding_pattern_update(self):

		check, _ = self.check_update_conditions()

		if check:

			self.function_holding_pattern = False
			
			self.user_update_now()


	# HOLDING PATTERN METHOD
	@clog(log)
	def holding_pattern_fetched(self, bypass=False):

		# stay in the holding pattern until the user returns to the Home screen
		if 'Home.xml' in xbmc.getInfoLabel('Window.Property(xmlfile)') or bypass:

			self.function_holding_pattern = False

			if not self.EXTERNAL_UPDATE_REQUIRED:

				install_now = DIALOG.yesno(lang(32072), lang(32073), lang(32074))

				if install_now:

					self.call_child_script('commit')

					return 'Called child script - commit'

			else:

				exit_install = DIALOG.yesno(lang(32072), lang(32075), lang(32076))

				if exit_install:

					exit_osmc_settings_addon()
					xbmc.sleep(1000)

					subprocess.Popen(['sudo', 'systemctl', 'start', 'manual-update'])

					return 'Running external update proceedure'


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

			return 'skip_update_check= %s' % self.skip_update_check


	# MAIN METHOD
	@clog(log)
	def exit_procedure(self):

		# stop the listener
		self.listener.stop()
		# del self.listener
		# log('listener cleaned up')

		# del self.monitor
		# log('del self.monitor')
		# del self.update_image
		# log('del self.update_image')

		# del self.window 
		# log('del self.window')

		# self.takedown_notification()
		# log('notification control removed from window(10000)')


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
	@clog(log)
	def check_update_conditions(self, media_only=False):
		''' Checks the users update conditions are met. The media-only flag restricts the condition check to
			only the media playing condition. '''

		result_raw = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "method": "Player.GetActivePlayers", "id": 1 }')

		result = json.loads(result_raw)
		
		log(result, 'result of Player.GetActivePlayers')
		
		players = result.get('result', False)
		
		if players:
		
			log('Update CONDITION : player playing')
		
			return False, 'Update CONDITION : player playing'

		idle = xbmc.getGlobalIdleTime()

		if self.s['update_on_idle'] and idle < 60 and not media_only:

			msg = 'Update CONDITION : idle time = %s' % idle

			return False, 'Update CONDITION : idle time = %s' % idle

		return True, ''


	# MAIN METHOD
	@clog(log)
	def takedown_notification(self):

		try:
			self.window.removeControl(self.update_image)
		except Exception as e:
			log(e, 'an EXCEPTION occurred')


	# MAIN METHOD
	@clog(log)
	def call_child_script(self, action):

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
	@clog(log, maxlength=1000)
	def update_settings(self):

		''' Updates the settings for the service while the service is still running '''

		if self.first_run:

			''' Construct the settings dicionary '''

			self.first_run = False

			self.scheduler_settings = ['check_freq', 'check_weekday', 'check_day', 'check_time', 'check_hour', 'check_minute']
			# self.icon_settings		= ['pos_x', 'pos_y']

			self.on_upd = [lang(x) for x in [32057,32058,32095,32060,32061]]
			# self.on_upd = [lang(x) for x in [32059,32061]]  # 2==> 0, 4 ==> 1
			
			self.s = {}

			log(__setting__('on_upd_detected'))

			self.s['on_upd_detected']	= int(			__setting__('on_upd_detected')		)
			self.s['check_freq'] 		= int(			__setting__('check_freq')			)
			self.s['check_weekday'] 	= int(float(	__setting__('check_weekday')		))
			self.s['check_day'] 		= int(float(	__setting__('check_day')			))
			self.s['check_time'] 		= int(float(	__setting__('check_time')			))
			self.s['check_hour'] 		= int(float(	__setting__('check_hour')			))
			self.s['check_minute'] 		= int(float(	__setting__('check_minute')			))
			# self.s['pos_x']				= int(float(	__setting__('pos_x')				))
			# self.s['pos_y']				= int(float(	__setting__('pos_y')				))
			self.s['suppress_progress']			= True if 	__setting__('suppress_progress') 		== 'true' else False
			self.s['suppress_icon']				= True if 	__setting__('suppress_icon') 			== 'true' else False
			self.s['update_on_idle']			= True if 	__setting__('update_on_idle') 			== 'true' else False
			self.s['home_prompts_only']			= True if 	__setting__('home_prompts_only') 		== 'true' else False
			self.s['export_library'] 			= True if 	__setting__('export_library')			== 'true' else False
			self.s['export_video'] 				= True if 	__setting__('export_video')				== 'true' else False
			self.s['multifile_vid_export'] 		= True if 	__setting__('multifile_vid_export')		== 'true' else False
			self.s['export_music'] 				= True if 	__setting__('export_music')				== 'true' else False
			self.s['create_tarball'] 			= True if 	__setting__('create_tarball')			== 'true' else False
			self.s['backup_location'] 			= __setting__('backup_location')
			self.s['tarball_count'] 			= int(float(	__setting__('tarball_count')		))
			self.s['backup_on_update'] 			= True if 	__setting__('backup_on_update')			== 'true' else False
			self.s['backup_addons'] 			= True if 	__setting__('backup_addons')			== 'true' else False
			self.s['backup_addon_data'] 		= True if 	__setting__('backup_addon_data')		== 'true' else False
			self.s['backup_Database'] 			= True if 	__setting__('backup_Database')			== 'true' else False
			self.s['backup_keymaps'] 			= True if 	__setting__('backup_keymaps')			== 'true' else False
			self.s['backup_library'] 			= True if 	__setting__('backup_library')			== 'true' else False
			self.s['backup_playlists'] 			= True if 	__setting__('backup_playlists')			== 'true' else False
			self.s['backup_Thumbnails']		 	= True if 	__setting__('backup_Thumbnails')		== 'true' else False
			self.s['backup_favourites'] 		= True if 	__setting__('backup_favourites')		== 'true' else False
			self.s['backup_keyboard'] 			= True if 	__setting__('backup_keyboard')			== 'true' else False
			self.s['backup_remote'] 			= True if 	__setting__('backup_remote')			== 'true' else False
			self.s['backup_LCD'] 				= True if 	__setting__('backup_LCD')				== 'true' else False
			self.s['backup_profiles'] 			= True if 	__setting__('backup_profiles')			== 'true' else False
			self.s['backup_RssFeeds'] 			= True if 	__setting__('backup_RssFeeds')			== 'true' else False
			self.s['backup_sources'] 			= True if 	__setting__('backup_sources')			== 'true' else False
			self.s['backup_upnpserver'] 		= True if 	__setting__('backup_upnpserver')		== 'true' else False
			self.s['backup_peripheral_data'] 	= True if 	__setting__('backup_peripheral_data')	== 'true' else False
			self.s['backup_guisettings'] 		= True if 	__setting__('backup_guisettings')		== 'true' else False
			self.s['backup_advancedsettings'] 	= True if 	__setting__('backup_advancedsettings')	== 'true' else False


			return "initial run", self.s

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
			# tmp_s['pos_x']				= int(float(	__setting__('pos_x')				))
			# tmp_s['pos_y']				= int(float(	__setting__('pos_y')				))			
			tmp_s['suppress_progress']			= True if 	__setting__('suppress_progress') 		== 'true' else False
			tmp_s['suppress_icon']				= True if 	__setting__('suppress_icon') 			== 'true' else False
			tmp_s['update_on_idle']				= True if 	__setting__('update_on_idle') 			== 'true' else False
			tmp_s['home_prompts_only']			= True if 	__setting__('home_prompts_only') 		== 'true' else False
			tmp_s['suppress_progress']			= True if 	__setting__('suppress_progress') 		== 'true' else False
			tmp_s['suppress_icon']				= True if 	__setting__('suppress_icon') 			== 'true' else False
			tmp_s['update_on_idle']				= True if 	__setting__('update_on_idle') 			== 'true' else False
			tmp_s['home_prompts_only']			= True if 	__setting__('home_prompts_only') 		== 'true' else False
			tmp_s['export_library'] 			= True if 	__setting__('export_library')			== 'true' else False
			tmp_s['export_video'] 				= True if 	__setting__('export_video')				== 'true' else False
			tmp_s['multifile_vid_export'] 		= True if 	__setting__('multifile_vid_export')		== 'true' else False
			tmp_s['export_music'] 				= True if 	__setting__('export_music')				== 'true' else False			
			tmp_s['create_tarball'] 			= True if 	__setting__('create_tarball')			== 'true' else False
			tmp_s['backup_location'] 			= __setting__('backup_location')
			tmp_s['tarball_count'] 				= int(float(	__setting__('tarball_count')		))
			tmp_s['backup_on_update'] 			= True if 	__setting__('backup_on_update')			== 'true' else False
			tmp_s['backup_addons'] 				= True if 	__setting__('backup_addons')			== 'true' else False
			tmp_s['backup_addon_data'] 			= True if 	__setting__('backup_addon_data')		== 'true' else False
			tmp_s['backup_Database'] 			= True if 	__setting__('backup_Database')			== 'true' else False
			tmp_s['backup_keymaps'] 			= True if 	__setting__('backup_keymaps')			== 'true' else False
			tmp_s['backup_library'] 			= True if 	__setting__('backup_library')			== 'true' else False
			tmp_s['backup_playlists'] 			= True if 	__setting__('backup_playlists')			== 'true' else False
			tmp_s['backup_Thumbnails']		 	= True if 	__setting__('backup_Thumbnails')		== 'true' else False
			tmp_s['backup_favourites'] 			= True if 	__setting__('backup_favourites')		== 'true' else False
			tmp_s['backup_keyboard'] 			= True if 	__setting__('backup_keyboard')			== 'true' else False
			tmp_s['backup_remote'] 				= True if 	__setting__('backup_remote')			== 'true' else False
			tmp_s['backup_LCD'] 				= True if 	__setting__('backup_LCD')				== 'true' else False
			tmp_s['backup_profiles'] 			= True if 	__setting__('backup_profiles')			== 'true' else False
			tmp_s['backup_RssFeeds'] 			= True if 	__setting__('backup_RssFeeds')			== 'true' else False
			tmp_s['backup_sources'] 			= True if 	__setting__('backup_sources')			== 'true' else False
			tmp_s['backup_upnpserver'] 			= True if 	__setting__('backup_upnpserver')		== 'true' else False
			tmp_s['backup_peripheral_data'] 	= True if 	__setting__('backup_peripheral_data')	== 'true' else False
			tmp_s['backup_guisettings'] 		= True if 	__setting__('backup_guisettings')		== 'true' else False
			tmp_s['backup_advancedsettings'] 	= True if 	__setting__('backup_advancedsettings')	== 'true' else False			

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

		return self.s


	# ACTION METHOD
	def apt_error(self, **kwargs):

		package = kwargs.get('package','not provided')

		log('apt_updater encountered and error: \nException : %s \nPackage : %s \nError : %s' % (kwargs.get('exception','not provided'),package,kwargs.get('error','not provided')))

		# kill the progress bar
		self.progress_bar(kill=True)

		# notify the user that an error has occured with an update
		ok = DIALOG.ok(lang(32087), lang(32088) % package, '', lang(32089))


	# ACTION METHOD
	def action_list(self,  action):

		subprocess.Popen(['sudo', 'python','%s/apt_cache_action.py' % __libpath__, 'action_list', action])


	def action_list_complete(self):

		# notify the user that the installation or uninstall of their desirec apfs has completed successfully
		ok = DIALOG.ok(lang(32090), lang(32091))

	# ACTION METHOD
	# @clog(log, maxlength=2500)
	def progress_bar(self, **kwargs):

		''' Controls the creation and updating of the background prgress bar in kodi.
			The data gets sent from the apt_cache_action script via the socket
			percent, 	must be an integer
			heading,	string containing the running total of items, bytes and speed
			message, 	string containing the name of the package or the active process.
		 '''

		# return immediately if the user has suppressed on-screen progress updates or kwargs is empty
		if self.s['suppress_progress'] or not kwargs: return

		# check for kill order in kwargs
		kill = kwargs.get('kill', False)

		if kill:
			# if it is present, kill the dialog and delete it
			
			try:
				self.pDialog.close()
				del self.pDialog
				return 'Killed pDialog'
			except:
				pass
				return 'Failed to kill pDialog'

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
			self.pDialog.create(lang(32077), lang(32078))

			self.pDialog.update(**update_args)

		except Exception as e:

			# on any other error, just log it and try to remove the dialog from the screen 

			log(e, 'pDialog has encountered and error')

			try:
				self.pDialog.close()
				del self.pDialog
				return 'Killed pDialog'
			except:
				pass
				return 'Failed to kill pDialog'


	# ACTION METHOD
	@clog(log)
	def kill_yourself(self):

		self.keep_alive = False 


	# ACTION METHOD
	@clog(log, nowait=True)
	def update_now(self):
		''' Calls for an update check via the external script. This method checks if media is playing or whether the system has 
			been idle for two minutes before allowing the update. If an update is requested, but media is playing or the system
			isnt idle, then the update request is put into a loop, with the daemon checking periodically to see if the situation 
			has changed. '''

		# do not do anything while there is something in the holding pattern
		if self.function_holding_pattern: return

		check, _ = self.check_update_conditions()

		if check:


			if self.s['backup_on_update']:

				# run backup

				self.update_settings()

				bckp = OSMC_Backups.osmc_backup(self.s, self.progress_bar, self.parent_queue)

				try:

					bckp.start_backup()

				except Exception as e:
				
					log('Backup Error Type and Args: %s : %s \n\n %s' % (type(e).__name__, e.args, traceback.format_exc()))

			else:

				# run the update

				self.call_child_script('update')
		
		else:

			self.function_holding_pattern = self.holding_pattern_update


	# ACTION METHOD
	@clog(log)
	def user_update_now(self):
		''' Similar to update_now, but as this is a users request, forego all the player and idle checks. '''

		self.call_child_script('update')


	# ACTION METHOD
	@clog(log)
	def pre_backup_complete(self):
		''' This method is called when the pre-update backup is completed. No need to worry about checking the 
		update conditions, just run the update. '''

		self.call_child_script('update')


	# ACTION METHOD
	@clog(log)
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


			# 0 "Prompt for all actions" -- PROMPT
			# 1 "Display icon on home screen only" -- PROMPT
			# 2 "Download updates, then prompt" -- PROMPT
			# 3 "Download and display icon" -- PROMPT
			# 4 "Download, install, prompt if restart needed" -- PROMPT

			# display dialogue saying that osmc needs to reboot
			reboot = DIALOG.yesno(lang(32077), lang(32079), lang(32080), yeslabel=lang(32081), nolabel=lang(32082))

			if reboot:

				exit_osmc_settings_addon()
				xbmc.sleep(1000)
				
				xbmc.executebuiltin('Reboot')

			else:
				# skip further update checks until osmc has rebooted
				self.skip_update_check = True 

			

	# ACTION METHOD
	@clog(log)
	def apt_fetch_complete(self):

		# Download and display icon
		if self.s['on_upd_detected'] == 3:

			# create the file that will prevent further update checks until the updates have been installed
			with open(self.block_update_file, 'w') as f:
				f.write('d')

			# turn on the "install now" setting in Settings.xml
			__addon__.setSetting('install_now_visible', 'true')
			
			return 'Download complete, leaving icon displayed'

		else:
			# Download updates, then prompt
			# Download, install, prompt if restart needed (restart is needed)
			# Prompt for all actions


			if self.s['home_prompts_only']:

				self.function_holding_pattern = self.holding_pattern_fetched

				return 'Download complete, putting into holding pattern'

			else:

				self.holding_pattern_fetched(bypass=True)

				return 'Download complete, prompting user'


	# ACTION METHOD
	@clog(log)
	def settings_command(self, action):

		if action == 'update':

			self.call_child_script('update_manual')

			return 'Called child action - update_manual'

		if action == 'backup':

			self.update_settings()

			bckp = OSMC_Backups.osmc_backup(self.s, self.progress_bar)

			try:

				bckp.start_backup()

			except Exception as e:
			
				log('Backup Error Type and Args: %s : %s \n\n %s' % (type(e).__name__, e.args, traceback.format_exc()))

				ok = DIALOG.ok(lang(32096), lang(32097))

			return 'Called BACKUP script complete'

		if action == 'restore':

			self.update_settings()

			bckp = OSMC_Backups.osmc_backup(self.s, self.progress_bar)

			try:

				bckp.start_restore()

			except Exception as e:
			
				log('Backup Error Type and Args: %s : %s \n\n %s' % (type(e).__name__, e.args, traceback.format_exc()))

				ok = DIALOG.ok(lang(32096), lang(32097))

			return 'Called RESTORE script complete'


		elif action == 'install':

			# check, _ = self.check_for_legit_updates()

			# if check == 'bail':

			# 	return  'Update not legit, bail'

			# if not self.EXTERNAL_UPDATE_REQUIRED:

			# 	__addon__.setSetting('install_now_visible', 'false')

			# 	self.call_child_script('commit')

			# 	return 'Called child action - commit'

			# else:

			ans = DIALOG.yesno(lang(32072), lang(32075), lang(32076))

			if ans:

				__addon__.setSetting('install_now_visible', 'false')
				
				exit_osmc_settings_addon()
				xbmc.sleep(1000)

				subprocess.Popen(['sudo', 'systemctl', 'start', 'manual-update'])	

				return "Calling external update"


	#ACTION METHOD
	@clog(log)
	def check_for_broken_installs(self):

		try:
			
			apt.apt_pkg.init_config()

			apt.apt_pkg.init_system()

			self.cache = apt.apt_pkg.Cache()

		except apt.cache.LockFailedException:

			return 'bail', 'global lock placed on package system'

		except:

			return 'bail', 'apt_pkg cache failed to open'

		dirty_states = {apt.apt_pkg.CURSTATE_HALF_CONFIGURED, apt.apt_pkg.CURSTATE_HALF_INSTALLED, apt.apt_pkg.CURSTATE_UNPACKED}

		try:

			for pkg in self.cache.packages:

				if pkg.current_state in dirty_states:

					log(' found in a partially installed state', pkg.name)

					self.EXTERNAL_UPDATE_REQUIRED = 1

					return 'broken install found', 'EXTERNAL_UPDATE_REQUIRED set to 1'

			else:

				return 'passed', 'no broken packages found'

		except:

			return 'bail', 'check for partially installed packages failed'


	# ACTION METHOD
	@clog(log)
	def check_for_legit_updates(self):

		self.EXTERNAL_UPDATE_REQUIRED = 0

		check, msg = self.check_for_broken_installs()

		if check == 'bail':
			
			return check, msg 

		try:
	
			self.cache = apt.Cache()

			self.cache.open(None)

		except apt.cache.LockFailedException:

			return 'bail', 'global lock placed on package system'

		except:

			return 'bail', 'apt cache failed to open'

		try:

			self.cache.upgrade(True)

		except:

			return 'bail', 'apt cache failed to upgrade'

		available_updates = []

		log('The following packages have newer versions and are upgradable: ')

		for pkg in self.cache:

			if pkg.is_upgradable:

				log(' is upgradeable', pkg.shortname)

				available_updates.append(pkg.shortname.lower())

		# if 'osmc' isnt in the name of any available updates, then return without doing anything
		if not any(['osmc' in x for x in available_updates]):

			self.window.setProperty('OSMC_notification', 'false')

			return 'bail', 'There are no osmc packages'

		if any(["mediacenter" in x or "lirc-osmc" in x or "eventlircd-osmc" in x or "dbus" in x or "dbus-x11" in x for x in available_updates]):

			self.EXTERNAL_UPDATE_REQUIRED = 1

		# display update available notification
		if not self.s['suppress_icon']:

			self.window.setProperty('OSMC_notification', 'true')

		return 'passed', 'legit updates available'


	# ACTION METHOD
	@clog(log)
	def apt_update_manual_complete(self):

		self.apt_update_complete(data='manual_update_complete')


	# ACTION METHOD
	@clog(log)
	def apt_update_complete(self, data=None):
		
		check, _ = self.check_for_legit_updates()

		if check == 'bail':

			if data == 'manual_update_complete':

				okey_dokey = DIALOG.ok(lang(32077), lang(32092))

				# send kill message to progress bar
				self.progress_bar(kill=True)
			
			return 'Updates not legit, bail'

		# The following section implements the procedure that the user has chosen to take place when updates are detected
		if self.s['on_upd_detected'] == 0 or data == 'manual_update_complete':
			# show all prompts (default)

			if self.EXTERNAL_UPDATE_REQUIRED == 1:

				# Downloading all the debs at once require su access. So we call an external script to download the updates 
				# to the default apt_cache. That other script provides a progress update to this parent script, 
				# which is displayed as a background progress bar

				self.call_child_script('fetch')

				return "We can't upgrade from within Kodi as it needs updating itself"

			else:

				install = DIALOG.yesno(lang(32072), lang(32083), lang(32084))

				if install:

					self.call_child_script('commit') # Actually installs

					self.window.setProperty('OSMC_notification', 'false')

				else:

					okey_dokey = DIALOG.ok(lang(32072), lang(32085), lang(32086))

					# send kill message to progress bar
					self.progress_bar(kill=True)

					# create the file that will prevent further update checks until the updates have been installed
					with open(self.block_update_file, 'w') as f:
						f.write('d')

					# trigger the flag to skip update checks
					self.skip_update_check = True

				return "Updates are available, no reboot is required"

		elif self.s['on_upd_detected'] == 1: 
			# Display icon on home screen only

			return 'Displaying icon on home screen only'

		elif (self.s['on_upd_detected'] in [2, 3]) or (self.s['on_upd_detected'] == 4 and self.EXTERNAL_UPDATE_REQUIRED):
			# Download updates, then prompt
			# Download and display icon
			# Download, install, prompt if restart needed (restart is needed)
			# Download, install, auto-restart if needed

			self.call_child_script('fetch')

			return 'Downloading updates'

		elif self.s['on_upd_detected'] == 4 and not self.EXTERNAL_UPDATE_REQUIRED:
			# Download, install, prompt if restart needed (restart is not needed)

			self.call_child_script('commit')
			
			return 'Download, install, prompt if restart needed'


	@clog(log)
	def check_if_reboot_required(self):
		''' Checks for the existence of two specific files that indicate an installed package mandates a reboot. '''

		flag_files = ['/tmp/reboot-needed', '/var/run/reboot-required']

		if any([os.path.isfile(x) for x in flag_files]):
			return True
		else:
			return False
