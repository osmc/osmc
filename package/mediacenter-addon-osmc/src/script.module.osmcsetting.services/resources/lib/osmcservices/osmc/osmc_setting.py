# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.services

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os

import xbmcgui
from osmccommon import osmc_setting
from osmccommon.osmc_logging import StandardLogger

from ..services_gui import ServiceSelectionGui

addon_id = "script.module.osmcsetting.services"
log = StandardLogger(addon_id, os.path.basename(__file__)).log


class OSMCSettingClass(osmc_setting.OSMCSettingClass):

    def __init__(self):
        super(OSMCSettingClass, self).__init__()

        self.addon_id = addon_id

        self.short_name = 'Services'
        self.short_name_i18n = 32058

        self.description = 'Control OSMC services'
        self.description_i18n = 32059

        self.setting_data_method = {
            'none': {
                'setting_value': '',
            }
        }

        self.reboot_required = False

    def run(self):
        xml = "ServiceBrowser_720OSMC.xml" \
            if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' \
            else "ServiceBrowser_OSMC.xml"

        creation = ServiceSelectionGui(xml, self.me.getAddonInfo('path'), 'Default', addon=self.me)
        creation.doModal()
        del creation
