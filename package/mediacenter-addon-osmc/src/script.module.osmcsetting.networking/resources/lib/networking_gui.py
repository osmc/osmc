# Standard Modules
from collections import namedtuple

# XBMC Modules
import xbmcaddon
import xbmcgui
import xbmc


__addon__              	= xbmcaddon.Addon()
__addonid__            	= __addon__.getAddonInfo('script.module.osmcsetting.networking')


def log(message):
	xbmc.log(str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


gui_ids = { \

100         :    'Headings -- Wired Network - Wireless Network - Bluetooth - Tethering(X) - VPN(X)',
101         :    'Heading Wired',
102         :    'Heading Wireless',
103         :    'Heading Bluetooth',
1010        :    'Panel Wired Network',
1020        :    'Panel Wireless Network',
1030        :    'Panel Bluetooth',
10111       :    'Wired - Automatically configure the network toggle',
10114       :    'Wired - Default Gateway',
910114      :    'Wired - Default Gateway VALUE',
10112       :    'Wired - IP Address',
910112      :    'Wired - IP Address VALUE',
10115       :    'Wired - Primary DNS',
910115      :    'Wired - Primary DNS VALUE',
10116       :    'Wired - Secondary DNS',
910116      :    'Wired - Secondary DNS VALUE',
10113       :    'Wired - Subnet Mask',
910113      :    'Wired - Subnet Mask VALUE',
10211       :    'Wireless - Automatically configure the network toggle',
10214       :    'Wireless - Default Gateway',
910214      :    'Wireless - Default Gateway VALUE',
10212       :    'Wireless - IP Address',
910212      :    'Wireless - IP Address VALUE',
10215       :    'Wireless - Primary DNS',
910215      :    'Wireless - Primary DNS VALUE',
10200       :    'Wireless - Scan for connections',
10216       :    'Wireless - Secondary DNS',
910216      :    'Wireless - Secondary DNS VALUE',
10213       :    'Wireless - Subnet Mask',
910213      :    'Wireless - Subnet Mask VALUE',

}

ip_controls = [10112,10113,10114,10115,10116,910112,910113,910114,910115,910116,10212,10213,10214,10215,10216,910212,910213,910214,910215,910216,]


class networking_gui(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):

		self.setting_values = kwargs.get('setting_values', {})


	def onInit(self):

		pass


	def onClick(self, controlID):

		if controlID in ip_controls:
			# display ip address subwindow

			pass



	def onAction(self, action):

		actionID = action.getId()
		focused_control = self.getFocusId()

		if actionID in (10, 92):
			self.close() 

		else:

			log('actionID = ' + str(actionID))
