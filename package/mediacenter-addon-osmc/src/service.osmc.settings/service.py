#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
 Copyright (C) 2014 KodeKarnage

 This Program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This Program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with XBMC; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 http://www.gnu.org/copyleft/gpl.html
'''

# Standard modules
import os
import shutil
import time
import re
import sys
import json
import Queue
import os
import threading
import datetime
import traceback

# XBMC modules
import xbmc
import xbmcaddon
import xbmcgui

# Custom modules
sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources','lib')))
import walkthru
import settings
import comms
import ubiquifonts


__addon__        = xbmcaddon.Addon()
__addonid__      = __addon__.getAddonInfo('id')
__setting__      = __addon__.getSetting
DIALOG           = xbmcgui.Dialog()


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


def log(message):
	xbmc.log('OSMC ADDON MAIN ' + str(message), level=xbmc.LOGDEBUG)

class Main(object):


	def __init__(self):

		log('main addon starting')

		# create the no_update file to block skin shortcuts from reload the skin
		if not os.path.isfile('/walkthrough_completed'):
			with open('/tmp/NO_UPDATE', 'w+') as f:
				pass

		# queue for communication with the comm and Main
		self.parent_queue = Queue.Queue()

		# create socket, listen for comms
		self.listener = comms.communicator(self.parent_queue, socket_file='/var/tmp/osmc.settings.sockfile')
		self.listener.start()

		# the gui is created and stored in memory for quick access
		# after a few hours, the gui should be removed from memory
		self.create_gui()
		self.gui_last_accessed = datetime.datetime.now()
		self.skip_check = True

		# monitor created to check for xbmc abort requests
		self.monitor = xbmc.Monitor()

		# current skin directory, used to detect when the user has changed skins and prompts a reconstruction of the gui
		self.skindir = xbmc.getSkinDir()

		# run the ubiquifonts script to import the needed fonts into the Font.xml
		response = ubiquifonts.import_osmc_fonts()

		if response == 'reload_please':

			log('Skin reload requested')

			xbmc.executebuiltin('ReloadSkin()')
			
		# daemon
		self._daemon()

		log('_daemon exited')


	def create_gui(self):

		self.stored_gui = settings.OSMCGui(queue=self.parent_queue)


	def _daemon(self):

		log('daemon started')

		if not os.path.isfile('/walkthrough_completed'):

			for module in self.stored_gui.live_modules:
				if 'id' in module:
					if module['id'] == "script.module.osmcsetting.networking":
						networking_instance = module['SET']

						walkthru.open_gui(networking_instance)

						with open('/tmp/walkthrough_completed', 'w+') as f:
							pass
						subprocess.call(['sudo', 'mv', '/tmp/walkthrough_completed', '/walkthrough_completed'])
						
						break
			
			else:
				log('Networking module not found')

		while True:

			# Check the current skin directory, if it is different to the previous one, then 
			# recreate the gui. This is required because reference in the gui left in memory
			# do not survive a refresh of the skins textures (???)
			if self.skindir != xbmc.getSkinDir():

				log('Old Skin: %s' % self.skindir)

				self.skindir = xbmc.getSkinDir()

				log('New Skin: %s' % self.skindir)

				try:
					resp = ubiquifonts.import_osmc_fonts()

					log('Ubiquifonts result: %s' % resp)

					if resp == 'reload_please':

						while True:

							xbmc.sleep(1000)

							xml = xbmc.getInfoLabel('Window.Property(xmlfile)')

							if xml not in ['DialogYesNo.xml', 'Dialogyesno.xml', 'DialogYesno.xml', 'DialogyesNo.xml', 'dialogyesno.xml']: 

								log('Skin reload requested')
								
								xbmc.executebuiltin('ReloadSkin()')
								
								break
				
				except Exception as e:

					log(traceback.format_exc())

				try:
					log('skin changed, reloading gui')
					del self.stored_gui
				except:
					pass

				self.create_gui()
			
			# if xbmc is aborting
			if self.monitor.waitForAbort(1):
				
				self.exit()

				break

			if not self.parent_queue.empty():

				response = self.parent_queue.get()

				log('response : %s' % response)

				self.parent_queue.task_done()
		
				if response == 'open':
					try:
						del self.stored_gui  	# TESTING: this will mean that the gui is populated and loaded every time it opens
					except:
						pass
					self.open_gui()

				elif response == 'refresh_gui':

					''' This may need to be moved to a separate thread, so that it doesnt hold up the other functions. '''

					# if the gui calls for its own refresh, then delete the existing one and open a new instance

					# del self.stored_gui

					self.open_gui()

				elif response == 'exit':

					self.exit()
					break

				elif response == 'walkthru':

					for module in self.stored_gui.live_modules:
						if 'id' in module:
							log(module['id'])
							if module['id'] == "script.module.osmcsetting.networking":
								networking_instance = module['SET']

								walkthru.open_gui(networking_instance)

								break
			
					else:
						log('Networking module not found')

				elif 'new_device:' in response:

					# a usb device is attached to the hardware
					
					# get the device id					
					device_id = response[len('new_device:'):]

					# proceed only if the device_id is not null
					if device_id:

						# get ignore list
						ignore_list_raw = __setting__('ignored_devices')
						ignore_list = ignore_list_raw.split('|')

						# get sources list
						media_string = self.get_sources_list()

						# post dialogs to ask the user if they want to add the source, or ignore the device
						if device_id not in ignore_list and device_id not in media_string:

							d1 = DIALOG.yesno(lang(32002), lang(32003),lang(32004))

							if d1:

								xbmc.executebuiltin("ActivateWindow(mediasource)")

							else:

								d2 = DIALOG.yesno(lang(32002), lang(32005))

								if d2:
									ignore_list.append(str(device_id))
									ignore_string = '|'.join(ignore_list)
									__addon__.setSetting('ignored_devices', ignore_string)

			# THIS PART MAY NOT BE NEEDED, BUT IS INCLUDED HERE ANYWAY FOR TESTING PURPOSES
			# if the gui was last accessed more than four hours
			if not self.skip_check and (datetime.datetime.now() - self.gui_last_accessed).total_seconds() > 14400:

				self.skip_check = True

				del self.stored_gui

		log('_daemon exiting')


	def exit(self):
		
		# try to kill the gui and comms
		# try:		
		# 	log('Closing gui')

		# 	# self.stored_gui.close()
		# 	# del self.stored_gui
			
		# except:
		# 	log('Failed to stop/delete stored_gui. (in wait)')	
			
		try:
			log('Stopping listener (in wait)')
			self.listener.stop()
			log('Deleting listener (in wait)')
			del self.listener
			log('Listener deleted.')

		except:
			log('Failed to stop/delete listener. (in wait)')	
			

	def get_sources_list(self):

		query = {"jsonrpc": "2.0","id": 1, "method": "Files.GetSources", "params": {}}
		xbmc_request = json.dumps(query)
		result_raw = xbmc.executeJSONRPC(xbmc_request)
		result = json.loads(result_raw)
		media_dict_raw = result.get('result', {}).get('sources', {})
		media_list_raw = [v.get('file', '') for k, v in media_dict_raw.iteritems()]
		media_string = ''.join(media_list_raw)

		return media_string


	def open_gui(self):

		log('firstrun? %s' % __setting__('firstrun'))

		log('Opening OSMC settings GUI')

		try:
			# try opening the gui
			threading.Thread(target=self.stored_gui.open()).start()
			self.gui_last_accessed = datetime.datetime.now()
			self.skip_check = False

		except:
			# if that doesnt work then it is probably because the gui was too old and has been deleted
			# so recreate the gui and open it

			self.create_gui()
			self.gui_last_accessed = datetime.datetime.now()
			self.skip_check = False

			threading.Thread(target=self.stored_gui.open()).start()
		log('gui threading finished')


if __name__ == "__main__":

	m = Main()

	log('Exiting OSMC Settings')



