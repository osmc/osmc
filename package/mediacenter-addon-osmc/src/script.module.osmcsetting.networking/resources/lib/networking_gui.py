# Standard Modules
from collections import namedtuple, OrderedDict
import socket
import random # farts, this is needed for testing only
import string # farts, this is needed for testing only

# XBMC Modules
import xbmcaddon
import xbmcgui
import xbmc


__addon__      	= xbmcaddon.Addon('script.module.osmcsetting.networking')
DIALOG 			= xbmcgui.Dialog()


def log(message):
	xbmc.log(str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


gui_ids = { \

1           :    'Headings -- Wired Network - Wireless Network - Bluetooth - Tethering(X) - VPN(X)',
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
5000		:	 'WiFi panel',

}

ip_controls 		= [10112,10113,10114,10115,10116,910112,910113,910114,910115,910116,10212,10213,10214,10215,10216,910212,910213,910214,910215,910216,]

hdg    				= namedtuple('hdg', ['name', 'lang_id', 'panel_id'])
heading_controls 	= [ 	
						hdg('wired', 		32001, 		1010), 
						hdg('wireless', 	32002, 		1020), 
						hdg('bluetooth', 	32003, 		1030),
					  ]

panel_controls 		= [1010, 1020, 1030]


class networking_gui(xbmcgui.WindowXMLDialog):


	def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):

		self.setting_values = kwargs.get('setting_values', {})


	def onInit(self):

		# heading control list (HCL)
		self.HCL = self.getControl(1)

		# wifi panel (WFP)
		self.WFP = self.getControl(5000)

		# bluetooth device panel (BTD)
		self.BTD = self.getControl(6000)		

		# list containing listitems of all wifi networks
		self.wifis = []

		# connected SSID, the ssid we are currently connected to
		self.conn_ssid = ''


		# populate the heading control list (HCL)
		for heading in heading_controls:

			# farts insert conditional checking of heading necessity here

			tmp = xbmcgui.ListItem(lang(heading.lang_id))
			tmp.setProperty('panel_id', str(heading.panel_id))

			self.HCL.addItem(tmp)


		# set all the panels to invisible except the first one
		self.toggle_panel_visibility(0)

		# set focus on the heading control list
		self.setFocusId(1)


	def onClick(self, controlID):

		if controlID in ip_controls:
			
			self.edit_ip_address(controlID)


	def onAction(self, action):

		actionID = action.getId()
		focused_control = self.getFocusId()

		log('actionID = ' + str(actionID))
		log('focused_control = %s,   %s' % (type(focused_control),focused_control))

		if actionID in (10, 92):
			self.close() 
		
		if focused_control == 1:
			# change to the required settings panel

			try:
				focused_position = int(xbmc.getInfoLabel('Container(1).Position'))
			except:
				focused_position = 0

			log('focused_position = %s' % focused_position)

			self.toggle_panel_visibility(focused_position)

			if focused_position == 1:

				self.populate_wifi_panel()

			if focused_position == 2:

				self.populate_bluetooth_panel()



	def toggle_panel_visibility(self, focused_position):
		''' Takes the focussed position in the Heading List Control and sets only the required panel to visible. '''
		target_panel = int(self.HCL.getListItem(focused_position).getProperty('panel_id'))

		# get the required panel
		try:		
			target_panel = int(self.HCL.getListItem(focused_position).getProperty('panel_id'))

		except:
			target_panel = 0

		log('target_panel = %s' % target_panel)

		for panel_id in panel_controls:

			self.getControl(panel_id).setVisible(True if target_panel == panel_id else False)


	def edit_ip_address(self, controlID):

		relevant_label_control 	= self.getControl(900000 + controlID)
		current_label 			= relevant_label_control.getLabel()

		if current_label == '___.___.___.___':
			current_label = ''

		user_input = DIALOG.input(lang(32004), current_label, type=xbmcgui.INPUT_IPADDRESS)

		if not user_input:

			relevant_label_control.setLabel('___.___.___.___')

		else:
			# validate ip_address format
			try:
				socket.inet_aton(user_input)

			except:
				ok = DIALOG.ok(lang(32004), lang(32005))

				self.edit_ip_address(controlID)

				return

			relevant_label_control.setLabel(user_input)

			return
				

	def populate_wifi_panel(self, wifi_dict={}):
		''' Populates the wifi panel with the information provided in the wifi_dict.

			wifi_dict = {
							'ssid' : {'encryption': True|False, 'strength': percent::int },
								...
						}
		'''
		wifi_dict = {}
		for x in range(22):
			word = self.randomword()
			encryption = random.choice([True,False])
			strength = random.randint(0,85)
			wifi_dict[word] = {'encryption': encryption, 'strength': strength }

		self.wifis = []

		for ssid, info in wifi_dict.iteritems():

			# hidden networks are ignored

			if ssid:

				itm = xbmcgui.ListItem(ssid)

				st = info['strength']

				# icon_tuple = (connected, encrypted, strength)
				icon_image = self.get_wifi_icon(connected, info['encryption'], (int(st) / 25 ) + 1)

				itm.setIconImage(icon_image)

				itm.setProperty('strength', str(st))

				self.wifis.append(itm)

		# sort the list of wifis based on signal strength
		self.wifis.sort(key=self.sort_strength, reverse=True)

		# remove everything from the existing panel
		self.WFP.reset()

		# add the top 20 new items to the panel
		self.WFP.addItems(self.wifis[:20])

		# set the current connection as selected
		for i, wifi in enumerate(self.wifis):
			if wifi.getLabel() == self.conn_ssid:
				self.WFP.selectItem(i)


		self.WFP.getListItem(random.randint(0,10)).select(True)

	def populate_bluetooth_panel(self, bluetooth_dict={}):
		'''
			Populates the bluetooth panel with devices from the bluetooth_dict.

			bluetooth_dict = {
								Address : { alias: __, paired: bool, connected: bool}
							}

		'''


	def get_wifi_icon(self, encrypted, strength, connected=False):

		icon_tuple = (connected, encrypted, strength)

		icons = {
					(True,   True, 0) : 'bar0_ce.png',
					(True,   True, 1) : 'bar1_ce.png',
					(True,   True, 2) : 'bar2_ce.png',
					(True,   True, 3) : 'bar3_ce.png',
					(True,   True, 4) : 'bar4_ce.png',
					(True,  False, 0) : 'bar0_cx.png',
					(True,  False, 1) : 'bar1_cx.png',
					(True,  False, 2) : 'bar2_cx.png',
					(True,  False, 3) : 'bar3_cx.png',
					(True,  False, 4) : 'bar4_cx.png',
					(False,  True, 0) : 'bar0_xe.png',
					(False,  True, 1) : 'bar1_xe.png',
					(False,  True, 2) : 'bar2_xe.png',
					(False,  True, 3) : 'bar3_xe.png',
					(False,  True, 4) : 'bar4_xe.png',
					(False, False, 0) : 'bar0_xx.png',
					(False, False, 1) : 'bar1_xx.png',
					(False, False, 2) : 'bar2_xx.png',
					(False, False, 3) : 'bar3_xx.png',
					(False, False, 4) : 'bar4_xx.png',
				}

		return icons.get(icon_tuple, 'bar0_xx.png')


	def sort_strength(self, itm):

		try:
			metric = int(itm.getProperty('strength'))
		except:
			metric = 0

		return metric


	def randomword(self):

		length = random.randint(0,25)

		return ''.join(random.choice(string.letters+string.digits) for i in range(length))