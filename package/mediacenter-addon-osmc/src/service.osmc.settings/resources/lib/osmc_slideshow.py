# XBMC modules
import xbmc
import xbmcaddon
import xbmcgui

# STANDARD library modules
import os
import sys
import threading

path       = xbmcaddon.Addon().getAddonInfo('path')
lib        = os.path.join(path, 'resources','lib')
media      = os.path.join(path, 'resources','skins','Default','media')

sys.path.append(xbmc.translatePath(lib))

__addon__  = xbmcaddon.Addon()
scriptPath = __addon__.getAddonInfo('path')
WINDOW     = xbmcgui.Window(10000)

# LEFT ACTIONS
ACTION_MOVE_LEFT 		= 1
ACTION_PAGE_DOWN 		= 6
ACTION_MOUSE_WHEEL_DOWN	= 105
ACTION_SCROLL_DOWN 		= 112
ACTION_CHANNEL_DOWN 	= 185


# RIGHT ACTIONS
ACTION_MOVE_RIGHT 		= 2
ACTION_PAGE_UP 			= 5
ACTION_MOUSE_WHEEL_UP 	= 104
ACTION_SCROLL_UP 		= 111
ACTION_CHANNEL_UP 		= 184


LEFT_ACTIONS  = [ACTION_MOVE_LEFT, ACTION_PAGE_DOWN, ACTION_MOUSE_WHEEL_DOWN, ACTION_SCROLL_DOWN, ACTION_CHANNEL_DOWN]
RIGHT_ACTIONS = [ACTION_MOVE_RIGHT, ACTION_PAGE_UP, ACTION_MOUSE_WHEEL_UP, ACTION_SCROLL_UP, ACTION_CHANNEL_UP]


def log(message):

	try:
		message = str(message)

	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )

	xbmc.log('osmc_settings: ' + str(message), level=xbmc.LOGDEBUG)


def lang(id):
	
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


'''
=========================
Slideshow JSON STRUCTURE
=========================

{
   "slideshowID": <-- must be unique, will be stored in the settings.xml for the addon

       {
           "version": {'minimum: x.x.xxx, maximum: y.y.yyy},
           "platform": ['pi2', 'vero', 'vero2'],
           "expiry_date" : "31/12/2015",
           "images" : [URL1, URL2, URL3, URL4] <--these should be named so that the files are in alpahbetical order image1.png, image2.png if you like
       },
}
'''


class OSMC_Slideshow(object):


	def __init__(self, json_data):

		pass


	def download_images(self):

		pass


	def progress_bar(self):

		pass


	def create_slideshow(self, image_list):

		xml = "osmc_slideshow_720.xml" if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' else "osmc_slideshow.xml"

		return OSMC_Slideshow_GUI(xml, __path__, 'Default', image_list=image_list)



class ConditionCallBack(threading.Thread):
	''' Takes a control -> xbmc.control, a timing (in ms) -> int, and a condition -> str
		Waits the set time, then applies the visible condition to the supplied control. '''

	def __init__(self, **kwargs):

		self.control 	= kwargs.get('control', None)
		self.condition 	= kwargs.get('condition', '')
		self.timing 	= max(kwargs.get('timing', 0), 3000)

		super(ConditionCallBack, self).__init__()


	def run(self):
		
		xbmc.sleep(self.timing)

		try:
			self.control.setVisibleCondition(self.condition)
		except:
			log('ConditionCallBack failed')


class OSMC_Slideshow_GUI(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):

		# images is a list of image path strings
		self.images  	= kwargs.get('images', [])
		self.image_list = None
		self.navi_list  = None
		self.navi_group = None
		
		log(self.images)


	def onInit(self):

		self.image_list = self.getControl(1)
		self.navi_list  = self.getControl(50)
		self.navi_group = self.getControl(555)

		# set the nagi group to visible, revert to waiting for system idle for 3 second condition after 3 seconds
		# self.navi_group.setVisibleCondition('True')

		# ccb = ConditionCallBack(control=self.navi_group, condition='!System.IdleTime(2)|Control.HasFocus(555)', timing=2000)
		# ccb.start()

		# populate the image list as well as the navi list
		for image in self.images:

			li = xbmcgui.ListItem(label=image, label2='', thumbnailImage =image)

			self.image_list.addItem(li)
			self.navi_list.addItem(li)

		if len(self.images) == 1:
			self.setFocusId(999)


	def onAction(self, action):

		actionID = action.getId()

		if (actionID in (10, 92)):

			self.close()

		elif actionID in LEFT_ACTIONS:
			# user is trying to move to the previous image
			pos = xbmc.getInfoLabel('Container(1).Position')

			if pos == 0:
				pass

			else:
				xbmc.executebuiltin('SetFocus(1,%s)' % pos - 1)

		elif actionID in RIGHT_ACTIONS:
			# user is trying to move to the next image
			pos = xbmc.getInfoLabel('Container(1).Position')

			if pos == len(self.images) - 1:
				# end of list
				self.setFocusId(999)

			else:
				xbmc.executebuiltin('SetFocus(1,%s)' % pos + 1)


	def onClick(self, controlID):

		if controlID == 999:

			self.close()

		elif controlID == 1:
			# user has clicked on the image list, cycle to next position, if last position move to exit button

			pos = xbmc.getInfoLabel('Container(1).Position')

			if pos == len(self.images) -1:
				# end of list
				self.setFocusId(999)
				self.navi_group.setVisibleCondition('True')
			
			else:
				xbmc.executebuiltin('SetFocus(1,%s)' % pos + 1)


	def onFocus(self, controlID):

		pass
