# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os

from osmcupdates import service_entry
from osmccommon.osmc_logging import StandardLogger

ADDON_ID = 'script.module.osmcsetting.updates'

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log

if __name__ == "__main__":
    log('Service starting ...')
    service_entry.Main()
    log('Service shutdown.')
