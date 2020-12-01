# XBMC Modules
import xbmc
import xbmcaddon
import xbmcgui
import subprocess
import sys
import time
import os

ACTION_PREVIOUS_MENU = 10
ACTION_NAV_BACK      = 92
SAVE                 = 5
HEADING              = 1
ACTION_SELECT_ITEM   = 7

__addon__   = xbmcaddon.Addon("script.module.osmcsetting.pi")
scriptPath  = __addon__.getAddonInfo('path')
DIALOG      = xbmcgui.Dialog()
IMAGE 		= os.path.join(scriptPath,'resources','osmc','FO_Icon.png')


def lang(id):
    san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
    return san 


def log(message):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )
		
	xbmc.log('OSMC PI Config Editor' + str(message), level=xbmc.LOGDEBUG)


class ConfigEditor(xbmcgui.WindowXMLDialog):


	def onInit(self):

		# give the settings enough time to be saved to the config.txt
		xbmc.sleep(150)

		# list of settings that are ignored in the duplicate check
		self.ignore_list = ['dtoverlay', 'device_tree', 'device_tree_param', 'device_tree_overlay', 'dtparam']

		self.del_string = ' [' + lang(32056) + ']'

		try:
			self.config = '/boot/config.txt'

			with open(self.config, 'r') as f:
				self.lines = f.readlines()			

		except:

			# FOR TESTING
			self.config = '/home/plaskev/Documents/config.txt'

			with open(self.config, 'r') as f:
				self.lines = f.readlines()

		log('lines = %s' % self.lines)

		self.lines = [line.replace('\n','') for line in self.lines if line not in ['\n', '', '\t']]
		
		# Save button
		self.ok = self.getControl(SAVE)
		self.ok.setLabel(lang(32050))

		# Heading
		self.hdg = self.getControl(HEADING)
		self.hdg.setLabel(lang(32051))
		self.hdg.setVisible(True)

		# Hide unused list frame
		self.x = self.getControl(3)
		self.x.setVisible(False)

		# Populate the list frame
		self.list_control      = self.getControl(6)
		self.list_control.setEnabled(True)

		self.items = [lang(32052)]
		self.items.extend(self.lines)

		self.item_count = len(self.items)

		# Start the window with the first item highlighted
		# self.list_control.getListItem(0).select(True)

		# Set action when clicking right from the Save button
		self.ok.controlRight(self.list_control)

		for i in self.items:
			# populate the random list
			self.tmp = xbmcgui.ListItem(i)#, thumbnailImage=IMAGE)
			self.list_control.addItem(self.tmp)

		self.changed = False

		self.setFocus(self.list_control)

		# check for duplications, warn the user if there are duplicates
		dup_check = [y for y in [x.split('=')[0] for x in self.grab_item_strings() if '=' in x] if y not in self.ignore_list]
		if len(dup_check) != len(set(dup_check)):
			ok = DIALOG.ok(lang(32051), lang(32065), lang(32066))


	def onAction(self, action):

		actionID = action.getId()
		if (actionID in (ACTION_PREVIOUS_MENU, ACTION_NAV_BACK)):
			log('CLOSE')
			self.close()


	def onClick(self, controlID):
		log('%s' % controlID)

		if controlID == SAVE:
			log('SAVE')

			if self.changed:

				final_action = DIALOG.yesno(lang(32052), lang(32053), nolabel=lang(32054), yeslabel=lang(32055))

				if final_action:

					log('final action')

					new_config = self.grab_item_strings()

					# temporary location for the config.txt
					tmp_loc = '/var/tmp/config.txt'

					# write the long_string_file to the config.txt
					with open(tmp_loc,'w') as f:
						for line in new_config:
							f.write(line.replace(" = ","=") + '\n')
							log('' + line)

					# backup existing config
					suffix = '_' + str(time.time()).split('.')[0]
					subprocess.call(["sudo", "cp", self.config, '/home/pi/' ])
					subprocess.call(["sudo", "mv", '/home/pi/config.txt', '/home/pi/config' + suffix + '.txt' ])

					# copy over the temp config.txt to /boot/ as superuser
					subprocess.call(["sudo", "mv", tmp_loc, self.config ])

					# THIS IS JUST FOR TESTING, LAPTOP DOESNT LIKE SUDO HERE
					try:
						subprocess.call(["mv", tmp_loc, self.config ])
					except:
						pass

					log('writing ended')

			self.close()

		else:
			selected_entry = self.list_control.getSelectedPosition()
			item = self.list_control.getSelectedItem()
			currentlabel = item.getLabel()
			
			if selected_entry != 0:

				if self.del_string not in currentlabel:
					action = DIALOG.yesno(lang(32051), lang(32057), nolabel=lang(32058), yeslabel=lang(32059))

					if action:
						# delete
						item.setLabel(currentlabel + self.del_string)
						self.changed = True

					else:
						# edit
						d = DIALOG.input(lang(32060), currentlabel, type=xbmcgui.INPUT_ALPHANUM)

						if d:

							self.check_for_duplicates(d, True)

							item.setLabel(d)
							self.changed = True

				else:
					action = DIALOG.yesno(lang(32051), lang(32061), nolabel=lang(32058), yeslabel=lang(32062))

					if action:
						# delete
						item.setLabel(currentlabel[:len(currentlabel) - len(self.del_string)])
						self.changed = True

					else:
						# edit
						d = DIALOG.input(lang(32063), currentlabel, type=xbmcgui.INPUT_ALPHANUM)

						if d:
							self.check_for_duplicates(d, edit=True)

							item.setLabel(d)
							self.changed = True

			else:
				d = DIALOG.input(lang(32064), type=xbmcgui.INPUT_ALPHANUM)

				if d:

					self.check_for_duplicates(d)

					# add the new item to the list
					tmp = xbmcgui.ListItem(d)#, thumbnailImage=IMAGE)
					self.list_control.addItem(tmp)
					
					self.changed = True

					self.item_count += 1


	def check_for_duplicates(self, d, edit=False):
		
		
		if '=' in d:
			dupe_check_raw = self.grab_item_strings()
			dupe_check = [x.split('=')[0] for x in dupe_check_raw]

			dupe = d.split('=')[0]


			if dupe not in self.ignore_list and ((edit and dupe_check.count(dupe) > 1) or (not edit and dupe in dupe_check)):

				ok = DIALOG.ok(lang(32051), lang(32067), lang(32066))


	def grab_item_strings(self):

		new_config = []

		for i in range(self.item_count):
			if i == 0: continue

			item = self.list_control.getListItem(i)

			currentlabel = item.getLabel()

			if self.del_string not in currentlabel:
				new_config.append(currentlabel)

		return new_config


if __name__ == "__main__":

	log('OPEN')
	CE = ConfigEditor("DialogSelect.xml", scriptPath, 'Default')
	
	CE.doModal()
	
	del CE

	log('CLOSED')
	
	xbmc.sleep(150)

	# __addon__.openSettings()

