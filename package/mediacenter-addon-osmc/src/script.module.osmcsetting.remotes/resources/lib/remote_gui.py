
# KODI modules
import xbmc
import xbmcaddon
import xbmcgui

# Standard modules
import sys
import os
import shutil
import subprocess

addonid 	= "script.module.osmcsetting.remotes"
__addon__  	= xbmcaddon.Addon(addonid)
__path__ 	= xbmc.translatePath(xbmcaddon.Addon(addonid).getAddonInfo('path'))

# Custom module path
sys.path.append(os.path.join(__path__, 'resources','lib'))

# OSMC SETTING Modules
from CompLogger import comprehensive_logger as clog


ACTION_PREVIOUS_MENU = 10
ACTION_NAV_BACK      = 92
SAVE                 = 5
HEADING              = 1
ACTION_SELECT_ITEM   = 7


def log(message):
	xbmc.log('OSMC APFStore gui : ' + str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


class remote_gui_launcher(object):

	def __init__(self):

		# container for any confs we want to ignore
		self.excluded = ['lircd.conf']

		self.lircd_path = '/home/osmc/lircd.conf'

		self.etc_lirc = '/etc/lirc/'

		# get the contents of /etc/lirc/
		local_confs_base = os.listdir(self.etc_lirc)
		local_confs_raw = [os.path.join(self.etc_lirc, conf) for conf in local_confs_base]
		local_confs_raw.sort()

		# filter list by files with size
		local_confs = []
		for conf in local_confs_raw:
			if os.path.basename(conf) in self.excluded: continue
			if not conf.endswith('.conf'): continue
			try:
				if os.stat(conf).st_size == 0: continue
			except:
				continue

			local_confs.append(self.construct_listitem(conf))

		self.remote_gui = remote_GUI("RemoteBrowser_OSMC.xml", __path__, 'Default', local_confs=local_confs)


	def open_gui(self):
		self.remote_gui.doModal()

		self.apply_selection(self.remote_gui.remote_selection)


	def construct_listitem(self, conf):

		path, filename = os.path.split(conf)

		# get conf name; check first line in file for "# name: "
		with open(conf, 'r') as f:
			lines = f.readlines()
			first_line = lines[0]
			if first_line.startswith("# name: "):
				name = first_line[len("# name: "):]
				name2 = filename
			else:
				name = filename
				name2 = '/etc/lirc/'

		# check for remote image, use it if it is available
		image_path = os.path.join(path, filename.replace('.conf','.png'))
		
		if os.path.isfile(image_path):

			tmp = xbmcgui.ListItem(label=name, label2=name2, thumbnailImage=image_path)

		else:

			tmp = xbmcgui.ListItem(label=name, label2=name2)

		tmp.setProperty('fullpath',conf)

		tmp.setInfo('video',{'title': ''.join(lines[:100])})

		return tmp


	def apply_selection(self, selection):
		# selection is a tuple of (source, file) where source is either 'local' or 'user'
		# local means that the file is in /etc/lirc
		# user means that the file was selected by the user

		if not selection: return 

		via, conf = selection

		if via == 'local':
			# symlink the local file to /home/osmc/lircd.conf

			if os.path.isfile(self.lircd_path):
				os.remove(self.lircd_path)

			os.symlink(conf, self.lircd_path)

		elif via == 'user':
			# copy the users file to /home/osmc/lircd.conf

			if os.path.isfile(self.lircd_path):
				os.remove(self.lircd_path)

			shutil.copyfile(conf, self.lircd_path)

		else:
			return

		subprocess.call(['sudo', 'systemctl', 'restart', 'lirc'])


class remote_GUI(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, local_confs):

		self.local_confs = local_confs

		self.remote_selection = None


	def onInit(self):

		self.list = self.getControl(500)
		self.list.setVisible(True)
		for x in self.local_confs:

			self.list.addItem(x)

		try:
			self.getControl(50).setVisible(False)
		except:
			pass


	def onClick(self, controlID):

		if controlID == 500:
			# user has selected a local file from /etc/lirc

			self.remote_selection = ('local', self.getControl(500).getSelectedItem().getProperty('fullpath'))
			self.close()

		elif controlID == 7:
			# user has selected Exit

			self.remote_selection = None

			self.close()

		elif controlID == 62:
			# user has chosen to browse for the file

			browser = xbmcgui.Dialog().browse(1, lang(32005), 'files', mask='.conf')

			if browser:

				self.remote_selection = ("user", browser)

				self.close()

			else:

				self.remote_selection = None



