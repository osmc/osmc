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
		kodi 				= True if __addon__.getSetting('kodi') == 'true' else False
		config 				= True if __addon__.getSetting('config') == 'true' else False
		packages 			= True if __addon__.getSetting('packages') == 'true' else False
		apt 				= True if __addon__.getSetting('apt') == 'true' else False
		cmdline 			= True if __addon__.getSetting('cmdline') == 'true' else False
		fstab 				= True if __addon__.getSetting('fstab') == 'true' else False
		advancedsettings 	= True if __addon__.getSetting('advancedsettings') == 'true' else False
		sources 			= True if __addon__.getSetting('sources') == 'true' else False
		keyboard 			= True if __addon__.getSetting('keyboard') == 'true' else False


		if grab_all:

			self.grab_config()
			self.grab_cmdline()
			self.grab_package_list()
			self.grab_apt_logs()
			self.grab_fstab()
			self.grab_advancedsettings()
			self.grab_sources()
			self.grab_keyboard()
			self.grab_kodi_logs()

		else:
			if kodi:
				self.grab_kodi_logs()
			if config:
				self.grab_config()
			if cmdline:
				self.grab_cmdline()
			if packages:
				self.grab_package_list()
			if apt:
				self.grab_apt_logs()
			if advancedsettings:
				self.grab_advancedsettings()
			if keyboard:
				self.grab_keyboard()
			if fstab:
				self.grab_fstab()
			if sources:
				self.grab_sources()

		self.tmp_log_location = '/var/tmp/uploadlog.txt'

		with open(self.tmp_log_location, 'w') as f:
			f.writelines(self.log_list)

		with os.popen('curl -X POST -s -T "%s" http://paste.osmc.io/documents' % self.tmp_log_location) as f:
			line = f.readline()
			key = line.replace('{"key":"','').replace('"}','')

		self.url = 'http://paste.osmc.io/%s' % key

		log(self.url)

		ok = xbmcgui.Dialog().ok(lang(32013), lang(32014) % self.url)


	def grab_kodi_logs(self):

		self.log_list.extend(['\n====================== Kodi Logs ======================\n'])

		location = '/home/osmc/.kodi/temp/kodi.log'

		try:
			with open (location, 'r') as f:
				lines = f.readlines
				self.log_list.extend(lines[len(lines)-500:])
		except:
			self.log_list.extend(['kodi logs not found'])


	def grab_config(self):

		self.log_list.extend(['\n====================== Pi Config ======================\n'])

		location = '/boot/config.txt'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines)
		except:
			self.log_list.extend(['config.txt not found'])


	def grab_package_list(self):

		self.log_list.extend(['\n====================== OSMC Packages ======================\n'])

		with os.popen('dpkg -l | grep osmc') as f:
			self.log_list.extend(f.readlines())


	def grab_apt_logs(self):

		self.log_list.extend(['\n====================== APT Logs ======================\n'])

		location = '/var/log/apt/term.log'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines)
		except:
			self.log_list.extend(['apt log not found'])


	def grab_cmdline(self):

		self.log_list.extend(['\n====================== Pi Cmdline ======================\n'])

		location = '/boot/cmdline.txt'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines)
		except:
			self.log_list.extend(['cmdline.txt not found'])


	def grab_advancedsettings(self):

		self.log_list.extend(['\n====================== Advanced Settings ======================\n'])

		location = '/home/osmc/.kodi/userdata/advancedsettings.xml'

		try:
			with open (location, 'r') as f:
				lines = f.readlines
				self.log_list.extend(f.readlines)
		except:
			self.log_list.extend(['advanced settings not found'])


	def grab_sources(self):

		self.log_list.extend(['\n====================== sources.xml ======================\n'])
		
		location = '/home/osmc/.kodi/userdata/sources.xml'

		try:
			with open (location, 'r') as f:
				lines = f.readlines
				self.log_list.extend(f.readlines)
		except:
			self.log_list.extend(['sources.xml not found'])


	def grab_fstab(self):

		self.log_list.extend(['\n====================== fstab ======================\n'])

		location = '/etc/fstab'

		try:
			with open (location, 'r') as f:
				self.log_list.extend(f.readlines)
		except:
			self.log_list.extend(['fstab not found'])


	def grab_keyboard(self):

		self.log_list.extend(['\n====================== keyboard.xml ======================\n'])

		location = '/home/osmc/.kodi/userdata/keyboard.xml'

		try:
			with open (location, 'r') as f:
				lines = f.readlines
				self.log_list.extend(f.readlines)
		except:
			self.log_list.extend(['keyboard.xml not found'])


if __name__ == "__main__":
	Main()		
