# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.apfstore

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import hashlib
import os
from io import open

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger
from osmccommon.osmc_logging import clog

ADDON_ID = "script.module.osmcsetting.apfstore"
ADDON_DATA = xbmc.translatePath('special://userdata/addon_data/%s/' % ADDON_ID)

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log

"""
=========================
APF JSON STRUCTURE
=========================

{
   "application": [
       {
           "id": "ssh-app-osmc",
           "name": "SSH Server",
           "shortdesc": "This allows you to connect to your OSMC device via SSH",
           "longdesc": "This installs an OpenSSH server on your OSMC device allowing you to log in to your device remotely as well as transfer files via SCP.",
           "maintained-by": "OSMC",
           "version": "1.0.0",
           "lastupdated": "2015-01-23",
       }
   ]
}
"""


class APFListItem(xbmcgui.ListItem):

    def __init__(self):
        super(APFListItem, self).__init__(offscreen=True)

        self._addon = None
        self._lang = None
        self._skin_media_folder = None

        self.id = 'none'
        self.name = 'none'
        self.shortdesc = ''
        self.longdesc = ''
        self.maintainedby = ''
        self.version = ''
        self.lastupdated = ''
        self.iconurl = '/none'
        self.iconhash = 0
        self.retrieve_icon = False
        self.current_icon = self.iconurl

        self.installed = False

    def populate(self, data):
        self.id = data.get('id', 'none')
        self.name = data.get('name', 'none')
        self.shortdesc = data.get('shortdesc', '')
        self.longdesc = data.get('longdesc', '')
        self.maintainedby = data.get('maintained-by', '')
        self.version = data.get('version', '')
        self.lastupdated = data.get('lastupdated', '')
        self.iconurl = data.get('iconurl', '/none')
        self.iconhash = data.get('iconhash', 0)
        self.retrieve_icon = False
        self.current_icon = self.check_icon(self.iconurl)

        self._addon = data.get('addon', None)

        self.installed = False

        self.setLabel(self.name)
        self.setProperty('Addon.Description', self.longdesc)
        self.setProperty('Addon.Creator', self.maintainedby)
        self.setProperty('Addon.Name', self.name)
        self.setProperty('Addon.Version', self.version)
        self.setArt({
            'icon': self.current_icon
        })

        return self

    @property
    def addon(self):
        if not self._addon:
            self._addon = xbmcaddon.Addon(ADDON_ID)
        return self._addon

    @property
    def skin_media_folder(self):
        if not self._skin_media_folder:
            self._skin_media_folder = os.path.join(self.addon.getAddonInfo('path'),
                                                   'resources', 'skins', 'Default', 'media')
        return self._skin_media_folder

    def lang(self, value):
        if not self._lang:
            retriever = LangRetriever(self.addon)
            self._lang = retriever.lang
        return self._lang(value)

    def set_installed(self, status):
        if status is True:
            self.installed = True
            self.setLabel2(self.lang(32005))

    def refresh_icon(self):
        self.current_icon = self.check_icon(self.iconurl)
        self.setArt({
            'icon': self.current_icon
        })

    @clog(logger=log)
    def check_icon(self, icon_url):
        """ Checks the addon data folder for the icon,
                if not found, check for an icon stored in the addon/media folder,
                    if not found there, mark the new icon for download by Main in a different thread,
                    use default substitute in the meantime
                if found, check the hash matches, if not mark new icon for download by Main in another thread,
                    use existing in meantime
        """

        if self.iconhash == 'NA':
            return os.path.join(self.skin_media_folder, 'osmc_osmclogo.png')

        icon_name = icon_url.split('/')[-1]

        if os.path.isfile(os.path.join(ADDON_DATA, icon_name)):
            # check userdata folder

            current_icon = os.path.join(ADDON_DATA, icon_name)

        elif os.path.isfile(os.path.join(self.skin_media_folder, icon_name)):
            # check addon art folder

            current_icon = os.path.join(self.skin_media_folder, icon_name)

        else:

            current_icon = os.path.join(self.skin_media_folder, 'osmc_osmclogo.png')

        log('current icon = %s' % current_icon)

        # get the hash
        with open(current_icon, 'rb') as open_file:
            icon_image = open_file.read()

        icon_hash = hashlib.md5(icon_image).hexdigest()

        if icon_hash != self.iconhash:
            self.retrieve_icon = True

        return current_icon
