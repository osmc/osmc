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
				'kodi' 				: {'function': self.grab_kodi_logs, 			'setting': False},
				'config' 			: {'function': self.grab_config, 				'setting': False},
				'packages' 			: {'function': self.grab_osmc_packages, 		'setting': False},
				'allothers' 		: {'function': self.grab_all_other_packages, 	'setting': False},
				'apt' 				: {'function': self.grab_apt_logs, 				'setting': False},
				'cmdline' 			: {'function': self.grab_cmdline, 				'setting': False},
				'fstab' 			: {'function': self.grab_fstab, 				'setting': False},
				'advancedsettings' 	: {'function': self.grab_advancedsettings, 		'setting': False},
				'sources' 			: {'function': self.grab_sources, 				'setting': False},
				'keyboard' 			: {'function': self.grab_keyboard, 				'setting': False},
				'remote' 			: {'function': self.grab_remote, 				'setting': False},
				'system' 			: {'function': self.grab_system_logs, 			'setting': False},
				'lirc' 				: {'function': self.grab_lirc_conf, 			'setting': False},
				'boot' 				: {'function': self.grab_boot_contents,			'setting': False},
				'uname' 			: {'function': self.grab_uname,					'setting': False},
				'initd' 			: {'function': self.grab_initd,					'setting': False},
				'systemd' 			: {'function': self.grab_systemd,				'setting': False},
				'mem' 				: {'function': self.grab_mem,					'setting': False},
				'diskspace' 		: {'function': self.grab_diskspace,				'setting': False},
				}

		keys = [
				'uname',
				'config',
				'cmdline',
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

		with os.popen('curl -X POST -s -T "%s" http://paste.osmc.io/documents' % self.tmp_log_location) as f:

			line = f.readline()
			
			key = line.replace('{"key":"','').replace('"}','')
			
			log('pastio key: %s' % key)

		self.pDialog.close()

		if not key:

			ok = xbmcgui.Dialog().ok(lang(32013), lang(32023))

		else:

			self.url = 'http://paste.osmc.io/%s' % key

			ok = xbmcgui.Dialog().ok(lang(32013), lang(32014) % self.url)


	def grab_mem(self):

		self.log_list.extend(['\n====================== Memory =========================\n'])

		with os.popen('free -m') as f:
			self.log_list.extend(f.readlines())		


	def grab_diskspace(self):

		self.log_list.extend(['\n====================== Diskspace ======================\n'])

		with os.popen('df -h') as f:
			self.log_list.extend(f.readlines())	


	def grab_initd(self):

		self.log_list.extend(['\n====================== init.d =========================\n'])

		with os.popen('ls -al /etc/init.d') as f:
			self.log_list.extend(f.readlines())	


	def grab_systemd(self):

		self.log_list.extend(['\n====================== systemd ========================\n'])

		with os.popen('ls -al /lib/systemd/system') as f:
			self.log_list.extend(f.readlines())	


	def grab_kodi_logs(self):

		self.log_list.extend(['\n====================== Kodi Old Log ======================\n'])

		location = '/home/osmc/.kodi/temp/kodi.old.log'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:

			self.log_list.extend(['kodi old logs not found'])

		self.log_list.extend(['\n====================== Kodi Log =======================\n'])

		location = '/home/osmc/.kodi/temp/kodi.log'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:

			self.log_list.extend(['kodi logs not found'])


	def grab_lirc_conf(self):

		self.log_list.extend(['\n====================== lircd.conf =========================\n'])

		location = '/etc/lirc/lircd.conf'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['lircd.conf not found'])


	def grab_config(self):

		self.log_list.extend(['\n====================== Pi config.txt ======================\n'])

		location = '/boot/config.txt'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['config.txt not found'])


	def grab_osmc_packages(self):

		self.log_list.extend(['\n====================== OSMC Packages ======================\n'])

		with os.popen('dpkg -l | grep osmc') as f:
			self.log_list.extend(f.readlines())


	def grab_uname(self):

		self.log_list.extend(['\n====================== UNAME ==============================\n'])

		with os.popen('uname -a') as f:
			self.log_list.extend(f.readlines())


	def grab_cmdline(self):

		self.log_list.extend(['\n====================== cmdline.txt =====================\n'])

		location = '/proc/cmdline.txt'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['cmdline.txt not found'])


	def grab_all_other_packages(self):

		self.log_list.extend(['\n====================== All Other Packages =================\n'])

		with os.popen('dpkg -l | grep -v osmc') as f:
			self.log_list.extend(f.readlines())


	def grab_apt_logs(self):

		self.log_list.extend(['\n====================== APT Logs ===========================\n'])

		if os.path.isfile('/var/log/apt/term.log'):
			with os.popen('grep -v "^(Reading database" /var/log/apt/term.log | tail -n 500') as f:
				self.log_list.extend(f.readlines())
		else:
			self.log_list.extend(['apt log not found'])


	def grab_advancedsettings(self):

		self.log_list.extend(['\n====================== advancedsettings.xml ===============\n'])

		location = '/home/osmc/.kodi/userdata/advancedsettings.xml'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['advanced settings not found'])


	def grab_sources(self):

		self.log_list.extend(['\n====================== sources.xml ========================\n'])
		
		location = '/home/osmc/.kodi/userdata/sources.xml'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['sources.xml not found'])


	def grab_fstab(self):

		self.log_list.extend(['\n====================== fstab ==============================\n'])

		location = '/etc/fstab'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['fstab not found'])


	def grab_keyboard(self):

		self.log_list.extend(['\n====================== keyboard.xml =======================\n'])

		location = '/home/osmc/.kodi/userdata/keyboard.xml'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['keyboard.xml not found'])


	def grab_remote(self):

		self.log_list.extend(['\n====================== remote.xml =======================\n'])

		location = '/home/osmc/.kodi/userdata/remote.xml'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['remote.xml not found'])			


	def grab_system_logs(self):

		self.log_list.extend(['\n====================== System Journal ====================\n'])

		try:
			with os.popen('sudo journalctl') as f:
				self.log_list.extend(f.readlines())
		except:
			self.log_list.extend(['system log not found'])


	def grab_boot_contents(self):

		self.log_list.extend(['\n====================== /boot Contents ===================\n'])

		with os.popen('ls -al /boot') as f:
			self.log_list.extend(f.readlines())


if __name__ == "__main__":
	Main()		
