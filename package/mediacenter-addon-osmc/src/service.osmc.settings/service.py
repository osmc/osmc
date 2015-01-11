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

# XBMC modules
import xbmc
import xbmcaddon
import xbmcgui

# Custom modules
sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources','lib')))
import walkthru
import settings
import comms


__addon__        = xbmcaddon.Addon()
__addonid__      = __addon__.getAddonInfo('id')
__setting__      = __addon__.getSetting
DIALOG           = xbmcgui.Dialog()


def log(message):
	xbmc.log(str(message))

class Main(object):


	def __init__(self):

		# queue for communication with the comm and Main
		self.parent_queue = Queue.Queue()

		# create socket, listen for comms
		self.listener = comms.communicator(self.parent_queue, socket_file='/var/tmp/osmc.settings.sockfile')
		self.listener.start()

		# the gui is created and stored in memory for quick access
		# after a few hours, the gui should be removed from memory
		self.stored_gui = settings.OSMCGui(queue=self.parent_queue)
		self.gui_last_accessed = datetime.datetime.now()
		self.skip_check = True

		# monitor created to check for xbmc abort requests
		self.monitor = xbmc.Monitor()

		# daemon
		self._daemon()

		log('_daemon exited')


	def _daemon(self):

		log('daemon started')

		while True:
			
			# if xbmc is aborting
			if self.monitor.waitForAbort(1):
				# try to kill the gui and comms
				try:
					log('Stopping listener (in wait)')
					self.listener.stop()
					log('Deleting listener (in wait)')
					del self.listener
					log('Listener deleted.')
				except:
					log('Failed to stop/delete listener. (in wait)')	
				
				try:		
					log('Closing gui')
					self.stored_gui.close()
				except:
					log('Failed to stop/delete stored_gui. (in wait)')	

				break

			if not self.parent_queue.empty():

				response = self.parent_queue.get()

				log('response : %s' % response)

				self.parent_queue.task_done()
		
				if response == 'open':

					self.open_gui()

				elif response == 'refresh_gui':

					''' This may need to be moved to a separate thread, so that it doesnt hold up the other functions. '''

					# if the gui calls for its own refresh, then delete the existing one and open a new instance

					del self.stored_gui

					self.open_gui()

				elif 'new_device:' in response:

					# a usb device is attached to the hardware
					
					# get the device id					
					device_id = response[len('new_device:'):]

					print device_id

					# proceed only if the device_id is not null
					if device_id:

						# get ignore list
						ignore_list_raw = __setting__('ignored_devices')
						ignore_list = ignore_list_raw.split('|')

						# get sources list
						query = {"jsonrpc": "2.0","id": 1, "method": "Files.GetSources", "params": {}}
						xbmc_request = json.dumps(query)
						result_raw = xbmc.executeJSONRPC(xbmc_request)
						result = json.loads(result_raw)
						media_dict_raw = result.get('result', {}).get('sources', {})
						media_list_raw = [v.get('file', '') for k, v in media_dict_raw.iteritems()]
						media_string = ''.join(media_list_raw)

						# post dialogs to ask the user if they want to add the source, or ignore the device
						if device_id not in ignore_list and device_id not in media_string:

							d1 = DIALOG.yesno('OSMC', 'A new device has been detected.','Would you like to add it as a library source?')

							if d1:

								xbmc.executebuiltin("ActivateWindow(mediasource)")

							else:

								d2 = DIALOG.yesno('OSMC', 'Would you like to ignore this device in the future?')

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


	def open_gui(self):

		log('firstrun? %s' % __setting__('firstrun'))
		
		if __setting__('firstrun') == 'fartstrue':

			log('Opening walkthru GUI')

		else:

			log('Opening OSMC settings GUI')

			try:
				# try opening the gui
				threading.Thread(target=self.stored_gui.open()).start()
				self.gui_last_accessed = datetime.datetime.now()
				self.skip_check = False

			except:
				# if that doesnt work then it is probably because the gui was too old and has been deleted
				# so recreate the gui and open it

				self.stored_gui = settings.OSMCGui(queue=self.parent_queue)
				self.gui_last_accessed = datetime.datetime.now()
				self.skip_check = False

				threading.Thread(target=self.stored_gui.open()).start()
			log('gui threading finished')

if __name__ == "__main__":

	m = Main()

	log('Deleting Main')
	# del Main

	log('Exiting OSMC Settings')



