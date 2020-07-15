# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of service.osmc.settings

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import threading

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger

# LEFT ACTIONS
ACTION_MOVE_LEFT = 1
ACTION_PAGE_DOWN = 6
ACTION_MOUSE_WHEEL_DOWN = 105
ACTION_SCROLL_DOWN = 112
ACTION_CHANNEL_DOWN = 185

# RIGHT ACTIONS
ACTION_MOVE_RIGHT = 2
ACTION_PAGE_UP = 5
ACTION_MOUSE_WHEEL_UP = 104
ACTION_SCROLL_UP = 111
ACTION_CHANNEL_UP = 184

LEFT_ACTIONS = [ACTION_MOVE_LEFT, ACTION_PAGE_DOWN, ACTION_MOUSE_WHEEL_DOWN,
                ACTION_SCROLL_DOWN, ACTION_CHANNEL_DOWN]
RIGHT_ACTIONS = [ACTION_MOVE_RIGHT, ACTION_PAGE_UP, ACTION_MOUSE_WHEEL_UP,
                 ACTION_SCROLL_UP, ACTION_CHANNEL_UP]

ADDON_ID = 'service.osmc.settings'
WINDOW = xbmcgui.Window(10000)

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log

"""
=========================
Slideshow JSON STRUCTURE
=========================

{
   "slideshowID": <-- must be unique, will be stored in the settings.xml for the addon
       {
           "version": {'minimum: x.x.xxx, maximum: y.y.yyy},
           "platform": ['pi2', 'vero', 'vero2'],
           "expiry_date" : "31/12/2015",
           "images" : [URL1, URL2, URL3, URL4] <--these should be named so that the files are 
                        in alphabetical order image1.png, image2.png if you like
       },
}
"""


class OSMCSlideshow(object):

    def __init__(self, json_data, addon=None):
        _ = json_data
        self._addon = addon
        self._lang = None
        self._path = ''

    @property
    def addon(self):
        if not self._addon:
            self._addon = xbmcaddon.Addon(ADDON_ID)
        return self._addon

    def lang(self, value):
        if not self._lang:
            retriever = LangRetriever(self.addon)
            self._lang = retriever.lang
        return self._lang(value)

    @property
    def path(self):
        if not self._path:
            self._path = self.addon.getAddonInfo('path')
        return self._path

    def download_images(self):
        pass

    def progress_bar(self):
        pass

    def create_slideshow(self, image_list):
        xml = "osmc_slideshow_720.xml" \
            if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' \
            else "osmc_slideshow.xml"

        return OSMCSlideshowGUI(xml, self.path, 'Default', image_list=image_list, addon=self.addon)


class ConditionCallBack(threading.Thread):
    """ Takes a control -> xbmc.control, a timing (in ms) -> int, and a condition -> str
        Waits the set time, then applies the visible condition to the supplied control. """

    def __init__(self, **kwargs):

        self.control = kwargs.get('control', None)
        self.condition = kwargs.get('condition', '')
        self.timing = max(kwargs.get('timing', 0), 3000)

        super(ConditionCallBack, self).__init__()

    def run(self):
        xbmc.sleep(self.timing)

        try:
            self.control.setVisibleCondition(self.condition)
        except:
            log('ConditionCallBack failed')


class OSMCSlideshowGUI(xbmcgui.WindowXMLDialog):

    def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):
        super(OSMCSlideshowGUI, self).__init__(xmlFilename=strXMLname,
                                               scriptPath=strFallbackPath,
                                               defaultSkin=strDefaultName)
        self._addon = kwargs.get('addon')
        self._lang = None

        # images is a list of image path strings
        self.images = kwargs.get('images', [])
        self.image_list = None
        self.navi_list = None
        self.navi_group = None

        log(self.images)

    @property
    def addon(self):
        if not self._addon:
            self._addon = xbmcaddon.Addon(ADDON_ID)
        return self._addon

    def lang(self, value):
        if not self._lang:
            retriever = LangRetriever(self.addon)
            self._lang = retriever.lang
        return self._lang(value)

    def onInit(self):
        self.image_list = self.getControl(1)
        self.navi_list = self.getControl(50)
        self.navi_group = self.getControl(555)

        # populate the image list as well as the navi list
        for image in self.images:
            li = xbmcgui.ListItem(label=image, label2='', offscreen=True)
            li.setArt({
                'thumb': image
            })

            self.image_list.addItem(li)
            self.navi_list.addItem(li)

        if len(self.images) == 1:
            self.setFocusId(999)

    def onAction(self, action):
        action_id = action.getId()

        if action_id in (10, 92):
            self.close()

        elif action_id in LEFT_ACTIONS:
            # user is trying to move to the previous image
            pos = int(xbmc.getInfoLabel('Container(1).Position'))

            if pos == 0:
                pass

            else:
                xbmc.executebuiltin('SetFocus(1,%d)' % (pos - 1))

        elif action_id in RIGHT_ACTIONS:
            # user is trying to move to the next image
            pos = int(xbmc.getInfoLabel('Container(1).Position'))

            if pos == len(self.images) - 1:
                # end of list
                self.setFocusId(999)

            else:
                xbmc.executebuiltin('SetFocus(1,%d)' % (pos + 1))

    def onClick(self, controlID):
        if controlID == 999:
            self.close()

        elif controlID == 1:
            # user has clicked on the image list, cycle to next position,
            # if last position move to exit button
            pos = int(xbmc.getInfoLabel('Container(1).Position'))

            if pos == len(self.images) - 1:
                # end of list
                self.setFocusId(999)
                self.navi_group.setVisibleCondition('True')

            else:
                xbmc.executebuiltin('SetFocus(1,%s)' % (pos + 1))

    def onFocus(self, controlID):
        pass
