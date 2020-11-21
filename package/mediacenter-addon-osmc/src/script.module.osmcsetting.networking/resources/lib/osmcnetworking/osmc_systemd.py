# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.networking

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import subprocess
import time


def is_service_running(service_name):
    enabled = is_service_enabled(service_name)
    active = is_service_active(service_name)
    return enabled and active


def toggle_service(service_name, enable):
    if enable:
        update_service(service_name, 'enable')
        update_service(service_name, 'start')
    else:
        update_service(service_name, 'disable')
        update_service(service_name, 'stop')


def is_service_enabled(service_name):
    with open(os.devnull, 'w') as fnull:
        process = subprocess.call(['/bin/systemctl', 'is-enabled', service_name],
                                  stderr=fnull, stdout=fnull)

    if process == 0:
        return True

    return False


def is_service_active(service_name):
    with open(os.devnull, 'w') as fnull:
        process = subprocess.call(['/bin/systemctl', 'is-active', service_name],
                                  stderr=fnull, stdout=fnull)

    if process == 0:
        return True

    return False


def update_service(service_name, service_status):
    with open(os.devnull, 'w') as fnull:
        subprocess.call(['sudo', '/bin/systemctl', service_status, service_name],
                        stderr=fnull, stdout=fnull)

    time.sleep(1)


'''
def is_service_running(service_name):
    with open(os.devnull, 'w') as fnull:
        process = subprocess.call(['/bin/systemctl', 'status', service_name],
                                  stderr=fnull, stdout=fnull)

    if process == 0:
        return True

    return False
'''
