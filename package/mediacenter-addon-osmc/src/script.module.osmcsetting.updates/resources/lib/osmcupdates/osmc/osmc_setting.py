# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import subprocess

from osmccommon import osmc_setting
from osmccommon.osmc_logging import StandardLogger

addon_id = "script.module.osmcsetting.updates"
log = StandardLogger(addon_id, os.path.basename(__file__)).log


class OSMCSettingClass(osmc_setting.OSMCSettingClass):

    def __init__(self):
        super(OSMCSettingClass, self).__init__()

        self.addon_id = addon_id

        self.short_name = 'Updates'
        self.short_name_i18n = 32060

        self.description = 'Manage updates'
        self.description_i18n = 32061

        self.reset_file = '/home/osmc/.factoryreset'

        self.setting_data_method = {}

        # 'mercury':    {
        #                   'setting_value' : '',
        #                   'apply'         : self.method_to_apply_changes_X,
        #                   'translate'     : self.translate_on_populate_X,
        #                   },

        # 'venus':  {'setting_value' : ''},
        # 'earth':  {'setting_value' : ''},
        # 'mars':   {'setting_value' : ''},
        # 'jupiter':    {'setting_value' : ''},
        # 'saturn':     {'setting_value' : ''},
        # 'uranus':     {'setting_value' : ''},
        # 'neptune':    {'setting_value' : ''},
        # 'pluto':  {'setting_value' : ''},

        # }

        self.populate_setting_data_method()

        self.reboot_required = False

        log('START')
        for x, k in self.setting_data_method.items():
            log("%s = %s" % (x, k.get('setting_value', 'no setting value')))

    def populate_setting_data_method(self):
        # this is the method to use if you are populating the dict from the settings.xml
        latest_settings = self.settings_retriever_xml()

        # cycle through the setting_data_method dict, and populate with the settings values
        for key in self.setting_data_method.keys():

            # grab the translate method (if there is one)
            translate_method = self.setting_data_method.get(key, {}).get('translate', {})

            # get the setting value, translate it if needed
            if translate_method:
                setting_value = translate_method(latest_settings[key])
            else:
                setting_value = latest_settings[key]

            # add it to the dictionary
            self.setting_data_method[key]['setting_value'] = setting_value

    def run(self):
        # check if kodi_reset file is present, if it is then set the bool as true, else set as false

        if os.path.isfile(self.reset_file):
            log('Kodi reset file found')
            self.me.setSetting('kodi_reset', 'true')
        else:
            log('Kodi reset file not found')
            self.me.setSetting('kodi_reset', 'false')

        self.me.openSettings()

        # check the kodi reset setting, if it is true then create the kodi_reset file, otherwise remove that file
        if self.me.getSetting('kodi_reset') == 'true':
            log('creating kodi reset file')
            subprocess.call(['sudo', 'touch', self.reset_file])
        else:
            subprocess.call(['sudo', 'rm', self.reset_file])

        log('END')
        for x, k in self.setting_data_method.items():
            log("%s = %s" % (x, k.get('setting_value', 'no setting value')))

    def settings_retriever_xml(self):
        latest_settings = {}

        for key in self.setting_data_method.keys():
            latest_settings[key] = self.me.getSetting(key)

        return latest_settings

    def method_to_apply_changes_X(self, data):

        """
            Method for implementing changes to setting x.

        """

        log('method_to_apply_changes_X')

    def translate_on_populate_X(self, data, reverse=False):

        """
            Method to translate the data before adding to the setting_data_method dict.

            This is useful if you are getting the populating from an external source like the Pi's config.txt.
            This method could end with a call to another method to populate the settings.xml from that same source.
        """

        # this is how you would negate the translating of the data when the settings window closes.
        if reverse:
            return data
