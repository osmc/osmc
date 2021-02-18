# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import json
import socket
import sys
from contextlib import closing

PY3 = sys.version_info.major == 3


def argv():
    return sys.argv


if len(argv()) > 1:
    msg = argv()[1]
    print('OSMC settings sending response, %s' % msg)

    message = ('settings_command', {
        'action': msg
    })
    message = json.dumps(message)

    with closing(socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)) as open_socket:
        open_socket.connect('/var/tmp/osmc.settings.update.sockfile')
        if PY3 and not isinstance(message, (bytes, bytearray)):
            message = message.encode('utf-8', 'ignore')
        open_socket.sendall(message)

    print('OSMC settings sent response, %s' % msg)
