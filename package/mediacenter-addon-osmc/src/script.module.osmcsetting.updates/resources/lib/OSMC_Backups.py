
# STANDARD Modules
import datetime as dt
import glob
import json
import math
import os
import re
import tarfile
import tempfile
import traceback

# KODI Modules
import xbmc
import xbmcgui
import xbmcaddon
import xbmcvfs

__addonid__	= 'OSMC Backup'
DIALOG = xbmcgui.Dialog()

TIME_PATTERN = '%Y_%m_%d_%H_%M_%S'
READ_PATTERN = '%Y-%m-%d %H:%M:%S'
APPENDAGE	 = '[0-9|_]*'
FILE_PATTERN = 'OSMCBACKUP_%s.tar.gz'
LOCATIONS = {

			'backup_addons'				:	'{kodi_folder}addons/',
			'backup_addon_data'			:	'{kodi_folder}userdata/addon_data/',
			'backup_Database'			:	'{kodi_folder}userdata/Database/',
			'backup_keymaps'			:	'{kodi_folder}userdata/keymaps/',
			'backup_library'			:	'{kodi_folder}userdata/library/',
			'backup_playlists'			:	'{kodi_folder}userdata/playlists/',
			'backup_Thumbnails'			:	'{kodi_folder}userdata/Thumbnails/',
			'backup_favourites'			:	'{kodi_folder}userdata/favourites.xml',
			'backup_keyboard'			:	'{kodi_folder}userdata/keyboard.xml',
			'backup_remote'				:	'{kodi_folder}userdata/remote.xml',
			'backup_LCD'				:	'{kodi_folder}userdata/LCD.xml',
			'backup_profiles'			:	'{kodi_folder}userdata/profiles.xml',
			'backup_RssFeeds'			:	'{kodi_folder}userdata/RssFeeds.xml',
			'backup_sources'			:	'{kodi_folder}userdata/sources.xml',
			'backup_upnpserver'			:	'{kodi_folder}userdata/upnpserver.xml',
			'backup_peripheral_data'	:	'{kodi_folder}userdata/peripheral_data.xml',
			'backup_guisettings'		:	'{kodi_folder}userdata/guisettings.xml',
			'backup_advancedsettings'	:	'{kodi_folder}userdata/advancedsettings.xml',

			}

LABELS = 	{

			'{kodi_folder}/addons'							: 'Directory - Addons',
			'{kodi_folder}/userdata/addon_data'				: 'Directory - Addon Data',
			'{kodi_folder}/userdata/Database'				: 'Directory - Database',
			'{kodi_folder}/userdata/keymaps'				: 'Directory - Keymaps',
			'{kodi_folder}/userdata/library'				: 'Directory - Library',
			'{kodi_folder}/userdata/playlists'				: 'Directory - Playlists',
			'{kodi_folder}/userdata/Thumbnails'				: 'Directory - Thumbnails',
			'{kodi_folder}/userdata/advancedsettings.xml'	: 'File - advancedsettings.xml',
			'{kodi_folder}/userdata/guisettings.xml'		: 'File - guisettings.xml',
			'{kodi_folder}/userdata/sources.xml'			: 'File - sources.xml',
			'{kodi_folder}/userdata/profiles.xml'			: 'File - File - profiles.xml',
			'{kodi_folder}/userdata/favourites.xml' 		: 'File - favourites.xml',
			'{kodi_folder}/userdata/keyboard.xml'			: 'File - keyboard.xml',
			'{kodi_folder}/userdata/remote.xml'				: 'File - remote.xml',
			'{kodi_folder}/userdata/LCD.xml'				: 'File - LCD.xml',
			'{kodi_folder}/userdata/RssFeeds.xml'			: 'File - RssFeeds.xml',
			'{kodi_folder}/userdata/upnpserver.xml'			: 'File - upnpserver.xml',
			'{kodi_folder}/userdata/peripheral_data.xml'	: 'File - peripheral_data.xml',

			}


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

	logmsg       = '%s : %s - %s ' % ('OSMC BACKUP: ' , str(label), str(message))
	xbmc.log(msg = logmsg, level=xbmc.LOGDEBUG)


class osmc_backup(object):

	def __init__(self, settings_dict, progress_function, parent_queue=None):

		log('osmc_backup INIT')

		self.s = settings_dict

		self.parent_queue = parent_queue

		self.progress = progress_function

		self.restoring_guisettings = False

		# backup candidates is a list of tuples that contain the folder/file path and the size in bytes of the entry
		self.backup_candidates = self.create_backup_file_list() #if self.s.get('create_tarball', False) else None


	def start_backup(self):

		''' This is the main method that walks through the backup process '''

		if self.check_backup_location():

			# if self.s['export_library']:

			# 	self.export_libraries()

			# if self.s['create_tarball']:

			self.create_tarball()

			msg = json.dumps(('pre_backup_complete', {}))

			try:

				self.parent_queue.put(msg)

			except:

				pass


	def check_backup_location(self):

		''' Tests the backup location for disk space and writeability '''

		self.location = self.s.get('backup_location', None)

		if not self.location:

			log('Location for backup not provided.')

			ok = DIALOG.ok('OSMC Backup', 'Location for backup not provided.', 'Set the backup folder in MyOSMC.')

			return False

		# check for available disk space at backup location
		if not self.check_target_location_for_size(self.location):

			log('Insufficent diskspace at target location')

			ok = DIALOG.ok('OSMC Backup', 'Insufficent diskspace at target location')

			return False

		else:
			log('Sufficent diskspace at target location')

		# check for write permission at backup location
		if not self.check_target_writeable(self.location):

			log('Backup location not writeable.')

			ok = DIALOG.ok('OSMC Backup', 'Backup location not writeable')

			return False

		else:
			log('Backup location writeable.')

		return True


	def kodi_location(self):

		''' returns the location of the kodi folder '''

		return xbmc.translatePath('special://home')


	def create_backup_file_list(self):

		''' creates a list of the items to back-up '''

		kodi_folder = self.kodi_location()

		backup_candidates = []

		for setting, location in LOCATIONS.iteritems():

			if self.s[setting]:

				# if the user is backing up guisettings.xml, then call for the settings to be saved using
				# custom method
				if setting == 'backup_guisettings':

					log('Trying to backup guisettings, running xbmc.saveSettings')
					try:
						xbmc.saveSettings()
					except Exception as e:
						log('xbmc.saveSetting failed')
						log(type(e).__name__)
						log(e.args)
						log(traceback.format_exc())

				path = location.format(kodi_folder=kodi_folder)

				size = self.calculate_byte_size(path)

				backup_candidates.append((path, size))

		return backup_candidates


	def check_target_location_for_size(self, location):

		''' Checks the target location to see if there is sufficient space for the tarball.
			Returns True if there is sufficient disk space '''

		# the backup file gets created locally first, and then gets transfered, so we have to make sure both location
		# have sufficient free space.
		# linux cannot query freespace from remote locations, so we just have to copy and hope

		# check locally
		try:
			st = os.statvfs(xbmc.translatePath('special://temp'))

			requirement = self.estimate_disk_requirement()
			if st.f_frsize:
				available = st.f_frsize * st.f_bavail
			else:
				available = st.f_bsize * st.f_bavail
			# available	= st.f_bfree/float(st.f_blocks) * 100 * st.f_bsize

			log('local required disk space: %s' % requirement)
			log('local available disk space: %s' % available)

			if not requirement < available:
				return False
		except:
			pass

		# check remote
		try:
			st = os.statvfs(location)

			requirement = self.estimate_disk_requirement()
			if st.f_frsize:
				available = st.f_frsize * st.f_bavail
			else:
				available = st.f_bsize * st.f_bavail
			# available	= st.f_bfree/float(st.f_blocks) * 100 * st.f_bsize

			log('remote required disk space: %s' % requirement)
			log('remote available disk space: %s' % available)

			return requirement < available
		except:
			return True


	def check_target_writeable(self, location):

		''' tests backup location for writeability, returns True is writeable '''

		temp_file = os.path.join(location, 'temp_write_test')

		f = xbmcvfs.File(temp_file, 'w')

		try:
			result = f.write('buffer')
		except:
			log('%s is not writeable' % location)
			f.close()
			return False

		f.close()

		try:
			xbmcvfs.delete(temp_file)
		except:
			log('Cannot delete temp file at %s' % location)

		return True


	def estimate_disk_requirement(self, func=None):

		sizes = [x[1] for x in self.backup_candidates]

		if func == 'log':

			sizes = [math.log(x) for x in sizes if x]

		return sum(sizes)


	def create_tarball(self):

		''' takes the file list and creates a tarball in the backup location '''

		location = self.s['backup_location']

		# get list of tarballs in backup location
		tarballs = self.list_current_tarballs(location)

		# check the users desired number of backups
		permitted_tarball_count = self.s['tarball_count']

		# determine how many extra tarballs there are
		extras = len(tarballs) - permitted_tarball_count + 1

		if extras > 0 and permitted_tarball_count != 0:
			remove_these = tarballs[:extras]
		else:
			remove_these = []

		# get the tag for the backup file
		tag = self.generate_tarball_name()

		# generate name for temporary tarball
		local_tarball_name = os.path.join(xbmc.translatePath('special://temp'), FILE_PATTERN % tag)

		# generate name for remote tarball
		remote_tarball_name = os.path.join(location, FILE_PATTERN % tag)

		# get the size of all the files that are being backed up
		total_size 		= max(1, self.estimate_disk_requirement(func='log'))
		progress_total 	= 0

		# create a progress bar
		''' Controls the creation and updating of the background prgress bar in kodi.
			The data gets sent from the apt_cache_action script via the socket
			percent, 	must be an integer
			heading,	string containing the running total of items, bytes and speed
			message, 	string containing the name of the package or the active process.
		'''
		pct = 0

		self.progress(**{'percent':  pct, 'heading':  'OSMC Backup', 'message': 'Starting tar ball backup' })

		new_root = xbmc.translatePath('special://home')

		try:

			f = xbmcvfs.File(local_tarball_name,'w')

			tar = tarfile.open(fileobj=f, mode="w:gz")

			for name, size in self.backup_candidates:

				self.progress(**{'percent':  pct, 'heading':  'OSMC Backup', 'message': '%s' % name})

				try:
					new_path = os.path.relpath(name, new_root)
					tar.add(name, arcname=new_path)
				except:
					log('%s failed to backup to tarball' % name)
					continue

				progress_total += math.log(max(size, 1))

				pct = int( (progress_total / float(total_size) ) * 100.0 )

			tar.close()

			f.close()

			# copy the local file to remote location
			self.progress(**{'percent':  100, 'heading':  'OSMC Backup', 'message': 'Transferring backup file'})

			log('local tarball name: %s'   % local_tarball_name)
			log('local tarball exists: %s' % os.path.isfile(local_tarball_name))
			log('remote tarball name: %s'  % remote_tarball_name)
			log('remote tarball exists: %s' % os.path.isfile(remote_tarball_name))

			success = xbmcvfs.copy(local_tarball_name, remote_tarball_name)

			if success:
				log('Backup file successfully transferred')

				try:
					xbmcvfs.delete(local_tarball_name)
				except:
					log('Cannot delete temp file at %s' % local_tarball_name)

			else:
				log('Transfer of backup file not successful: %s' % success)

				self.progress(kill=True)

				ok = DIALOG.ok('Backup failed', 'Backup failed to copy the tar file.')

                                try:
                                        xbmcvfs.delete(local_tarball_name)
                                except:
                                        log('Cannot delete temp file at %s' % local_tarball_name)

				return 'failed'

			# remove the unneeded backups (this will only occur if the tarball is successfully created)
			log('Removing these files: %s' % remove_these)
			for r in remove_these:
				try:
					self.progress(**{'percent':  100, 'heading':  'OSMC Backup', 'message': 'Removing old backup file: %s' % r})
					xbmcvfs.delete(os.path.join(location, r))
				except Exception as e:
					log('Deleting tarball failed: %s' % r)
					log(type(e).__name__)
					log(e.args)
					log(traceback.format_exc())

			self.progress(kill=True)

		except Exception as e:

			self.progress(kill=True)

			log('Creating tarball failed')
			log(type(e).__name__)
			log(e.args)
			log(traceback.format_exc())

			return 'failed'


	def start_restore(self):

		''' Posts a list of backup files in the location, allows the user to choose one (including browse to a different location,
			allows the user to choose what to restore, including an ALL option.
		'''

		location = self.s['backup_location']

		self.success = 'Full'

		current_tarballs = self.list_current_tarballs(location)

		# the first entry on the list dialog
		dialog_list = [(None, 'Browse for backup file')]

		# strip the boilerplate from the file and just show the name
		current_tarballs = [(name, self.strip_name(name, location)) for name in current_tarballs]

		# sort the list by date stamp (reverse)
		current_tarballs.sort(key=lambda x: x[1], reverse=True)

		# join the complete list together
		dialog_list.extend(current_tarballs)

		back_to_select = True

		while back_to_select:

			# display the list
			file_selection = DIALOG.select('Select a backup file', [x[1] for x in dialog_list])

			if file_selection == -1:
				back_to_select = False
				continue

			elif file_selection == 0:
				# open the browse dialog

				backup_file = DIALOG.browse(1, 'Browse to backup file', 'files')

				log('User selected backup file: %s' % backup_file)

				if not backup_file:
					# return to select window
					log('User has not selected backup file.')
					continue

			else:
				# read the tar_file, post dialog with the contents

				# get file_selection
				remote_file = os.path.join(location, dialog_list[file_selection][0])
				basename = os.path.basename(remote_file)

				log('User selected backup file: %s' % remote_file)

				# this requires copying the tar_file from its stored location, to kodi/temp 
				# xbmcvfs cannot read into tar files without copying the whole thing to memory
				temp_copy = os.path.join(xbmc.translatePath('special://temp'), basename)

				result = xbmcvfs.copy(remote_file, temp_copy)

				if not result:
					# copy of file failed

					log('Failed to copy file to special://temp location')

					ok = DIALOG.ok('Restore failed', 'Restore failed to copy the file.')
					back_to_select = False
					continue

				backup_file = temp_copy

			# open local copy and check for contents
			try:
				with tarfile.open(backup_file, 'r:gz') as t:
					members = t.getmembers()

					# log('tarfile members: %s' % members)

			except Exception as e:

				log('Opening and reading tarball failed')
				log(type(e).__name__)
				log(e.args)
				log(traceback.format_exc())

				ok = DIALOG.ok('Restore failed', 'Failure to read the file.')

				continue

			if members:

				# the first entry on the list dialog, tuple is (member, display name, name in tarfile, restore location)
				tar_contents_list = [(None, 'Everything')]

				# create list of items in the tar_file for the user to select from, these are prime members; either they are the
				# xml files or they are the base folder

				menu_items = []
				for k, v in LABELS.iteritems():
					for member in members:
						if k.endswith(member.name):
							menu_items.append((member, v))
							break

				menu_items.sort(key=lambda x: x[1])
				tar_contents_list.extend(menu_items)

				if len(tar_contents_list) < 2:

					log('Could not identify contents of backup file')

					ok = DIALOG.ok('Restore', 'Could not identify contents of backup file.')

					continue

				# at the moment this only allows for a single item to be selected, however we can build our own dialog that
				# can allow multiple selection, with the action only taking place on users OK
				item_selection = DIALOG.select('Select items to restore', [x[1] for x in tar_contents_list])

				if item_selection == -1:

					continue

				elif item_selection == 0:

					log('User has chosen to restore all items')
					# restore all items

					restore_items = members

				else:

					# restore single item
					restore_item = tar_contents_list[item_selection]

					# if the item is a single xml file, then restore that member, otherwise loop through the file members
					# and collect the ones in the relevant folder
					# the identification of a single file is hackish relying upon the presence of .xml, but for the moment
					# we arent backing up any other single files

					if restore_item[0].name.endswith('.xml'):
						restore_items = []
						restore_items.append(restore_item[0])

						log('User has chosen to restore a single item: %s' % restore_item[1])

					else:
						log('User is restoring a folder: %s' % restore_item[1])
						restore_items = []
						for member in members:
							if member.name.startswith(restore_item[0].name):
								restore_items.append(member)

				# confirm with user that they want to overwrite existing files OR extract to a different location
				overwrite = DIALOG.select('OSMC Restore', ['Overwrite existing files', 'Select new restore folder'])

				if overwrite == -1:
					log('User has escaped restore dialog')
					continue

				if overwrite == 0:
					# restore over existing files
					log('User has chosen to overwrite existing files.')
					restore_location = xbmc.translatePath('special://home')

					# check the restore_items for guisettings.xml, if found then restore that to the addon_data folder,
					# create the /RESET_GUISETTINGS file and write in the location of the restored guisettings.xml
					# change the restoring_guisettings flag so the user is informed that this change requires a restart,
					# or will take effect on next boot
					if any( [True for x in restore_items if x.name.endswith('userdata/guisettings.xml') ] ):
						self.restoring_guisettings = True


				elif overwrite == 1:
					# select new folder
					log('User has chosen to browse for a new restore location')
					restore_location = DIALOG.browse(3, 'Browse to restore location', 'files')

				else:
					log('User has escaped restore dialog')
					continue

				if restore_location == '':
					log('User has failed to select a restore location')
					continue


				with tarfile.open(backup_file, 'r') as t:
					for member in restore_items:

						log('attempting to restore %s to %s' % (member.name, restore_location))

						try:
							# if the item is userdata/guisettings.xml then restore it to /tmp and rename it
							# the external script will pick it up there and overwrite the live version
							if member.name.endswith('userdata/guisettings.xml') and self.restoring_guisettings:

								member.name = os.path.basename(member.name)

								t.extract(member, '/tmp/')

								os.rename('/tmp/guisettings.xml', '/tmp/guisettings.restore')

							else:

								t.extract(member, restore_location)

						except Exception as e:

							log('Extraction of %s failed' % member.name)
							log(type(e).__name__)
							log(e.args)
							log(traceback.format_exc())

							if self.success == 'Full':	self.success = []

							self.success.append(member.name)

							continue

			if self.success == 'Full' and not self.restoring_guisettings:
				ok = DIALOG.ok('OSMC Restore', 'Items successfully restored')

			elif self.success == 'Full' and self.restoring_guisettings:

				back_to_select = False

			else:
				back_to_select = False

			try:
				xbmcvfs.delete(temp_copy)
			except:
				log('Deleting temp_copy failed. Dont worry, it might not exist.')

		self.progress(kill=True)


	def strip_name(self, name, location):

		''' Returns the tarball file name with the boilerplate removed '''

		f = xbmcvfs.File(os.path.join(location, name))
		s = f.size()
		f.close()

		date_string = name.replace('.tar.gz', '').replace('OSMCBACKUP_','')

		letters = []
		count = 0
		for char in date_string:
			if char == '_':
				if count < 2:
					letters.append('-')
				elif count == 2:
					letters.append('  ')
				else:
					letters.append(':')
				count += 1
			else:
				letters.append(char)

		if s:
			return 'Backup File  -  %s  -  %.*f MB' % (''.join(letters), 1, float(s)/1024/1024)
		else:
			return 'Backup File  -  %s' % ''.join(letters)


	def generate_tarball_name(self):

		''' Returns the name for the new tarball '''

		file_tag = dt.datetime.strftime(dt.datetime.now(), TIME_PATTERN)

		return file_tag


	def list_current_tarballs(self, location):

		''' Returns a list of the tarballs in the current backup location, from youngest to oldest '''

		pattern = FILE_PATTERN % APPENDAGE

		dirs, tarball_list = xbmcvfs.listdir(location)

		log('Contents of remote location: %s' % tarball_list)

		regex = re.compile(pattern)
		tarball_list = [i for i in tarball_list if regex.search(i)]

		tarball_list.sort(key = lambda x: self.time_from_filename(x, pattern, location))

		log('tarball list from location: %s' % tarball_list)

		return tarball_list


	def time_from_filename(self, filename, pattern, location):

		''' Returns the date of the backup that is embedded in the backup filename '''

		prefix = FILE_PATTERN % APPENDAGE

		# extract just the relevant part of the string
		string = filename.replace(prefix[:prefix.index(APPENDAGE)], '').replace('.tar.gz', '')

		try:
			return dt.datetime.strptime(string, TIME_PATTERN)

		except:
			return dt.datetime(1,1,1,1)


	def calculate_byte_size(self, candidate):

		if os.path.isfile(candidate):

			return os.path.getsize(candidate)

		if os.path.isdir(candidate):
			total_size = 0
			for dirpath, dirnames, filenames in os.walk(candidate):
				for f in filenames:
					fp = os.path.join(dirpath, f)
					try:
						total_size += os.path.getsize(fp)
					except:
						pass

			return total_size

		return 0


	def count_stored_tarballs(self, location):

		''' Counts the number of tarballs that are stored. Returns the count, along with the date of the earliest ball. '''


	def export_libraries(self):

		''' calls on kodi to export the selected libraries to a single .xml file '''

		# exportlibrary(music,false,filepath)
		# The music library will be exported to a single file stored at filepath location.

		# exportlibrary(video,true,thumbs,overwrite,actorthumbs)
		# The video library is exported to multiple files with the given options.
		# Here thumbs, overwrite and actorthumbs are boolean values (true or false).




if __name__ == "__main__":

	osmc_backup()
