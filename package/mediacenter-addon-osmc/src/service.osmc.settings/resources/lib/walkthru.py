# XBMC modules
import xbmc
import xbmcgui
import xbmcaddon

# STANDARD library modules
import ast
import datetime
import json
import os
import pickle
import Queue
import select
import socket
import threading
import time
import sys
# sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources','lib')))

def log(message):
	xbmc.log(str(message))

class walkthru_gui(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName):

		pass

	def onInit(self):

		self.hdg = self.getControl(110)
		self.hdg.setLabel('Exit')
		self.hdg.setVisible(True)

		self.cicles = self.getControl(900)

		for x in range(5):

			self.tmp = xbmcgui.ListItem(label=str(x), label2='', thumbnailImage='')
			self.cicles.addItem(self.tmp)

	def onClick(self, controlID):

		if controlID == 110:
			self.close()


def open():

	__addon__        = xbmcaddon.Addon()
	scriptPath       = __addon__.getAddonInfo('path')
	xmlfile = 'walkthru.xml'

	GUI = walkthru_gui(xmlfile, scriptPath, 'Default')

	GUI.doModal()

	log('Exiting GUI')

	del GUI

