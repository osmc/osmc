
# KODI modules
import xbmc
import xbmcaddon
import xbmcgui

# Standard modules
import sys
import os


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
	xbmc.log('OSMC APFStore gui : ' + str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


class apf_GUI(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, apf_dict):

		self.apf_dict = apf_dict


	def onInit(self):

		self.list = self.getControl(50)
		for x, y in self.apf_dict.iteritems():
			self.current_icon = '/home/kubkev/.kodi/addons/script.module.osmcsetting.apfstore/resources/skins/Default/media/osmc_logo.png'

			self.list.addItem(y)

			y.setArt(
				{
				'thumb':self.current_icon,
				'poster':self.current_icon,
				'banner':self.current_icon,
				'fanart':self.current_icon,
				'clearart':self.current_icon,
				'clearlogo':self.current_icon,
				'landscape':self.current_icon,
				})

			y.setIconImage(self.current_icon)
			y.setThumbnailImage(self.current_icon)
			

		self.getControl(3).setLabel('Exit')
		self.getControl(3).setEnabled(True)
		self.getControl(5).setVisible(False)
		self.getControl(6).setVisible(False)
		self.getControl(7).setLabel('Permit Install')


