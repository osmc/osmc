
# STANDARD Modules
import tarfile
import os
import glob
import datetime as dt
import traceback
import math

# KODI Modules
import xbmc
import xbmcgui
import xbmcaddon

__addonid__	= 'OSMC Backup'
DIALOG = xbmcgui.Dialog()

TIME_PATTERN = '%Y_%m_%d_%H_%M_%S'
APPENDAGE	 = '[0-9]*'
FILE_PATTERN = 'OSMCBACKUP_%s.tar.gz'
LOCATIONS = {

			'backup_addons'				:	'{kodi_folder}/addons/',
			'backup_addon_data'			:	'{kodi_folder}/userdata/addon_data',
			'backup_Database'			:	'{kodi_folder}/userdata/Database/',
			'backup_keymaps'			:	'{kodi_folder}/userdata/keymaps/',
			'backup_library'			:	'{kodi_folder}/userdata/library/',
			'backup_playlists'			:	'{kodi_folder}/userdata/playlists/',
			'backup_Thumbnails'			:	'{kodi_folder}/userdata/Thumbnails/',
			'backup_favourites'			:	'{kodi_folder}/userdata/favourites.xml',
			'backup_keyboard'			:	'{kodi_folder}/userdata/keyboard.xml',
			'backup_remote'				:	'{kodi_folder}/userdata/remote.xml',
			'backup_LCD'				:	'{kodi_folder}/userdata/LCD.xml',
			'backup_profiles'			:	'{kodi_folder}/userdata/profiles.xml',
			'backup_RssFeeds'			:	'{kodi_folder}/userdata/RssFeeds.xml',
			'backup_sources'			:	'{kodi_folder}/userdata/sources.xml',
			'backup_upnpserver'			:	'{kodi_folder}/userdata/upnpserver.xml',
			'backup_peripheral_data'	:	'{kodi_folder}/userdata/peripheral_data.xml',
			'backup_guisettings'		:	'{kodi_folder}/userdata/guisettings.xml',
			'backup_advancedsettings'	:	'{kodi_folder}/userdata/advancedsettings.xml',

			}


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


def log(message, label = ''):
	logmsg       = '%s : %s - %s ' % ('OSMC BACKUP' , str(label), str(message))
	xbmc.log(msg = logmsg, level=xbmc.LOGDEBUG)


class osmc_backup(object):

	def __init__(self, settings_dict, progress_function):

		log('osmc_backup INIT')

		self.s = settings_dict

		self.progress = progress_function

		# backup candidates is a list of tuples that contain the folder/file path and the size in bytes of the entry
		self.backup_candidates = self.create_backup_file_list() if self.s.get('create_tarball', False) else None


	def start_backup(self):

		''' This is the main method that walks through the backup process '''

		if self.check_backup_location():

			if self.s['export_library']:

				self.export_libraries()

			if self.s['create_tarball']:

				self.create_tarball()


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

		# check for write permission at backup location
		if not self.check_target_writeable(self.location):

			log('Backup location not writeable.')

			ok = DIALOG.ok('OSMC Backup', 'Backup location not writeable')

			return False

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

				path = location.format(kodi_folder=kodi_folder)

				size = self.calculate_byte_size(path)

				backup_candidates.append((path, size))

		return backup_candidates


	def check_target_location_for_size(self, location):

		''' Checks the target location to see if there is sufficient space for the tarball.
			Returns True if there is sufficient disk space '''

		st = os.statvfs(location)

		requirement = self.estimate_disk_requirement()
		if st.f_frsize:
			available = st.f_frsize * st.f_bavail
		else:
			available = st.f_bsize * st.f_bavail
		# available	= st.f_bfree/float(st.f_blocks) * 100 * st.f_bsize

		log('required disk space: %s' % requirement)
		log('available disk space: %s' % available)
		
		log('f_bfree = %s' % st.f_bfree)
		log('f_bsize = %s' % st.f_bsize)
		log('f_blocks = %s' % st.f_blocks)
		log('f_bavail = %s' % st.f_bavail)
		log('f_frsize = %s' % st.f_frsize)

		return  requirement < available


	def check_target_writeable(self, location):

		''' tests backup location for writeability, returns True is writeable '''

		temp_file = os.path.join(location, 'temp_write_test')

		try:
			with open(temp_file, 'w') as f:
				f.write(' ')
		except:
			log('%s is not writeable' % location)
			return False

		try:
			os.remove(temp_file)
		except:
			log('Cannot delete temp file at %s' % location)
			return False 

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

		# generate name for new tarball
		tarball_name = self.generate_tarball_name(location)

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

		try:
			with tarfile.open(tarball_name, "w:gz") as tar:
				for name, size in self.backup_candidates:

					self.progress(**{'percent':  pct, 'heading':  'OSMC Backup', 'message': '%s' % name})
					
					try:
						tar.add(name)
					except:
						log('%s failed to backup to tarball' % name)
						continue

					progress_total += math.log(max(size, 1))

					pct = int( (progress_total / float(total_size) ) * 100.0 )


			# remove the unneeded backups (this will only occur if the tarball is successfully created)
			for r in remove_these:
				try:
					self.progress(**{'percent':  100, 'heading':  'OSMC Backup', 'message': 'Removing old backup file: %s' % r})
					os.remove(r)
				except:
					log('Failed to remove excess tarball: %s' % r)

			self.progress(kill=True)

		except Exception as e:

			self.progress(kill=True)

			log('Creating tarball failed')
			log(type(e).__name__)
			log(e.args)
			log(traceback.format_exc())

			return 'failed'


	def generate_tarball_name(self, location):

		''' Returns the name for the new tarball '''

		file_tag = dt.datetime.strftime(dt.datetime.now(), TIME_PATTERN)

		return os.path.join(location, FILE_PATTERN % file_tag)


	def list_current_tarballs(self, location):

		''' Returns a list of the tarballs in the current backup location, from youngest to oldest '''

		pattern = os.path.join(location, FILE_PATTERN % APPENDAGE)

		tarball_list = glob.glob(pattern)

		tarball_list.sort(key = lambda x: self.time_from_filename(x, pattern, location))

		return tarball_list


	def time_from_filename(self, filename, pattern, location):

		''' Returns the date of the backup that is embedded in the backup filename '''

		prefix = os.path.join(location, FILE_PATTERN % APPENDAGE)

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
					total_size += os.path.getsize(fp)

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