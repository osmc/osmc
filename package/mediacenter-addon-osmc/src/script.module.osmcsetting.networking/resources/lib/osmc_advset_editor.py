
import os
import re
import subprocess
import sys

import xbmc
import xbmcaddon

__addon__ = xbmcaddon.Addon('script.module.osmcsetting.networking')

# Custom modules
sys.path.append(xbmc.translatePath(os.path.join(__addon__.getAddonInfo('path'), 'resources', 'lib')))

import xmltodict as xmltodict


class AdvancedSettingsEditor(object):


	def __init__(self, logging_function=None):

		if logging_function is None:
			self.log = self.null_log

		else:
			self.log = logging_function


	def null_log(self):

		pass


	def parse_advanced_settings(self):
		''' Parses the advancedsettings.xml file. Returns a dict with ALL the details. '''

		user_data = xbmc.translatePath( 'special://userdata')
		loc       = os.path.join(user_data,'advancedsettings.xml')

		null_doc  = {'advancedsettings': {}}

		self.log('advancedsettings file exists = %s' % os.path.isfile(loc))

		if not os.path.isfile(loc): return null_doc

		try:
			with open(loc, 'r') as f:
				lines = f.readlines()
			
			if not lines:
				self.log('advancedsettings.xml file is empty')
				raise

			with open(loc, 'r') as f:
				doc = xmltodict.parse(f)

			# ensure empty advancedsettings nodes are ignored
			if not doc.get('advancedsettings', None):
				self.log('advancedsettings node in advancedsettings.xml file is empty')
				raise

			else:
				return doc

		except:
			self.log('error occured reading advancedsettings.xml file')

			return null_doc



	def server_not_localhost(self, dictionary):
		''' Checks the MySQL settings to ensure neither server is on the localhost '''

		dbs = [ dictionary.get('advancedsettings',{}).get('musicdatabase',{}), 
				dictionary.get('advancedsettings',{}).get('videodatabase',{})]

		pattern = re.compile(r'(127.\d+.\d+.\d+|localhost|::1)')

		# local_indicators = ['127.0.0.1', '127.0.1.1','localhost', '::1']

		for db in dbs:
			host = db.get('host', None)
			if host:
				if not pattern.match(host):
					return True

		return False


	def validate_advset_dict(self, dictionary, reject_empty=False, exclude_name=False):
		''' Checks whether the provided dictionary is fully populated with MySQL settings info.
			If reject_empty is False, then Blank dictionaries are rejected, but dictionaries with no video or music database dicts are passed.
			If reject_empty is True,  then Blank dictionaries are rejected, AND dictionaries with no video or music database dicts are also rejected.
			exclude_name means that the name sql item can be ignored (it is not strictly required, but the GUI ALWAYS adds it.'''

		main = dictionary.get('advancedsettings', {})

		if not main:

			return False, 'empty'

		if exclude_name:
			sql_subitems = ['host', 'port', 'user', 'pass']	
		else:
			sql_subitems = ['name', 'host', 'port', 'user', 'pass']

		if 'videodatabase' in main:
			# fail if the items aren't filled in or are the default up value
			for item in sql_subitems:
				subitem = main.get('videodatabase',{}).get(item, False)
				if not subitem or subitem == '___ : ___ : ___ : ___':
					return False, 'missing mysql'

		if 'musicdatabase' in main:
			for item in sql_subitems:
				subitem = main.get('musicdatabase',{}).get(item, False)
				if not subitem or subitem == '___ : ___ : ___ : ___':
					return False, 'missing mysql'

		if reject_empty:
			if not any(['musicdatabase' in main, 'videodatabase' in main]):
				return False, 'empty db fields'

		return True, 'complete'


	def write_advancedsettings(self, loc, dictionary):
		''' Writes the supplied dictionary back to the advancedsettings.xml file '''

		if not dictionary.get('advancedsettings', None):

			self.log('Empty dictionary passed to advancedsettings file writer. Preventing write, backing up and removing file.')

			subprocess.call(['sudo', 'cp', loc, loc.replace('advancedsettings.xml', 'advancedsettings_backup.xml')])

			subprocess.call(['sudo', 'rm', '-f', loc])

			return

		with open(loc, 'w') as f:
			xmltodict.unparse(  input_dict = dictionary, 
								output = f, 
								pretty = True)
