# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.

    This script is run as root by the osmc update module.
"""

import json
import socket
import sys
from contextlib import closing
from datetime import datetime

PY3 = sys.version_info.major == 3


def argv():
    return sys.argv


def call_parent(raw_message, data=None):
    print('%s %s sending response' % (datetime.now(), 'apt_cache_action.py'))
    if data is None:
        data = {}
    message = (raw_message, data)
    message = json.dumps(message)

    try:
        with closing(socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)) as open_socket:
            open_socket.connect('/var/tmp/osmc.settings.update.sockfile')
            if PY3 and not isinstance(message, (bytes, bytearray)):
                message = message.encode('utf-8', 'ignore')
            open_socket.sendall(message)

    except Exception as e:
        print('%s %s failed to connect to parent - %s' % (datetime.now(), 'apt_cache_action.py', e))

    print('%s %s response sent' % (datetime.now(), 'apt_cache_action.py'))


if __name__ == "__main__":
    if len(argv()) > 1:
        call_parent(str(argv()[1]))
