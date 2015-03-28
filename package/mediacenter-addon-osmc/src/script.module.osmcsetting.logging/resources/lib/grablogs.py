# XBMC Modules
import xbmc
import xbmcgui
import xbmcaddon

# Standard Modules
import sys
import os
import smtplib
from email.mime.text import MIMEText

addonid = "script.module.osmcsetting.logging"
__addon__  = xbmcaddon.Addon(addonid)


# OSMC SETTING Modules
sys.path.append(xbmc.translatePath(os.path.join(xbmcaddon.Addon(addonid).getAddonInfo('path'), 'resources','lib')))
from CompLogger import comprehensive_logger as clog


USER_ACTION = sys.argv[1]


def log(message):
	xbmc.log('OSMC LOGGING ' + str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 
	

class Main(object):


	def __init__(self):

		self.log_list = []

		grab_all 			= True if __addon__.getSetting('all') == 'true' else False

		sets =	{
				'kodi' 				: {'function': self.grab_kodi_logs, 			'setting': False, 'pointer': [('Kodi Log', 'HyhIT4UP'), ('Kodi Old Log', '2qaAc90c')]},
				'config' 			: {'function': self.grab_config, 				'setting': False, 'pointer': [('Pi config', 'Ul2H1CLu')]},
				'packages' 			: {'function': self.grab_osmc_packages, 		'setting': False, 'pointer': [('OSMC Packages', '7nQvfy9a')]},
				'allothers' 		: {'function': self.grab_all_other_packages, 	'setting': False, 'pointer': [('All Other Packages', 'hwvkLCMX')]},
				'apt' 				: {'function': self.grab_apt_logs, 				'setting': False, 'pointer': [('APT Logs', 'RcBRrsRs'), ('History Logs', 'B8sj7DO8Fj')]},
				'fstab' 			: {'function': self.grab_fstab, 				'setting': False, 'pointer': [('fstab', 'qiE9Dtax')]},
				'advancedsettings' 	: {'function': self.grab_advancedsettings, 		'setting': False, 'pointer': [('advancedsettings.xml', 'C7hKmH1p')]},
				'sources' 			: {'function': self.grab_sources, 				'setting': False, 'pointer': [('sources.xml', 'SGkuGLGj')]},
				'keyboard' 			: {'function': self.grab_keyboard, 				'setting': False, 'pointer': [('keyboard.xml', 'MBom5YV6')]},
				'remote' 			: {'function': self.grab_remote, 				'setting': False, 'pointer': [('remote.xml', '5jmphjm3')]},
				'system' 			: {'function': self.grab_system_logs, 			'setting': False, 'pointer': [('System Journal', 'MyqVXi2x')]},
				'lirc' 				: {'function': self.grab_lirc_conf, 			'setting': False, 'pointer': [('lircd.conf', 'kdgLUcwP')]},
				'boot' 				: {'function': self.grab_boot_contents,			'setting': False, 'pointer': [('/boot Contents', 'H3gEog10')]},
				'uname' 			: {'function': self.grab_uname,					'setting': False, 'pointer': [('UNAME', '0wwkXuO5')]},
				'initd' 			: {'function': self.grab_initd,					'setting': False, 'pointer': [('init.d', 'Vr58kq0w')]},
				'systemd' 			: {'function': self.grab_systemd,				'setting': False, 'pointer': [('systemd', '86JFGfNO')]},
				'mem' 				: {'function': self.grab_mem,					'setting': False, 'pointer': [('Memory', 'eWTP1Mc8')]},
				'diskspace' 		: {'function': self.grab_diskspace,				'setting': False, 'pointer': [('Diskspace', 'qZy25Yas')]},
				}

		keys = [
				'uname',
				'config',
				'advancedsettings',
				'keyboard',
				'remote',
				'sources',
				'fstab',
				'packages',
				'allothers',
				'apt',
				'system',
				'lirc',
				'initd',
				'systemd',
				'mem',
				'diskspace',
				'boot',
				'kodi',
				]	


		for key in keys:
			if grab_all and key not in []:
				sets[key]['setting'] = True
			else:
				sets[key]['setting'] = True if __addon__.getSetting(key) == 'true' else False

		self.number_of_actions = sum(1 for key in keys if sets.get(key, {}).get('setting', False))

		log(self.number_of_actions)

		self.pDialog = xbmcgui.DialogProgressBG()
		self.pDialog.create(lang(32024), lang(32025))

		# add the quick look-up references
		for key in keys:
			if sets.get(key,{}).get('setting',False):
				pntr = sets.get(key,{}).get('pointer',[])
				for p in pntr:
					# p is a tuple of the Label and lookup value
					self.log_list.append(p[1] + '  :  ' + p[0] + '\n')
		self.log_list.append('\n')

		# add the logs themselves
		count =0
		for key in keys:

			if sets.get(key,{}).get('setting',False):
				count += 1
				pct = int(100.0 * float(count) / float(self.number_of_actions))
				self.pDialog.update(percent=pct, message=lang(32036) % key)
				sets.get(key,{})['function']()

		self.tmp_log_location = '/var/tmp/uploadlog.txt'

		self.pDialog.update(percent=100, message=lang(32027))

		with open(self.tmp_log_location, 'w') as f:

			f.writelines(self.log_list)

		self.pDialog.update(percent=100, message=lang(32026))

		if USER_ACTION == 'copy':
			
			os.popen('sudo cp -rf %s /boot/' % self.tmp_log_location)

			ok = xbmcgui.Dialog().ok(lang(32013), lang(32040))

			self.pDialog.close()

		else:

			with os.popen('curl -X POST -s -T "%s" http://paste.osmc.io/documents' % self.tmp_log_location) as f:

				line = f.readline()
				
				key = line.replace('{"key":"','').replace('"}','').replace('\n','')
				
				log('pastio line: %s' % repr(line))

			self.pDialog.close()

			if not key:

				copy = xbmcgui.Dialog().yesno(lang(32013), lang(32023), lang(32039))

				if copy:

					os.popen('sudo cp -rf %s /boot/' % self.tmp_log_location)

			else:

				self.url = 'http://paste.osmc.io/ %s' % key

				ok = xbmcgui.Dialog().ok(lang(32013), lang(32014) % self.url)


	def grab_mem(self):

		self.log_list.extend(['\n====================== Memory =========================eWTP1Mc8\n'])

		with os.popen('free -m') as f:
			self.log_list.extend(f.readlines())		


	def grab_diskspace(self):

		self.log_list.extend(['\n====================== Diskspace ======================qZy25Yas\n'])

		with os.popen('df -h') as f:
			self.log_list.extend(f.readlines())	


	def grab_initd(self):

		self.log_list.extend(['\n====================== init.d =========================Vr58kq0w\n'])

		with os.popen('ls -al /etc/init.d') as f:
			self.log_list.extend(f.readlines())	


	def grab_systemd(self):

		self.log_list.extend(['\n====================== systemd ========================86JFGfNO\n'])

		with os.popen('ls -al /lib/systemd/system') as f:
			self.log_list.extend(f.readlines())	


	def grab_kodi_logs(self):

		self.log_list.extend(['\n====================== Kodi Old Log ======================2qaAc90c\n'])

		location = '/home/osmc/.kodi/temp/kodi.old.log'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:

			self.log_list.extend(['kodi old logs not found'])

		self.log_list.extend(['\n====================== Kodi Log =======================HyhIT4UP\n'])

		location = '/home/osmc/.kodi/temp/kodi.log'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:

			self.log_list.extend(['kodi logs not found'])


	def grab_lirc_conf(self):

		self.log_list.extend(['\n====================== lircd.conf =========================kdgLUcwP\n'])

		location = '/etc/lirc/lircd.conf'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['lircd.conf not found'])


	def grab_config(self):

		self.log_list.extend(['\n====================== Pi config.txt ======================Ul2H1CLu\n'])

		location = '/boot/config.txt'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['config.txt not found'])


	def grab_osmc_packages(self):

		self.log_list.extend(['\n====================== OSMC Packages ======================7nQvfy9a\n'])

		with os.popen('dpkg -l | grep osmc') as f:
			self.log_list.extend(f.readlines())


	def grab_uname(self):

		self.log_list.extend(['\n====================== UNAME ==============================0wwkXuO5\n'])

		try:
			with os.popen('uname -a') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['uname not found'])

		self.log_list.extend(['\n====================== cmdline =========================\n'])

		location = '/proc/cmdline'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['cmdline not found'])


	def grab_all_other_packages(self):

		self.log_list.extend(['\n====================== All Other Packages =================hwvkLCMX\n'])

		with os.popen('dpkg -l | grep -v osmc') as f:
			self.log_list.extend(f.readlines())


	def grab_apt_logs(self):

		self.log_list.extend(['\n====================== APT Logs ===========================RcBRrsRs\n'])

		if os.path.isfile('/var/log/apt/term.log'):
			with os.popen('grep -v "^(Reading database" /var/log/apt/term.log | tail -n 500') as f:
				self.log_list.extend(f.readlines())
		else:
			self.log_list.extend(['apt log not found'])

		self.log_list.extend(['\n====================== History Logs ===========================B8sj7DO8Fj\n'])

		if os.path.isfile('/var/log/apt/history.log'):
			with os.popen('grep -v "^(Reading database" /var/log/apt/history.log | tail -n 500') as f:
				self.log_list.extend(f.readlines())
		else:
			self.log_list.extend(['apt log not found'])


	def grab_advancedsettings(self):

		self.log_list.extend(['\n====================== advancedsettings.xml ===============C7hKmH1p\n'])

		location = '/home/osmc/.kodi/userdata/advancedsettings.xml'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['advanced settings not found'])


	def grab_sources(self):

		self.log_list.extend(['\n====================== sources.xml ========================SGkuGLGj\n'])
		
		location = '/home/osmc/.kodi/userdata/sources.xml'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['sources.xml not found'])


	def grab_fstab(self):

		self.log_list.extend(['\n====================== fstab ==============================qiE9Dtax\n'])

		location = '/etc/fstab'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['fstab not found'])


	def grab_keyboard(self):

		self.log_list.extend(['\n====================== keyboard.xml =======================MBom5YV6\n'])

		location = '/home/osmc/.kodi/userdata/keyboard.xml'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['keyboard.xml not found'])


	def grab_remote(self):

		self.log_list.extend(['\n====================== remote.xml =======================5jmphjm3\n'])

		location = '/home/osmc/.kodi/userdata/remote.xml'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['remote.xml not found'])			


	def grab_system_logs(self):

		self.log_list.extend(['\n====================== System Journal ====================MyqVXi2x\n'])

		try:
			with os.popen('sudo journalctl') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['system log not found'])


	def grab_boot_contents(self):

		self.log_list.extend(['\n====================== /boot Contents ===================H3gEog10\n'])

		with os.popen('ls -al /boot') as f:
			self.log_list.extend(f.readlines())


if __name__ == "__main__":
	Main()		
