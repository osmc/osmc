#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmccommon

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import argparse
import os
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
except ValueError:
    def lang(value):
        return value


    def log(value):
        print(value)

PY2 = sys.version_info.major == 2

SECTION_START = '\n====================== %s =================== %s\n'
SECTION_END = '\n---------------------- %s END --------------- %s\n\n'
USERDATA = '/home/osmc/.kodi/userdata/'
TEMP_LOG_FILENAME = 'uploadlog.txt'
TEMP_LOG_FILE = '/var/tmp/' + TEMP_LOG_FILENAME
UPLOAD_LOC = 'https://paste.osmc.tv'

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
            },
        ],
    },

    'guisettings_ab': {
        'order': 2.25,
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
                'actn': '/usr/bin/readgui'
            },
        ],
    },

    'guisettings': {
        'order': 2.5,
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
            },
        ],

    },

    'advancedsettings': {
        'order': 3,
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
            },
        ],
    },

    'keyboard': {
        'order': 4,
        'active': False,
        'help': 'keyboard.xml',
        'dest': 'keyboard',
        'action': 'store_true',
        'flags': ['-k', '--keyboard'],
        'logs': [
            {
                'name': 'keyboard.xml',
                'key': 'MBom5YV6',
                'ltyp': 'file_log',
                'actn': USERDATA + 'keyboard.xml',
            },
        ],
    },

    'remote': {
        'order': 5,
        'active': False,
        'help': 'remote.xml',
        'dest': 'remote',
        'action': 'store_true',
        'flags': ['-r', '--remote'],
        'logs': [
            {
                'name': 'remote.xml',
                'key': '5jmphjm3',
                'ltyp': 'file_log',
                'actn': USERDATA + 'remote.xml',
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
                'actn': 'sudo journalctl',
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
                'name': 'Display Cap',
                'key': 'g0gjk991',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/disp_cap',
            },
            {
                'name': 'Display Mode',
                'key': 'Q72ho215',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/disp_mode',
            },
            {
                'name': 'EDID',
                'key': 'wE0go885',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/edid',
            },
            {
                'name': 'Audio Cap',
                'key': 'k3dRrf31',
                'ltyp': 'file_log',
                'actn': '/sys/class/amhdmitx/amhdmitx0/aud_cap',
            },
            {
                'name': 'Pi EDID',
                'key': 'wes0DM2l',
                'ltyp': 'cl_log',
                'actn': '/opt/vc/bin/tvservice -s',
            },
            {
                'name': 'Pi CEA',
                'key': 'su34JRse',
                'ltyp': 'cl_log',
                'actn': '/opt/vc/bin/tvservice -m CEA',
            },
            {
                'name': 'Pi DMT',
                'key': 'zsl2D3rt',
                'ltyp': 'cl_log',
                'actn': '/opt/vc/bin/tvservice -m DMT',
            },
            {
                'name': 'Pi Audio Cap',
                'key': 'szl3J3wq',
                'ltyp': 'cl_log',
                'actn': '/opt/vc/bin/tvservice -a',
            },
            {
                'name': 'MPG2 codec_enabled',
                'key': 'DjfSD1Fa',
                'ltyp': 'cl_log',
                'actn': 'vcgencmd codec_enabled MPG2',
            },
            {
                'name': 'WVC1 codec_enabled',
                'key': 'dDR3l5zx',
                'ltyp': 'cl_log',
                'actn': 'vcgencmd codec_enabled WVC1',
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
            },
            {
                'name': 'Kodi Old Log',
                'key': '2qaAc90c',
                'ltyp': 'file_log',
                'actn': '/home/osmc/.kodi/temp/kodi.old.log',
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

    def grab_log(self, ltyp, actn, name, key):
        """ Method grabs the logs from either a file or the command line."""

        self.log_blotter.extend([SECTION_START % (name, key)])

        try:
            if ltyp == 'file_log':
                with open(actn, encoding='utf-8') as f:
                    self.log_blotter.extend(f.readlines())
            else:
                with CommandLineInterface(actn) as f:
                    self.log_blotter.extend(f.readlines())
        except:
            self.log_blotter.extend(['%s error' % name])

        self.log_blotter.extend([SECTION_END % (name, key)])

    def write_to_screen(self):

        print(''.join(self.log_blotter))

    def write_to_temp_file(self):
        """ Writes the logs to a single temporary file """
        # clean up the blotter
        self.log_blotter = [x.replace('\0', '') for x in self.log_blotter if hasattr(x, 'replace')]
        if PY2:
            self.log_blotter = [
                x.decode('utf-8', 'ignore') if isinstance(x, str) else x
                for x in self.log_blotter
            ]
        try:
            with open(TEMP_LOG_FILE, 'w', encoding='utf-8') as f:

                # write the blotter contents
                f.writelines(self.log_blotter)

            return True

        except:

            log('Unable to write temporary log to %s' % TEMP_LOG_FILE)
            log('Failed')

            return

    def dispatch_logs(self):
        """ Either copies the combined logs to the SD Card or Uploads them to the pastebin. """
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

                        line = open_file.readline()

                        key = line.replace('{"key":"', '').replace('"}', '').replace('\n', '')

                        if xbmc:
                            log('pastio line: %s' % repr(line))

                    self.progress_dialog.update(percent=pct, message=lang(32010))

                    if not key:
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
