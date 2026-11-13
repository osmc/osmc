# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2026 OSMC (KodeKarnage)

    This file is part of script.module.osmccommon

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import subprocess


def set_unit_enabled(unit, enabled):
    """
        Enable or disable a systemd unit without sudo, through osmc-enable@.service or
        osmc-disable@.service from base-files-osmc. They accept the units named in
        /etc/osmc/apps.d, plus connman-wait-for-network.service; anything else fails, and
        'journalctl -u osmc-enable@*' says why. Pass the full unit name ('ssh.service', not
        'ssh'). This does not start or stop the unit. Returns whether it succeeded.
    """
    instance = subprocess.check_output(['systemd-escape', '--', unit],
                                       universal_newlines=True).strip()
    template = 'osmc-enable' if enabled else 'osmc-disable'

    return subprocess.call(['/bin/systemctl', 'start',
                            '%s@%s.service' % (template, instance)]) == 0
