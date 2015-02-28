
# KODI modules
import xbmc
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

addonid 	= "script.module.osmcsetting.apfstore"
__addon__  	= xbmcaddon.Addon(addonid)
__path__ 	= xbmc.translatePath(xbmcaddon.Addon(addonid).getAddonInfo('path'))

# Custom module path
sys.path.append(os.path.join(__path__, 'resources','lib'))

# OSMC SETTING Modules
from CompLogger import comprehensive_logger as clog
from apf_class import APF_obj
from apf_gui import apf_GUI


ADDONART = os.path.join(__path__, 'resources','skins', 'Default', 'media')
USERART  = os.path.join(xbmc.translatePath('special://userdata/'),'addon_data', addonid)


def log(message):
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


class APF_STORE(object):


	def __init__(self):

		self.touch_addon_data_folder()

		self.install_status_cache = {x.split('=')[0] : x.split('=')[1] for x in __addon__.getSetting('install_status_cache').split(':_:') if '=' in x}

		json_req = self.get_list_from_sam()

		if json_req == 'failed':

			log('Failed to retrieve osmcdev= from /proc/cmdline')

			return

		elif not json_req:

			log('Failed to retrieve data from %s' % self.URL)

			return

		self.apf_dict = self.generate_apf_dict(json_req)

		self.apf_GUI = self.create_apf_store_gui(self.apf_dict)

		self.retrieve_install_status()

		self.retrieve_icons()

		self.apf_GUI.doModal()


	@clog(logger=log, maxlength=10000)
	def generate_apf_dict(self, json_req):

		apf_list = json_req.get('application', [])

		obj_list = [APF_obj() for x in apf_list if x['id']]

		return { x['id']: obj_list[i-1].populate(x) for i, x in enumerate(apf_list) if x['id'] }


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

						log('APF data URL: %s' % self.URL)

						break

				else:

					# this is for testing only
					self.URL = 'http://download.osmc.tv/apps/rbp'

					# return 'failed'

		except:

			self.URL = 'http://download.osmc.tv/apps/rbp'
			

		r = requests.get(self.URL.replace('\n','').replace('\t','').replace('\n',''))

		try:

			q = r.json()

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

		try:
			self.cache = apt.Cache()
			self.cache.open()

			for pkg in self.cache:

				log(pkg.shortname)


			thread_queue = Queue.Queue()

			for ident, apf in self.apf_dict.iteritems():

				thread_queue.put(apf)

			# spawn some workers
			# for i in range(1):

			t = threading.Thread(target=self.grab_install_status, args=(thread_queue,))
			t.daemon = True

			# reset all cached install status
			__addon__.setSetting('install_status_cache', '')

			t.start()

		except apt.cache.LockFailedException:

			# if the cache is locked then use the stored version of the install status

			for idee, apf in self.apf_dict.iteritems():

				if self.install_status_cache.get(idee, False):

					apf.set_installed(True)


	@clog(logger=log)
	def grab_install_status(self, thread_queue):

		while True:

			try:
				# grabs the item from the queue
				# the get BLOCKS and waits 1 second before throwing a Queue Empty error
				q_item = thread_queue.get(True, 1)
				

				# check the install status of this package
				pkg = self.cache[q_item.id]

				log('package = %s' % pkg.shortname)

				if pkg.is_installed:

					log('%s IS Installed' % pkg.shortname)

					q_item.set_installed(True)

					tmp = __addon__.getSetting('install_status_cache') + ':_:' + q_item.id + '=installed'

					__addon__.setSetting('install_status_cache', tmp)

				else:

					log('%s is NOT Installed' % pkg.shortname)
				
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

		if 'osmc' in xbmc.getSkinDir().lower():

			return apf_GUI("APFBrowser_OSMC.xml", __path__, 'Default', apf_dict=apf_dict)

		else:

			return apf_GUI("APFBrowser.xml", __path__, 'Default', apf_dict=apf_dict)

