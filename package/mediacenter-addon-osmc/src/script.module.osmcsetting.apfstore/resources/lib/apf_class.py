
# KODI modules
import xbmc
import xbmcaddon
import sys
import os
import hashlib


addonid 	= "script.module.osmcsetting.apfstore"
__addon__  	= xbmcaddon.Addon(addonid)
__path__ 	= xbmc.translatePath(xbmcaddon.Addon(addonid).getAddonInfo('path'))

# Custom module path
sys.path.append(os.path.join(__path__, 'resources','lib'))

# OSMC SETTING Modules
from CompLogger import comprehensive_logger as clog

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


class APF_obj(object):

	def __init__(self, *args, **kwargs):

		self.id 			= kwargs.get('id', 'none')
		self.name 			= kwargs.get('name', 'none')
		self.shortdesc 		= kwargs.get('shortdesc', '')
		self.longdesc 		= kwargs.get('longdesc', '')
		self.maintainedby 	= kwargs.get('maintainedby', '')
		self.version 		= kwargs.get('version', '')
		self.lastupdated 	= kwargs.get('lastupdated', '')
		self.iconurl 		= kwargs.get('iconurl', '/none')
		self.iconhash 		= kwargs.get('iconhash', 0)
		self.retrieve_icon  = False
		self.current_icon   = self.check_icon(self.iconurl)

		

	@clog(logger=log)
	def check_icon(self, iconurl):
		''' Checks the addon data folder for the icon,
				if not found, check for an icon stored in the addon/media folder, 
					if not found there, mark the new icon for download by Main in a different thread, 
					use default substitute in the meantime
				if found, check the hash matches, if not mark new icon for download by Main in another thread,
					use existing in meantime
		'''

		icon_name = iconurl.split('/')[-1]

		if os.path.isfile(os.path.join(USERART, icon_name)):
			# check userdata folder

			current_icon = os.path.join(USERART, icon_name)

		elif os.path.isfile(os.path.join(ADDONART, icon_name)):
			# check addon art folder

			current_icon = os.path.join(ADDONART, icon_name)

		else:

			current_icon = os.path.isfile(os.path.join(USERART, 'default.png'))

		# get the hash
		icon_hash = hashlib.md5(open(current_icon).read()).hexdigest()

		if icon_hash != self.iconhash:

			self.retrieve_icon  = True

		return current_icon
