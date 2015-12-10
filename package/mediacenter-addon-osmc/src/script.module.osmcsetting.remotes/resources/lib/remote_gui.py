
# KODI modules
import xbmc
import xbmcaddon
import xbmcgui

# Standard modules
import sys
import os
import shutil
import subprocess
import threading

addonid 	= "script.module.osmcsetting.remotes"
__addon__  	= xbmcaddon.Addon("script.module.osmcsetting.remotes")
__path__ 	= xbmc.translatePath(xbmcaddon.Addon(addonid).getAddonInfo('path'))
DIALOG      = xbmcgui.Dialog()

# Custom module path
sys.path.append(os.path.join(__path__, 'resources','lib'))

# OSMC SETTING Modules
from CompLogger import comprehensive_logger as clog


ACTION_PREVIOUS_MENU = 10
ACTION_NAV_BACK      = 92
SAVE                 = 5
HEADING              = 1
ACTION_SELECT_ITEM   = 7

LIRCD_PATH = '/etc/lirc/lircd.conf'
ETC_LIRC   = '/etc/lirc'

if not os.path.isdir(ETC_LIRC):
	LIRCD_PATH = '/home/kubkev/temp/lirc/lircd.conf'
	ETC_LIRC   = '/home/kubkev/temp/lirc'	


def log(message):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )
		
	xbmc.log('REMOTE: ' + str(message), level=xbmc.LOGDEBUG)


@clog(log)
def lang(id):

	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


def construct_listitem(conf):

	path, filename = os.path.split(conf)

	# get conf name; check first line in file for "# name:"
	with open(conf, 'r') as f:
		lines = f.readlines()
		first_line = lines[0]
		if first_line.startswith("# name:"):
			name = first_line[len("# name:"):]
			name2 = filename
		else:
			name = filename.replace('.conf', '')
			name2 = conf

	# check for remote image, use it if it is available
	image_path = os.path.join(path, filename.replace('.conf','.png'))
	
	if os.path.isfile(image_path):

		tmp = xbmcgui.ListItem(label=name, label2=name2, thumbnailImage=image_path)

	else:

		tmp = xbmcgui.ListItem(label=name, label2=name2)

	tmp.setProperty('fullpath',conf)

	tmp.setInfo('video',{'title': ''.join(lines[:100])})

	return tmp


def test_custom(conf):

	''' Returns a boolean indicating whether the supplied conf file is a custom conf file. '''

	try:
		path, filename = os.path.split(conf)

		if path != ETC_LIRC:
			return True
		else:
			return False

	except:
		return False


class remote_gui_launcher(object):

	def __init__(self):

		# flag to idicate whether the GUI should re-open upon close. This is for when the remote changes do not stick.
		self.reopen = True

		# container for any confs we want to ignore
		self.excluded = ['lircd.conf']

		self.active_conf = os.path.realpath(LIRCD_PATH)

		# check if the target file actually exists, if it doesnt, then set the active conf file as None,
		# if it does, then check whether it is a custom file
		if os.path.isfile(self.active_conf):

			custom = test_custom(self.active_conf)

		else:

			custom = False
			self.active_conf = None

		# get the contents of /etc/lirc/
		local_confs_base = os.listdir(ETC_LIRC)
		local_confs_raw = [os.path.join(ETC_LIRC, conf) for conf in local_confs_base]
		local_confs_raw.sort()

		# filter list by files with size (this just removes any empty confs)
		local_confs = []
		for conf in local_confs_raw:
			if os.path.basename(conf) in self.excluded: continue
			if not conf.endswith('.conf'): continue
			try:
				if os.stat(conf).st_size == 0: continue
			except:
				continue

			local_confs.append(construct_listitem(conf))

		if custom:
			# self.active_conf can only be None if custom is False, so there is no risk in this
			# reconstruction of the local_confs
			local_confs = [construct_listitem(self.active_conf)] + local_confs

		xml = "RemoteBrowser_720OSMC.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "RemoteBrowser_OSMC.xml"

		self.remote_gui = remote_GUI(xml, __path__, 'Default', local_confs=local_confs, active_conf=self.active_conf)


	def open_gui(self):

		while self.reopen:

			self.reopen = False

			self.remote_gui.doModal()


	


class remote_GUI(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, local_confs, active_conf):

		self.local_confs = local_confs
		self.active_conf = active_conf

		self.remote_selection = None


	def onInit(self):

		self.list = self.getControl(500)
		self.list.setVisible(True)

		for i, x in enumerate(self.local_confs):
			self.list.addItem(x)

		self.highlight_selected()

		try:
			self.getControl(50).setVisible(False)
		except:
			pass


	def find_custom_item(self):

		log('Finding custom item in list')

		for i in range(0,self.list.size()):
			tmp = self.list.getListItem(i)
			tmp_path = tmp.getLabel2()
			if test_custom(tmp_path):
				log('Custom item found')
				return i, tmp

		return 0, 'failed'


	def highlight_selected(self):

		log('Changing highlighting to %s' % self.active_conf)

		for i in range(0,self.list.size()):
			tmp = self.list.getListItem(i)

			tmp_path = tmp.getLabel2()

			# if self.active_conf is None (i.e. the user deleted it externally) then no item will be selected
			if self.active_conf == tmp_path:
				tmp.select(True)
			else:
				tmp.select(False)


	def onClick(self, controlID):

		if controlID == 500:
			# user has selected a local file from /etc/lirc

			self.remote_selection = self.getControl(500).getSelectedItem().getProperty('fullpath')
			result = self.test_selection()

			if result == 'success':

				log('User confirmed the remote changes work')

				# change the highlighted remote to the new selection
				self.active_conf = self.remote_selection


			elif result == 'service_dead':
				log('Remote service failed to restart.')

				ok = DIALOG.ok(lang(32006), lang(32013))

				self.remote_selection = None


			else:

				log('User did not confirm remote changes')

				self.remote_selection = None

			self.highlight_selected()

		elif controlID == 7:
			# user has selected Exit

			self.remote_selection = None

			self.close()


		elif controlID == 62:
			# user has chosen to browse for the file

			log('User is browsing for remote conf')

			browser = xbmcgui.Dialog().browse(1, lang(32005), 'files', mask='.conf')

			if browser:

				log('User selected remote conf: %s' % self.remote_selection)

				self.remote_selection = browser

				result = self.test_selection()

				if result == 'success':

					log('user confirmed the remote changes work')

					# change the highlighted remote to the new selection
					self.active_conf = self.remote_selection

					# see if there is a custom file in the list, delete it if there is
					i, custom = self.find_custom_item()

					if custom:
						self.list.removeItem(i)

					# add the new custom as an item
					# self.active_conf cannot be None at this point, as the user must have selected one
					tmp = construct_listitem(self.active_conf)
					self.list.addItem(tmp)

					self.highlight_selected()

				elif result == 'service_dead':
					log('Remote service failed to restart.')

					ok = DIALOG.ok(lang(32006), lang(32013))

					self.remote_selection = None

				else:

					self.remote_selection = None

			else:

				self.remote_selection = None


	def test_selection(self):

		log('Testing remote conf selection: %s' % self.remote_selection)

		if os.path.isfile(self.remote_selection):

			# read the symlink target
			original_target = os.readlink( LIRCD_PATH )

			log('Original lircd_path target: %s' % original_target)
			
			# symlink the master conf to the new selection
			subprocess.call(['sudo', 'ln', '-sf', self.remote_selection, LIRCD_PATH])


			# open test dialog
			xml = "OSMC_remote_testing720.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "OSMC_remote_testing.xml"
			self.remote_test = remote_test(xml, __path__, 'Default', self.remote_selection)
			self.remote_test.doModal()

			log('Testing complete, result: %s' % self.remote_test.test_successful)

			# if the test wasnt successful, then revert to the previous conf
			if not self.remote_test.test_successful:

				subprocess.call(['sudo', 'ln', '-sf', original_target, LIRCD_PATH])

				subprocess.call(['sudo', 'systemctl', 'restart', 'lircd_helper@*'])

				# add busy dialog, loop until service restarts

				if not self.remote_test.service_running:

					return 'service_dead'

				else:

					return 'failed'

			return 'success'

		return 'failed'



class remote_test(xbmcgui.WindowXMLDialog):

	# control IDs

	# 90		restarting service label
	# 91		service restarted label, informs the user that the service has restarted and to confirm using the test button
	# 25		test button, user clicks this to confirm that the remotes changes have been successful
	# 45		countdown label, is controlled by the timer, and counts down the seconds to revert
	# 55 		quick revert button

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, selection):

		self.test_successful = False

		self.service_running = True

		self.selection = selection

		self.countdown_limit = 20

		self.quick_revert = False

		self.countdown_timer = countdown_timer(self)
		self.countdown_timer.setDaemon(True)

		# setup the service checker straight away
		self.service_checker = service_checker(self)
		self.service_checker.setDaemon(True)


	def onInit(self):

		log('Opening test dialog')

		self.restarting_service_label 	= self.getControl(90)
		self.check_remote_label 		= self.getControl(91)
		# self.countdown_label   		= self.getControl(45)
		self.test_button 				= self.getControl(25)
		self.progress_bar				= self.getControl(101)

		self.initial_state()

		# start the service_checker AFTER the class attributes have been set (prevents race condition)
		self.service_checker.start()


	def initial_state(self):

		''' the dialog is telling the user that the service if restarting, and to please wait '''

		log('Setting initial state of test dialog')

		# change label to say remote service restarting
		self.progress_bar.setVisible(False)
		self.restarting_service_label.setVisible(True)
		self.check_remote_label.setVisible(False)
		self.test_button.setVisible(False)
		# self.countdown_label.setVisible(False)
		# self.countdown_label.setLabel(str(self.countdown_limit))


	def second_state(self):

		''' the service has been confirmed to be running again, and now the dialog is telling the user to click on
			on the Confirm button. This will confirm that they have been able to navigate down to the button, and 
			click on it. '''

		log('Setting second state of test dialog')

		# change the label to say that the remote service has restarted and does the user want to keep the changes
		self.restarting_service_label.setVisible(False)
		self.check_remote_label.setVisible(True)	
		self.progress_bar.setVisible(True)	
		self.test_button.setVisible(True)

		# display the exit button (controlID 25)
		# self.countdown_label.setVisible(True)

		# start the timer
		self.countdown_timer.start()


	def service_dead_state(self):

		''' the service has not been detected to have started within 20 seconds.
			inform the user with OK style dialog
		'''

		log('Service is dead')

		self.service_running = False

		self.close()
		


	def onClick(self, controlID):

		if controlID == 25:
			''' user has clicked the test successful button, keep the changes,
				this is the only place that the new conf can be confirmed
			'''

			log('User has confirmed that the new conf is working.')

			self.test_successful = True
			self.countdown_timer.exit = True
			try:
				self.service_checker.exit = True
			except:
				pass
			self.close()

		elif controlID == 55:

			''' The user has decided to end the test, and would like to revert to the previous conf. This
				is likely to only occur while the service is being checked. 
			'''

			log('User has decided to revert to the previous conf.')
			try:
				self.service_checker.exit = True
			except:
				pass
			self.countdown_timer.exit = True
			self.close()


class service_checker(threading.Thread):

	''' Rsstarts the remote service, and waits for the response that it is running. '''

	def __init__(self, parent):

		super(service_checker, self).__init__(name='service_checker')
		self.parent = parent
		self.exit = False

	def run(self):

		log('Remote service checker thread active.')

		counter = 0

		# restart the service for the changes to take effect
		proc = subprocess.Popen(['sudo', 'systemctl', 'restart', 'lircd_helper@*'])

		# loop until the service has restarted (or too much time has elapsed, in which case fail out)
		while counter < 40 and not self.exit:

			p = proc.poll()

			if p is None:
				counter += 1
				xbmc.sleep(250)
				continue

			elif p == 0:
				break

		else:
			# if the process times out or the exit signal is recieved, then return nothing
			# on a timeout however, enter something in the log

			if counter >= 40:
				# this is reached if the counter reaches 40, meaning the process check timed out
				self.parent.service_dead_state()

			elif self.exit:
				# this occurs when the user has clicked cancel or back
				# there is no need to do anything
				pass

			elif p != 0:
				# this occurs if there is an error code returned by the process
				log('Error code from systemctl restart lircd-helper: %s' % p)

			return


		# this point it only reached if proc.poll returns 0
		self.parent.second_state()


class countdown_timer(threading.Thread):

	def __init__(self, parent):

		super(countdown_timer, self).__init__(name='countdown_timer')
		self.parent = parent
		self.exit = False
		self.countdown = self.parent.countdown_limit

		

	def run(self):

		''' Update the label on the dialog to show how many seconds until the conf reverts to the previous state 
		'''

		log('Countdown timer thread active')

		while not self.exit and self.countdown:


			# self.parent.countdown_label.setLabel(str(self.countdown))
			self.parent.progress_bar.setWidth(self.countdown * 60)

			xbmc.sleep(1000)

			self.countdown -= 1

		self.parent.close()



