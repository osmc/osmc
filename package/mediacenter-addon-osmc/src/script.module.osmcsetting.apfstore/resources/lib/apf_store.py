
# KODI modules
import xbmc
import xbmcaddon
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

ADDONART = os.path.join(__path__, 'resources','skins', 'Default', 'media')
USERART  = os.path.join(xbmc.translatePath('special://userdata/'),'addon_data ', addonid)


def log(message):
	xbmc.log('OSMC APFStore APF' + str(message), level=xbmc.LOGDEBUG)


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
       }
   ]
}
'''


class APF_STORE(object):


	def __init__(self):

		json_req = self.get_list_from_sam()

		self.apf_dict = self.generate_apf_dict(json_req)

		self.apf_GUI = self.create_apf_store_gui(self.apf_dict)

		self.retrieve_icons()

		self.apf_GUI.doModal()



	@clog(logger=log)
	def generate_apf_dict(self):

		apf_list = json_req.get('application', [])

		return { x['id']: APF_obj(x) for x in apf_list if x['id'] }


	@clog(logger=log)
	def get_list_from_sam(self):

		r = requests.get('https://api.github.com/user', auth=('user', 'pass'))

		return r.json()


	@clog(logger=log)
	def retrieve_icons(self):

		thread_queue = Queue.Queue()

		for ident, apf in self.apf_dict.iteritems():

			if apf.retrieve_icon:

				thread_queue.put(apf.iconurl)

		# spawn some workers
		for i in range(3):

			t = threading.Thread(target=self.grab_icon_from_sam, args=(thread_queue))
			t.start()


	@clog(logger=log)
	def grab_icon_from_sam(self, thread_queue):

		while True:

			try:
				# grabs the item from the queue
				# the get BLOCKS and waits 1 second before throwing a Queue Empty error
				q_item = thread_queue.get(True, 1)

				# download the icon and save it in USERART

				response = requests.get(q_item, stream=True)

				icon_name = q_item.split('/')[-1]

				with open(icon_name, 'wb') as out_file:

				    shutil.copyfileobj(response.raw, out_file)

				del response

				thread_queue.task_done()

			except Queue.Empty:

				log('Queue.Empty error')

				break


	@clog(logger=log)
	def create_apf_store_gui(self, apf_list):

		return apf_GUI("AddonBrowser.xml", scriptPath, 'Default', apf_list=apf_list)

