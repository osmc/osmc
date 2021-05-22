#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of service.osmc.settings

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os

import xbmcgui
from osmcsettings import service_entry
from osmccommon.osmc_logging import StandardLogger

ADDON_ID = 'service.osmc.settings'

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log

if __name__ == "__main__":
    log('Service starting ...')
    service_entry.Main(window=xbmcgui.Window(10000))
    log('Service shutdown.')
