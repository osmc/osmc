
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

		# self.getControl(3).setLabel('Exit')
		# self.getControl(3).setEnabled(True)
		# self.getControl(5).setVisible(False)
		# self.getControl(6).setVisible(False)
		# self.getControl(7).setLabel('Permit Install')


	@clog(logger=log)
	def onClick(self, controlID):

		if controlID == 500:

			try:

				starting_action = sel_item.installed

			except:

				starting_action = False

			container = self.getControl(500)

			sel_pos = container.getSelectedPosition()

			sel_item = self.apf_dict[self.apf_order_list[sel_pos]]

			# create addon info window (prepare this so it loads faster)
			self.addon_gui = addon_info_gui("APFAddonInfo.xml", __path__, 'Default', sel_item=sel_item)
			
			self.addon_gui.doModal()

			ending_action = self.addon_gui.action

			if ending_action != starting_action:

				if ending_action == 'Install':

					self.action_dict[sel_item.id] = 'Install' 

				else:
					
					self.action_dict[sel_item.id] = 'Remove' 

			del self.addon_gui

			log(self.action_dict)



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

		self.action = ''

		self.sel_item = sel_item


	def onInit(self):

		self.getControl(50001).setLabel(self.sel_item.shortdesc)
		self.getControl(50002).setText(self.sel_item.longdesc)
		self.getControl(50003).setLabel(self.sel_item.version)
		self.getControl(50004).setLabel(self.sel_item.maintainedby)
		self.getControl(50005).setLabel(self.sel_item.lastupdated)
		self.getControl(50006).setImage(self.sel_item.current_icon, True)
		self.getControl(50007).setLabel(self.sel_item.name)


	def onClick(self, controlID):

		if controlID == 6:

			lbl = self.getControl(6).getLabel()

			if lbl == 'Install':
				self.action = 'Install'
			else:
				self.action = 'Remove'

			self.close()

		elif controlID == 7:

			self.close()


