# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmccommon

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import sys
import time
from functools import wraps

try:
    import xbmc
except:
    xbmc = None

TEST_LOG_BOOL = True
PY2 = sys.version_info.major == 2


def test_logger(msg):
    print('test-' + msg)


class StandardLogger(object):
    """ Standard kodi logger. Used to add entries to the Kodi.log.
        Best usage:
            from osmc_logging import StandardLogger
            standard_logger = StandardLogger(addon_id)
            log = standard_logger.log

        """

    def __init__(self, addon_id='osmc', module=''):
        self.addon_id = addon_id
        try:
            self.module = module.replace('.pyo', '').replace('.pyc', '').replace('.py', '')
        except:
            self.module = module

    def log(self, message, label=''):

        if isinstance(message, bytes):
            message = message.decode('utf-8', 'ignore')

        if PY2 and isinstance(message, unicode):
            message = message.encode('utf-8')

        if isinstance(label, bytes):
            label = label.decode('utf-8', 'ignore')

        if PY2 and isinstance(label, unicode):
            label = label.encode('utf-8')

        if label and self.module:
            logmsg = '%s[%s] : %s - %s ' % (self.addon_id, self.module, label, message)
        elif label:
            logmsg = '%s : %s - %s ' % (self.addon_id, label, message)
        elif self.module:
            logmsg = '%s[%s] : %s ' % (self.addon_id, self.module, message)
        else:
            logmsg = '%s : %s ' % (self.addon_id, message)

        if xbmc:
            xbmc.log(msg=logmsg, level=xbmc.LOGDEBUG)
        else:
            print(logmsg)


def comprehensive_logger(logger=None, logging=True, maxlength=250, nowait=False):
    """
        Decorator to log the inputs and outputs of functions, as well as the time
        taken to run the function.

        Requires: time, functools
        logger: 	[opt] logging function, if not provided print is used
        logging: 	[opt] boolean, turn logging on and off, default is True
        maxlength:	[opt] integer, sets the maximum length an argument or returned
        variable cant take, default 25
        nowait:		[opt] boolean, instructs the logger not to wait for the
        function to finish, default is False
    """
    standard_logger = StandardLogger()

    def default_logger(msg):
        standard_logger.log(msg)

    if logger is None:
        logger = default_logger

    def get_args(*args, **kwargs):

        all_args = []

        for i, arg in enumerate(args):
            itm = 'pos' + str(i) + ": " + str(arg)[:maxlength]

            all_args.append(itm)

        for k, v in kwargs.items():
            itm = str(k) + ": " + str(v)[:maxlength]

            all_args.append(itm)

        return all_args

    def decorater(func):

        @wraps(func)
        def wrapper(*args, **kwargs):

            if logging and logger is not None:
                logger(func.__module__ + '.' + func.__name__ + " received: " +
                       ", ".join(get_args(*args, **kwargs)))

            if nowait:

                func(*args, **kwargs)

                logger(func.__module__ + '.' + func.__name__ + " -nowait")

                return

            else:

                start = time.time()

                result = func(*args, **kwargs)

                end = time.time()

                if logging and logger is not None:
                    logger(func.__module__ + '.' + func.__name__ +
                           " [" + str(end - start) + "] " + ' returns: ' + str(result)[:maxlength])

                return result

        return wrapper

    return decorater


clog = comprehensive_logger
