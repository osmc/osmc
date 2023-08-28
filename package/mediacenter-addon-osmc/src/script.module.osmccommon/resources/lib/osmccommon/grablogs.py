#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmccommon

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import time
import traceback
from datetime import datetime
from io import open

ADDON_ID = "script.module.osmccommon"
try:

    import xbmc
    import xbmcgui
    import xbmcaddon

    ADDON = xbmcaddon.Addon(ADDON_ID)
    DIALOG = xbmcgui.Dialog()
except ImportError:
    xbmc = None
    xbmcgui = None
    xbmcaddon = None

    ADDON = None
    DIALOG = None

try:
    from .osmc_language import LangRetriever
    from .osmc_logging import StandardLogger

    log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log
    lang = LangRetriever(ADDON).lang
except (ValueError, ModuleNotFoundError, ImportError):
    def lang(value):
        return value


    def log(value):
        print(value)

SECTION_START = '\n====================== %s =================== %s\n'
SECTION_END = '\n---------------------- %s END --------------- %s\n\n'
USERDATA = '/home/osmc/.kodi/userdata/'
TEMP_LOG_FILENAME = 'uploadlog.txt'
TEMP_LOG_FILE = '/var/tmp/' + TEMP_LOG_FILENAME
UPLOAD_LOC = 'https://paste.osmc.tv'

RE_MASKS = {
    '00': re.compile(r'((?:OAuth|Bearer)\s)[^\'"]+'),  # oauth tokens
    '01': re.compile(r'(["\']client_secret["\']:\s*[\'"])[^\'"]+'),  # client secret
    '02': re.compile(r'(client_secret=).+?(&|$|\|)'),  # client secret
    '03': re.compile(r'(["\'](?:nauth)*sig["\']: ["\'])[^\'"]+'),  # signature
    '04': re.compile(r'(<[pP]ass(?:word)?>)[^<]+(</[pP]ass(?:word)?>)'),  # pass[word]
    '05': re.compile(r'(["\']password["\']:\s*[\'"])[^\'"]+'),  # password
    '06': re.compile(r'(password=).+?(&|$|\|)'),  # password
    '07': re.compile(r'(\w://.+?:).+?(@\w+)'),  # basic authentication
    '08': re.compile(r'([aA]ccess[_-]*?[tT]oken=).+?(&|$|\|)'),  # access tokens
    '09': re.compile(r'([xX]-[a-zA-Z]+?-[tT]oken=).+?(&|$|\|)'),  # access tokens (plex/emby)
    '10': re.compile(r'(<setting\s[^>]*password[^>]+>)[^<]+'),  # settings v2.0 values with password
}

SETS = {
    'uname': {
        'order': 1,
        'active': False,
        'help': 'System Information',
        'dest': 'uname',
        'action': 'store_true',
        'flags': ['-I', '--systeminfo'],
        'logs': [
            {
                'name': 'UNAME',
                'key': '0wwkXuO5',
                'ltyp': 'cl_log',
                'actn': 'uname -a',
            },
            {
                'name': 'cmdline',
                'key': '0wwYYuO5',
                'ltyp': 'file_log',
                'actn': '/proc/cmdline',
            },
            {
                'name': 'Debian version',
                'key': 'm4ls932a',
                'ltyp': 'file_log',
                'actn': '/etc/debian_version',
            },
            {
                'name': 'OSMC Build Information',
                'key': 'ps9k90Ws',
                'ltyp': 'file_log',
                'actn': '/etc/osmc_build_info',
            },
        ],
    },

    'config': {
        'order': 2,
        'active': False,
        'help': 'Pi config.txt',
        'dest': 'config',
        'action': 'store_true',
        'flags': ['-p', '--piconfig'],
        'logs': [
            {
                'name': 'Pi config',
                'key': 'Ul2H1CLu',
                'ltyp': 'file_log',
                'actn': '/boot/config.txt',
                'hwid': 'rbp',
            },
            {
                'name': 'Pi config-user',
                'key': '7kfykHPJ',
                'ltyp': 'file_log',
                'actn': '/boot/config-user.txt',
                'hwid': 'rbp',
            },
        ],
    },

    'guisettings_ab': {
        'order': 3,
        'active': False,
        'help': 'GUI Settings (abridged)',
        'dest': 'guisettings_ab',
        'action': 'store_true',
        'flags': ['-G', '--guisetab'],
        'logs': [
            {
                'name': 'GUI Settings (abridged)',
                'key': 'z9Z12KgS',
                'ltyp': 'cl_log',
                'actn': '/usr/bin/readgui',
            },
        ],
    },

    'guisettings': {
        'order': 4,
        'active': False,
        'help': 'guisettings.xml',
        'dest': 'guisettings',
        'action': 'store_true',
        'flags': ['-g', '--guiset'],
        'logs': [
            {
                'name': 'guisettings.xml',
                'key': 'zm2LhjK1',
                'ltyp': 'file_log',
                'actn': USERDATA + 'guisettings.xml',
                'mask': True
            },
        ],

    },

    'advancedsettings': {
        'order': 5,
        'active': False,
        'help': 'advancedsettings.xml',
        'dest': 'advancedsettings',
        'action': 'store_true',
        'flags': ['-v', '--advset'],
        'logs': [
            {
                'name': 'advancedsettings.xml',
                'key': 'C7hKmH1p',
                'ltyp': 'file_log',
                'actn': USERDATA + 'advancedsettings.xml',
                'mask': True
            },
        ],
    },

    'sources': {
        'order': 6,
        'active': False,
        'help': 'sources.xml',
        'dest': 'sources',
        'action': 'store_true',
        'flags': ['-s', '--sources'],
        'logs': [
            {
                'name': 'sources.xml',
                'key': 'SGkuGLGj',
                'ltyp': 'file_log',
                'actn': USERDATA + 'sources.xml',
                'mask': True
            },
        ],
    },

    'fstabmounts': {
        'order': 7,
        'active': False,
        'help': 'fstab file & mounts',
        'dest': 'fstabmounts',
        'action': 'store_true',
        'flags': ['-f', '--fstab'],
        'logs': [
            {
                'name': 'fstab',
                'key': 'qiE9Dtax',
                'ltyp': 'file_log',
                'actn': '/etc/fstab',
                'mask': True
            },
            {
                'name': 'mounts',
                'key': 'eWl77s9A',
                'ltyp': 'file_log',
                'actn': '/proc/mounts',
            },
        ],
    },

    'packages': {
        'order': 8,
        'active': False,
        'help': 'OSMC Packages',
        'dest': 'packages',
        'action': 'store_true',
        'flags': ['-O', '--packages'],
        'logs': [
            {
                'name': 'OSMC Packages',
                'key': '7nQvfy9a',
                'ltyp': 'cl_log',
                'actn': 'dpkg -l | grep osmc',
            },
        ],
    },

    'allothers': {
        'order': 9,
        'active': False,
        'help': 'All Other Packages',
        'dest': 'allothers',
        'action': 'store_true',
        'flags': ['-o', '--othpack'],
        'logs': [
            {
                'name': 'All Other Packages',
                'key': 'hwvkLCMX',
                'ltyp': 'cl_log',
                'actn': 'dpkg -l | grep -v osmc',
            },
        ],
    },

    'apt': {
        'order': 10,
        'active': False,
        'help': 'APT term.log, history.log, sources.list, apt.conf.d, preferences.d',
        'dest': 'apt',
        'action': 'store_true',
        'flags': ['-a', '--apt'],
        'logs': [
            {
                'name': 'APT term.log',
                'key': 'RcBRrsRs',
                'ltyp': 'cl_log',
                'actn': 'grep -v -a "^(Reading database" /var/log/apt/term.log | tail -n 500',
            },
            {
                'name': 'APT history.log',
                'key': 'B8sj7DO8',
                'ltyp': 'cl_log',
                'actn': 'grep -v -a "^(Reading database" /var/log/apt/history.log | tail -n 500',
            },
            {
                'name': 'APT sources.list',
                'key': 'ZZz2wrJ1',
                'ltyp': 'file_log',
                'actn': '/etc/apt/sources.list',
            },
            {
                'name': 'APT apt.conf.d',
                'key': 'fFsk1x85',
                'ltyp': 'cl_log',
                'actn': 'ls -al /etc/apt/apt.conf.d',
            },
            {
                'name': 'APT preferences.d',
                'key': 'vSKj25Lq',
                'ltyp': 'cl_log',
                'actn': 'ls -al /etc/apt/preferences.d',
            },
            {
                'name': 'APT sources.list.d',
                'key': 'KjLq37hD',
                'ltyp': 'cl_log',
                'actn': 'ls -al /etc/apt/sources.list.d',
            },
        ],
    },

    'system': {
        'order': 11,
        'active': False,
        'help': 'System Journal',
        'dest': 'system',
        'action': 'store_true',
        'flags': ['-J', '--sysjrn'],
        'logs': [
            {
                'name': 'System Journal',
                'key': 'MyqVXi2x',
                'ltyp': 'cl_log',
                'actn': '/bin/bash -c "sudo journalctl --flush && sudo journalctl -n 30000 --since -24h"',
            },
        ],
    },

    'lirc': {
        'order': 12,
        'active': False,
        'help': 'lirc.conf file',
        'dest': 'lirc',
        'action': 'store_true',
        'flags': ['-l', '--lirc'],
        'logs': [
            {
                'name': 'lircd.conf',
                'key': 'kdgLUcwP',
                'ltyp': 'file_log',
                'actn': '/etc/lirc/lircd.conf',
            },
        ],
    },

    'initd': {
        'order': 13,
        'active': False,
        'help': 'init.d directory',
        'dest': 'initd',
        'action': 'store_true',
        'flags': ['-i', '--initd'],
        'logs': [
            {
                'name': 'init.d',
                'key': 'Vr58kq0w',
                'ltyp': 'cl_log',
                'actn': 'ls -al /etc/init.d',
            },
        ],
    },

    'systemd': {
        'order': 14,
        'active': False,
        'help': 'systemd directory',
        'dest': 'systemd',
        'action': 'store_true',
        'flags': ['-d', '--systemd'],
        'logs': [
            {
                'name': 'systemd',
                'key': '86JFGfNO',
                'ltyp': 'cl_log',
                'actn': 'ls -al /lib/systemd/system',
            },
        ],
    },

    'dmesg': {
        'order': 15,
        'active': False,
        'help': 'Kernel Message Log',
        'dest': 'dmesg',
        'action': 'store_true',
        'flags': ['-K', '--kernel'],
        'logs': [
            {
                'name': 'Kernel Message Log',
                'key': 'Ad2zzd21',
                'ltyp': 'cl_log',
                'actn': 'dmesg',
            },
        ],
    },

    'mem': {
        'order': 16,
        'active': False,
        'help': 'System Memory (total & available)',
        'dest': 'mem',
        'action': 'store_true',
        'flags': ['-m', '--memory'],
        'logs': [
            {
                'name': 'Memory',
                'key': 'eWTP1Mc8',
                'ltyp': 'cl_log',
                'actn': 'free -m',
            },
        ],
    },

    'diskspace': {
        'order': 17,
        'active': False,
        'help': 'Diskspace (total & available)',
        'dest': 'diskspace',
        'action': 'store_true',
        'flags': ['-D', '--disk'],
        'logs': [
            {
                'name': 'Diskspace',
                'key': 'qZy25Yas',
                'ltyp': 'cl_log',
                'actn': 'df -h',
            },
        ],
    },

    'boot': {
        'order': 18,
        'active': False,
        'help': 'Contents of /boot/',
        'dest': 'boot',
        'action': 'store_true',
        'flags': ['-b', '--boot'],
        'logs': [
            {
                'name': '/boot Contents',
                'key': 'H3gEog10',
                'ltyp': 'cl_log',
                'actn': 'ls -al /boot',
            },
        ],
    },

    'disp_info': {
        'order': 19,
        'active': False,
        'help': 'Display Information (disp_cap, disp_mode, edid, aud_cap)',
        'dest': 'disp_info',
        'action': 'store_true',
        'flags': ['-z', '--disp_info'],
        'logs': [
            {
                'name': 'Display Cap CTA',
                'key': 'g0gjk991',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/disp_cap',
                'hwid': '!rbp',
            },
            {
                'name': 'Display Cap VESA',
                'key': 'xyGFZe4j',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/vesa_cap',
                'hwid': '!rbp',
            },
            {
                'name': 'Display Cap 3D',
                'key': 'knxJbydS',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/disp_cap_3d',
                'hwid': '!rbp',
            },
            {
                'name': 'User Display Overrides',
                'key': 'BzKtAx6S',
                'ltyp': 'cl_log',
                'actn': 'bash -c "for f in disp_cap vesa_cap disp_cap_3d; do echo -e \\"$f file\\n\\"; cat /home/osmc/.kodi/userdata/$f 2> /dev/null; echo; done"',
                'hwid': '!rbp',
            },
            {
                'name': 'Display Mode',
                'key': 'Q72ho215',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/disp_mode',
                'hwid': '!rbp',
            },
            {
                'name': 'EDID',
                'key': 'wE0go885',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/edid',
                'hwid': '!rbp',
            },
            {
                'name': 'Audio Cap',
                'key': 'k3dRrf31',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/aud_cap',
                'hwid': '!rbp',
            },
            {
                'name': 'edid-decode',
                'key': 'wes0DM2l',
                'ltyp': 'cl_log',
                'actn': '/usr/bin/edid-decode /sys/devices/virtual/amhdmitx/amhdmitx0/rawedid',
                'hwid': 'vero',
            },
            {
                'name': 'edid-decode',
                'key': 'su34JRse',
                'ltyp': 'cl_log',
                'actn': '/usr/bin/edid-decode /sys/class/drm/card1-HDMI-A-1/edid',
                'hwid': 'rbp',
            },
        ],
    },

    'ifconfig': {
        'order': 20,
        'active': False,
        'help': 'ifconfig',
        'dest': 'ifconfig',
        'action': 'store_true',
        'flags': ['-n', '--ifconfig'],
        'logs': [
            {
                'name': 'ifconfig',
                'key': 'pi3lDrO1',
                'ltyp': 'cl_log',
                'actn': 'ifconfig',
            },
        ],
    },

    'kodi': {
        'order': 21,
        'active': False,
        'help': 'Kodi log files (includes log from previous boot)',
        'dest': 'kodi',
        'action': 'store_true',
        'flags': ['-X', '--kodi', '--xbmc'],
        'logs': [
            {
                'name': 'Kodi Log',
                'key': 'HyhIT4UP',
                'ltyp': 'file_log',
                'actn': '/home/osmc/.kodi/temp/kodi.log',
                'mask': True
            },
            {
                'name': 'Kodi Old Log',
                'key': '2qaAc90c',
                'ltyp': 'file_log',
                'actn': '/home/osmc/.kodi/temp/kodi.old.log',
                'mask': True
            },
        ],
    },
    'provision': {
        'order': 22,
        'active': False,
        'help': 'OSMC Vero provisioning status',
        'dest': 'provision',
        'action': 'store_true',
        'flags': ['-Y', '--provision'],
        'logs': [
            {
                'name': 'provision',
                'key': 've5xhi74',
                'ltyp': 'cl_log',
                'actn': '/opt/securevero/secureos/bin/tee_osmc -d',
                'hwid': 'vero5',
            },
        ],
    },
}


class CommandLine(object):

    def __init__(self, command_list):
        self.command_list = command_list

    def readlines(self):
        ps = subprocess.Popen((''), shell=True)
        while '|' in self.command_list:
            idx = self.command_list.index('|')
            chunk, self.command_list = self.command_list[:idx], self.command_list[idx + 1:]
            ps = subprocess.Popen(chunk, stdin=ps.stdout, stdout=subprocess.PIPE)

        res = subprocess.check_output(self.command_list, stdin=ps.stdout)
        res = res.decode('utf-8')

        return res


class CommandLineInterface(object):

    def __init__(self, command_string):
        self.command_string = command_string

    def __enter__(self):
        command_list = shlex.split(self.command_string)
        return CommandLine(command_list=command_list)

    def __exit__(self, *args):
        pass


def right_now(raw=False):
    """ Returns the current time.
        raw=True will return the raw datetime, but default is to convert it into a string.

        Sometimes dt.now() throws some sort of "lock" error,
        so we will give the function 1 second to throw errors and retrieve the time.
    """

    for _ in range(5):
        try:
            if raw:
                return datetime.now()
            else:
                with open('/proc/uptime', 'r', encoding='utf-8') as f:
                    uptime_seconds = f.readline().split()[0]
                uptime = " - (Uptime = " + uptime_seconds + ")"
                return datetime.now().strftime('%Y-%m-%d %H:%M:%S') + uptime
        except:
            time.sleep(0.2)

    return 'Failed to retrieve time'


def argv():
    return sys.argv


def parse_arguments():
    """ Parses the arguments provided by the user and activates the entries in SETS.
        Returns a bool determining whether the user wants to copy the logs to the SD Card.
        If help is true, then the help dialog is displayed. """
    global SETS

    parser = argparse.ArgumentParser(description='Uploads vital logs to %s. If the network is '
                                                 'unavailable, logs are copied to the SD Card.' %
                                                 UPLOAD_LOC)

    arguments = [v for k, v in SETS.items()]
    arguments.sort(key=lambda x: x.get('order', 99))

    parser.add_argument('-A', '--all', action='store_true', dest='all', help='Include all logs')
    parser.add_argument('-T', action='store', dest='filename',
                        help='Override default name and location of temporary log file')
    parser.add_argument('-C', '--copy', action='store_true', dest='copy',
                        help='Copy logs to /boot (SD Card)')
    parser.add_argument('-P', '--print', action='store_true', dest='termprint',
                        help='Print logs to screen (no upload or copy)')

    ignored_args = ['copy', 'all', 'termprint', 'filename']

    for a in arguments:
        parser.add_argument(*a['flags'], action=a['action'], dest=a['dest'], help=a['help'])

    args = parser.parse_args()

    # Exit if there are no arguments or there is only the COPY argument
    if any([
        (len(argv()) == 1),
        (len(argv()) == 2 and (args.copy or args.termprint)),
        (len(argv()) == 3 and (args.copy and args.termprint))
    ]):
        parser.print_help()

        return None, None

    # if 'all' is specified then include all logs
    if args.all:

        for k, v in SETS.items():
            SETS[k]['active'] = True

    else:

        for k, arg in vars(args).items():
            if k not in ignored_args:
                SETS[k]['active'] = arg

    # if a different temporary location is provided, then use that in place of the
    # global TEMP_LOG_FILE
    if args.filename:
        global TEMP_LOG_FILE
        TEMP_LOG_FILE = args.filename

    return args.copy, args.termprint


def retrieve_settings(addon=None):
    """ Gets the settings from Kodi and activates the relevant entries in SETS.
        Returns a bool determining whether the user wants to copy the logs to the SD Card.  """
    global SETS

    if addon is None:
        addon = ADDON

    excluded_from_all = []

    grab_all = True if addon.getSetting('all') == 'true' else False

    for key in SETS:
        if grab_all and key not in excluded_from_all:
            SETS[key]['active'] = True
        else:
            SETS[key]['active'] = True if addon.getSetting(key) == 'true' else False

    return argv()[1] == 'copy', False


class DummyProgressDialog(object):
    """ Substitute progress dialog class to save having to try/except all progress_dialog calls. """

    def create(self, *args, **kwargs):
        pass

    def update(self, *args, **kwargs):
        pass

    def close(self, *args, **kwargs):
        pass


class Main(object):

    def __init__(self, copy, termprint):

        self.copy_to_boot = copy

        self.termprint = termprint

        self.log_blotter = []  # list to hold all the lines that need to be printed/uploaded

        self.url = ''

        self.progress_dialog = None

        self.stage_dialog()

        self.number_of_actions = sum(1 for k, v in SETS.items() if v.get('active', False))

        self.progress_dialog.create(lang(32001), lang(32004))

        self.arguments = [(k, v) for k, v in SETS.items()]

        self.arguments.sort(key=lambda x: x[1].get('order', 99))

        self._hwid = ''

    def hwid(self):
        if self._hwid:
            return self._hwid

        with open('/proc/cmdline', 'r', encoding='utf-8') as f:
            line = f.readline()

        settings = line.split(' ')
        settings = [item.split('=', maxsplit=1) for item in settings]
        settings = [item for item in settings if not (len(item) == 1 and not item[0])]
        settings = [item + [''] if len(item) == 1 else [item[0], item[1].strip('\n')]
                    for item in settings]

        for setting, value in settings:
            if setting == 'osmcdev':
                self._hwid = value
                break

        return self._hwid

    def valid_hardware(self, hwid):
        if not hwid:
            return True

        actual_hwid = self.hwid()

        # hwid of 'vero' matches all vero hardware, 'vero3' matches exactly vero3
        generic_match = not hwid[-1].isdigit()
        # hwid of '!rpb' will matches all non rbp hardware
        negative_match = hwid.startswith('!')

        if negative_match:
            # we know it's a negative match, remove ! for future comparisons
            hwid = hwid.lstrip('!')

        if generic_match:
            # we know it's a generic match, remove the digit from our
            # actual hardware id for future comparisons
            if actual_hwid[-1].isdigit():
                actual_hwid = actual_hwid[:-1]

        hardware_match = actual_hwid == hwid
        if hwid == 'rbp1':
            # exception to the rule, rbp1 == rbp
            hardware_match = actual_hwid == hwid[-1]

        if negative_match:
            return not hardware_match

        return hardware_match

    def stage_dialog(self):
        try:
            if self.progress_dialog:
                self.progress_dialog.close()
                del self.progress_dialog
        except:
            try:
                del self.progress_dialog
            except:
                pass
        try:
            self.progress_dialog = xbmcgui.DialogProgressBG()
        except:
            self.progress_dialog = DummyProgressDialog()

    def launch_process(self):

        self.add_content_index()

        self.process_logs()

        if self.termprint:
            self.write_to_screen()
            return

        result = self.write_to_temp_file()

        if result:
            self.dispatch_logs()

    def add_content_index(self):
        """ Adds the quick look-up references to the start of the log file """

        # insert the date at the very top
        self.log_blotter.append('Logs created on: %s\n\n' % right_now())
        default_entry = {
            'key': '',
            'name': '',
        }
        for k, v in self.arguments:

            if v.get('active', False):

                for log_entry in v.get('logs', default_entry):
                    if not self.valid_hardware(log_entry.get('hwid', '')):
                        continue

                    self.log_blotter.append(log_entry['key'] + '  :  ' + log_entry['name'] + '\n')

        self.log_blotter.append('\n')

    def process_logs(self):
        """
            Runs the specific function for the active logs, and appends
            the contents to the blotter.
        """

        # add the logs themselves
        count = 0
        for k, v in self.arguments:

            if v.get('active', False):

                count += 1

                pct = int(100.0 * float(count) / float(self.number_of_actions))

                self.progress_dialog.update(percent=pct,
                                            message='' if not xbmc else lang(32006) % k)

                for log_entry in v['logs']:
                    self.grab_log(**log_entry)

        self.progress_dialog.update(percent=100, message=lang(32005))
        self.progress_dialog.close()

    def grab_log(self, ltyp, actn, name, key, hwid='', mask=False):
        """ Method grabs the logs from either a file or the command line."""

        if not self.valid_hardware(hwid):
            return

        self.log_blotter.extend([SECTION_START % (name, key)])
        print('Grabbing log {name} ...'.format(name=name))
        try:
            if ltyp == 'file_log':
                with open(actn, 'r', encoding='utf-8') as f:
                    readlines = f.readlines()
                    if mask:
                        readlines = self._mask_sensitive(readlines)
                    self.log_blotter.extend(readlines)
            else:
                with CommandLineInterface(actn) as f:
                    readlines = f.readlines()
                    if mask:
                        readlines = self._mask_sensitive(readlines)
                    self.log_blotter.extend(readlines)
        except:
            print('An error occurred while grabbing %s:\n %s' % (name, traceback.format_exc().splitlines()[-1]))
            self.log_blotter.extend(['An error occurred while grabbing %s:\n %s' % (name, traceback.format_exc().splitlines()[-1])])

        self.log_blotter.extend([SECTION_END % (name, key)])

    @staticmethod
    def _mask_sensitive(lines_to_mask):
        # mask potentially sensitive information in blotter
        print('Masking private information ...')

        def _mask(message):
            mask = '**masked*by*grab-logs**'

            masked_message = RE_MASKS['00'].sub(r'\1' + mask, message)
            masked_message = RE_MASKS['01'].sub(r'\1' + mask, masked_message)
            masked_message = RE_MASKS['02'].sub(r'\1' + mask + r'\2', masked_message)
            masked_message = RE_MASKS['03'].sub(r'\1' + mask, masked_message)
            masked_message = RE_MASKS['04'].sub(r'\1' + mask + r'\2', masked_message)
            masked_message = RE_MASKS['05'].sub(r'\1' + mask, masked_message)
            masked_message = RE_MASKS['06'].sub(r'\1' + mask + r'\2', masked_message)
            masked_message = RE_MASKS['07'].sub(r'\1' + mask + r'\2', masked_message)
            masked_message = RE_MASKS['08'].sub(r'\1' + mask + r'\2', masked_message)
            masked_message = RE_MASKS['09'].sub(r'\1' + mask + r'\2', masked_message)
            masked_message = RE_MASKS['10'].sub(r'\1' + mask, masked_message)

            return masked_message

        return [_mask(line) for line in lines_to_mask]

    def write_to_screen(self):
        self.write_to_temp_file()

        with open(TEMP_LOG_FILE, 'rb') as f:
            lines = f.readlines()

        lines = [line.decode('utf-8') if isinstance(line, bytes) else line for line in lines]
        screen_dump = ''.join(lines)

        print(screen_dump)

    def write_to_temp_file(self):
        """ Writes the logs to a single temporary file """
        # clean up the blotter
        print('Writing logs to temp file ...')
        self.log_blotter = [x.replace('\0', '').replace('\ufeff', '').encode('utf-8')
                            for x in self.log_blotter if hasattr(x, 'replace')]

        if os.path.isfile(TEMP_LOG_FILE):
            slept = 0
            sleep_inc = 0.5
            with os.popen('sudo rm %s' % TEMP_LOG_FILE) as _:
                pass

            while slept <= 3:
                if not os.path.isfile(TEMP_LOG_FILE):
                    break
                time.sleep(sleep_inc)
                slept += sleep_inc

        try:
            with open(TEMP_LOG_FILE, 'wb') as f:
                # write the blotter contents
                f.writelines(self.log_blotter)

            return True

        except:

            log('Unable to write temporary log to %s' % TEMP_LOG_FILE)
            log('Failed')

            return

    def dispatch_logs(self):
        """ Either copies the combined logs to the SD Card or Uploads them to the pastebin. """
        print('Dispatching logs ...')
        self.stage_dialog()

        if self.copy_to_boot:
            self.progress_dialog.create(
                lang(32001), '' if not xbmc else lang(32009) % ('/boot/' + TEMP_LOG_FILENAME)
            )

            os.popen('sudo cp -rf %s /boot/' % TEMP_LOG_FILE)

            self.progress_dialog.update(percent=100, message=lang(32008))

            if xbmc:
                _ = DIALOG.ok(lang(32001), lang(32008))

            else:

                log('Logs copied to /boot/%s on the SD card FAT partition' % TEMP_LOG_FILENAME)
            self.progress_dialog.close()
            del self.progress_dialog
        else:
            self.progress_dialog.create(lang(32001), lang(32010))

            attempts = [
                'curl -X POST -s    -T',
                'curl -X POST -s -0 -T'
            ]

            steps = len(attempts) + 1
            step_pct = int(100 // steps)
            pct = 0

            upload_exception = None
            key = None

            for attempt in attempts:
                pct += step_pct
                try:
                    with os.popen('%s "%s" %s/documents' %
                                  (attempt, TEMP_LOG_FILE, UPLOAD_LOC)) as open_file:

                        response = open_file.read()

                    key = None
                    try:
                        payload = json.loads(response)
                        if 'key' in payload:
                            key = payload['key']
                    except:
                        match = re.search(r'"key":"(?P<key>[^"]+?)"', response)
                        if match:
                            key = match.group('key')

                    if xbmc:
                        log('pastio response: %s' % repr(response))

                    self.progress_dialog.update(percent=pct, message=lang(32010))

                    if not key:
                        match = re.search(r'.*?413\sRequest\sEntity\sToo\sLarge.*?', response)
                        if match:
                            if xbmc:
                                xbmcgui.Dialog().notification(lang(32001), lang(32012), ADDON.getAddonInfo('icon'))
                                break
                            else:
                                raise Exception('Log file too large for upload')
                        # the upload returning an empty string is considered a specific Exception
                        # every other exception is caught and will be printed as well
                        # but only for the second (fallback) attempt
                        raise ValueError('Upload Returned Empty String')

                    else:
                        break

                except Exception:
                    self.progress_dialog.update(percent=pct, message=lang(32010))

                    upload_exception = traceback.format_exc()

            self.progress_dialog.update(percent=100, message=lang(32011))
            time.sleep(0.5)
            self.progress_dialog.close()
            del self.progress_dialog

            if not key:

                if upload_exception:
                    log('Exception Details:\n')
                    log(upload_exception)

                if xbmc:

                    self.copy_to_boot = DIALOG.yesno(lang(32001),
                                                     '[CR]'.join([lang(32003), lang(32007)]))

                else:

                    self.copy_to_boot = True

                    log("Failed to upload log files, copying to /boot instead. (Unable to verify)")

                if self.copy_to_boot:
                    os.popen('sudo cp -rf %s /boot/' % TEMP_LOG_FILE)

            else:

                self.url = UPLOAD_LOC + '/ %s' % key

                if xbmc:

                    _ = DIALOG.ok(lang(32001), lang(32002) % self.url)

                else:

                    log("Logs successfully uploaded.")
                    log("Logs available at %s" % self.url.replace(' ', ''))


if __name__ == "__main__":

    if not xbmc:
        _copy, _termprint = parse_arguments()
    else:
        _copy, _termprint = retrieve_settings()

    if _copy is not None:
        m = Main(_copy, _termprint)

        m.launch_process()
