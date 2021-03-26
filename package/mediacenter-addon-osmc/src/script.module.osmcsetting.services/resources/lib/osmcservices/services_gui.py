# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.services

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import subprocess
from collections import OrderedDict
from io import open

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger
from osmccommon.osmc_logging import clog

ACTION_PREVIOUS_MENU = 10
ACTION_NAV_BACK = 92
SAVE = 5
HEADING = 1
ACTION_SELECT_ITEM = 7

ADDON_ID = "script.module.osmcsetting.services"

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class MaitreD(object):

    def __init__(self, addon=None):
        self._addon = addon
        self._lang = None

        self.services = {}

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

    @clog(log)
    def enable_service(self, s_entry):
        for service in s_entry:
            os.system("sudo /bin/systemctl enable " + service)
            os.system("sudo /bin/systemctl start " + service)

    @clog(log)
    def disable_service(self, s_entry):
        for service in s_entry:
            os.system("sudo /bin/systemctl disable " + service)
            os.system("sudo /bin/systemctl stop " + service)

    @clog(log)
    def is_running(self, s_entry):
        for service in s_entry:
            p = subprocess.call(["sudo", "/bin/systemctl", "is-active", service])

            if p != 0:
                return False

        return True

    @clog(log)
    def is_enabled(self, s_entry):
        for service in s_entry:
            p = subprocess.call(["sudo", "/bin/systemctl", "is-enabled", service])

            if p != 0:
                # if a single service in the s_entry is not enabled, then return false
                return False

        return True

    @clog(log, maxlength=500)
    def discover_services(self):
        """ Returns a dict of service tuples. {s_name: (entry, service_name, running, enabled)}
            s_name is the name shown in the GUI
            s_entry is the actual service name used to enabling and running, this is a LIST to 
            handle bundled services """

        svcs = {}

        for service_name in os.listdir("/etc/osmc/apps.d"):
            service_name = service_name.replace('\n', '')

            if os.path.isfile("/etc/osmc/apps.d/" + service_name):

                with open("/etc/osmc/apps.d/" + service_name, encoding='utf-8') as open_file:
                    lines = open_file.readlines()

                s_name = lines.pop(0).replace('\n', '')
                s_entry = [line.replace('\n', '') for line in lines]

                log("MaitreD: Service Friendly Name: %s" % s_name)
                log("MaitreD: Service Entry Point(s): %s" % s_entry)

                enabled = self.is_enabled(s_entry)
                run_check = self.is_running(s_entry)

                if run_check:
                    running = self.lang(32003)
                else:
                    running = self.lang(32005)

                svcs[s_name] = (s_entry, service_name, running, enabled)

        # this last part creates a dictionary ordered by the services friendly name
        self.services = OrderedDict()
        for key in sorted(svcs.keys()):
            self.services[key] = svcs[key]

        return self.services

    @clog(log, maxlength=250)
    def process_user_changes(self, initiants, finitiants):
        """ User selection is a list of tuples (s_name::str, service_name::list, enable::bool) """

        for clean_name, s_entry in initiants:
            self.enable_service(s_entry)

        for clean_name, s_entry in finitiants:
            self.disable_service(s_entry)


class ServiceSelectionGui(xbmcgui.WindowXMLDialog):

    def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):
        super(ServiceSelectionGui, self).__init__(xmlFilename=strXMLname,
                                                  scriptPath=strFallbackPath,
                                                  defaultSkin=strDefaultName)

        self._addon = kwargs.get('addon')
        self._lang = None

        self.name_list = None

        # the lists to hold the services to start, and to finish
        self.initiants = []
        self.finitiants = []

        self.garcon = MaitreD(self.addon)
        self.service_list = self.garcon.discover_services()

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

    @staticmethod
    def dummy():
        print('logger not provided')

    def onInit(self):
        # Populate the list frame
        self.name_list = self.getControl(500)
        self.name_list.setEnabled(True)

        item_pos = 0

        for s_name, service_tup in self.service_list.items():
            # populate the list
            s_entry, service_name, running, enabled = service_tup

            if running != self.lang(32003):
                if enabled is True:
                    sundry = self.lang(32004)
                else:
                    sundry = self.lang(32005)

            else:
                sundry = self.lang(32003)

            sub_label = ','.join(s_entry)

            list_item = xbmcgui.ListItem(label=s_name + sundry, label2=sub_label, offscreen=True)
            self.name_list.addItem(list_item)

            item_pos += 1

        self.setFocus(self.name_list)

    def onAction(self, action):
        action_id = action.getId()
        if action_id in (ACTION_PREVIOUS_MENU, ACTION_NAV_BACK):
            self.close()

    def onClick(self, controlID):
        if controlID == 7:
            self.close()

        elif controlID == 6:
            self.process()
            self.close()

        elif controlID == 500:
            selected_item = self.name_list.getSelectedItem().getLabel()
            # s_entry is a comma separated string of the services, this changes it into a list
            s_entry = self.name_list.getSelectedItem().getLabel2().split(',')
            clean_name = selected_item.replace(self.lang(32003), '')
            clean_name = clean_name.replace(self.lang(32004), '')
            clean_name = clean_name.replace(self.lang(32005), '')
            clean_name = clean_name.replace('\n', '')

            item_tup = (clean_name, s_entry)

            if (selected_item.endswith(self.lang(32003)) or
                    selected_item.endswith(self.lang(32004))):
                # if the item is currently enabled or running
                if item_tup in self.finitiants:
                    self.finitiants.remove(item_tup)
                else:
                    self.finitiants.append(item_tup)

            else:
                # if the item is not enabled
                if item_tup in self.initiants:
                    self.initiants.remove(item_tup)
                else:
                    self.initiants.append(item_tup)

            xbmc.sleep(10)

            self.update_checklist()

    def update_checklist(self):
        if not self.initiants and not self.finitiants:
            self.getControl(1102).setText(' ')
            return

        todo_list = '-= Pending Actions =-\n\n'

        if self.initiants:
            todo_list += "Enable:\n   - " + '\n   - '.join([x[0] for x in self.initiants])

            if self.finitiants:
                todo_list += '\n\n'

        if self.finitiants:
            todo_list += "Disable:\n   - " + '\n   - '.join([x[0] for x in self.finitiants])

        self.getControl(1102).setText(todo_list)

    def process(self):
        self.garcon.process_user_changes(self.initiants, self.finitiants)
