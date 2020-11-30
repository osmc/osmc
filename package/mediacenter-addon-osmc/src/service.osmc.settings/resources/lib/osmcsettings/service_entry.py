#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of service.osmc.settings

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import datetime
import json
import os
import socket
import subprocess
import sys
import traceback
from contextlib import closing
from copy import deepcopy
from io import open

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_comms import Communicator
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger

from . import osmc_settings_gui
from . import osmc_ubiquifonts
from . import osmc_walkthru

try:
    import queue as Queue
except ImportError:
    import Queue

ADDON_ID = 'service.osmc.settings'
ADDON = xbmcaddon.Addon(ADDON_ID)

DIALOG = xbmcgui.Dialog()
PY3 = sys.version_info.major == 3

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log
lang = LangRetriever(ADDON).lang


class Main(object):

    def __init__(self, window):
        log('main addon starting')

        self.window = window
        self.set_version()

        # queue for communication with the comm and Main
        self.parent_queue = Queue.Queue()

        # create socket, listen for comms
        self.listener = Communicator(self.parent_queue, socket_file='/var/tmp/osmc.settings.sockfile')
        self.listener.start()

        # the gui is created and stored in memory for quick access
        # after a few hours, the gui should be removed from memory
        self.stored_gui = None
        self.create_gui()
        self.gui_last_accessed = datetime.datetime.now()
        self.skip_check = True

        # monitor created to check for xbmc abort requests
        self.monitor = xbmc.Monitor()

        # current skin directory, used to detect when the user has changed skins and prompts a reconstruction of the gui
        self.skin_dir = xbmc.getSkinDir()

        self.fonts = osmc_ubiquifonts.UbiquiFonts(ADDON_ID, ADDON, self.window)

        # run the ubiquifonts script to import the needed fonts into the Font.xml
        _ = self.fonts.import_osmc_fonts()

        if not os.path.isfile('/walkthrough_completed'):
            # Tell Kodi that OSMC is running the walkthrough
            try:
                xbmc.setosmcwalkthroughstatus(1)
            except Exception:
                log(traceback.format_exc())
        else:
            try:
                # Tell Kodi that OSMC is done
                xbmc.setosmcwalkthroughstatus(2)
            except Exception:
                log(traceback.format_exc())

        # daemon
        self._daemon()

        log('_daemon exited')

    def create_gui(self):
        self.stored_gui = osmc_settings_gui.GuiThread(queue=self.parent_queue,
                                                      addon=ADDON, window=self.window)
        self.stored_gui.setDaemon(True)

    def _daemon(self):
        log('daemon started')

        if not os.path.isfile('/walkthrough_completed'):
            self._walk_through()  # start walk through

        while not self.monitor.abortRequested():
            if not self.monitor.abortRequested() and not self.parent_queue.empty():
                response = self.parent_queue.get()

                log('response : %s' % response)

                self.parent_queue.task_done()

                abort_requested = self._handle_response(response=response)
                if abort_requested:
                    break

            # sleep for one second, exit if Kodi is shutting down
            if self.monitor.waitForAbort(1):
                break

            abort_requested = self._check_skin()
            if abort_requested:
                break

        log('_daemon exiting')
        self.exit()
        log('_daemon shutdown')

    def exit(self):
        # try to kill the gui and comms
        try:
            log('Stopping listener (in wait)')
            self.listener.stop()
            log('Deleting listener (in wait)')
            del self.listener
            log('Listener deleted.')

        except:
            log('Failed to stop/delete listener. (in wait)')

    @staticmethod
    def get_sources_list():
        query = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "Files.GetSources",
            "params": {}
        }
        xbmc_request = json.dumps(query)
        result_raw = xbmc.executeJSONRPC(xbmc_request)
        result = json.loads(result_raw)
        media_dict_raw = result.get('result', {}).get('sources', {})
        media_list_raw = [v.get('file', '') for k, v in media_dict_raw.items()]
        media_string = ''.join(media_list_raw)

        return media_string

    def open_gui(self):
        log('Opening OSMC settings GUI')

        self.gui_last_accessed = datetime.datetime.now()
        self.skip_check = False
        try:
            # try opening the gui
            self.stored_gui.start()
        except:
            # if that doesnt work then it is probably because the gui was too old and has been deleted
            # so recreate the gui and open it
            self.create_gui()
            self.stored_gui.start()

        log('gui threading finished')

    @staticmethod
    def set_walkthrough_status(value):
        try:
            # Tell Kodi that OSMC is done
            xbmc.setosmcwalkthroughstatus(int(value))
        except Exception:
            log(traceback.format_exc())

    def check_vendor(self):
        """ Checks whether OSMC is being installed via N00bs or ts.
            Returns None if the vendor file is not present or does not contain 'noobs' or 'ts'.
            Vendor is pass to the Main settings service, which then asks the user whether they would like
            to update (noobs or ts only).
        """
        current_vendor = self.window.getProperty('osmc_vendor')
        if current_vendor:
            return current_vendor

        current_vendor = None
        if os.path.isfile('/vendor'):
            with open('/vendor', 'r', encoding='utf-8') as f:
                line = f.readline()

            vendors = ['noobs', 'ts']
            for vendor in vendors:
                if vendor in line:
                    current_vendor = vendor
                    break

        self.window.clearProperty('osmc_vendor')
        if current_vendor:
            self.window.setProperty('osmc_vendor', current_vendor)

        return current_vendor

    def set_version(self, overwrite=False):
        """ Loads the current OSMC version into the Home window for display in MyOSMC """

        # Check for "upgraded" Alpha 4 and earlier

        if not overwrite and self.window.getProperty('osmc_version'):
            return

        with open(os.devnull, 'w') as fnull:
            process = subprocess.call(['/usr/bin/dpkg-query', '-l', 'rbp-mediacenter-osmc'], stderr=fnull, stdout=fnull)

        if process == 0:

            version_string = 'Unsupported OSMC Alpha release'

        else:

            version = []

            with open('/etc/os-release', 'r', encoding='utf-8') as f:
                lines = f.readlines()

            tags = ['NAME=', 'VERSION=', 'VERSION_ID=']
            for line in lines:

                for tag in tags:

                    if line.startswith(tag):
                        version.append(line[len(tag):].replace('"', '').replace('\n', ''))

            version_string = ' '.join(version)

        log('Current Version: %s' % version_string)

        self.window.setProperty('osmc_version', version_string)

    def _check_skin(self):
        # Check the current skin directory, if it is different to the previous one, then
        # recreate the gui. This is required because reference in the gui left in memory
        # do not survive a refresh of the skins textures (???)
        if self.skin_dir != xbmc.getSkinDir():

            log('Old Skin: %s' % self.skin_dir)

            self.skin_dir = xbmc.getSkinDir()

            log('New Skin: %s' % self.skin_dir)

            try:
                resp = self.fonts.import_osmc_fonts()

                log('Ubiquifonts result: %s' % resp)

                if resp == 'reload_please':

                    while not self.monitor.abortRequested():

                        if self.monitor.waitForAbort(1):
                            return True

                        xml = xbmc.getInfoLabel('Window.Property(xmlfile)')

                        if xml not in ['DialogYesNo.xml', 'Dialogyesno.xml', 'DialogYesno.xml', 'DialogyesNo.xml', 'dialogyesno.xml']:
                            log('Skin reload requested')

                            xbmc.executebuiltin('ReloadSkin()')

                            break

            except Exception:
                log(traceback.format_exc())

            try:
                log('skin changed, reloading gui')
                del self.stored_gui
            except:
                pass

            self.create_gui()

        return False

    def _walk_through(self):
        # Tell Kodi that OSMC is running the walkthrough
        self.window.setProperty("walkthrough_is_running", 'any_value')
        self.set_walkthrough_status(1)

        network_module = next(iter(module for module in self.stored_gui.live_modules
                                   if module.get('id') == 'osmcnetworking'), None)

        if not network_module:
            log('Networking module not found')
        else:
            vendor = self.check_vendor()
            log("Vendor is %s" % vendor)

            osmc_walkthru.open_gui(ADDON, networking_instance=network_module['class_instance'])

            with open('/tmp/walkthrough_completed', 'w+', encoding='utf-8') as _:
                log('/tmp/walkthrough_completed written')

            subprocess.call(['sudo', 'mv', '/tmp/walkthrough_completed', '/walkthrough_completed'])

            self.set_walkthrough_status(2)
            self.window.clearProperty('walkthrough_is_running')
            del self.stored_gui
            xbmc.executebuiltin('ReloadSkin()')

            log('Skin reloaded')

            # Query user about whether they would like to update now
            update_check_now = False

            if vendor == 'noobs':
                update_check_now = DIALOG.yesno(lang(32026), '[CR]'.join([lang(32027), lang(32028), lang(32029)]))

            elif vendor == 'ts':
                update_check_now = DIALOG.yesno(lang(32026), '[CR]'.join([lang(32030), lang(32031), lang(32029)]))

            if update_check_now:
                log('User elected to update now')

                try:
                    message = ('settings_command', {
                        'action': 'update'
                    })
                    message = json.dumps(message)

                    with closing(socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)) as open_socket:
                        open_socket.connect('/var/tmp/osmc.settings.update.sockfile')
                        if PY3 and not isinstance(message, (bytes, bytearray)):
                            message = message.encode('utf-8', 'ignore')
                        open_socket.sendall(message)

                except Exception:
                    log(traceback.format_exc())

    def _handle_response(self, response):
        if response == 'open':
            self.open_gui()

        elif response == 'refresh_gui':
            ''' 
                This may need to be moved to a separate thread, so that it doesnt hold up the other functions.
                if the gui calls for its own refresh, then delete the existing one and open a new instance 
            '''
            self.open_gui()

        elif response == 'exit':
            return True  # signal abort

        elif response == 'walkthru':
            log('Running manually called walkthru')

            osmc_walkthru.open_gui(ADDON, networking_instance=None, testing=True)

        elif 'new_device:' in response:
            # a usb device is attached to the hardware

            # get the device id
            device_id = response[len('new_device:'):]

            # proceed only if the device_id is not null
            if device_id:

                # get ignore list
                ignore_list_raw = ADDON.getSetting('ignored_devices')
                ignore_list_initial = ignore_list_raw.split('|')
                ignore_list = deepcopy(ignore_list_initial)

                # get sources list
                media_string = self.get_sources_list()

                # post dialogs to ask the user if they want to add the source, or ignore the device
                if device_id not in ignore_list and device_id not in media_string:

                    result = DIALOG.yesno(lang(32002), '[CR]'.join([lang(32003), lang(32004)]))

                    if result:
                        xbmc.executebuiltin("ActivateWindow(mediasource)")

                    else:
                        result = DIALOG.yesno(lang(32002), lang(32005))

                        if result:
                            ignore_list.append(str(device_id))

                if ignore_list_initial != ignore_list:
                    ignore_string = '|'.join(ignore_list)
                    ADDON.setSetting('ignored_devices', ignore_string)

        else:
            # check whether the response is one of the live_modules, if it is then launch that module
            for module in self.stored_gui.live_modules:

                module_id = module.get('id', 'id_not_found')
                if response != module_id:
                    continue

                class_instance = module.get('class_instance', None)
                module_instance = module.get('module_instance', None)

                if class_instance and class_instance.isAlive():
                    log('Opening %s from widget' % module_id)
                    class_instance.run()

                elif module_instance:
                    log('Starting %s from widget' % module_id)

                    class_instance = module_instance.OSMCSettingClass()
                    class_instance.setDaemon(True)

                    module['class_instance'] = class_instance

                    class_instance.start()

                break

        return False
