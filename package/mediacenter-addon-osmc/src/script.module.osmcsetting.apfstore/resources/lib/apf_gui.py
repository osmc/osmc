
# KODI modules
import xbmc
import xbmcaddon
import xbmcgui

# Standard modules
import sys
import os
import socket
import json


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

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )
		
	xbmc.log('OSMC APFStore gui : ' + str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


class apf_GUI(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, apf_dict):

		self.apf_dict = apf_dict

		self.apf_order_list = []


		self.action_dict = {}


	def onInit(self):

		self.list = self.getControl(500)
		self.list.setVisible(True)
		for x, y in self.apf_dict.iteritems():
			# self.current_icon = '/home/kubkev/.kodi/addons/script.module.osmcsetting.apfstore/resources/skins/Default/media/osmc_logo.png'

			self.list.addItem(y)
			self.apf_order_list.append(x)

		try:
			self.getControl(50).setVisible(False)
		except:
			pass


		self.check_action_dict()


	@clog(logger=log)
	def check_action_dict(self):

		install = 0
		removal = 0

		for x, y in self.action_dict.iteritems():

			if y == 'Install':

				install += 1

			elif y == 'Uninstall':

				removal += 1

		if not install and not removal:

			self.getControl(6).setVisible(False)
			self.getControl(61).setVisible(False)
			self.getControl(62).setVisible(False)

			return

		if install:

			self.getControl(61).setLabel(lang(32001) % install)
			self.getControl(6).setVisible(True)
			self.getControl(61).setVisible(True)

		else:
			
			self.getControl(61).setVisible(False)			

		if removal:

			self.getControl(62).setLabel(lang(32002) % removal)
			self.getControl(6).setVisible(True)
			self.getControl(62).setVisible(True)			

		else:
			
			self.getControl(62).setVisible(False)			



	@clog(logger=log)
	def onClick(self, controlID):

		if controlID == 500:

			container = self.getControl(500)

			sel_pos = container.getSelectedPosition()

			sel_item = self.apf_dict[self.apf_order_list[sel_pos]]

			xml = "APFAddonInfo_720OSMC.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "APFAddonInfo_OSMC.xml"

			self.addon_gui = addon_info_gui(xml, __path__, 'Default', sel_item=sel_item)

			self.addon_gui.doModal()

			ending_action = self.addon_gui.action

			if ending_action == 'Install':

				self.action_dict[sel_item.id] = 'Install' 

			elif ending_action == 'Uninstall':
				
				self.action_dict[sel_item.id] = 'Uninstall' 

			elif sel_item.id in self.action_dict:
					
				del self.action_dict[sel_item.id]

			self.check_action_dict()

			del self.addon_gui

			log(self.action_dict)

		elif controlID == 7:

			self.close()

		elif controlID == 6:

			# send install and removal list to Update Service

			action_list = ['install_' + k if v == 'Install' else 'removal_' + k for k, v in self.action_dict.iteritems()]

			action_string = '|=|'.join(action_list)

			self.contact_update_service(action_string)

			self.close()


	@clog(logger=log)
	def contact_update_service(self, action_string):

		address = '/var/tmp/osmc.settings.update.sockfile'

		message = ('action_list', {'action': action_string})

		message = json.dumps(message)

		sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
		sock.connect(address)

		sock.sendall(message) 
		sock.close()


class addon_info_gui(xbmcgui.WindowXMLDialog):

	'''
	Controls
	==============================
	50001	Shortdesc
	50002	Longdesc
	50003	Version
	50004	Maintainer
	50005	LastUpdated
	50006	Icon
	50007	Name
	'''

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, sel_item):

		self.action = False

		self.sel_item = sel_item


	def onInit(self):

		self.getControl(50001).setLabel(self.sel_item.shortdesc)
		self.getControl(50002).setText(self.sel_item.longdesc)
		self.getControl(50003).setLabel(self.sel_item.version)
		self.getControl(50004).setLabel(self.sel_item.maintainedby)
		self.getControl(50005).setLabel(self.sel_item.lastupdated)
		self.getControl(50006).setImage(self.sel_item.current_icon, True)
		self.getControl(50007).setLabel(self.sel_item.name)

		if self.sel_item.installed:

			self.getControl(6).setLabel(lang(32004))

		else:

			self.getControl(6).setLabel(lang(32003))
			


	def onClick(self, controlID):

		if controlID == 6:

			lbl = self.getControl(6).getLabel()

			if lbl == lang(32003):
				self.action = 'Install'
			else:
				self.action = 'Uninstall'

			self.close()

		elif controlID == 7:

			self.close()


