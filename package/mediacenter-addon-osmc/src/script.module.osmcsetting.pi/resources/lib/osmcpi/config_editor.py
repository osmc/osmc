# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.pi

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import subprocess
import sys
import time
from io import open

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger

ACTION_PREVIOUS_MENU = 10
ACTION_NAV_BACK = 92
SAVE = 5
HEADING = 1
ACTION_SELECT_ITEM = 7

ADDON_ID = "script.module.osmcsetting.pi"
DIALOG = xbmcgui.Dialog()
PY2 = sys.version_info.major == 2

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class ConfigEditorGui(xbmcgui.WindowXMLDialog):
    def __init__(self, strXMLname, strFallbackPath, strDefaultName, addon=None):
        super(ConfigEditorGui, self).__init__(xmlFilename=strXMLname,
                                              scriptPath=strFallbackPath,
                                              defaultSkin=strDefaultName)
        self._addon = addon
        self._lang = None

        self.ignore_list = ['dtoverlay', 'device_tree', 'device_tree_param',
                            'device_tree_overlay', 'dtparam']
        self.del_string = ' [' + self.lang(32056) + ']'

        self.config = '/boot/config.txt'

        self.list_control = None
        self.changed = False
        self.item_count = 0

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
        # give the settings enough time to be saved to the config.txt
        xbmc.sleep(150)

        # list of settings that are ignored in the duplicate check
        with open(self.config, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        log('lines = %s' % lines)

        lines = [line.replace('\n', '') for line in lines if line not in ['\n', '', '\t']]

        # Save button
        ok_control = self.getControl(SAVE)
        ok_control.setLabel(self.lang(32050))

        # Heading
        heading_control = self.getControl(HEADING)
        heading_control.setLabel(self.lang(32051))
        heading_control.setVisible(True)

        # Hide unused list frame
        list_frame = self.getControl(3)
        list_frame.setVisible(False)

        # Populate the list frame
        self.list_control = self.getControl(6)
        self.list_control.setEnabled(True)

        items = [self.lang(32052)]
        items.extend(lines)

        self.item_count = len(items)

        # Start the window with the first item highlighted
        # self.list_control.getListItem(0).select(True)

        # Set action when clicking right from the Save button
        ok_control.controlRight(self.list_control)

        for item in items:
            # populate the random list
            list_item = xbmcgui.ListItem(item, offscreen=True)
            self.list_control.addItem(list_item)

        self.changed = False
        self.setFocus(self.list_control)

        # check for duplications, warn the user if there are duplicates
        dup_check = [y for y in
                     [x.split('=')[0] for x in self.grab_item_strings() if '=' in x]
                     if y not in self.ignore_list]

        if len(dup_check) != len(set(dup_check)):
            _ = DIALOG.ok(self.lang(32051), '[CR]'.join([self.lang(32065), self.lang(32066)]))

    def onAction(self, action):

        action_id = action.getId()
        if action_id in (ACTION_PREVIOUS_MENU, ACTION_NAV_BACK):
            log('CLOSE')
            self.close()

    def onClick(self, controlID):
        log('%s' % controlID)

        if controlID == SAVE:
            log('SAVE')

            if self.changed:
                final_action = DIALOG.yesno(self.lang(32052), self.lang(32053),
                                            nolabel=self.lang(32054), yeslabel=self.lang(32055))

                if final_action:
                    log('final action')

                    new_config = self.grab_item_strings()

                    # temporary location for the config.txt
                    tmp_loc = '/var/tmp/config.txt'

                    # write the long_string_file to the config.txt
                    with open(tmp_loc, 'w', encoding='utf-8') as open_file:
                        for line in new_config:
                            _line = line.replace(" = ", "=") + '\n'
                            if PY2 and isinstance(_line, str):
                                _line = _line.decode('utf-8')
                            open_file.write(_line)
                            log('' + _line)

                    # backup existing config
                    suffix = '_' + str(time.time()).split('.')[0]
                    subprocess.call(["sudo", "cp", self.config, '/home/pi/'])
                    subprocess.call(["sudo", "mv", '/home/pi/config.txt',
                                     '/home/pi/config' + suffix + '.txt'])

                    # copy over the temp config.txt to /boot/ as superuser
                    subprocess.call(["sudo", "mv", tmp_loc, self.config])

                    # THIS IS JUST FOR TESTING, LAPTOP DOESNT LIKE SUDO HERE
                    try:
                        subprocess.call(["mv", tmp_loc, self.config])
                    except:
                        pass

                    log('writing ended')

            self.close()

        else:
            selected_entry = self.list_control.getSelectedPosition()
            item = self.list_control.getSelectedItem()
            current_label = item.getLabel()

            if selected_entry != 0:

                if self.del_string not in current_label:
                    action = DIALOG.yesno(self.lang(32051), self.lang(32057),
                                          nolabel=self.lang(32058), yeslabel=self.lang(32059))

                    if action:
                        # delete
                        item.setLabel(current_label + self.del_string)
                        self.changed = True

                    else:
                        # edit
                        result = DIALOG.input(self.lang(32060), current_label,
                                              type=xbmcgui.INPUT_ALPHANUM)

                        if result:
                            self.check_for_duplicates(result, True)

                            item.setLabel(result)
                            self.changed = True

                else:
                    action = DIALOG.yesno(self.lang(32051), self.lang(32061),
                                          nolabel=self.lang(32058), yeslabel=self.lang(32062))

                    if action:
                        # delete
                        item.setLabel(current_label[:len(current_label) - len(self.del_string)])
                        self.changed = True

                    else:
                        # edit
                        result = DIALOG.input(self.lang(32063), current_label,
                                              type=xbmcgui.INPUT_ALPHANUM)

                        if result:
                            self.check_for_duplicates(result, edit=True)

                            item.setLabel(result)
                            self.changed = True

            else:
                result = DIALOG.input(self.lang(32064), type=xbmcgui.INPUT_ALPHANUM)

                if result:
                    self.check_for_duplicates(result)

                    # add the new item to the list
                    list_item = xbmcgui.ListItem(result, offscreen=True)
                    self.list_control.addItem(list_item)

                    self.changed = True

                    self.item_count += 1

    def check_for_duplicates(self, d, edit=False):
        if '=' in d:
            dupe_check_raw = self.grab_item_strings()
            dupe_check = [x.split('=')[0] for x in dupe_check_raw]

            dupe = d.split('=')[0]

            if (dupe not in self.ignore_list and
                    ((edit and dupe_check.count(dupe) > 1) or (not edit and dupe in dupe_check))):
                _ = DIALOG.ok(self.lang(32051), '[CR]'.join([self.lang(32067), self.lang(32066)]))

    def grab_item_strings(self):
        new_config = []

        for i in range(self.item_count):
            if i == 0:
                continue

            item = self.list_control.getListItem(i)

            current_label = item.getLabel()

            if self.del_string not in current_label:
                new_config.append(current_label)

        return new_config


if __name__ == "__main__":
    log('OPEN')

    _addon = xbmcaddon.Addon(ADDON_ID)
    gui = ConfigEditorGui("DialogSelect.xml", _addon.getAddonInfo('path'), 'Default', addon=_addon)
    gui.doModal()
    del gui

    log('CLOSED')
    xbmc.sleep(150)
