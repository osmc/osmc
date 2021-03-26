# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.apfstore

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import json
import os
import socket
import sys
from contextlib import closing

import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger
from osmccommon.osmc_logging import clog

ADDON_ID = "script.module.osmcsetting.apfstore"
PY3 = sys.version_info.major == 3

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class APFGui(xbmcgui.WindowXMLDialog):

    def __init__(self, strXMLname, strFallbackPath, strDefaultName, apf_dict, addon=None):
        super(APFGui, self).__init__(xmlFilename=strXMLname,
                                     scriptPath=strFallbackPath,
                                     defaultSkin=strDefaultName)
        self.apf_dict = apf_dict

        self._addon = addon
        self._lang = None
        self._path = ''

        self.list_control = None
        self.addon_gui = None

        self.apf_order_list = []

        self.action_dict = {}

    def onInit(self):

        self.list_control = self.getControl(500)
        self.list_control.setVisible(True)

        for key, value in self.apf_dict.items():
            self.list_control.addItem(value)
            self.apf_order_list.append(key)

        try:
            self.getControl(50).setVisible(False)
        except:
            pass

        self.check_action_dict()

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

    @clog(logger=log)
    def check_action_dict(self):

        install = 0
        removal = 0

        for _, value in self.action_dict.items():
            if value == 'Install':
                install += 1

            elif value == 'Uninstall':
                removal += 1

        if not install and not removal:
            self.getControl(6).setVisible(False)
            self.getControl(61).setVisible(False)
            self.getControl(62).setVisible(False)
            return

        if install:
            self.getControl(61).setLabel(self.lang(32001) % install)
            self.getControl(6).setVisible(True)
            self.getControl(61).setVisible(True)

        else:
            self.getControl(61).setVisible(False)

        if removal:
            self.getControl(62).setLabel(self.lang(32002) % removal)
            self.getControl(6).setVisible(True)
            self.getControl(62).setVisible(True)

        else:
            self.getControl(62).setVisible(False)

    @clog(logger=log)
    def onClick(self, controlID):

        if controlID == 500:
            container = self.getControl(500)

            sel_pos = container.getSelectedPosition()
            sel_item = self.apf_dict[self.apf_order_list[sel_pos]]

            xml = "APFAddonInfo_720OSMC.xml" \
                if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' \
                else "APFAddonInfo_OSMC.xml"

            self.addon_gui = AddonInfoGui(xml, self.path, 'Default',
                                          sel_item=sel_item, addon=self.addon)
            self.addon_gui.doModal()

            ending_action = self.addon_gui.action

            if ending_action == 'Install':
                self.action_dict[sel_item.id] = 'Install'

            elif ending_action == 'Uninstall':
                self.action_dict[sel_item.id] = 'Uninstall'

            elif sel_item.id in self.action_dict:
                del self.action_dict[sel_item.id]

            self.check_action_dict()
            del self.addon_gui
            log(self.action_dict)

        elif controlID == 7:
            self.close()

        elif controlID == 6:
            # send install and removal list to Update Service
            action_list = [
                'install_' + k
                if v == 'Install'
                else 'removal_' + k
                for k, v in self.action_dict.items()
            ]
            action_string = '|=|'.join(action_list)

            self.contact_update_service(action_string)
            self.close()

    @clog(logger=log)
    def contact_update_service(self, action_string):
        message = ('action_list', {
            'action': action_string
        })

        message = json.dumps(message)

        with closing(socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)) as open_socket:
            open_socket.connect('/var/tmp/osmc.settings.update.sockfile')
            if PY3 and not isinstance(message, (bytes, bytearray)):
                message = message.encode('utf-8', 'ignore')
            open_socket.sendall(message)


class AddonInfoGui(xbmcgui.WindowXMLDialog):
    """
    Controls
    ==============================
    50001	Shortdesc
    50002	Longdesc
    50003	Version
    50004	Maintainer
    50005	LastUpdated
    50006	Icon
    50007	Name
    """

    def __init__(self, strXMLname, strFallbackPath, strDefaultName, sel_item, addon=None):
        super(AddonInfoGui, self).__init__(xmlFilename=strXMLname,
                                           scriptPath=strFallbackPath,
                                           defaultSkin=strDefaultName)

        self._addon = addon
        self._lang = None
        self.action = False
        self.sel_item = sel_item

    def onInit(self):
        self.getControl(50001).setLabel(self.sel_item.shortdesc)
        self.getControl(50002).setText(self.sel_item.longdesc)
        self.getControl(50003).setLabel(self.sel_item.version)
        self.getControl(50004).setLabel(self.sel_item.maintainedby)
        self.getControl(50005).setLabel(self.sel_item.lastupdated)
        self.getControl(50006).setImage(self.sel_item.current_icon, True)
        self.getControl(50007).setLabel(self.sel_item.name)

        if self.sel_item.installed:
            self.getControl(6).setLabel(self.lang(32004))

        else:
            self.getControl(6).setLabel(self.lang(32003))

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

    def onClick(self, controlID):
        if controlID == 6:
            lbl = self.getControl(6).getLabel()

            if lbl == self.lang(32003):
                self.action = 'Install'
            else:
                self.action = 'Uninstall'

            self.close()

        elif controlID == 7:
            self.close()
