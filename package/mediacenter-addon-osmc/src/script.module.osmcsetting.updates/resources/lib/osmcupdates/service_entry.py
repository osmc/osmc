# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import decimal
import json
import os
import random
import socket
import subprocess
import sys
import traceback
from contextlib import closing
from datetime import datetime
from io import open

import apt
import xbmc
import xbmcaddon
import xbmcgui
from osmccommon import osmc_comms
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger
from osmccommon.osmc_scheduler import SimpleScheduler

from .osmc_backups import OSMCBackup

try:
    import queue as Queue
except ImportError:
    import Queue

ADDON_ID = 'script.module.osmcsetting.updates'
DIALOG = xbmcgui.Dialog()
PY2 = sys.version_info.major == 2
PY3 = sys.version_info.major == 3

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


def exit_osmc_settings_addon():
    message = 'exit'
    with closing(socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)) as open_socket:
        open_socket.connect('/var/tmp/osmc.settings.sockfile')
        if PY3 and not isinstance(message, (bytes, bytearray)):
            message = message.encode('utf-8', 'ignore')
        open_socket.sendall(message)

    return 'OSMC Settings addon called to exit'


def get_hardware_prefix():
    """ Returns the prefix for the hardware type. rbp, rbp2, etc """

    with open('/proc/cmdline', 'r', encoding='utf-8') as f:
        line = f.readline()

    settings = line.split(' ')

    for setting in settings:
        if setting.startswith('osmcdev='):
            return setting[len('osmcdev='):]

    return None


class UpdatesMonitor(xbmc.Monitor):

    def __init__(self, **kwargs):
        super(UpdatesMonitor, self).__init__()
        self.parent_queue = kwargs['parent_queue']

    def onAbortRequested(self):
        msg = json.dumps(('kill_yourself', {}))
        self.parent_queue.put(msg)

    def onSettingsChanged(self):
        msg = json.dumps(('update_settings', {}))
        self.parent_queue.put(msg)


class Main(object):
    """
        This service allows for the checking for new updates, then:
            - posts a notification on the home screen to say there is an update available, or
            - calls for the download of the updates
            - calls for the installation of the updates
            - restarts Kodi to implement changes
        The check for updates is done using the python-apt module. This module must be run as root,
        so is being called in external scripts from the command line using sudo. The other script
        communicates with the update service using a socket file.
    """

    def __init__(self, addon=None):
        self._addon = addon
        self._lang = None
        self._path = ''
        self._lib_path = ''

        self.first_run = True

        # set the hardware prefix
        self.hw_prefix = get_hardware_prefix()

        # list of packages that require an external update
        self.EXTERNAL_UPDATE_REQUIRED_LIST = [
            "mediacenter",
            "lirc-osmc",
            "eventlircd-osmc",
            "libcec-osmc",
            "dbus",
            "dbus-x11"
        ]

        # list of packages that may break compatibility with addons and databases.
        self.UPDATE_WARNING = False
        self.UPDATE_WARNING_LIST = [
            "-mediacenter-osmc",
        ]

        # Items that start with a hyphen should have the hardware prefix attached
        self.UPDATE_WARNING_LIST = [(str(self.hw_prefix) + x)
                                    if x[0] == '-'
                                    else x for x in self.UPDATE_WARNING_LIST]
        log('UPDATE_WARNING_LIST: %s' % self.UPDATE_WARNING_LIST)

        # the time that the service started
        self.service_start = datetime.now()

        # dictionary containing the permissible actions (communicated from the child apt scripts)
        # and the corresponding methods in the parent
        self.action_dict = {
            'apt_cache update complete': self.apt_update_complete,
            'apt_cache update_manual complete': self.apt_update_manual_complete,
            'apt_cache commit complete': self.apt_commit_complete,
            'apt_cache fetch complete': self.apt_fetch_complete,
            'progress_bar': self.progress_bar,
            'update_settings': self.update_settings,
            'update_now': self.update_now,
            'user_update_now': self.user_update_now,
            'kill_yourself': self.kill_yourself,
            'settings_command': self.settings_command,
            'apt_error': self.apt_error,
            'apt_action_list_error': self.apt_action_list_error,
            'action_list': self.action_list,
            'apt_cache action_list complete': self.action_list_complete,
            'pre_backup_complete': self.pre_backup_complete,

        }

        self.icon_settings = []
        self.on_upd = []
        self.scheduler_settings = []
        self.setting_dict = {}

        self.cache = None
        self.progress_dialog = None

        # queue for communication with the comm and Main
        self.parent_queue = Queue.Queue()

        self.random_id = random.randint(0, 1000)

        self.EXTERNAL_UPDATE_REQUIRED = 1

        # create socket, listen for comms
        self.listener = \
            osmc_comms.Communicator(self.parent_queue,
                                    socket_file='/var/tmp/osmc.settings.update.sockfile')
        self.listener.start()

        # grab the settings, saves them into a dict called seld.s
        self.update_settings()

        # a class to handle scheduling update checks
        self.scheduler = SimpleScheduler(self.setting_dict)
        log(self.scheduler.trigger_time, 'trigger_time')

        # this holding pattern holds a function that represents the completion of a
        # process that was put on hold while the user was watching media or the system was active
        self.function_holding_pattern = None

        # monitor for identifying addon settings updates and kodi abort requests
        self.monitor = UpdatesMonitor(parent_queue=self.parent_queue)

        # window onto which to paste the update notification
        self.window = xbmcgui.Window(10000)

        # property which determines whether the notification should be pasted to the window
        self.window.setProperty('OSMC_notification', 'false')

        image = os.path.join(self.path, 'resources', 'media', 'update_available.png')
        self.update_image = xbmcgui.ControlImage(50, 1695, 175, 75, image)
        self.try_image_position_again = False
        self.try_count = 0
        self.position_icon()
        self.window.addControl(self.update_image)
        self.update_image.setVisibleCondition('[String.Contains(Window(Home)'
                                              '.Property(OSMC_notification), true)]')

        # this flag is present when updates have been downloaded but the user wants to
        # choose when to install using the manual control in the settings
        self.block_update_file = '/var/tmp/.suppress_osmc_update_checks'

        # if the file is present, then suppress further update checks and show the notification
        if os.path.isfile(self.block_update_file):
            self.skip_update_check = True

            # if the user has suppressed icon notification of updates and has chosen not
            # to install the updates it's their own damned fault if osmc never get updated
            if not self.setting_dict['suppress_icon']:
                self.window.setProperty('OSMC_notification', 'true')

        else:
            self.skip_update_check = False

        # check for the external update failed
        fail_check_file = '/var/tmp/.osmc_failed_update'
        if os.path.isfile(fail_check_file):
            with open(fail_check_file, 'r', encoding='utf-8') as f:
                package = f.readline()

            _ = DIALOG.ok(self.lang(32087),
                          '[CR]'.join([self.lang(32088) % package, '', self.lang(32089)]))

            try:
                os.remove(fail_check_file)
            except:
                pass

        # change this to 'apt' to give the user the option to clean the apt files
        self.freespace_remedy = 'reboot'
        self.freespace_supressor = 172200

        self.keep_alive = True
        # keep alive method
        self._daemon()

    @property
    def addon(self):
        if not self._addon:
            self._addon = xbmcaddon.Addon(ADDON_ID)
        return self._addon

    def lang(self, value):
        if not self._lang:
            retriever = LangRetriever(self.addon)
            self._lang = retriever.lang
        return self._lang(value)

    @property
    def path(self):
        if not self._path:
            self._path = self.addon.getAddonInfo('path')
        return self._path

    @property
    def lib_path(self):
        if not self._lib_path:
            self._lib_path = xbmc.translatePath(os.path.join(self.path, 'resources',
                                                             'lib', 'osmcupdates'))
        return self._lib_path

    def _daemon(self):
        self.keep_alive = True
        while self.keep_alive and not self.monitor.abortRequested():
            # freespace checker, (runs 5 minutes after boot)
            self.automatic_freespace_checker()

            # check the scheduler for the update trigger
            if self.scheduler.check_trigger():
                self.update_now()
                log(self.scheduler.trigger_time, 'trigger_time')

            # check the action queue
            self.check_action_queue()

            # check the holding pattern, call item in holding pattern
            if self.function_holding_pattern:
                self.function_holding_pattern()

            # try to position the icon again, ubiquifonts may not have had time to post
            # the screen height and width to Home yet.
            if self.try_image_position_again:
                self.position_icon()

            # check for an early exit
            if not self.keep_alive:
                break

            # this controls the frequency of the instruction processing
            if self.monitor.waitForAbort(0.5):
                break

        self.exit_procedure()

    def holding_pattern_update(self):
        check, _ = self.check_update_conditions()

        if check:
            self.function_holding_pattern = None

            self.user_update_now()

    def holding_pattern_fetched(self, bypass=False):
        # stay in the holding pattern until the user returns to the Home screen
        if 'Home.xml' in xbmc.getInfoLabel('Window.Property(xmlfile)') or bypass:
            # if there is an update warning (for a major version change in Kodi)
            # then alert the user
            if self.UPDATE_WARNING:
                confirm_update = self.display_update_warning()

                if not confirm_update:
                    # remove the function from the holding pattern
                    self.function_holding_pattern = None

                    # skip all future update checks (the user will have to run
                    # the check for updates manually.)
                    self.skip_future_update_checks()

                    return 'User declined to update major version of Kodi, ' \
                           'skipping future update checks'

            self.function_holding_pattern = None

            if not self.EXTERNAL_UPDATE_REQUIRED:
                install_now = DIALOG.yesno(self.lang(32072),
                                           '[CR]'.join([self.lang(32073), self.lang(32074)]))

                if install_now:
                    self.call_child_script('commit')

                    return 'Called child script - commit'

            else:
                exit_install = DIALOG.yesno(self.lang(32072),
                                            '[CR]'.join([self.lang(32075), self.lang(32076)]))

                if exit_install:
                    exit_osmc_settings_addon()
                    xbmc.sleep(1000)

                    subprocess.Popen(['sudo', 'systemctl', 'start', 'manual-update'])

                    return 'Running external update proceedure'

            # if the code reaches this far, the user has elected not to install right away
            # so we will need to suppress further update checks until the update occurs
            # we put a file there to make sure the suppression carries over after a reboot
            self.skip_future_update_checks()

            if not self.setting_dict['suppress_icon']:
                self.window.setProperty('OSMC_notification', 'true')

            return 'skip_update_check= %s' % self.skip_update_check

    def skip_future_update_checks(self):
        """
            Sets the conditions for future update checks to be blocked.
        """

        # create the file that will prevent further update checks until the updates
        # have been installed
        with open(self.block_update_file, 'w', encoding='utf-8') as f:
            f.write(u'd' if PY2 else 'd')

        # trigger the flag to skip update checks
        self.skip_update_check = True

    def exit_procedure(self):
        # stop the listener
        self.listener.stop()

    def check_action_queue(self):
        """
            Checks the queue for data, if present it calls the appropriate method and
            supplies the data
        """
        try:
            # the only thing the script should be sent is a tuple
            # ('instruction as string', data as dict), everything else is ignored
            raw_comm_from_script = self.parent_queue.get(False)

            # tell the queue that we are done with the task at hand
            self.parent_queue.task_done()

            # de-serialise the message into its original tuple
            comm_from_script = json.loads(raw_comm_from_script)

            log(comm_from_script, 'comm_from_script')

            # process the information from the child scripts
            if comm_from_script:
                # retrieve the relevant method
                method = self.action_dict.get(comm_from_script[0], None)
                if method:
                    # call the appropriate method with the data
                    method(**comm_from_script[1])

                else:
                    log(comm_from_script, 'instruction has no assigned method')

        except Queue.Empty:
            # the only exception that should be handled is when the queue is empty
            pass

    @staticmethod
    def check_platform_conditions():
        if os.path.isfile('/platform_no_longer_updates'):
            return False, 'Update CONDITION : platform no longer receives updates'

        return True, ''

    def check_update_conditions(self, connection_only=False):
        """
            Checks the users update conditions are met.
            Checks for:
                    - /platform_no_longer_updates file
                    - active player
                    - idle time
                    - internet connectivity
                connection_only, limits the check to just the internet connection
        """

        if not connection_only:
            check_platform, _ = self.check_platform_conditions()

            if not check_platform:
                log('Update CONDITION : platform no longer maintained')
                return False, 'Update CONDITION : platform no longer maintained'

            result_raw = xbmc.executeJSONRPC('{"jsonrpc": "2.0", '
                                             '"method": "Player.GetActivePlayers", '
                                             '"id": 1}')
            result = json.loads(result_raw)

            log(result, 'result of Player.GetActivePlayers')

            players = result.get('result', False)
            if players:
                log('Update CONDITION : player playing')

                return False, 'Update CONDITION : player playing'

            idle = xbmc.getGlobalIdleTime()
            if self.setting_dict['update_on_idle'] and idle < 60:
                return False, 'Update CONDITION : idle time = %s' % idle

        return True, ''

    def takedown_notification(self):
        try:
            self.window.removeControl(self.update_image)
        except Exception as e:
            log(e, 'an EXCEPTION occurred')

    def call_child_script(self, action):
        # check whether the install is an alpha version
        if self.check_for_unsupported_version() == 'alpha':
            return

        subprocess.Popen(['sudo', 'python', '%s/apt_cache_action.py' % self.lib_path, action])

    def position_icon(self):
        """
            Sets the position of the icon.
            Original image dimensions are 175 wide and 75 tall. This is for 1080p
        """
        self.try_image_position_again = False

        pos_horiz = self.setting_dict['pos_x'] / 100.0
        pos_vertic = self.setting_dict['pos_y'] / 100.0

        width = 175  # as % of 1920: 0.0911458333333
        height = 75  # as % of 1080: 0.0694444444444
        width_pct = 0.0911458333333
        height_pct = 0.0694444444444

        # retrieve the skin height and width (supplied by ubiquifonts and stored in Home)
        try:
            screen_height = self.window.getProperty("SkinHeight")
            screen_width = self.window.getProperty("SkinWidth")
        except:
            screen_height = 1080
            screen_width = 1920

        if screen_height == '':
            if self.try_count >= 50:
                self.try_count = 0
                screen_height = 1080
                screen_width = 1920
            else:
                self.try_image_position_again = True
                self.try_count += 1
                return

        # if the screen width is blank (for whatever reason) use the screen height to
        # estimate the width this should be very, VERY rare, and might only happen
        # due to freakish timing
        if screen_width == '':
            screen_width = screen_height * 1.7777777777

        screen_height = int(screen_height)
        screen_width = int(screen_width)

        # determine new dimensions of the image
        img_height = int(round(decimal.Decimal(screen_height * height_pct), 0))
        img_width = int(round(decimal.Decimal(screen_width * width_pct), 0))

        # determine the new coordinates of the image
        adj_height = screen_height - img_height
        adj_width = screen_width - img_width

        pos_top = int(round(decimal.Decimal(adj_height * pos_vertic), 0))
        pos_left = int(round(decimal.Decimal(adj_width * pos_horiz), 0))

        log('=============================')
        log(screen_height)
        log(screen_width)
        log(adj_height)
        log(adj_width)
        log(img_height)
        log(img_width)
        log(pos_top)
        log(pos_left)
        log('=============================')

        # reposition the image
        self.update_image.setPosition(pos_left, pos_top)

        # rescale the image
        self.update_image.setWidth(img_width)
        self.update_image.setHeight(img_height)

    def update_settings(self):
        """
            Updates the settings for the service while the service is still running
        """
        if self.first_run:
            # Construct the settings dicionary
            self.first_run = False

            self.scheduler_settings = ['check_freq', 'check_weekday', 'check_day',
                                       'check_time', 'check_hour', 'check_minute']
            self.icon_settings = ['pos_x', 'pos_y']

            self.on_upd = [self.lang(x) for x in [32057, 32058, 32095, 32060, 32061]]

            self.setting_dict = {
                'check_freq': self.addon.getSettingInt('check_freq'),
                'check_weekday': int(float(self.addon.getSetting('check_weekday'))),
                'check_day': int(float(self.addon.getSetting('check_day'))),
                'check_time': int(float(self.addon.getSetting('check_time'))),
                'check_hour': int(float(self.addon.getSetting('check_hour'))),
                'check_minute': int(float(self.addon.getSetting('check_minute'))),
                'pos_x': int(float(self.addon.getSetting('pos_x'))),
                'pos_y': int(float(self.addon.getSetting('pos_y'))),
                'suppress_progress': self.addon.getSettingBool('suppress_progress'),
                'suppress_icon': self.addon.getSettingBool('suppress_icon'),
                'update_on_idle': self.addon.getSettingBool('update_on_idle'),
                'home_prompts_only': self.addon.getSettingBool('home_prompts_only'),
                'location_selection': self.addon.getSettingInt('location_selection'),
                'backup_location': self.addon.getSetting('backup_location'),
                'backup_location_typed': self.addon.getSetting('backup_location_typed'),
                'tarball_count': int(float(self.addon.getSetting('tarball_count'))),
                'backup_on_update': self.addon.getSettingBool('backup_on_update'),
                'backup_addons': self.addon.getSettingBool('backup_addons'),
                'backup_addon_data': self.addon.getSettingBool('backup_addon_data'),
                'backup_Database': self.addon.getSettingBool('backup_Database'),
                'backup_keymaps': self.addon.getSettingBool('backup_keymaps'),
                'backup_library': self.addon.getSettingBool('backup_library'),
                'backup_playlists': self.addon.getSettingBool('backup_playlists'),
                'backup_profilesF': self.addon.getSettingBool('backup_profilesF'),
                'backup_Thumbnails': self.addon.getSettingBool('backup_Thumbnails'),
                'backup_favourites': self.addon.getSettingBool('backup_favourites'),
                'backup_keyboard': self.addon.getSettingBool('backup_keyboard'),
                'backup_remote': self.addon.getSettingBool('backup_remote'),
                'backup_LCD': self.addon.getSettingBool('backup_LCD'),
                'backup_profiles': self.addon.getSettingBool('backup_profiles'),
                'backup_RssFeeds': self.addon.getSettingBool('backup_RssFeeds'),
                'backup_sources': self.addon.getSettingBool('backup_sources'),
                'backup_upnpserver': self.addon.getSettingBool('backup_upnpserver'),
                'backup_peripheral_data': self.addon.getSettingBool('backup_peripheral_data'),
                'backup_guisettings': self.addon.getSettingBool('backup_guisettings'),
                'backup_fstab': self.addon.getSettingBool('backup_fstab'),
                'backup_advancedsettings': self.addon.getSettingBool('backup_advancedsettings'),
                'on_upd_detected': self.addon.getSettingInt('on_upd_detected')
            }
            # this is to deprecate the automatic installation of non-system updates
            # changed to Download, and Prompt
            if self.setting_dict['on_upd_detected'] == 4:
                self.addon.setSettingInt('on_upd_detected', 2)
                self.setting_dict['on_upd_detected'] = 2

            return "initial run", self.setting_dict

        else:
            '''
            Construct a temporary dictionary for comparison with the existing settings dict
            '''
            temp_settings_dict = {
                'on_upd_detected': self.addon.getSettingInt('on_upd_detected'),
                'check_freq': self.addon.getSettingInt('check_freq'),
                'check_weekday': int(float(self.addon.getSetting('check_weekday'))),
                'check_day': int(float(self.addon.getSetting('check_day'))),
                'check_time': int(float(self.addon.getSetting('check_time'))),
                'check_hour': int(float(self.addon.getSetting('check_hour'))),
                'check_minute': int(float(self.addon.getSetting('check_minute'))),
                'pos_x': int(float(self.addon.getSetting('pos_x'))),
                'pos_y': int(float(self.addon.getSetting('pos_y'))),
                'suppress_progress': self.addon.getSettingBool('suppress_progress'),
                'suppress_icon': self.addon.getSettingBool('suppress_icon'),
                'update_on_idle': self.addon.getSettingBool('update_on_idle'),
                'home_prompts_only': self.addon.getSettingBool('home_prompts_only'),
                'location_selection': self.addon.getSettingInt('location_selection'),
                'backup_location': self.addon.getSetting('backup_location'),
                'backup_location_typed': self.addon.getSetting('backup_location_typed'),
                'tarball_count': int(float(self.addon.getSetting('tarball_count'))),
                'backup_on_update': self.addon.getSettingBool('backup_on_update'),
                'backup_addons': self.addon.getSettingBool('backup_addons'),
                'backup_addon_data': self.addon.getSettingBool('backup_addon_data'),
                'backup_Database': self.addon.getSettingBool('backup_Database'),
                'backup_keymaps': self.addon.getSettingBool('backup_keymaps'),
                'backup_library': self.addon.getSettingBool('backup_library'),
                'backup_playlists': self.addon.getSettingBool('backup_playlists'),
                'backup_profilesF': self.addon.getSettingBool('backup_profilesF'),
                'backup_Thumbnails': self.addon.getSettingBool('backup_Thumbnails'),
                'backup_favourites': self.addon.getSettingBool('backup_favourites'),
                'backup_keyboard': self.addon.getSettingBool('backup_keyboard'),
                'backup_remote': self.addon.getSettingBool('backup_remote'),
                'backup_LCD': self.addon.getSettingBool('backup_LCD'),
                'backup_profiles': self.addon.getSettingBool('backup_profiles'),
                'backup_RssFeeds': self.addon.getSettingBool('backup_RssFeeds'),
                'backup_sources': self.addon.getSettingBool('backup_sources'),
                'backup_upnpserver': self.addon.getSettingBool('backup_upnpserver'),
                'backup_peripheral_data': self.addon.getSettingBool('backup_peripheral_data'),
                'backup_guisettings': self.addon.getSettingBool('backup_guisettings'),
                'backup_fstab': self.addon.getSettingBool('backup_fstab'),
                'backup_advancedsettings': self.addon.getSettingBool('backup_advancedsettings')
            }

        # flags to determine whether the update scheduler needs to be
        # reconstructed or icon repositioned
        update_scheduler = False
        reposition_icon = False

        # check the items in the temp dict and if they are different from the current
        # settings, change the current settings, prompt action if certain settings are
        # changed (like the scheduler settings)
        for k, v in temp_settings_dict.items():
            if v == self.setting_dict[k]:
                continue

            else:
                self.setting_dict[k] = v
                if k in self.scheduler_settings:
                    update_scheduler = True

                elif k in self.icon_settings:
                    reposition_icon = True

        # if the user has elected to type the backup location, then overwrite the
        # backup_location with the typed version
        if self.setting_dict['location_selection'] == 1:
            self.setting_dict['backup_location'] = self.setting_dict['backup_location_typed']

        # reconstruct the scheduler if needed
        if update_scheduler:
            self.scheduler = SimpleScheduler(self.setting_dict)

        # reposition the icon on the home screen
        if reposition_icon:
            self.position_icon()

        log(self.scheduler.trigger_time, 'trigger_time')

        return self.setting_dict

    def apt_error(self, **kwargs):
        package = kwargs.get('package', 'not provided')

        log('apt_updater encountered and error: \nException : %s \nPackage : %s \nError : %s' %
            (kwargs.get('exception', 'not provided'), package, kwargs.get('error', 'not provided')))

        # kill the progress bar
        self.progress_bar(kill=True)

        # specifically handle a failure to connect to the apt server
        if 'Unable to connect to' in kwargs.get('exception', ''):
            _ = DIALOG.ok(self.lang(32087), '[CR]'.join([self.lang(32131), self.lang(32132)]))

        else:
            # generic error handling
            # notify the user that an error has occured with an update
            _ = DIALOG.ok(self.lang(32087),
                          '[CR]'.join([self.lang(32088) % package, '', self.lang(32089)]))

    def apt_action_list_error(self, **kwargs):
        package = kwargs.get('package', 'not provided')

        log('apt_updater encountered and error: \nException : %s \nPackages : %s \nError : %s' %
            (kwargs.get('exception', 'not provided'), package, kwargs.get('error', 'not provided')))

        # kill the progress bar
        self.progress_bar(kill=True)

        # notify the user that an error has occured with an update
        _ = DIALOG.ok(self.lang(32087), '[CR]'.join([self.lang(32112), '', self.lang(32113)]))

    def action_list(self, action):
        # check whether the install is an alpha version
        if self.check_for_unsupported_version() == 'alpha':
            return

        # check for sufficient space, only proceed if it is available
        root_space, _ = self.check_target_location_for_size(location='/', requirement=300)
        if root_space:
            subprocess.Popen(['sudo', 'python', '%s/apt_cache_action.py' % self.lib_path,
                              'action_list', action])

        else:
            _ = DIALOG.ok(self.lang(32077), '[CR]'.join([self.lang(32129), self.lang(32130)]))

    def action_list_complete(self):

        # notify the user that the installation or uninstall of their desired apfs has
        # completed successfully prompt for immediate reboot if needed.

        if any([os.path.isfile('/tmp/reboot-needed'),
                os.path.isfile('fname/var/run/reboot-required')]):
            reboot = DIALOG.yesno(self.lang(32090),
                                  '[CR]'.join([self.lang(32091), self.lang(32133)]),
                                  yeslabel=self.lang(32081), nolabel=self.lang(32082))

            if reboot:
                exit_osmc_settings_addon()
                xbmc.sleep(1000)
                xbmc.executebuiltin('Reboot')

        else:
            _ = DIALOG.ok(self.lang(32090), self.lang(32091))

    def progress_bar(self, **kwargs):
        """
            Controls the creation and updating of the background prgress bar in kodi.
            The data gets sent from the apt_cache_action script via the socket
            percent, 	must be an integer
            heading,	string containing the running total of items, bytes and speed
            message, 	string containing the name of the package or the active process.
         """

        # return immediately if the user has suppressed on-screen progress
        # updates or kwargs is empty
        if self.setting_dict['suppress_progress'] or not kwargs:
            return

        # check for kill order in kwargs
        kill = kwargs.get('kill', False)

        if kill:
            # if it is present, kill the dialog and delete it

            try:
                self.progress_dialog.close()
                del self.progress_dialog
                return 'Killed pDialog'
            except:
                pass
                return 'Failed to kill pDialog'

        # retrieve the necessary data for the progress dialog, if the data isn't
        # supplied, then use 'nix' in its place the progress dialog update has
        # 3 optional arguments
        percent = kwargs.get('percent', 'nix')
        heading = kwargs.get('heading', 'nix')
        message = kwargs.get('message', 'nix')

        # create a dict of the actionable arguments
        keys = ['percent', 'heading', 'message']
        args = [percent, heading, message]
        update_args = {k: v for k, v in zip(keys, args) if v != 'nix'}

        # try to update the progress dialog
        try:
            log(update_args, 'update_args')
            self.progress_dialog.update(**update_args)

        except AttributeError:
            # on an AttributeError create the dialog and start showing it,
            # the AttributeError will be raised if pDialog doesnt exist
            self.progress_dialog = xbmcgui.DialogProgressBG()
            self.progress_dialog.create(self.lang(32077), self.lang(32078))

            self.progress_dialog.update(**update_args)

        except Exception as error:
            # on any other error, just log it and try to remove the dialog from the screen
            log(error, 'pDialog has encountered and error')

            try:
                self.progress_dialog.close()
                del self.progress_dialog
                return 'Killed pDialog'
            except:
                pass
                return 'Failed to kill pDialog'

    def kill_yourself(self):
        self.keep_alive = False

    def update_now(self):
        """
            Calls for an update check via the external script. This method checks if media is
            playing or whether the system has been idle for two minutes before allowing the
            update. If an update is requested, but media is playing or the system isn't idle,
            then the update request is put into a loop, with the daemon checking periodically
            to see if the situation has changed.
        """
        # do not do anything while there is something in the holding pattern
        if self.function_holding_pattern:
            return

        # check whether the install is an alpha version
        if self.check_for_unsupported_version() == 'alpha':
            return

        check, _ = self.check_update_conditions()
        if check:
            if self.setting_dict['backup_on_update']:
                # run the backup, once the backup is completed the script calls
                # pre_backup_complete to continue with the update that is the reason for the "else"
                self.update_settings()

                bckp = OSMCBackup(self.setting_dict, self.progress_bar, self.parent_queue)
                try:
                    bckp.start_backup()
                except Exception as e:
                    # if there is an error, then abort the Update.
                    # We dont want to run the update unless the user has backed up
                    log('Backup Error Type and Args: %s : %s \n\n %s' %
                        (type(e).__name__, e.args, traceback.format_exc()))

            else:
                # run the update
                self.call_child_script('update')

        else:
            self.function_holding_pattern = self.holding_pattern_update

    def user_update_now(self):
        """
            Similar to update_now, but as this is a users request, forego all the player and idle checks.
        """
        # check whether the install is an alpha version
        check_platform, _ = self.check_platform_conditions()

        if not check_platform:
            _ = DIALOG.ok(self.lang(32136), self.lang(32166))
            return

        if self.check_for_unsupported_version() == 'alpha':
            return

        self.call_child_script('update')

    def pre_backup_complete(self):
        """
            This method is called when the pre-update backup is completed. No need to worry
            about checking the update conditions, just run the update.
        """
        self.call_child_script('update')

    def apt_commit_complete(self):
        # on commit complete, remove the notification from the Home window
        self.window.setProperty('OSMC_notification', 'false')

        # remove the file that blocks further update checks
        try:
            os.remove(self.block_update_file)
        except:
            pass

        # run an apt-cache clean
        self.clean_apt_cache()

        if self.check_if_reboot_required():
            # the files flagging that an installed package needs a reboot are present

            # 0 "Prompt for all actions" -- PROMPT
            # 1 "Display icon on home screen only" -- PROMPT
            # 2 "Download updates, then prompt" -- PROMPT
            # 3 "Download and display icon" -- PROMPT
            # 4 "Download, install, prompt if restart needed" -- PROMPT

            # display dialogue saying that osmc needs to reboot
            reboot = DIALOG.yesno(self.lang(32077),
                                  '[CR]'.join([self.lang(32079), self.lang(32080)]),
                                  yeslabel=self.lang(32081), nolabel=self.lang(32082))

            if reboot:
                exit_osmc_settings_addon()
                _ = self.monitor.waitForAbort(1)
                xbmc.executebuiltin('Reboot')

            else:
                # skip further update checks until osmc has rebooted
                self.skip_update_check = True

    def apt_fetch_complete(self):
        # Download and display icon
        if self.setting_dict['on_upd_detected'] == 3:

            # create the file that will prevent further update checks until
            # the updates have been installed
            with open(self.block_update_file, 'w', encoding='utf-8') as f:
                f.write(u'd' if PY2 else 'd')

            # turn on the "install now" setting in Settings.xml
            self.addon.setSettingBool('install_now_visible', True)
            return 'Download complete, leaving icon displayed'

        else:
            # Download updates, then prompt
            # Download, install, prompt if restart needed (restart is needed)
            # Prompt for all actions
            if self.setting_dict['home_prompts_only']:
                self.function_holding_pattern = self.holding_pattern_fetched
                return 'Download complete, putting into holding pattern'

            else:
                self.holding_pattern_fetched(bypass=True)
                return 'Download complete, prompting user'

    def settings_command(self, action):
        """
            Dispatch user call from the addons settings.
        """
        result = ''

        if action == 'update':
            result = self.settings_command_action()

        elif action == 'backup':
            result = self.settings_command_backup()

        elif action == 'restore':
            result = self.settings_command_restore()

        elif action == 'install':
            result = self.settings_command_install()

        return result

    def settings_command_action(self):
        """
            User called for a manual update
        """
        check_connection, _ = self.check_update_conditions(connection_only=True)
        check_platform, _ = self.check_platform_conditions()

        if not check_platform:
            _ = DIALOG.ok(self.lang(32136), self.lang(32166))
            return 'manual update cancelled, platform no longer maintained'

        elif not check_connection:
            _ = DIALOG.ok(self.lang(32136), '[CR]'.join([self.lang(32137), self.lang(32138)]))
            return 'manual update cancelled, no connection'

        else:
            self.call_child_script('update_manual')
            return 'Called child action - update_manual'

    def settings_command_backup(self):
        """
            User called to initiate a backup
        """
        self.update_settings()

        bckp = OSMCBackup(self.setting_dict, self.progress_bar)
        try:
            bckp.start_backup()
        except Exception as e:
            log('Backup Error Type and Args: %s : %s \n\n %s' %
                (type(e).__name__, e.args, traceback.format_exc()))
            _ = DIALOG.ok(self.lang(32096), self.lang(32097))

        return 'Called BACKUP script complete'

    def settings_command_restore(self):
        """
            User called to initiate a restore
        """
        self.update_settings()

        bckp = OSMCBackup(self.setting_dict, self.progress_bar)
        try:
            bckp.start_restore()
            restart_required = bckp.restoring_guisettings
            if bckp.success != 'Full':
                _ = DIALOG.ok(self.lang(32139), '[CR]'.join([self.lang(32140), self.lang(32141)]))

                for x in bckp.success:
                    if x.endswith('userdata/guisettings.xml'):
                        restart_required = False

            if restart_required:
                user_input_restart_now = DIALOG.yesno(self.lang(32110),
                                                      '[CR]'.join([self.lang(32098),
                                                                   self.lang(32099)]),
                                                      yeslabel=self.lang(32100),
                                                      nolabel=self.lang(32101))

                if user_input_restart_now:
                    subprocess.Popen(['sudo', 'systemctl', 'restart', 'mediacenter'])

        except Exception as e:
            log('Backup Error Type and Args: %s : %s \n\n %s' %
                (type(e).__name__, e.args, traceback.format_exc()))
            _ = DIALOG.ok(self.lang(32096), self.lang(32097))

        return 'Called RESTORE script complete'

    def settings_command_install(self):
        """
            User called to install updates
        """
        # warn the user if there is a major Kodi update that will be installed
        # bail if they decide not to proceed
        if self.UPDATE_WARNING:
            confirm = self.display_update_warning()
            if not confirm:
                return

        ans = DIALOG.yesno(self.lang(32072), '[CR]'.join([self.lang(32075), self.lang(32076)]))

        if ans:
            self.addon.setSettingBool('install_now_visible', False)
            exit_osmc_settings_addon()
            xbmc.sleep(1000)

            subprocess.Popen(['sudo', 'systemctl', 'start', 'manual-update'])

            return "Calling external update"

    def check_for_broken_installs(self):
        try:
            apt.apt_pkg.init_config()
            apt.apt_pkg.init_system()

            self.cache = apt.apt_pkg.Cache()

        except apt.cache.LockFailedException:
            return 'bail', 'global lock placed on package system'

        except:
            return 'bail', 'apt_pkg cache failed to open'

        dirty_states = {
            apt.apt_pkg.CURSTATE_HALF_CONFIGURED,
            apt.apt_pkg.CURSTATE_HALF_INSTALLED,
            apt.apt_pkg.CURSTATE_UNPACKED
        }

        try:
            for pkg in self.cache.packages:
                if pkg.current_state in dirty_states:
                    log(' found in a partially installed state', pkg.name)
                    self.EXTERNAL_UPDATE_REQUIRED = 1
                    return 'broken install found', 'EXTERNAL_UPDATE_REQUIRED set to 1'

            else:
                return 'passed', 'no broken packages found'

        except:
            return 'bail', 'check for partially installed packages failed'

    def check_for_legit_updates(self):
        self.UPDATE_WARNING = False
        self.EXTERNAL_UPDATE_REQUIRED = 1

        # check for sufficient disk space, requirement in MB
        root_space, _ = self.check_target_location_for_size(location='/', requirement=300)
        boot_space, _ = self.check_target_location_for_size(location='/boot', requirement=30)

        if not root_space or not boot_space:
            _ = DIALOG.ok(self.lang(32077), '[CR]'.join([self.lang(32129), self.lang(32130)]))
            return 'bail', 'Sufficient freespace: root=%s, boot=%s' % (root_space, boot_space)

        check, msg = self.check_for_broken_installs()
        if check == 'bail':
            return check, msg

        try:
            self.cache = apt.Cache()
            self.cache.open(None)

        except apt.cache.LockFailedException:
            return 'bail', 'global lock placed on package system'

        except:
            return 'bail', 'apt cache failed to open'

        try:
            self.cache.upgrade(True)

        except:
            return 'bail', 'apt cache failed to upgrade'

        available_updates = []

        log('The following packages have newer versions and are upgradable: ')

        for pkg in self.cache.get_changes():
            if pkg.is_upgradable:
                log(' is upgradeable', pkg.shortname)

                available_updates.append(pkg.shortname.lower())

                # check whether the package is one that should be monitored for
                # significant version change
                if pkg.shortname in self.UPDATE_WARNING_LIST:
                    # send the package for a major update check
                    self.UPDATE_WARNING = self.check_for_major_release(pkg)

        # if 'osmc' isnt in the name of any available updates, then return without doing anything
        if not any(['osmc' in x for x in available_updates]):
            # suppress the on-screen update notification
            self.window.setProperty('OSMC_notification', 'false')

            # delete the block_update_file if it exists, so that the icon doesn't
            # display on next boot
            try:
                os.remove(self.block_update_file)
            except:
                pass

            return 'bail', 'There are no osmc packages'

        if not any([bl in av
                    for bl in self.EXTERNAL_UPDATE_REQUIRED_LIST for av in available_updates]):
            # self.EXTERNAL_UPDATE_REQUIRED = 0
            # changed to force all updates to occur with Kodi closed.
            self.EXTERNAL_UPDATE_REQUIRED = 1

        # display update available notification
        if not self.setting_dict['suppress_icon']:
            self.window.setProperty('OSMC_notification', 'true')

        # display a warning to the user
        if self.UPDATE_WARNING:
            if self.setting_dict['on_upd_detected'] not in [1, 2, 3, 4]:
                confirm_update = self.display_update_warning()

                if not confirm_update:
                    return 'bail', 'User declined to update major version of Kodi'

        return 'passed', 'legit updates available'

    def display_update_warning(self):
        """
            Displays a modal warning to the user that a major update is available,
            but that this could potentially cause addon or database incompatibility.
        """
        user_confirm = DIALOG.yesno(self.lang(32077),
                                    '[CR]'.join([self.lang(32128),
                                                 self.lang(32127),
                                                 self.lang(32126)]),
                                    yeslabel=self.lang(32125), nolabel=self.lang(32124))
        return user_confirm

    def apt_update_manual_complete(self):
        self.apt_update_complete(data='manual_update_complete')

    def apt_update_complete(self, data=None):
        check, result = self.check_for_legit_updates()

        if check == 'bail':
            if 'Sufficient freespace:' in result:
                # send kill message to progress bar
                self.progress_bar(kill=True)

            elif data == 'manual_update_complete':
                _ = DIALOG.ok(self.lang(32077), self.lang(32092))
                # send kill message to progress bar
                self.progress_bar(kill=True)

            return 'Updates not legit, bail'

        # The following section implements the procedure that the user has chosen to take
        # place when updates are detected
        if self.setting_dict['on_upd_detected'] == 0 or data == 'manual_update_complete':
            # show all prompts (default)
            if self.EXTERNAL_UPDATE_REQUIRED == 1:
                # Downloading all the debs at once require su access. So we call an external
                # script to download the updates to the default apt_cache. That other script
                # provides a progress update to this parent script, which is displayed as a
                # background progress bar
                self.call_child_script('fetch')
                return "We can't upgrade from within Kodi as it needs updating itself"

            else:
                install = DIALOG.yesno(self.lang(32072),
                                       '[CR]'.join([self.lang(32083), self.lang(32084)]))
                if install:
                    self.call_child_script('commit')  # Actually installs

                    self.window.setProperty('OSMC_notification', 'false')

                else:
                    _ = DIALOG.ok(self.lang(32072),
                                  '[CR]'.join([self.lang(32085), self.lang(32086)]))

                    # send kill message to progress bar
                    self.progress_bar(kill=True)

                    # create the file that will prevent further update checks until the
                    # updates have been installed
                    with open(self.block_update_file, 'w', encoding='utf-8') as f:
                        f.write(u'd' if PY2 else 'd')

                    # trigger the flag to skip update checks
                    self.skip_update_check = True

                return "Updates are available, no reboot is required"

        elif self.setting_dict['on_upd_detected'] == 1:
            # Display icon on home screen only
            return 'Displaying icon on home screen only'

        elif (self.setting_dict['on_upd_detected'] in [2, 3] or
              (self.setting_dict['on_upd_detected'] == 4 and self.EXTERNAL_UPDATE_REQUIRED)):
            # Download updates, then prompt
            # Download and display icon
            # Download, install, prompt if restart needed (restart is needed)
            # Download, install, auto-restart if needed
            self.call_child_script('fetch')
            return 'Downloading updates'

        elif self.setting_dict['on_upd_detected'] == 4 and not self.EXTERNAL_UPDATE_REQUIRED:
            # Download, install, prompt if restart needed (restart is not needed)
            if self.UPDATE_WARNING:
                confirm = self.display_update_warning()
                if not confirm:
                    return 'user declined to do a major version update'

            self.call_child_script('commit')
            return 'Download, install, prompt if restart needed'

    @staticmethod
    def check_for_major_release(pkg):
        """
            Checks a package to see whether it is a major release. This should trigger a
            warning to users that things might break
        """
        dig = '1234567890'

        log('Checking package (%s) for major version change.' % pkg.shortname)

        # get version of current package, raw_local_version_string
        rlv = subprocess.check_output(["/usr/bin/dpkg-query", "-W", "-f", "'${version}\n'",
                                       pkg.shortname])
        log('dpkg query results: %s' % rlv)

        lv = ''.join([x for x in rlv[:rlv.index(b"." if PY3 else ".")] if x in list(dig)])
        log('Local version number: %s' % lv)

        # get version of updating package, raw_remote_version_string
        versions = pkg.versions
        log('Versions available: %s' % versions)

        if not versions:
            return False

        rrv = versions[0].version
        log('First version selected: %s' % rrv)

        rv = ''.join([x for x in rrv[:rrv.index(".")] if x in list(dig)])
        log('Available version string: %s' % rv)

        try:
            if int(lv) < int(rv):
                return True
        except:
            pass

        return False

    @staticmethod
    def check_if_reboot_required():
        """
            Checks for the existence of two specific files that indicate an installed
            package mandates a reboot.
        """
        flag_files = ['/tmp/reboot-needed', '/var/run/reboot-required']
        return bool(any([os.path.isfile(x) for x in flag_files]))

    @staticmethod
    def clean_apt_cache():
        try:
            os.system('sudo apt-cache clean')
        except:
            pass

    def check_for_unsupported_version(self):
        """
            Checks if this version is an Alpha, prevent updates
        """
        with open(os.devnull, 'w') as fnull:
            process = subprocess.call(['/usr/bin/dpkg-query', '-l', 'rbp-mediacenter-osmc'],
                                      stderr=fnull, stdout=fnull)

        if process == 0:
            _ = DIALOG.ok(self.lang(32102), '[CR]'.join([self.lang(32103), self.lang(32104)]))
            return 'alpha'

        else:
            return 'proceed'

    @staticmethod
    def check_target_location_for_size(location, requirement):
        """
            Checks the target location to see if there is sufficient space for the update.
            Returns tuple of boolean if there is sufficient disk space and actual
            free space recorded
        """
        mb_to_b = requirement * 1048576.0

        try:
            st = os.statvfs(location)

            if st.f_frsize:
                available = st.f_frsize * st.f_bavail
            else:
                available = st.f_bsize * st.f_bavail
            # available	= st.f_bfree/float(st.f_blocks) * 100 * st.f_bsize

            log('local required disk space: %s' % mb_to_b)
            log('local available disk space: %s' % available)

            return mb_to_b < available, available / 1048570

        except:
            return False, 0

    def automatic_freespace_checker(self):
        """
            Daily checker of free space on /. Notifies user in Home window when there is
            less than 50mb remaining.
        """
        if self.freespace_supressor > 172800:
            self.freespace_supressor = 0

            freespace, value = self.check_target_location_for_size(location='/', requirement=250)
            if not freespace:
                if 'Home.xml' in xbmc.getInfoLabel('Window.Property(xmlfile)'):
                    if self.freespace_remedy == 'apt':
                        # THIS SECTION IS CURRENTLY DISABLED
                        # TO ENABLE IT CHANGE THE INIT FREESPACE_REMEDY TO 'apt'
                        resp = DIALOG.yesno(self.lang(32136),
                                            '[CR]'.join([self.lang(32142) % int(value),
                                                         self.lang(32143)]))

                        if resp:
                            subprocess.Popen(['sudo', 'apt-get', 'autoremove', '&&',
                                              'apt-get', 'clean'])

                            self.freespace_remedy = 'reboot'
                            # wait 10 minutes before next space check
                            self.freespace_supressor = 171600

                    else:
                        _ = DIALOG.ok(self.lang(32136),
                                      '[CR]'.join([self.lang(32142) % int(value),
                                                   self.lang(32144)]))
