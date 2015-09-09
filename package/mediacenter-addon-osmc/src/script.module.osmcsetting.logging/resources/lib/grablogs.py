import sys
import os
import argparse

try:

	import xbmc
	import xbmcgui
	import xbmcaddon

	addonid = "script.module.osmcsetting.logging"
	__addon__  = xbmcaddon.Addon(addonid)

	DIALOG = xbmcgui.Dialog()

	CALLER = 'kodi'

except ImportError:

	CALLER = 'user'

SETS =	{
		'uname' 			: { 'order':	1, 
								'function': 'grab_uname',					
								'active': 	False, 
								'pointer': [('UNAME', '0wwkXuO5')],
								'help': 'System Information',
								'dest': 'uname',
								'action': 'store_true',
								'flags' : ['-I','--systeminfo']},

		'config' 			: { 'order':	2, 
								'function': 'grab_config', 				
								'active': 	False, 
								'pointer': [('Pi config', 'Ul2H1CLu')],
								'help': 'Pi config.txt',
								'dest': 'config',
								'action': 'store_true',
								'flags' : ['-p','--piconfig']},

		'advancedsettings' 	: { 'order':	3, 
								'function': 'grab_advancedsettings', 		
								'active': 	False, 
								'pointer': [('advancedsettings.xml', 'C7hKmH1p')],
								'help': 'advancedsettings.xml',
								'dest': 'advancedsettings',
								'action': 'store_true',
								'flags' : ['-v','--advset']},

		'keyboard' 			: { 'order':	4, 
								'function': 'grab_keyboard', 				
								'active': 	False, 
								'pointer': [('keyboard.xml', 'MBom5YV6')],
								'help': 'keyboard.xml',
								'dest': 'keyboard',
								'action': 'store_true',
								'flags' : ['-k','--keyboard']},

		'remote' 			: { 'order':	5, 
								'function': 'grab_remote', 				
								'active': 	False, 
								'pointer': [('remote.xml', '5jmphjm3')],
								'help': 'remote.xml',
								'dest': 'remote',
								'action': 'store_true',
								'flags' : ['-r', '--remote']},

		'sources' 			: { 'order':	6, 
								'function': 'grab_sources', 				
								'active': 	False, 
								'pointer': [('sources.xml', 'SGkuGLGj')],
								'help': 'sources.xml',
								'dest': 'sources',
								'action': 'store_true',
								'flags' : ['-s', '--sources']},

		'fstab' 			: { 'order':	7, 
								'function': 'grab_fstab', 				
								'active': 	False, 
								'pointer': [('fstab', 'qiE9Dtax')],
								'help': 'fstab file',
								'dest': 'fstab',
								'action': 'store_true',
								'flags' : ['-f','--fstab']},

		'packages' 			: { 'order':	8, 
								'function': 'grab_osmc_packages', 		
								'active': 	False, 
								'pointer': [('OSMC Packages', '7nQvfy9a')],
								'help': 'OSMC Packages',
								'dest': 'packages',
								'action': 'store_true',
								'flags' : ['-O','--packages']},

		'allothers' 		: { 'order':	9, 
								'function': 'grab_all_other_packages', 	
								'active': 	False, 'pointer': [('All Other Packages', 'hwvkLCMX')],
								'help': 'All Other Packages',
								'dest': 'allothers',
								'action': 'store_true',
								'flags' : ['-o','--othpack']},

		'apt' 				: { 'order':  10, 
								'function': 'grab_apt_logs', 				
								'active': 	False, 
								'pointer': [	
											('APT term.log', 'RcBRrsRs'), 
											('APT history.log', 'B8sj7DO8'), 
											('APT sources.list', 'ZZz2wrJ1'), 
											('APT apt.conf.d', 'fFsk1x85'), 
											('APT preferences.d', 'vSKj25Lq')
										 ],
								'help': 'APT term.log, history.log, sources.list, apt.conf.d, preferences.d',
								'dest': 'apt',
								'action': 'store_true',
								'flags' : ['-A', '--apt']},

		'system' 			: { 'order':  11, 
								'function': 'grab_system_logs', 			
								'active': 	False, 
								'pointer': [('System Journal', 'MyqVXi2x')],
								'help': 'System Journal',
								'dest': 'system',
								'action': 'store_true',
								'flags' : ['-J','--sysjrn']},

		'lirc' 				: { 'order':  12, 
								'function': 'grab_lirc_conf', 			
								'active': 	False, 
								'pointer': [('lircd.conf', 'kdgLUcwP')],
								'help': 'lirc.conf file',
								'dest': 'lirc',
								'action': 'store_true',
								'flags' : ['-l','--lirc']},

		'initd' 			: { 'order':  13, 
								'function': 'grab_initd',					
								'active': 	False, 
								'pointer': [('init.d', 'Vr58kq0w')],
								'help': 'init.d file',
								'dest': 'initd',
								'action': 'store_true',
								'flags' : ['-i','--initd']},

		'systemd' 			: { 'order':  14, 
								'function': 'grab_systemd',				
								'active': 	False, 
								'pointer': [('systemd', '86JFGfNO')],
								'help': 'systemd file',
								'dest': 'systemd',
								'action': 'store_true',
								'flags' : ['-d','--systemd']},

		'dmesg' 			: { 'order':  15, 
								'function': 'grab_dmesg',					
								'active': 	False, 
								'pointer': [('Kernel Message Log', 'Ad2zzd21')],
								'help': 'Kernel Message Log',
								'dest': 'dmesg',
								'action': 'store_true',
								'flags' : ['-K', '--kernel']},

		'mem' 				: { 'order':  16, 
								'function': 'grab_mem',					
								'active': 	False, 'pointer': [('Memory', 'eWTP1Mc8')],
								'help': 'System Memory (total & available)',
								'dest': 'mem',
								'action': 'store_true',
								'flags' : ['-m','--memory']},

		'diskspace' 		: { 'order':  17, 
								'function': 'grab_diskspace',				
								'active': 	False, 
								'pointer': [('Diskspace', 'qZy25Yas')],
								'help': 'Diskspace (total & available)',
								'dest': 'diskspace',
								'action': 'store_true',
								'flags' : ['-D','--disk']},

		'boot' 				: {	'order':  18, 
								'function': 'grab_boot_contents',			
								'active': 	False, 
								'pointer': [('/boot Contents', 'H3gEog10')],
								'help': 'Contents of /boot/',
								'dest': 'boot',
								'action': 'store_true',
								'flags' : ['-b', '--boot']},

		'kodi' 				: {	'order':  19, 
								'function': 'grab_kodi_logs', 			
								'active': 	False, 
								'pointer': [
											('Kodi Log', 'HyhIT4UP'), 
											('Kodi Old Log', '2qaAc90c')
											],
								'help': 'Kodi log files (includes log from previous boot)',
								'dest': 'kodi',
								'action': 'store_true',
								'flags' : ['-X', '--kodi', '--xbmc']},

		}


def log(message):
	try:
		xbmc.log('OSMC LOGGING ' + str(message), level=xbmc.LOGDEBUG)
	except:
		print message


def lang(id):
	try:
		san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
		return san 

	except:
		return '%s'


def parse_arguments():
	''' Parses the arguments provided by the user and activates the entries in SETS.
		Returns a bool determining whether the user wants to copy the logs to the SD Card.
		If help is true, then the help dialog is displayed. '''

	parser = argparse.ArgumentParser(description='Uploads vital logs to online pastebin site. If network is unavailable, logs are copied to SD Card.')

	arguments = [v for k, v in SETS.iteritems()]
	arguments.sort(key = lambda x: x.get('order', 99))

	parser.add_argument('-C', '--copy', action='store_true', dest='copy', help='Copy logs to /boot (SD Card)', default=False)
	parser.add_argument('-P', '--print',  action='store_true', dest='termprint',  help='Print logs to screen (no upload or copy')
	parser.add_argument('-a', '--all',  action='store_true', dest='all',  help='Include all logs')
	
	for a in arguments: parser.add_argument(*a['flags'], action=a['action'], dest=a['dest'], help=a['help'])

	args = parser.parse_args()

	# Exit if there are no arguments or there is only the COPY argument
	if any([
			(len(sys.argv) == 1),
			(len(sys.argv) == 2 and (args.copy or args.termprint)),
			(len(sys.argv) == 3 and (args.copy and args.termprint))
			]):

		parser.print_help()

		return None, None

	# if 'all' is specified then include all logs
	if args.all:

		for k, v in SETS.iteritems():
			SETS[k]['active'] = True

	else:

		for k, arg in vars(args).iteritems():
			if k not in ['copy', 'all', 'termprint']: 
				SETS[k]['active'] = arg

	return args.copy, args.termprint


def retrieve_settings():
	''' Gets the settings from Kodi and activates the relevant entries in SETS.
		Returns a bool determining whether the user wants to copy the logs to the SD Card.  '''

	grab_all = True if __addon__.getSetting('all') == 'true' else False

	for key in SETS:
		if grab_all and key not in []:
			SETS[key]['active'] = True
		else:
			SETS[key]['active'] = True if __addon__.getSetting(key) == 'true' else False

	return sys.argv[1] == 'copy', False


class Dummy_Progress_Dialog(object):
	''' Substitute progress dialog class to save having to try/except all pDialog calls. '''

	def create(self, *args, **kwargs):

		pass

	def update(self, *args, **kwargs):

		pass

	def close(self, *args, **kwargs):

		pass
	

class Main(object):


	def __init__(self, copy, termprint):

		self.copy_to_boot = copy

		self.termprint = termprint

		self.tmp_log_location = '/var/tmp/uploadlog.txt'
		
		self.log_blotter = [] # list to hold all the lines that need to be printed/uploaded

		try:
			self.pDialog = xbmcgui.DialogProgressBG()
		except:
			self.pDialog = Dummy_Progress_Dialog()

		self.number_of_actions = sum(1 for k, v in SETS.iteritems() if v.get('active', False))

		self.pDialog.create(lang(32024), lang(32025))

		self.arguments = [(k, v) for k, v in SETS.iteritems()]
		self.arguments.sort(key = lambda x: x[1].get('order', 99))


	def launch_process(self):

		self.add_content_index()

		self.process_logs()

		if self.termprint:
			self.write_to_screen()
			return

		self.write_to_temp_file()

		self.dispatch_logs()


	def add_content_index(self):
		''' Adds the quick look-up references to the start of the log file '''

		for k, v in self.arguments:

			if v.get('active', False):

				pntr = v.get('pointer',[])

				for p in pntr:

					# p is a tuple of the Label and lookup value
					self.log_blotter.append(p[1] + '  :  ' + p[0] + '\n')

		self.log_blotter.append('\n')


	def process_logs(self):
		''' Runs the specific function for the active logs, and appends the contents to the blotter. '''

		# add the logs themselves
		count = 0
		for k, v in self.arguments:

			if v.get('active',False):

				count += 1

				pct = int(100.0 * float(count) / float(self.number_of_actions))

				self.pDialog.update(percent=pct, message=lang(32036) % k)

				getattr(self, v['function'])()

		self.pDialog.update(percent=100, message=lang(32027))


	def write_to_screen(self):

		for line in self.log_blotter:

			print line


	def write_to_temp_file(self):
		''' Writes the logs to a single temporary file '''
		try:
			with open(self.tmp_log_location, 'w') as f:

				f.writelines(self.log_blotter)
		except:

			log('Unable to write temporary log to %s' % self.tmp_log_location)

			return

		self.pDialog.update(percent=100, message=lang(32026))


	def dispatch_logs(self):
		''' Either copies the combined logs to the SD Card or Uploads them to the pastebin. '''

		if self.copy_to_boot:
			
			os.popen('sudo cp -rf %s /boot/' % self.tmp_log_location)

			if CALLER == 'kodi':
				ok = DIALOG.ok(lang(32013), lang(32040))

			else:

				log("Complete")

			self.pDialog.close()

		else:

			try:
				with os.popen('curl -X POST -s -T "%s" http://paste.osmc.io/documents' % self.tmp_log_location) as f:

					line = f.readline()
					
					key = line.replace('{"key":"','').replace('"}','').replace('\n','')
					
					log('pastio line: %s' % repr(line))
			except:

				key = False

			self.pDialog.close()

			if not key:

				if CALLER == 'kodi':

					self.copy_to_boot = DIALOG.yesno(lang(32013), lang(32023), lang(32039))

				else:

					self.copy_to_boot = True

					log("Failed to upload log files, copying to /boot instead. (Unable to verify)")

				if self.copy_to_boot:

					os.popen('sudo cp -rf %s /boot/' % self.tmp_log_location)

			else:

				self.url = 'http://paste.osmc.io/ %s' % key

				if CALLER == 'kodi':

					ok = DIALOG.ok(lang(32013), lang(32014) % self.url)

				else:

					log("Logs successfully uploaded to %s" % self.url)


	def grab_mem(self):

		self.log_blotter.extend(['\n====================== Memory ========================= eWTP1Mc8\n'])

		with os.popen('free -m') as f:
			self.log_blotter.extend(f.readlines())		

		self.log_blotter.extend(['\n====================== Memory END ===================== eWTP1Mc8\n'])


	def grab_diskspace(self):

		self.log_blotter.extend(['\n====================== Diskspace ====================== qZy25Yas\n'])

		with os.popen('df -h') as f:
			self.log_blotter.extend(f.readlines())	
		
		self.log_blotter.extend(['\n====================== Diskspace END ================== qZy25Yas\n'])


	def grab_initd(self):

		self.log_blotter.extend(['\n====================== init.d ========================= Vr58kq0w\n'])

		with os.popen('ls -al /etc/init.d') as f:
			self.log_blotter.extend(f.readlines())	

		self.log_blotter.extend(['\n====================== init.d END ===================== Vr58kq0w\n'])


	def grab_systemd(self):

		self.log_blotter.extend(['\n====================== systemd ======================== 86JFGfNO\n'])

		with os.popen('ls -al /lib/systemd/system') as f:
			self.log_blotter.extend(f.readlines())	

		self.log_blotter.extend(['\n====================== systemd END ==================== 86JFGfNO\n'])


	def grab_dmesg(self):

		self.log_blotter.extend(['\n====================== Kernel Message Log (dmesg) ========================= Ad2zzd21\n'])

		with os.popen('dmesg') as f:
			self.log_blotter.extend(f.readlines())	

		self.log_blotter.extend(['\n====================== Kernel Message Log (dmesg) END ===================== Ad2zzd21\n'])


	def grab_kodi_logs(self):

		self.log_blotter.extend(['\n====================== Kodi Log ======================= HyhIT4UP\n'])

		location = '/home/osmc/.kodi/temp/kodi.log'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:

			self.log_blotter.extend(['kodi.log not found'])

		self.log_blotter.extend(['\n====================== Kodi Log END =================== HyhIT4UP\n'])

		self.log_blotter.extend(['\n====================== Kodi Old Log ====================== 2qaAc90c\n'])

		location = '/home/osmc/.kodi/temp/kodi.old.log'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:

			self.log_blotter.extend(['kodi.old.log not found'])

		self.log_blotter.extend(['\n====================== Kodi Old Log END ================== 2qaAc90c\n'])


	def grab_lirc_conf(self):

		self.log_blotter.extend(['\n====================== lircd.conf ========================= kdgLUcwP\n'])

		location = '/etc/lirc/lircd.conf'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['lircd.conf not found'])

		self.log_blotter.extend(['\n====================== lircd.conf END ===================== kdgLUcwP\n'])


	def grab_config(self):

		self.log_blotter.extend(['\n====================== Pi config.txt ====================== Ul2H1CLu\n'])

		location = '/boot/config.txt'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['config.txt not found'])

		self.log_blotter.extend(['\n====================== Pi config.txt END ================== Ul2H1CLu\n'])


	def grab_osmc_packages(self):

		self.log_blotter.extend(['\n====================== OSMC Packages ====================== 7nQvfy9a\n'])

		with os.popen('dpkg -l | grep osmc') as f:
			self.log_blotter.extend(f.readlines())

		self.log_blotter.extend(['\n====================== OSMC Packages END ================== 7nQvfy9a\n'])


	def grab_uname(self):

		self.log_blotter.extend(['\n====================== UNAME ============================== 0wwkXuO5\n'])

		try:
			with os.popen('uname -a') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['uname not found'])

		self.log_blotter.extend(['\n====================== UNAME END ========================== 0wwkXuO5\n'])

		self.log_blotter.extend(['\n====================== cmdline =========================\n'])

		location = '/proc/cmdline'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['cmdline not found'])

		self.log_blotter.extend(['\n====================== cmdline END =====================\n'])


	def grab_all_other_packages(self):

		self.log_blotter.extend(['\n====================== All Other Packages ================= hwvkLCMX\n'])

		with os.popen('dpkg -l | grep -v osmc') as f:
			self.log_blotter.extend(f.readlines())

		self.log_blotter.extend(['\n====================== All Other Packages END ============= hwvkLCMX\n'])


	def grab_apt_logs(self):

		self.log_blotter.extend(['\n====================== APT term.log ======================= RcBRrsRs\n'])

		if os.path.isfile('/var/log/apt/term.log'):
			with os.popen('grep -v "^(Reading database" /var/log/apt/term.log | tail -n 500') as f:
				self.log_blotter.extend(f.readlines())
		else:
			self.log_blotter.extend(['apt term.log not found'])

		self.log_blotter.extend(['\n====================== APT term.log END =================== RcBRrsRs\n'])

		self.log_blotter.extend(['\n====================== APT history.log ======================== B8sj7DO8\n'])

		if os.path.isfile('/var/log/apt/history.log'):
			with os.popen('grep -v "^(Reading database" /var/log/apt/history.log | tail -n 500') as f:
				self.log_blotter.extend(f.readlines())
		else:
			self.log_blotter.extend(['apt history.log not found'])

		self.log_blotter.extend(['\n====================== APT history.log END ==================== B8sj7DO8\n'])

		self.log_blotter.extend(['\n====================== APT sources.list ======================== ZZz2wrJ1\n'])

		location = '/etc/apt/sources.list'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['APT sources.list not found'])

		self.log_blotter.extend(['\n====================== APT sources.list END ==================== ZZz2wrJ1\n'])

		self.log_blotter.extend(['\n====================== APT apt.conf.d ======================== fFsk1x85\n'])

		with os.popen('ls -al /etc/apt/apt.conf.d') as f:
			self.log_blotter.extend(f.readlines())

		self.log_blotter.extend(['\n====================== APT apt.conf.d END ==================== fFsk1x85\n'])

		self.log_blotter.extend(['\n====================== APT preferences.d ======================== vSKj25Lq\n'])

		with os.popen('ls -al /etc/apt/preferences.d') as f:
			self.log_blotter.extend(f.readlines())

		self.log_blotter.extend(['\n====================== APT preferences.d END ==================== vSKj25Lq\n'])


	def grab_advancedsettings(self):

		self.log_blotter.extend(['\n====================== advancedsettings.xml =============== C7hKmH1p\n'])

		location = '/home/osmc/.kodi/userdata/advancedsettings.xml'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['advancedsettings.xml not found'])

		self.log_blotter.extend(['\n====================== advancedsettings.xml END =========== C7hKmH1p\n'])


	def grab_sources(self):

		self.log_blotter.extend(['\n====================== sources.xml ======================== SGkuGLGj\n'])
		
		location = '/home/osmc/.kodi/userdata/sources.xml'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['sources.xml not found'])

		self.log_blotter.extend(['\n====================== sources.xml END ==================== SGkuGLGj\n'])


	def grab_fstab(self):

		self.log_blotter.extend(['\n====================== fstab ============================== qiE9Dtax\n'])

		location = '/etc/fstab'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['fstab not found'])

		self.log_blotter.extend(['\n====================== fstab END ========================== qiE9Dtax\n'])


	def grab_keyboard(self):

		self.log_blotter.extend(['\n====================== keyboard.xml ====================== MBom5YV6\n'])

		location = '/home/osmc/.kodi/userdata/keyboard.xml'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['keyboard.xml not found'])

		self.log_blotter.extend(['\n====================== keyboard.xml END ================== MBom5YV6\n'])


	def grab_remote(self):

		self.log_blotter.extend(['\n====================== remote.xml =================== 5jmphjm3\n'])

		location = '/home/osmc/.kodi/userdata/remote.xml'

		try:
			with open (location, 'r') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['remote.xml not found'])	

		self.log_blotter.extend(['\n====================== remote.xml END =================== 5jmphjm3\n'])		


	def grab_system_logs(self):

		self.log_blotter.extend(['\n====================== System Journal ==================== MyqVXi2x\n'])

		try:
			with os.popen('sudo journalctl') as f:
				self.log_blotter.extend(f.readlines())
		except:
			self.log_blotter.extend(['system log not found'])

		self.log_blotter.extend(['\n====================== System Journal END =============== MyqVXi2x\n'])


	def grab_boot_contents(self):

		self.log_blotter.extend(['\n====================== /boot Contents =================== H3gEog10\n'])

		with os.popen('ls -al /boot') as f:
			self.log_blotter.extend(f.readlines())

		self.log_blotter.extend(['\n====================== /boot Contents END =============== H3gEog10\n'])


if __name__ == "__main__":

	log('Caller is: %s' % CALLER)

	if CALLER == 'user':
		copy, termprint = parse_arguments()
	else:
		copy, termprint = retrieve_arguments()

	if copy is not None:

		m = Main(copy, termprint)

		m.launch_process()	