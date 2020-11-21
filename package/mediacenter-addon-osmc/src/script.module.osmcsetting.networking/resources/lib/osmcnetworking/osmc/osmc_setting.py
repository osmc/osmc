# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.networking

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import sys

import xbmcaddon
import xbmcgui
from osmccommon import osmc_setting
from osmccommon.osmc_logging import StandardLogger

from .. import osmc_network
from ..networking_gui import NetworkingGui
from ..osmc_advset_editor import AdvancedSettingsEditor

addon_id = "script.module.osmcsetting.networking"

DIALOG = xbmcgui.Dialog()
PY2 = sys.version_info.major == 2

log = StandardLogger(addon_id, os.path.basename(__file__)).log


class OSMCSettingClass(osmc_setting.OSMCSettingClass):

    def __init__(self):
        super(OSMCSettingClass, self).__init__()

        self.addon_id = addon_id

        self.short_name = 'Network'
        self.short_name_i18n = 32052

        self.description = 'This is network settings, it contains settings for the network.'
        self.description_i18n = 32053

        self.setting_data_method = {}

        # populate the settings data in the setting_data_method
        self.populate_setting_data_method()

        # create the advanced settings reader to determine if Wait_for_Network should be activated
        self.ASE = AdvancedSettingsEditor(log)

        # read advancedsettings.xml and convert it into a dictionary
        advset_dict = self.ASE.parse_advanced_settings()

        # check whether the advanced settings dict contains valid MySQL information
        valid_advset_dict, _ = self.ASE.validate_advset_dict(advset_dict,
                                                             reject_empty=True,
                                                             exclude_name=True)

        # when a valid MySQL advanced settings file is found, toggle the
        # Wait_for_Network setting to ON
        if valid_advset_dict:
            # only proceed if the (either) server is not on the localhost
            if self.ASE.server_not_localhost(advset_dict):
                # confirm that wait_for_network is not already enabled
                if not osmc_network.is_connman_wait_for_network_enabled():
                    undo_change = DIALOG.yesno('MyOSMC', self.lang(32078),
                                               nolabel=self.lang(32080),
                                               yeslabel=self.lang(32079),
                                               autoclose=10000)

                    if not undo_change:
                        osmc_network.toggle_wait_for_network(True)

        # a flag to determine whether a setting change requires a reboot to take effect
        self.reboot_required = False

        self.gui = None

        log('START')
        for x, k in self.setting_data_method.items():
            log("%s = %s" % (x, k.get('setting_value', 'no setting value')))

    def populate_setting_data_method(self):
        # this is the method to use if you are populating the dict from the settings.xml
        latest_settings = self.settings_retriever_xml()

        # cycle through the setting_data_method dict, and populate with the settings values
        for key in self.setting_data_method.keys():

            # grab the translate method (if there is one)
            translate_method = self.setting_data_method.get(key, {}).get('translate', None)

            # get the setting value, translate it if needed
            setting_value = latest_settings[key]
            if translate_method:
                setting_value = translate_method(latest_settings[key])

            # add it to the dictionary
            self.setting_data_method[key]['setting_value'] = setting_value

    def run(self, use_preseed=False):
        log(self.addon_id)

        xml = "network_gui_720.xml" \
            if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' \
            else "network_gui.xml"

        self.gui = NetworkingGui(xml, self.me.getAddonInfo('path'), 'Default', addon=self.me)
        self.gui.set_use_preseed(use_preseed)
        self.gui.doModal()

        del self.gui
        log('END')

    def apply_settings(self):
        # retrieve the current settings from the settings.xml
        # (this is where the user has made changes)
        new_settings = self.settings_retriever_xml()

        self.first_method()

        # apply the individual settings changes
        for k, v in self.setting_data_method.items():

            # get the application method and stored setting value from the dictionary
            method = v.get('apply', False)
            value = v.get('setting_value', '')

            # if the new setting is different to the stored setting then change the
            # dict and run the 'apply' method
            if new_settings[k] != value:

                # change stored setting_value to the new value
                self.setting_data_method[k]['setting_value'] = new_settings[k]

                # if a specific apply method exists for the setting, then call that
                try:
                    method(new_settings[k])
                except:
                    pass

        self.final_method()

    def settings_retriever_xml(self):
        latest_settings = {}

        addon = xbmcaddon.Addon(self.addon_id)

        for key in self.setting_data_method.keys():
            latest_settings[key] = addon.getSetting(key)

        return latest_settings

    @staticmethod
    def check_network(online):
        return osmc_network.has_network_connection(online)

    @staticmethod
    def is_ftr_running():
        return osmc_network.is_ftr_running()

    @staticmethod
    def method_to_apply_changes_X(data):
        """
            Method for implementing changes to setting x.
        """
        log('method_to_apply_changes_X ' + data)

    @staticmethod
    def translate_on_populate_X(data, reverse=False):
        """
            Method to translate the data before adding to the setting_data_method dict.

            This is useful if you are getting the populating from an external source like
            the Pi's config.txt.
            This method could end with a call to another method to populate the settings.xml
            from that same source.
        """
        # this is how you would negate the translating of the data when the settings window closes.
        if reverse:
            return data
