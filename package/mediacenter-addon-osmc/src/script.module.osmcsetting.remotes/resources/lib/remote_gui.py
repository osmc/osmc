
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
	xbmc.log('REMOTE: ' + str(message), level=xbmc.LOGDEBUG)


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


class remote_gui_launcher(object):

	def __init__(self):

		# flag to idicate whether the GUI should re-opn upon close. This is for when the remote changes do not stick.
		self.open_gui = True

		# container for any confs we want to ignore
		self.excluded = ['lircd.conf']

		self.lircd_path = '/etc/lirc/lircd.conf'
		self.etc_lirc = '/etc/lirc'

		# self.lircd_path = '/home/kubkev/temp/lirc/lircd.conf'
		# self.etc_lirc = '/home/kubkev/temp/lirc'		

		try:
			real_file = os.path.realpath(self.lircd_path)
			path, filename = os.path.split(real_file)

			log(path)
			log(filename)

			if path != self.etc_lirc:
				custom = True
			else:
				custom = False

		except:
			real_file = None
			custom = False

		log(real_file)
		log('custom=' + str(custom))

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

		xml = "RemoteBrowser_720OSMC.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "RemoteBrowser_OSMC.xml"

		self.remote_gui = remote_GUI(xml, __path__, 'Default', local_confs=local_confs, real_file=real_file, custom=custom)


	def open_gui(self):

		while self.reopen:

			self.open_gui = False

			self.remote_gui.doModal()

			self.apply_selection(self.remote_gui.remote_selection)


	def construct_listitem(self, conf):

		path, filename = os.path.split(conf)

		# get conf name; check first line in file for "# name:"
		with open(conf, 'r') as f:
			lines = f.readlines()
			first_line = lines[0]
			if first_line.startswith("# name:"):
				name = first_line[len("# name:"):]
				name2 = filename
			else:
				name = filename.replace('.conf', '')
				name2 = conf

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

		CONFIRM_DURATION = 10

		if not selection: return 

		via, conf = selection

		if os.path.isfile(conf):

			# read the symlink target
			original_target = os.readlink( self.lircd_path )
			
			subprocess.call(['sudo', 'ln', '-sf', conf, self.lircd_path])

			subprocess.call(['sudo', 'systemctl', 'restart', 'lircd_helper@*'])

		# get the user to confirm that this works
		user_confirm = xbmcgui.Dialog.yesno(heading=lang(32006), line1=lang(32007), line2=lang(32008) % CONFIRM_DURATION, nolabel=lang(32009), yeslabel=lang(32010), autoclose=CONFIRM_DURATION * 1000 )

		if not user_confirm:

			subprocess.call(['sudo', 'ln', '-sf', original_target, self.lircd_path])

			subprocess.call(['sudo', 'systemctl', 'restart', 'lircd_helper@*'])

			self.open_gui = True



class remote_GUI(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, local_confs, real_file, custom):

		self.local_confs = local_confs
		self.real_file = real_file
		self.custom = custom

		self.remote_selection = None


	def onInit(self):

		self.list = self.getControl(500)
		self.list.setVisible(True)

		if self.custom:
			cst = xbmcgui.ListItem(label=os.path.basename(self.real_file).replace('.conf',''), label2=self.real_file)
			self.list.addItem(cst)
			self.list.getListItem(0).select(True)

		for i, x in enumerate(self.local_confs):

			self.list.addItem(x)

			if x.getLabel2() == self.real_file:
				idx = i + 1 if self.custom else i
				self.list.getListItem(idx).select(True)

			log(x.getLabel2())
			log(self.real_file)
			log(self.real_file == x.getLabel2())

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



