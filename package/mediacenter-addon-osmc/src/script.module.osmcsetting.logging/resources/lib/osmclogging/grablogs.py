#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.logging

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

from osmccommon import grablogs

try:
    import xbmc
    import xbmcaddon
except ImportError:
    xbmc = None
    xbmcaddon = None

if __name__ == "__main__":

    addon_id = "script.module.osmcsetting.logging"

    if not xbmc:
        copy, term_print = grablogs.parse_arguments()
    else:
        copy, term_print = grablogs.retrieve_settings(xbmcaddon.Addon(addon_id))

    if copy is not None:
        main = grablogs.Main(copy, term_print)
        main.launch_process()
