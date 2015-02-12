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

		grab_all 	= True if __addon__.getSetting('all') == 'true' else False
		kodi 		= True if __addon__.getSetting('kodi') == 'true' else False
		config 		= True if __addon__.getSetting('config') == 'true' else False
		packages 	= True if __addon__.getSetting('packages') == 'true' else False
		apt 		= True if __addon__.getSetting('apt') == 'true' else False
		action      = __addon__.getSetting('mode') 
		self.email       = __addon__.getSetting('email') 

		log(type(action))

		if self.email == '':
			action = '0'

		if grab_all:

			self.grab_config()
			self.grab_package_list()
			self.grab_apt_logs()
			self.grab_kodi_logs()

		else:
			if kodi:
				self.grab_kodi_logs()
			if config:
				self.grab_config()
			if packages:
				self.grab_package_list()
			if apt:
				self.grab_apt_logs()

		self.tmp_log_location = '/var/tmp/uploadlog.txt'

		with open(self.tmp_log_location, 'w') as f:
			f.writelines(self.log_list)

		with os.popen('curl -X POST -s -T "/var/tmp/uploadlog.txt" http://paste.osmc.io/documents') as f:
			line = f.readline()
			key = line.replace('{"key":"','').replace('"}','')

		self.url = 'http://paste.osmc.io/%s' % key

		log(self.url)

		if action == '0':

			ok = xbmcgui.Dialog().ok(lang(32013), lang(32014) % self.url)

		elif action == '1':

			self.send_email('url')

			ok = xbmcgui.Dialog().ok(lang(32013), lang(32015))

		elif action == '2':

			self.send_email('log')

			ok = xbmcgui.Dialog().ok(lang(32013), lang(32015))


	def send_email(self, item):

		try:
			recipient = 'subliminal.karnage@gmail.com'

			body = '<table border="1">'

			if item == 'log':

				body += '\n'.join(self.log_list)

			else:
				body += self.url

			body += '</table>'

			msg = MIMEText(body, 'html')
			msg['Subject'] = 'OSMC Log Uploader'
			msg['From'] = 'OSMC'
			msg['To'] = self.email
			msg['X-Mailer'] = 'OSMC Log Uploader'

			smtp = smtplib.SMTP('alt4.gmail-smtp-in.l.google.com')

			smtp.sendmail(msg['From'], msg['To'], msg.as_string())
			smtp.quit()
		
		except:

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

if __name__ == "__main__":
	Main()		
