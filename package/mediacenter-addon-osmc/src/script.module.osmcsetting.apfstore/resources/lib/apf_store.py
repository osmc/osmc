
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

		json_req = self.get_list_from_sam()

		if json_req == 'failed':

			log('Failed to retrieve osmcdev= from /proc/cmdline')

			return

		elif not json_req:

			log('Failed to retrieve data from %s' % self.URL)

			return

		self.apf_dict = self.generate_apf_dict(json_req)

		self.apf_GUI = self.create_apf_store_gui(self.apf_dict)

		self.retrieve_icons()

		self.apf_GUI.doModal()


	@clog(logger=log, maxlength=10000)
	def generate_apf_dict(self, json_req):

		apf_list = json_req.get('application', [])

		obj_list = [APF_obj() for x in apf_list if x['id']]

		return { x['id']: obj_list[i-1].populate(x) for i, x in enumerate(apf_list) if x['id'] }


	@clog(logger=log)
	def get_list_from_sam(self):

		# generate the URL
		with open('/proc/cmdline', 'r') as f:

			line = f.readline()

			settings = line.split(' ')

			suffix = None

			for setting in settings:

				if setting.startswith('osmcdev='):

					self.URL = 'http://download.osmc.tv/apps/%s' % setting[len('osmcdev='=):]

					log('APF data URL: %s' % self.URL)

					break

			finally:

				return 'failed'

		r = requests.get(self.URL)

		r.json()

		return r


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

