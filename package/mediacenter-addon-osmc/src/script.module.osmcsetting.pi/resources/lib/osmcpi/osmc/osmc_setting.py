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
import traceback

import xbmcaddon
import xbmcgui
from osmccommon import osmc_setting
from osmccommon.osmc_logging import StandardLogger

from .. import osmc_reparser

ADDON_ID = "script.module.osmcsetting.pi"
DIALOG = xbmcgui.Dialog()
PY3 = sys.version_info.major == 3

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class OSMCSettingClass(osmc_setting.OSMCSettingClass):

    def __init__(self):
        super(OSMCSettingClass, self).__init__()

        self.addon_id = ADDON_ID

        self.short_name = 'Pi Config'
        self.short_name_i18n = 32054

        self.description = 'The Raspberry Pi doesn\'t have a conventional BIOS. ' \
                           'System configuration parameters are stored in a "config.txt" file. ' \
                           'For more detail, visit http://elinux.org/RPiconfig[CR]This settings ' \
                           'module allows you to edit your config.txt from within OSMC using a ' \
                           'graphical interface.[CR][CR]The module includes:' \
                           '[CR]- display rotation[CR]- hdmi_safe & hdmi_boost' \
                           '[CR]- hdmi_group & hdmi_mode[CR]- function to save edid to file[CR]' \
                           '- sdtv_mode & sdtv_aspect[CR]- GPU memory split[CR]' \
                           '- MPG2 & WVC1 licences (including status)[CR]' \
                           '- your Pi\'s serial number[CR][CR]Finally, there is a Config Editor ' \
                           'that will allow you to quickly add, edit, or delete lines in your ' \
                           'config.txt.[CR][CR]Overclock settings are set using the ' \
                           'Pi Overclock module.'
        self.description_i18n = 32055

        self.config_location = '/boot/config.txt'
        self.populate_misc_info()

        try:
            self.clean_user_config()
        except Exception:

            log('Error cleaning users config')
            log(traceback.format_exc())

    def run(self):
        # read the config.txt file every time the settings are opened. This is unavoidable because
        # it is possible for the user to have made manual changes to the config.txt while
        # OSG is active.
        config = osmc_reparser.read_config_file(self.config_location)

        extracted_settings = osmc_reparser.config_to_kodi(osmc_reparser.MASTER_SETTINGS, config)

        # load the settings into kodi
        log('Settings extracted from the config.txt')
        for k, v in extracted_settings.items():
            log("%s : %s" % (k, v))
            self.me.setSetting(k, str(v))

        # open the settings GUI and let the user monkey about with the controls
        self.me.openSettings()

        # retrieve the new settings from kodi
        new_settings = self.settings_retriever_xml()

        log('New settings applied to the config.txt')
        for k, v in new_settings.items():
            log("%s : %s" % (k, v))

        # read the config into a list of lines again
        config = osmc_reparser.read_config_file(self.config_location)

        # construct the new set of config lines using the protocols and the new settings
        new_settings = osmc_reparser.kodi_to_config(osmc_reparser.MASTER_SETTINGS,
                                                    config, new_settings)

        # write the new lines to the temporary config file
        osmc_reparser.write_config_file('/var/tmp/config.txt', new_settings)

        # copy over the temp config.txt to /boot/ as superuser
        subprocess.call(["sudo", "mv", '/var/tmp/config.txt', self.config_location])

        DIALOG.notification(self.lang(32095), self.lang(32096))

    def settings_retriever_xml(self):
        latest_settings = {}

        addon = xbmcaddon.Addon(self.addon_id)

        for key in osmc_reparser.MASTER_SETTINGS.keys():
            latest_settings[key] = addon.getSetting(key)

        return latest_settings

    def populate_misc_info(self):
        # grab the Pi serial number and check to see whether the codec licences are enabled
        mpg = subprocess.check_output(["/opt/vc/bin/vcgencmd", "codec_enabled", "MPG2"])
        wvc = subprocess.check_output(["/opt/vc/bin/vcgencmd", "codec_enabled", "WVC1"])
        serial_raw = subprocess.check_output(["cat", "/proc/cpuinfo"])

        if PY3:
            if isinstance(mpg, (bytes, bytearray)):
                mpg = mpg.decode('utf-8', 'ignore')
            if isinstance(wvc, (bytes, bytearray)):
                wvc = wvc.decode('utf-8', 'ignore')
            if isinstance(serial_raw, (bytes, bytearray)):
                serial_raw = serial_raw.decode('utf-8', 'ignore')

        # grab just the serial number
        serial = serial_raw[serial_raw.index('Serial') + len('Serial'):].replace('\n', '') \
            .replace(':', '').replace(' ', '').replace('\t', '')

        # load the values into the settings gui
        self.me.setSetting('codec_check', mpg.replace('\n', '') + ', ' + wvc.replace('\n', ''))
        self.me.setSetting('serial', serial)

    def clean_user_config(self):
        """ Comment out problematic lines in the users config.txt """
        patterns = [
            r".*=.*\[remove\].*",
            r".*=remove",
        ]

        config = osmc_reparser.read_config_file(self.config_location)
        new_config = osmc_reparser.clean_config(config, patterns)

        # write the new lines to the temporary config file
        osmc_reparser.write_config_file('/var/tmp/config.txt', new_config)

        # copy over the temp config.txt to /boot/ as superuser
        subprocess.call(["sudo", "mv", '/var/tmp/config.txt', self.config_location])
