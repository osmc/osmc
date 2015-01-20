# Standard Modules
from collections import namedtuple, OrderedDict
import socket

# XBMC Modules
import xbmcaddon
import xbmcgui
import xbmc


__addon__      	= xbmcaddon.Addon('script.module.osmcsetting.networking')
DIALOG 			= xbmcgui.dialog


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

		# list containing listitems of all wifi networks
		self.wifis = []

		# populate the heading control list (HCL)
		for heading in heading_controls:

			# farts insert conditional checking of heading necessity here

			tmp = xbmcgui.ListItem(lang(heading.lang_id))
			tmp.setProperty('panel_id', heading.panel_id)

			self.HCl.addItem(tmp)


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

		if actionID in (10, 92):
			self.close() 
		
		if focused_control == 1:
			# change to the required settings panel

			focused_position = xbmc.getInfoLabel(Container(1).Position)
			self.toggle_panel_visibility(focused_position)


		log('actionID = ' + str(actionID))



	def toggle_panel_visibility(self, focused_position):
		''' Takes the focussed position in the Heading List Control and sets only the required panel to visible. '''

		# get the required panel		
		target_panel = self.HCL.getListItem(focused_position).getProperty('panel_id')

		for panel_id in panel_controls:

			self.getControl(panel_id).setVisible(True if target_panel == panel_id else False)




	def edit_ip_address(self, controlID):

		relevant_label_control 	= self.getControl(90000 + controlID)
		current_label 			= relevant_label_control.getLabel()

		if current_label == '_ . _ . _ . _':
			current_label = ''

		user_input = xbmc.Keyboard(current_label, lang(32004))

		user_input.doModal()

		if not user_input.isConfirmed():

			return

		else:
			text = user_input.getText()

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


		icons:
				bar1_enc.png
				bar2_enc.png
				bar3_enc.png
				bar4_enc.png

				bar1_opn.png
				bar2_opn.png
				bar3_opn.png
				bar4_opn.png
		'''

		self.wifis = []

		for ssid, info in wifi_dict.iteritems():

			# hidden networks are ignored

			if ssid:

				itm = xbmcgui.ListItem(ssid)

				st = info['strength']

				if info['encryption'] == True:

					if st < 25:
						itm.setIconImage('bar1_enc.png')
					elif st < 50:
						itm.setIconImage('bar2_enc.png')
					elif st < 75:
						itm.setIconImage('bar3_enc.png')
					elif st <= 100:
						itm.setIconImage('bar4_enc.png')
					else:
						continue

				else:

					if st < 25:
						itm.setIconImage('bar1_opn.png')
					elif st < 50:
						itm.setIconImage('bar2_opn.png')
					elif st < 75:
						itm.setIconImage('bar3_opn.png')
					elif st <= 100:
						itm.setIconImage('bar4_opn.png')
					else:
						continue

				itm.setProperty('strength', st)

				self.wifis.append(itm)

		# sort the list of wifis based on signal strength
		self.wifis.sort(key=lambda x: x.getProperty('strength'), ascending=False)

		# remove everything from the existing panel
		self.WFP.reset()

		# add the new items to the panel
		self.WFP.addItems(self.wifis)

