
# KODI modules
import xbmc
import xbmcgui
import xbmcaddon

# Standard modules
import sys
import os
import hashlib
import threading
import json
import requests
import Queue
import shutil
import apt
import traceback
import datetime as dt
import subprocess

addonid 	= "script.module.osmcsetting.apfstore"
__addon__  	= xbmcaddon.Addon(addonid)
__path__ 	= xbmc.translatePath(xbmcaddon.Addon(addonid).getAddonInfo('path'))
__setting__ = __addon__.getSetting

# Custom module path
sys.path.append(os.path.join(__path__, 'resources','lib'))

# OSMC SETTING Modules
from CompLogger import comprehensive_logger as clog
from apf_class import APF_obj
from apf_gui import apf_GUI


ADDONART = os.path.join(__path__, 'resources','skins', 'Default', 'media')
USERART  = os.path.join(xbmc.translatePath('special://userdata/'),'addon_data', addonid)


def log(message):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )
		
	xbmc.log('OSMC APFStore store : ' + str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


'''
=========================
APF JSON STRUCTURE
=========================

{
   "application": [
       {
           "id": "ssh-app-osmc",
           "name": "SSH Server",
           "shortdesc": "This allows you to connect to your OSMC device via SSH",
           "longdesc": "This installs an OpenSSH server on your OSMC device allowing you to log in to your device remotely as well as transfer files via SCP.",
           "maintained-by": "OSMC",
           "version": "1.0.0",
           "lastupdated": "2015-01-23",
           "iconurl": "http://blah",
           "iconhash": 0,
       }
   ]
}
'''


def check_for_unsupported_version():

	''' Checks if this version is an Alpha, prevent updates '''

	fnull = open(os.devnull, 'w')

	process = subprocess.call(['/usr/bin/dpkg-query', '-l', 'rbp-mediacenter-osmc'], stderr=fnull, stdout=fnull)

	fnull.close()

	if process == 0:

		ok = xbmcgui.Dialog().ok(lang(32017), lang(32018), lang(32019))

		return 'alpha'

	else:

		return 'proceed'



class APF_STORE(object):


	def __init__(self):

		# do not proceed if the version is alpha
		if check_for_unsupported_version() == 'alpha': return

		self.CACHETIMEOUT = 6

		self.TIME_STRING_PATTERN = '%Y/%m/%d %H:%M:%S'

		self.touch_addon_data_folder()

		self.install_status_cache 	= {x.split('=')[0] : x.split('=')[1] for x in __setting__('install_status_cache').split(':_:') if '=' in x}

		json_data = self.get_apf_data()

		if json_data == 'failed':

			log('Failed to retrieve JSON apf data')

			return

		self.apf_dict = self.generate_apf_dict(json_data)

		self.apf_GUI = self.create_apf_store_gui(self.apf_dict)

		self.retrieve_install_status()

		self.retrieve_icons()

		self.apf_GUI.doModal()


	def get_apf_data(self):

		self.use_cache = self.check_lastupdated()

		if self.use_cache == True:

			cache = self.read_jsoncache()

			if cache == 'failed':

				return self.get_remote_json()

			else:

				return cache


		else:

			return self.get_remote_json()


	def get_remote_json(self):

		json_req = self.get_list_from_sam()

		if json_req == 'failed':

			log('Failed to retrieve osmcdev= from /proc/cmdline')

			return 'failed'

		elif not json_req:

			log('Failed to retrieve data from %s' % self.URL)

			return 'failed'

		else:

			return json_req


	def read_jsoncache(self):

		self.json_cache = __setting__('json_cache')

		if self.json_cache == 'empty': return 'failed'

		try:

			return json.loads(self.json_cache)

		except:

			return 'failed'


	def check_lastupdated(self):

		current_time = self.get_current_time()

		if current_time == 'failed': return False

		self.json_lastupdated_record = __setting__('json_lastupdated')

		if self.json_lastupdated_record == 'never': return False

		try:
			date_object = dt.strptime(self.json_lastupdated_record, self.TIME_STRING_PATTERN)
		except:
			return False

		trigger = date_object + dt.timedelta(hours=self.CACHETIMEOUT)

		if trigger > current_time:
			log('JSON Cache is fresh')
			return True

		return False


	@clog(logger=log, maxlength=10000)
	def generate_apf_dict(self, json_req):

		apf_list = json_req.get('application', [])

		obj_list = [APF_obj() for x in apf_list if x['id']]

		return { x['id']: obj_list[i-1].populate(x) for i, x in enumerate(apf_list) if x['id'] }


	def get_current_time(self):

		# this method is necessary as datetime.now() has issues with the GIL and throws an error at random

		for x in range(50):

			try:
				return dt.datetime.now()
			except:
				pass

		else:
			log('retrieving current time failed')
			return 'failed'


	@clog(logger=log)
	def get_list_from_sam(self):

		try: 

			# generate the URL
			with open('/proc/cmdline', 'r') as f:

				line = f.readline()

				settings = line.split(' ')

				suffix = None

				for setting in settings:

					if setting.startswith('osmcdev='):

						self.URL = 'http://download.osmc.tv/apps/%s' % setting[len('osmcdev='):]

						break

				else:

					# this is for testing only
					self.URL = 'http://download.osmc.tv/apps/rbp2'

					# return 'failed'

		except:

			self.URL = 'http://download.osmc.tv/apps/rbp2'

		log('APF data URL: %s' % self.URL)
		
		try:
			r = requests.get(self.URL.replace('\n','').replace('\t','').replace('\n',''))

		except:
			log('Connection to %s failed' % self.URL)

			return 'failed'

		try:

			q = r.json()

			__addon__.setSetting('json_cache', json.dumps(q))

			current_time = self.get_current_time()

			if current_time != 'failed':

				__addon__.setSetting('json_lastupdated', current_time.strftime(self.TIME_STRING_PATTERN))

			return q
			
		except:

			log('JSON couldnt be read: %s' % r.text)

			return 'failed'


	@clog(logger=log)
	def retrieve_icons(self):

		thread_queue = Queue.Queue()

		for ident, apf in self.apf_dict.iteritems():

			if apf.retrieve_icon:

				thread_queue.put(apf)

		# spawn some workers
		# for i in range(1):

		t = threading.Thread(target=self.grab_icon_from_sam, args=(thread_queue,))
		t.daemon = True
		t.start()


	@clog(logger=log)
	def grab_icon_from_sam(self, thread_queue):

		while True:

			try:
				# grabs the item from the queue
				# the get BLOCKS and waits 1 second before throwing a Queue Empty error
				q_item = thread_queue.get(True, 1)
				
				thread_queue.task_done()

				# download the icon and save it in USERART

				response = requests.get(q_item.iconurl, stream=True)

				icon_name = q_item.iconurl.split('/')[-1]

				with open(os.path.join(USERART, icon_name), 'wb') as out_file:

				    shutil.copyfileobj(response.raw, out_file)

				del response

				q_item.refresh_icon()

			except Queue.Empty:

				log('Queue.Empty error')

				break


	@clog(logger=log)
	def retrieve_install_status(self):


		with os.popen('dpkg -l') as f:
			self.package_list = ''.join(f.readlines())

		thread_queue = Queue.Queue()

		for ident, apf in self.apf_dict.iteritems():

			thread_queue.put(apf)

		t = threading.Thread(target=self.grab_install_status, args=(thread_queue,))
		t.daemon = True

		t.start()


	@clog(logger=log)
	def grab_install_status(self, thread_queue):

		while True:

			try:
				# grabs the item from the queue
				# the get BLOCKS and waits 1 second before throwing a Queue Empty error
				apf = thread_queue.get(True, 1)

				install_query = ['dpkg-query', '-W', '-f="${Status}"', apf.id]
				
				fnull = open(os.devnull, 'w')

				try:
					output = subprocess.check_output(install_query, stderr=fnull)

				except subprocess.CalledProcessError as e:
				# raise RuntimeError("command '{}' return with error (code {}): {}".format(e.cmd, e.returncode, e.output))
					output = e.output

				fnull.close()
				
				if "ok installed" in output:
					log('%s IS Installed' % apf.name)

					apf.set_installed(True)

				else:

					log('%s is NOT Installed' % apf.name)
				
				thread_queue.task_done()

			except Queue.Empty:

				log('Queue.Empty error')

				break

			except Exception as e:

				log(traceback.format_exc())

				break


	@clog(logger=log)
	def touch_addon_data_folder(self):

		if not os.path.isdir(USERART):
			os.makedirs(USERART)

		return USERART


	@clog(logger=log)
	def create_apf_store_gui(self, apf_dict):

		xml = "APFBrowser_720OSMC.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "APFBrowser_OSMC.xml"

		return apf_GUI(xml, __path__, 'Default', apf_dict=apf_dict)

