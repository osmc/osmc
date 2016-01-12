# Standard Modules
from datetime import datetime
import json
import os
import requests
import shlex
import subprocess
import sys
import traceback

# Kodi Modules
import xbmc
import xbmcaddon
import xbmcgui

__addon__              	= xbmcaddon.Addon('script.module.osmcsetting.updates')
__addonid__            	= __addon__.getAddonInfo('id')
__scriptPath__         	= __addon__.getAddonInfo('path')
__setting__            	= __addon__.getSetting

DIALOG = xbmcgui.Dialog()


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


def log(message, label = ''):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )

	try:
		label = str(label)
	except UnicodeEncodeError:
		label = label.encode('utf-8', 'ignore' )

	logmsg       = '%s : %s - %s ' % (__addonid__ , str(label), str(message))
	xbmc.log(msg = logmsg, level=xbmc.LOGWARNING)


class HotFix(object):

	def __init__(self):

		self.tmp_hfo_location = '/var/tmp/uploadHotFixOutput.txt'

		hf_key = self.user_enters_key()

		hf_cypher = self.convert_key_to_cypher(hf_key)

		hf_rawtext = self.retrieve_hotfix(hf_cypher)

		hf_parsed = self.parse_hotfix(hf_rawtext)

		user_confirmation = self.confirm_instruction()

		self.display_description(hf_parsed['description'])

		self.display_instruction(hf_parsed['instruction'])

		if not user_confirmation: return

		results = self.apply_instruction(hf_parsed['instruction'])

		self.save_temp_hotfix_output(results)

		self.resolution_dispatcher(results, hf_parsed['resolution'])


	def user_enters_key(self):

		''' Provides a dialog through which the user enters a 5-digit key code '''

		# DEFAULT VALUE ONLY USED FOR TESTING
		hf_key = DIALOG.input(lang(32115), 'lanevudaqu', type=xbmcgui.INPUT_ALPHANUM)
		# hf_key = DIALOG.input('Enter HotFix ID', type=xbmcgui.INPUT_ALPHANUM)

		log(label='User entered hotfix ID', message=hf_key)

		return hf_key


	def convert_key_to_cypher(self, hf_key):

		''' Takes a 5-digit key code and converts it to the cypher used in retrieving the HotFix from paste.osmc.io '''

		# YET TO BE IMPLEMENTED

		hf_cypher = ''

		return hf_key


	def retrieve_hotfix(self, hf_cypher):

		''' Takes a five-digit hotfix key and retrieve the hotfix from paste.osmc.io. 
			Returns the raw contents at that location as a string. '''

		URL = 'http://paste.osmc.io/raw/%s' % hf_cypher

		log(label='Retrieving hotfix from', message=URL)

		raw_result = requests.get(URL)

		hf_rawtext = raw_result.text

		log(label='HotFix Result', message=hf_rawtext)

		return hf_rawtext


	def parse_hotfix(self, hf_result):

		''' Takes the raw string from the paste.osmc.io location and parses it.
			Returns a dictionary with the DESCRIPTION of the hotfix, the INSTRUCTION (what is run on the command line)
			and the RESOLUTION as a list (the actions to take after the INSTRUCTION is run).

			The structure of the file is as follows:

				DESCRIPTION (optional): 
					- one line string only
					- must start with "DESCRIPTION:", otherwise treated as part of the INSTRUCTION

				INSTRUCTION: 
					- can be multiple lines
					- multiple lines will be run consecutively only if the exit code is 0
					- can start with "INSTRUCTION:" but not required

				RESOLUTION (optional): 
					- one line string with resolution actions
					- can be comma, space, bar, colon, or semi-colon delimited
					- must be the last line in the result
					- and must start with "RESOLUTION:", otherwise it will be treated as part of the INSTRUCTION SET
					- UPLOAD: upload the results of the INSTRUCTION to paste.osmc.io and provide the user with the link
					- SAVE: to save the results to a specific file
					- LOG: write the results to the kodi log 
					- LOG is a mandatory RESOLUTION and is done everytime by default

			 '''

		description = 'No description available'
		instruction = []
		resolution	= []

		delimiters = [' ', ',','.','|',':',':']

		result_list = hf_result.split('\n')

		for i, line in enumerate(result_list):

			if i == 0:
				if line.startswith('DESCRIPTION:'):
					description = line.replace('DESCRIPTION:','').strip()
					continue

			if i == len(result_list) - 1:
				if line.startswith('RESOLUTION:'):
					desc = line.replace('RESOLUTION:','').strip()
					for d in delimiters:
						if d in desc:
							resolution = desc.split(d)
							break
					continue

			instruction.append(line.replace('INSTRUCTION:','').strip())

		log(label='Description', message=description)
		log(label='Instruction', message=instruction)
		log(label='Resolution',  message=resolution)

		hf_parsed = {'description': description, 'instruction': instruction, 'resolution': resolution}

		return hf_parsed


	def display_description(self, description):

		''' Displays a description of the hotfix on-screen for the user '''

		# YET TO BE IMPLEMENTED

		pass


	def display_instruction(self, instruction):

		''' Displays the specific instructions on-screen for the user '''

		# YET TO BE IMPLEMENTED

		pass


	def confirm_instruction(self):

		''' Asks the user to confirm that they wish to apply the instruction.
			Returns TRUE, only if user clicks Yes. '''

		user_confirmation = DIALOG.yesno(lang(32116), lang(32117), lang(32118), lang(32119))

		return user_confirmation


	def apply_instruction(self, instruction):

		''' Applies the instruction via the command line.
			Returns the resulting output in a list of lines.'''

		dangerous = ['rm -rf /', ]

		results = []

		for line in instruction:

			try:

				instruct = shlex.split(line)
				results.append('>>>>> INSTRUCTION >>>>> %s\n' % ' '.join(instruct))
				
				try:
					results.append(subprocess.check_output(instruct))

				except subprocess.CalledProcessError as e:
					# raise RuntimeError("command '{}' return with error (code {}): {}".format(e.cmd, e.returncode, e.output))
					log(label='Non-zero exit code from line', message=e.output)
					results.append(e.output)
					break

			except Exception as e:

				results.append('Error: %s\n%s' % (e.message, traceback.format_exc()))

				break

		return results


	def resolution_dispatcher(self, results, resolutions=[]):

		''' Applies the stated resolutions in the file.
			If LOG is not in resolutions, then add it in.'''

		resolution_map = {

						'UPLOAD' 	: self.resolution_upload,
						'SAVE'		: self.resolution_save,
						'LOG'		: self.resolution_log,

						}

		if 'LOG' not in resolutions: resolutions.append('LOG')

		for resolution in resolutions:

			func = resolution_map.get(resolution, self.missing_resolution)

			func(results)


	def missing_resolution(self, results):

		''' Method that is run when the dispatcher recieves a resolution it doesnt understand '''

		log('Unknown request received by dispatcher.')


	def resolution_upload(self, results):

		''' Uploads the results stored in the temporary file to paste.osmc.io and provide the user with the URL '''

		with os.popen('curl -X POST -s -T "%s" http://paste.osmc.io/documents' % self.tmp_hfo_location) as f:

			line = f.readline()
			
			key = line.replace('{"key":"','').replace('"}','').replace('\n','')

			log('pastio line: %s' % repr(line))

		if not key:

			log("OSMC HotFix upload failed.")

			save = DIALOG.yesno(lang(32120), lang(32121), lang(32122))

			if save:

				self.resolution_save(results=None)

		else:

			url = 'http://paste.osmc.io/ %s' % key

			log(label="HotFix output uploaded to", message=url.replace(' ',''))

			ok = DIALOG.ok(lang(32120), lang(32123), "URL: %s" % url)


	def save_temp_hotfix_output(self, results):

		''' Saves the hotfix output to a temporary file '''

		with open(self.tmp_hfo_location, 'w') as f:

			f.writelines(results)


	def resolution_save(self, results):

		''' Save the results to a file at the SD card '''

		log('Copying HotFix output to /boot/')
			
		os.popen('sudo cp -rf %s /boot/' % self.tmp_hfo_location)


	def resolution_log(self, results):

		''' Write the results to the kodi log file. Debug is not required.'''

		for line in results:
			log(label='HotFix Output', message=line)


if __name__ == "__main__":

	hotfix = HotFix()

