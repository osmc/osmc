# Standard Modules
import apt
from datetime import datetime
import decimal
import json
import os
import Queue
import random
import socket
import subprocess
import sys
import traceback

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

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )

	try:
		label = str(label)
	except UnicodeEncodeError:
		label = label.encode('utf-8', 'ignore' )

	logmsg       = '%s : %s - %s ' % (__addonid__ , str(label), str(message.encode( 'utf-8', 'ignore' )))
	xbmc.log(msg = logmsg, level=xbmc.LOGDEBUG)

@clog(log)
def exit_osmc_settings_addon():
	address = '/var/tmp/osmc.settings.sockfile'
	sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
	sock.connect(address)
	sock.sendall('exit')
	sock.close()

	return 'OSMC Settings addon called to exit'

def get_hardware_prefix():
	''' Returns the prefix for the hardware type. rbp, rbp2, etc '''	

	with open('/proc/cmdline', 'r') as f:

		line = f.readline()

		settings = line.split(' ')

		prefix = None

		for setting in settings:

			if setting.startswith('osmcdev='):

				return setting[len('osmcdev='):]

	return None


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

		# set the hardware prefix
		self.hw_prefix = get_hardware_prefix()

		# list of packages that require an external update
		self.EXTERNAL_UPDATE_REQUIRED_LIST = 	[
												"mediacenter",
												"lirc-osmc",
												"eventlircd-osmc",
												"libcec-osmc",
												"dbus",
												"dbus-x11"
												]

		# list of packages that may break compatibility with addons and databases.
		self.UPDATE_WARNING = False
		self.UPDATE_WARNING_LIST = 	[
									"-mediacenter-osmc",
									]

		# Items that start with a hyphen should have the hardware prefix attached
		self.UPDATE_WARNING_LIST = [(str(self.hw_prefix) + x) if x[0] =='-' else x for x in self.UPDATE_WARNING_LIST]
		log('UPDATE_WARNING_LIST: %s' % self.UPDATE_WARNING_LIST)

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
								'apt_action_list_error'				: self.apt_action_list_error,
								'action_list'						: self.action_list,
								'apt_cache action_list complete'	: self.action_list_complete,
								'pre_backup_complete'				: self.pre_backup_complete, 

							}

		# queue for communication with the comm and Main
		self.parent_queue = Queue.Queue()

		self.randomid = random.randint(0,1000)

		self.EXTERNAL_UPDATE_REQUIRED = 1

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
		self.update_image = xbmcgui.ControlImage(50, 1695, 175, 75, __image_file__)
		self.try_image_position_again = False
		self.try_count = 0
		self.position_icon()
		self.window.addControl(self.update_image)
		self.update_image.setVisibleCondition('[SubString(Window(Home).Property(OSMC_notification), true, left)]')
		# self.window.setProperty('OSMC_notification', 'true')    # USE THIS TO TEST THE UPDATE_ICON

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

		self.freespace_supressor = 172200
		self.freespace_remedy    = 'reboot' # change this to 'apt' to give the user the option to clean the apt files

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
			count += 1 								# FOR TESTING ONLY
			# FOR TESTING ONLY

			# freespace checker, (runs 5 minutes after boot)
			self.automatic_freespace_checker()
			
			# check the scheduler for the update trigger
			if self.scheduler.check_trigger():
				self.update_now()
				log(self.scheduler.trigger_time, 'trigger_time')

			# check the action queue
			self.check_action_queue()

			# check the holding pattern, call item in holding pattern
			if self.function_holding_pattern:

				self.function_holding_pattern()

			# try to position the icon again, ubiquifonts may not have had time to post the screen height and width 
			# to Home yet.
			if self.try_image_position_again:
				self.position_icon()

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

			# if there is an update warning (for a major version change in Kodi) then alert the user
			if self.UPDATE_WARNING:

				confirm_update = self.display_update_warning()

				if not confirm_update:

					# remove the function from the holding pattern
					self.function_holding_pattern = False

					# skip all future update checks (the user will have to run the check for updates manually.)
					self.skip_future_update_checks()

					return 'User declined to update major version of Kodi, skipping future update checks'

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
			self.skip_future_update_checks()

			if not self.s['suppress_icon']:
				self.window.setProperty('OSMC_notification', 'true')

			return 'skip_update_check= %s' % self.skip_update_check


	# MAIN METHOD
	def skip_future_update_checks(self):
		''' Sets the conditions for future update checks to be blocked. '''

		# create the file that will prevent further update checks until the updates have been installed
		with open(self.block_update_file, 'w') as f:
			f.write('d')

		# trigger the flag to skip update checks
		self.skip_update_check = True


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
	def check_update_conditions(self, connection_only=False):
		''' Checks the users update conditions are met. 
			Checks for:
					- active player 
					- idle time
					- internet connectivity
				connection_only, limits the check to just the internet connection
					'''
		if not connection_only:

			result_raw = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "method": "Player.GetActivePlayers", "id": 1 }')

			result = json.loads(result_raw)
			
			log(result, 'result of Player.GetActivePlayers')

			players = result.get('result', False)

			if players:
			
				log('Update CONDITION : player playing')
			
				return False, 'Update CONDITION : player playing'

			idle = xbmc.getGlobalIdleTime()

			if self.s['update_on_idle'] and idle < 60:

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

		# check whether the install is an alpha version
		if self.check_for_unsupported_version() == 'alpha': return

		subprocess.Popen(['sudo', 'python','%s/apt_cache_action.py' % __libpath__, action])


	# MAIN METHOD
	def position_icon(self):
		''' Sets the position of the icon.

			Original image dimensions are 175 wide and 75 tall. This is for 1080p '''

		self.try_image_position_again = False
		

		pos_horiz  = self.s['pos_x'] / 100.0
		pos_vertic = self.s['pos_y'] / 100.0

		width  = 175  # as % of 1920: 0.0911458333333
		height = 75   # as % of 1080: 0.0694444444444
		width_pct  = 0.0911458333333
		height_pct = 0.0694444444444

		# retrieve the skin height and width (supplied by ubiquifonts and stored in Home)
		try:
			screen_height = self.window.getProperty("SkinHeight")
			screen_width  = self.window.getProperty("SkinWidth")
		except:
			screen_height = 1080
			screen_width  = 1920

		if screen_height == '':
			if self.try_count >= 50:
				self.try_count = 0
				screen_height = 1080
				screen_width  = 1920
			else:
				self.try_image_position_again = True
				self.try_count += 1
				return

		screen_height = int(screen_height)
		screen_width  = int(screen_width)

		# determine new dimensions of the image
		img_height = int(round(decimal.Decimal(screen_height * height_pct), 0))
		img_width  = int(round(decimal.Decimal(screen_width  * width_pct), 0))

		# determine the new coordinates of the image
		adj_height = screen_height - img_height
		adj_width  = screen_width  - img_width

		pos_top  = int(round(decimal.Decimal(adj_height * pos_vertic), 0))
		pos_left = int(round(decimal.Decimal(adj_width  * pos_horiz),  0))

		log('=============================')
		log(screen_height)
		log(screen_width)
		log(adj_height)
		log(adj_width)
		log(img_height)
		log(img_width)
		log(pos_top)
		log(pos_left)
		log('=============================')

		# reposition the image
		self.update_image.setPosition(pos_left, pos_top)

		# rescale the image
		self.update_image.setWidth(img_width)
		self.update_image.setHeight(img_height)


	# MAIN METHOD
	@clog(log, maxlength=1000)
	def update_settings(self):
		''' Updates the settings for the service while the service is still running '''

		if self.first_run:

			''' Construct the settings dicionary '''

			self.first_run = False

			self.scheduler_settings = ['check_freq', 'check_weekday', 'check_day', 'check_time', 'check_hour', 'check_minute']
			self.icon_settings		= ['pos_x', 'pos_y']

			self.on_upd = [lang(x) for x in [32057,32058,32095,32060,32061]]
			# self.on_upd = [lang(x) for x in [32059,32061]]  # 2==> 0, 4 ==> 1
			
			self.s = {}

			self.s['on_upd_detected']         = int(		__setting__('on_upd_detected')		)
			# this is to deprecate the automatic installation of non-system updates
			# changed to Download, and Prompt
			if self.s['on_upd_detected'] == 4:
				__addon__.setSetting('on_upd_detected', '2')
				self.s['on_upd_detected'] = 2
			self.s['check_freq']              = int(		__setting__('check_freq')			)
			self.s['check_weekday']           = int(float(	__setting__('check_weekday')		))
			self.s['check_day']               = int(float(	__setting__('check_day')			))
			self.s['check_time']              = int(float(	__setting__('check_time')			))
			self.s['check_hour']              = int(float(	__setting__('check_hour')			))
			self.s['check_minute']            = int(float(	__setting__('check_minute')			))
			self.s['pos_x']                   = int(float(	__setting__('pos_x')				))
			self.s['pos_y']                   = int(float(	__setting__('pos_y')				))
			self.s['suppress_progress']       = True if 	__setting__('suppress_progress') 		== 'true' else False
			self.s['suppress_icon']           = True if 	__setting__('suppress_icon') 			== 'true' else False
			self.s['update_on_idle']          = True if 	__setting__('update_on_idle') 			== 'true' else False
			self.s['home_prompts_only']       = True if 	__setting__('home_prompts_only') 		== 'true' else False
			# self.s['export_library']        = True if 	__setting__('export_library')			== 'true' else False
			# self.s['export_video']          = True if 	__setting__('export_video')				== 'true' else False
			# self.s['multifile_vid_export']  = True if 	__setting__('multifile_vid_export')		== 'true' else False
			# self.s['export_music']          = True if 	__setting__('export_music')				== 'true' else False
			# self.s['create_tarball']        = True if 	__setting__('create_tarball')			== 'true' else False
			self.s['location_selection']      = 			__setting__('location_selection')
			self.s['backup_location']         = 			__setting__('backup_location')
			self.s['backup_location_typed']   = 			__setting__('backup_location_typed')
			self.s['tarball_count']           = int(float(	__setting__('tarball_count')		))
			self.s['backup_on_update']        = True if 	__setting__('backup_on_update')			== 'true' else False
			self.s['backup_addons']           = True if 	__setting__('backup_addons')			== 'true' else False
			self.s['backup_addon_data']       = True if 	__setting__('backup_addon_data')		== 'true' else False
			self.s['backup_Database']         = True if 	__setting__('backup_Database')			== 'true' else False
			self.s['backup_keymaps']          = True if 	__setting__('backup_keymaps')			== 'true' else False
			self.s['backup_library']          = True if 	__setting__('backup_library')			== 'true' else False
			self.s['backup_playlists']        = True if 	__setting__('backup_playlists')			== 'true' else False
			self.s['backup_Thumbnails']       = True if 	__setting__('backup_Thumbnails')		== 'true' else False
			self.s['backup_favourites']       = True if 	__setting__('backup_favourites')		== 'true' else False
			self.s['backup_keyboard']         = True if 	__setting__('backup_keyboard')			== 'true' else False
			self.s['backup_remote']           = True if 	__setting__('backup_remote')			== 'true' else False
			self.s['backup_LCD']              = True if 	__setting__('backup_LCD')				== 'true' else False
			self.s['backup_profiles']         = True if 	__setting__('backup_profiles')			== 'true' else False
			self.s['backup_RssFeeds']         = True if 	__setting__('backup_RssFeeds')			== 'true' else False
			self.s['backup_sources']          = True if 	__setting__('backup_sources')			== 'true' else False
			self.s['backup_upnpserver']       = True if 	__setting__('backup_upnpserver')		== 'true' else False
			self.s['backup_peripheral_data']  = True if 	__setting__('backup_peripheral_data')	== 'true' else False
			self.s['backup_guisettings']      = True if 	__setting__('backup_guisettings')		== 'true' else False
			self.s['backup_advancedsettings'] = True if 	__setting__('backup_advancedsettings')	== 'true' else False


			return "initial run", self.s

		else:

			''' Construct a temporary dictionary for comparison with the existing settings dict '''

			tmp_s = {}
			
			tmp_s['on_upd_detected']         = int(			__setting__('on_upd_detected')		)
			tmp_s['check_freq']              = int(			__setting__('check_freq')			)
			tmp_s['check_weekday']           = int(float(	__setting__('check_weekday')		))
			tmp_s['check_day']               = int(float(	__setting__('check_day')			))
			tmp_s['check_time']              = int(float(	__setting__('check_time')			))
			tmp_s['check_hour']              = int(float(	__setting__('check_hour')			))
			tmp_s['check_minute']            = int(float(	__setting__('check_minute')			))
			tmp_s['pos_x']                   = int(float(	__setting__('pos_x')				))
			tmp_s['pos_y']                   = int(float(	__setting__('pos_y')				))			
			tmp_s['suppress_progress']       = True if 		__setting__('suppress_progress') 		== 'true' else False
			tmp_s['suppress_icon']           = True if 		__setting__('suppress_icon') 			== 'true' else False
			tmp_s['update_on_idle']          = True if 		__setting__('update_on_idle') 			== 'true' else False
			tmp_s['home_prompts_only']       = True if 		__setting__('home_prompts_only') 		== 'true' else False
			tmp_s['suppress_progress']       = True if 		__setting__('suppress_progress') 		== 'true' else False
			tmp_s['suppress_icon']           = True if 		__setting__('suppress_icon') 			== 'true' else False
			tmp_s['update_on_idle']          = True if 		__setting__('update_on_idle') 			== 'true' else False
			tmp_s['home_prompts_only']       = True if 		__setting__('home_prompts_only') 		== 'true' else False
			# tmp_s['export_library']        = True if 		__setting__('export_library')			== 'true' else False
			# tmp_s['export_video']          = True if 		__setting__('export_video')				== 'true' else False
			# tmp_s['multifile_vid_export']  = True if 		__setting__('multifile_vid_export')		== 'true' else False
			# tmp_s['export_music']          = True if 		__setting__('export_music')				== 'true' else False			
			# tmp_s['create_tarball']        = True if 		__setting__('create_tarball')			== 'true' else False
			tmp_s['location_selection']      = 				__setting__('location_selection')
			tmp_s['backup_location']         = 				__setting__('backup_location')
			tmp_s['backup_location_typed']   = 				__setting__('backup_location_typed')
			tmp_s['tarball_count']           = int(float(	__setting__('tarball_count')		))
			tmp_s['backup_on_update']        = True if 		__setting__('backup_on_update')			== 'true' else False
			tmp_s['backup_addons']           = True if 		__setting__('backup_addons')			== 'true' else False
			tmp_s['backup_addon_data']       = True if 		__setting__('backup_addon_data')		== 'true' else False
			tmp_s['backup_Database']         = True if 		__setting__('backup_Database')			== 'true' else False
			tmp_s['backup_keymaps']          = True if 		__setting__('backup_keymaps')			== 'true' else False
			tmp_s['backup_library']          = True if 		__setting__('backup_library')			== 'true' else False
			tmp_s['backup_playlists']        = True if 		__setting__('backup_playlists')			== 'true' else False
			tmp_s['backup_Thumbnails']       = True if 		__setting__('backup_Thumbnails')		== 'true' else False
			tmp_s['backup_favourites']       = True if 		__setting__('backup_favourites')		== 'true' else False
			tmp_s['backup_keyboard']         = True if 		__setting__('backup_keyboard')			== 'true' else False
			tmp_s['backup_remote']           = True if 		__setting__('backup_remote')			== 'true' else False
			tmp_s['backup_LCD']              = True if 		__setting__('backup_LCD')				== 'true' else False
			tmp_s['backup_profiles']         = True if 		__setting__('backup_profiles')			== 'true' else False
			tmp_s['backup_RssFeeds']         = True if 		__setting__('backup_RssFeeds')			== 'true' else False
			tmp_s['backup_sources']          = True if 		__setting__('backup_sources')			== 'true' else False
			tmp_s['backup_upnpserver']       = True if 		__setting__('backup_upnpserver')		== 'true' else False
			tmp_s['backup_peripheral_data']  = True if 		__setting__('backup_peripheral_data')	== 'true' else False
			tmp_s['backup_guisettings']      = True if 		__setting__('backup_guisettings')		== 'true' else False
			tmp_s['backup_advancedsettings'] = True if 		__setting__('backup_advancedsettings')	== 'true' else False			

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
				elif k in self.icon_settings:
					reposition_icon = True

		# if the user has elected to type the backup location, then overwrite the backup_location with the typed version
		if self.s['location_selection'] == '1':
			self.s['backup_location']  = self.s['backup_location_typed'] 

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

		# specifically handle a failure to connect to the apt server
		if 'Unable to connect to' in kwargs.get('exception', ''):

			ok = DIALOG.ok(lang(32087), lang(32131), lang(32132))

		else:

			# generic error handling

			# notify the user that an error has occured with an update
			ok = DIALOG.ok(lang(32087), lang(32088) % package, '', lang(32089))


	# ACTION METHOD
	def apt_action_list_error(self, **kwargs):

		package = kwargs.get('package','not provided')

		log('apt_updater encountered and error: \nException : %s \nPackages : %s \nError : %s' % (kwargs.get('exception','not provided'),package,kwargs.get('error','not provided')))

		# kill the progress bar
		self.progress_bar(kill=True)

		# notify the user that an error has occured with an update
		ok = DIALOG.ok(lang(32087), lang(32112), '', lang(32113))


	# ACTION METHOD
	def action_list(self,  action):

		# check whether the install is an alpha version
		if self.check_for_unsupported_version() == 'alpha': return

		# check for sufficient space, only proceed if it is available
		root_space, _ = self.check_target_location_for_size(location='/', requirement=300)
		if root_space:

			subprocess.Popen(['sudo', 'python','%s/apt_cache_action.py' % __libpath__, 'action_list', action])

		else:

			okey_dokey = DIALOG.ok(lang(32077), lang(32129), lang(32130))


	def action_list_complete(self):

		# notify the user that the installation or uninstall of their desired apfs has completed successfully
		# prompt for immediate reboot if needed.

		if any([os.path.isfile('/tmp/reboot-needed'), os.path.isfile('fname/var/run/reboot-required')]):
			reboot = DIALOG.yesno(lang(32090), lang(32091), lang(32133), yeslabel=lang(32081), nolabel=lang(32082))

			if reboot:

				exit_osmc_settings_addon()
				
				xbmc.sleep(1000)
				
				xbmc.executebuiltin('Reboot')

		else:
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

		# check whether the install is an alpha version
		if self.check_for_unsupported_version() == 'alpha': return

		check, _ = self.check_update_conditions()

		if check:

			if self.s['backup_on_update']:

				# run the backup, once the backup is completed the script calls pre_backup_complete to continue with the update
				# that is the reason for the "else"

				self.update_settings()

				bckp = OSMC_Backups.osmc_backup(self.s, self.progress_bar, self.parent_queue)

				try:

					bckp.start_backup()

				except Exception as e:

					# if there is an error, then abort the Update. We dont want to run the update unless the user has backed up
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

		# check whether the install is an alpha version
		if self.check_for_unsupported_version() == 'alpha': return

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

		# run an apt-cache clean
		self.clean_apt_cache()

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
		''' Dispatch user call from the addons settings. '''

		if   action == 'update':

			result = self.settings_command_action()

		elif action == 'backup':

			result = self.settings_command_backup()

		elif action == 'restore':

			result = self.settings_command_restore()

		elif action == 'install':

			result = self.settings_command_install()

		return result


	#ACTION METHOD
	def settings_command_action(self):
		''' User called for a manual update '''

		check_connection, _ = self.check_update_conditions(connection_only=True)

		if not check_connection:

			DIALOG.ok('OSMC', 'Update not permitted.', 'Unable to reach internet.')

			return 'manual update cancelled, no connection'

		else:

			self.call_child_script('update_manual')

			return 'Called child action - update_manual'


	#ACTION METHOD
	def settings_command_backup(self):
		''' User called to initiate a backup '''

		self.update_settings()

		bckp = OSMC_Backups.osmc_backup(self.s, self.progress_bar)

		try:

			bckp.start_backup()

		except Exception as e:
		
			log('Backup Error Type and Args: %s : %s \n\n %s' % (type(e).__name__, e.args, traceback.format_exc()))

			ok = DIALOG.ok(lang(32096), lang(32097))

		return 'Called BACKUP script complete'


	#ACTION METHOD
	def settings_command_restore(self):		
		''' User called to inititate a restore '''	

		self.update_settings()

		bckp = OSMC_Backups.osmc_backup(self.s, self.progress_bar)

		try:

			bckp.start_restore()

			restart_required = bckp.restoring_guisettings

			if bckp.success != 'Full':

				ok = DIALOG.ok('Restore','Some items failed to restore.','See log for details.')

				for x in bckp.success:

					if x.endswith('userdata/guisettings.xml'):

						restart_required = False

			if restart_required:

				user_input_restart_now = DIALOG.yesno(lang(32110), lang(32098), lang(32099), yeslabel=lang(32100), nolabel=lang(32101))

				if user_input_restart_now:

					subprocess.Popen(['sudo', 'systemctl', 'restart', 'mediacenter'])

		except Exception as e:
		
			log('Backup Error Type and Args: %s : %s \n\n %s' % (type(e).__name__, e.args, traceback.format_exc()))

			ok = DIALOG.ok(lang(32096), lang(32097))

		return 'Called RESTORE script complete'


	#ACTION METHOD
	def settings_command_install(self):
		''' User called to install updates '''

		# check, _ = self.check_for_legit_updates()

		# if check == 'bail':

		# 	return  'Update not legit, bail'

		# if not self.EXTERNAL_UPDATE_REQUIRED:

		# 	__addon__.setSetting('install_now_visible', 'false')

		# 	self.call_child_script('commit')

		# 	return 'Called child action - commit'

		# else:

		# warn the user if there is a major Kodi update that will be installed
		# bail if they decide not to proceed
		if self.UPDATE_WARNING:
			confirm = self.display_update_warning()
			if not confirm: return

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

		self.UPDATE_WARNING = False

		self.EXTERNAL_UPDATE_REQUIRED = 1

		# check for sufficient disk space, requirement in MB
		root_space, _ = self.check_target_location_for_size(location='/', requirement=300)
		boot_space, _ = self.check_target_location_for_size(location='/boot', requirement=30)

		if not root_space or not boot_space:

			okey_dokey = DIALOG.ok(lang(32077), lang(32129), lang(32130))

			return 'bail', 'Sufficient freespace: root=%s, boot=%s' % (root_space, boot_space)

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

		for pkg in self.cache.get_changes():

			if pkg.is_upgradable:

				log(' is upgradeable', pkg.shortname)

				available_updates.append(pkg.shortname.lower())

				# check whether the package is one that should be monitored for significant version change
				if pkg.shortname in self.UPDATE_WARNING_LIST:

					#send the package for a major update check
					self.UPDATE_WARNING = self.check_for_major_release(pkg)

		# if 'osmc' isnt in the name of any available updates, then return without doing anything
		if not any(['osmc' in x for x in available_updates]):

			# suppress the on-screen update notification
			self.window.setProperty('OSMC_notification', 'false')

			# delete the block_update_file if it exists, so that the icon doesnt display on next boot
			try:
				os.remove(self.block_update_file)
			except:
				pass

			return 'bail', 'There are no osmc packages'

		if not any([bl in av for bl in self.EXTERNAL_UPDATE_REQUIRED_LIST for av in available_updates]):

			# self.EXTERNAL_UPDATE_REQUIRED = 0		##### changed to force all updates to occur with Kodi closed.
			self.EXTERNAL_UPDATE_REQUIRED = 1

		# display update available notification
		if not self.s['suppress_icon']:

			self.window.setProperty('OSMC_notification', 'true')

		# display a warning to the user
		if self.UPDATE_WARNING:
			if self.s['on_upd_detected'] not in [1, 2, 3, 4]:
				confirm_update = self.display_update_warning()

				if not confirm_update:

					return 'bail', 'User declined to update major version of Kodi'

		return 'passed', 'legit updates available'


	def display_update_warning(self):
		''' Displays a modal warning to the user that a major update is available, but that this could potentially cause
			addon or database incompatibility.'''

		user_confirm = DIALOG.yesno(lang(32077), lang(32128), lang(32127), lang(32126), yeslabel=lang(32125), nolabel=lang(32124))

		return user_confirm


	# ACTION METHOD
	@clog(log)
	def apt_update_manual_complete(self):

		self.apt_update_complete(data='manual_update_complete')


	# ACTION METHOD
	@clog(log)
	def apt_update_complete(self, data=None):
		
		check, result = self.check_for_legit_updates()

		if check == 'bail':

			if 'Sufficient freespace:' in result:

				# send kill message to progress bar
				self.progress_bar(kill=True)

			elif data == 'manual_update_complete':

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

			if self.UPDATE_WARNING:

				confirm = self.display_update_warning()
				
				if not confirm: return 'user declined to do a major version update'

			self.call_child_script('commit')
			
			return 'Download, install, prompt if restart needed'


	@clog(log)
	def check_for_major_release(self, pkg):
		''' Checks a package to see whether it is a major release. This should trigger a warning to users that things might break'''

		dig = '1234567890'

		log('Checking package (%s) for major version change.' % pkg.shortname)

		# get version of current package, raw_local_version_string
		rlv = subprocess.check_output(["/usr/bin/dpkg-query", "-W", "-f", "'${version}\n'", pkg.shortname])
		log('dpkg query results: %s' % rlv)

		lv = ''.join([x for x in rlv[:rlv.index(".")] if x in list(dig)])
		log('Local version number: %s' % lv)

		# get version of updating package, raw_remote_version_string
		versions = pkg.versions
		log('Versions available: %s' % versions)

		if not versions: return False

		rrv = versions[0].version
		log('First version selected: %s' % rrv)

		rv = ''.join([x for x in rrv[:rrv.index(".")] if x in list(dig)])
		log('Available version string: %s' % rv)

		try:
			if int(lv) < int(rv):
				return True
		except:
			pass

		return False


	@clog(log)
	def check_if_reboot_required(self):
		''' Checks for the existence of two specific files that indicate an installed package mandates a reboot. '''

		flag_files = ['/tmp/reboot-needed', '/var/run/reboot-required']

		if any([os.path.isfile(x) for x in flag_files]):
			return True
		else:
			return False


	def clean_apt_cache(self):

		try:
			os.system('sudo apt-cache clean')
		except:
			pass


	def check_for_unsupported_version(self):

		''' Checks if this version is an Alpha, prevent updates '''

		fnull = open(os.devnull, 'w')

		process = subprocess.call(['/usr/bin/dpkg-query', '-l', 'rbp-mediacenter-osmc'], stderr=fnull, stdout=fnull)

		fnull.close()

		if process == 0:

			ok = DIALOG.ok(lang(32102), lang(32103), lang(32104))

			return 'alpha'

		else:

			return 'proceed'


	def check_target_location_for_size(self, location, requirement):

		''' Checks the target location to see if there is sufficient space for the update.
			Returns tuple of boolean if there is sufficient disk space and actual freespace recorded '''

		mb_to_b = requirement * 1048576.0

		try:
			st = os.statvfs(location)

			if st.f_frsize:
				available = st.f_frsize * st.f_bavail
			else:
				available = st.f_bsize * st.f_bavail
			# available	= st.f_bfree/float(st.f_blocks) * 100 * st.f_bsize

			log('local required disk space: %s' % mb_to_b)
			log('local available disk space: %s' % available)

			return mb_to_b < available, available / 1048570
				
		except:

			return False, 0


	def automatic_freespace_checker(self):
		''' Daily checker of freespace on /. Notifies user in Home window when there is less than 50mb remaining. '''

		if self.freespace_supressor > 172800:

			self.freespace_supressor = 0

			freespace, value = self.check_target_location_for_size(location='/', requirement=250)

			if not freespace:

				if 'Home.xml' in xbmc.getInfoLabel('Window.Property(xmlfile)'):

					if self.freespace_remedy == 'apt':

						# THIS SECTION IS CURRENTLY DISABLED
						# TO ENABLE IT CHANGE THE INIT FREESPACE_REMEDY TO 'apt'

						resp = DIALOG.yesno(	'OSMC', 
												'Your system is running out of storage (<%sMB left).' % int(value), 
												'Would you like to try and clear unused system files?'
												)

						if resp:

							subprocess.Popen(['sudo', 'apt-get', 'autoremove', '&&', 'apt-get', 'clean'])

							self.freespace_remedy = 'reboot'

							# wait 10 minutes before next space check
							self.freespace_supressor = 171600

					else: # self.freespace_remedy == 'reboot'

						resp = DIALOG.ok(	'OSMC', 
											'Your system is running out of storage (<%sMB left).' % int(value), 
											'Try rebooting a couple times to clear out temporary files.'
											)

