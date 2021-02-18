# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.networking

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import json
import os
import sys

import pexpect
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger

from . import bluetooth
from . import connman
from . import osmc_systemd

try:
    import xbmc
    import xbmcgui
    import xbmcaddon
except ImportError:
    xbmc = None
    xbmcgui = None
    xbmcaddon = None

DEVICE_PATH = 'org.bluez.Device1'
PAIRING_AGENT = 'osmc_bluetooth_agent.py'

BLUETOOTH_SERVICE = 'bluetooth.service'

PEXPECT_SOL = 'SOL@'
PEXPECT_EOL = '@EOL'

ADDON_ID = 'script.module.osmcsetting.networking'

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class OSMCBluetooth:

    def __init__(self, addon=None):
        self._addon = addon
        self._lang = None
        self._path = None
        self._lib_path = None

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
            self._lib_path = os.path.join(self.path, 'resources', 'lib').rstrip('/') + '/'
        return self._lib_path

    @staticmethod
    def is_bluetooth_available():
        return connman.is_technology_available('bluetooth')

    @staticmethod
    def is_bluetooth_enabled():
        connman_status = connman.is_technology_enabled('bluetooth')
        return connman_status

    @staticmethod
    def is_bluetooth_active():
        connman_status = connman.is_technology_enabled('bluetooth')
        service_status = osmc_systemd.is_service_running(BLUETOOTH_SERVICE)
        adapter_found = False
        if connman_status and service_status:
            try:
                bluetooth.get_adapter()
                adapter_found = True
            except:  # catch issue where connman reports BT but Bluez can't find an adapter
                adapter_found = False
        return connman_status and service_status and adapter_found

    @staticmethod
    def toggle_bluetooth_state(state):
        connman.toggle_technology_state('bluetooth', state)

    @staticmethod
    def get_adapter_property(key):
        return bluetooth.get_adapter_property(key)

    @staticmethod
    def set_adapter_property(key, value):
        bluetooth.set_adapter_property(key, value)

    @staticmethod
    def is_discovering():
        return bluetooth.get_adapter_property('Discovering')

    @staticmethod
    def start_discovery():
        bluetooth.start_discovery()

    @staticmethod
    def stop_discovery():
        bluetooth.stop_discovery()

    def list_paired_devices(self):
        return self.list_devices('Paired', True)

    def list_trusted_devices(self):
        return self.list_devices('Trusted', True)

    def list_discovered_devices(self):
        # assuming discovered device are non Trusted
        # (e.g. ps3 controller is never paired)
        return self.list_devices('Trusted', False)

    @staticmethod
    def get_device_property(device_address, key):
        return bluetooth.get_device_property(device_address, key)

    @staticmethod
    def set_device_property(device_address, key, value):
        bluetooth.set_device_property(device_address, key, value)

    @staticmethod
    def is_device_paired(device_address):
        return bluetooth.get_device_property(device_address, 'Paired')

    @staticmethod
    def remove_device(device_address):
        bluetooth.remove_device(device_address)

    @staticmethod
    def is_device_trusted(device_address):
        return bluetooth.get_device_property(device_address, 'Trusted')

    def set_device_trusted(self, device_address, value):
        self.set_device_property(device_address, 'Trusted', value)

    @staticmethod
    def is_device_connected(device_address):
        return bluetooth.get_device_property(device_address, 'Connected')

    def connect_device(self, device_address):
        try:
            bluetooth.connect_device(device_address)
            self.set_device_trusted(device_address, True)
        except:
            return False

        return True

    @staticmethod
    def disconnect_device(device_address):
        bluetooth.disconnect_device(device_address)

    @staticmethod
    def list_devices(filter_key=None, expected_value=None):
        """
            returns a dictionary with the key being the device address
            and value being a dictionary of device information
        """
        devices = {}
        managed_objects = bluetooth.get_managed_objects()

        for path in managed_objects.keys():
            if path.startswith('/org/bluez/hci') and DEVICE_PATH in managed_objects[path].keys():
                dbus_dict = managed_objects[path][DEVICE_PATH]
                device_dict = {}

                # remove dbus.String from the key
                for key in dbus_dict:
                    device_dict[str(key)] = dbus_dict[key]

                if filter_key is None or device_dict[filter_key] == expected_value:
                    devices[str(device_dict['Address'])] = device_dict

        return devices

    @staticmethod
    def encode_return(result, messages):
        return_value = {
            result: messages
        }
        return PEXPECT_SOL + json.dumps(return_value) + PEXPECT_EOL

    def pair_device(self, device_address, script_path=''):
        if not script_path:
            script_path = self.lib_path

        print('Attempting to pair with ' + device_address)
        paired = self.pair_using_agent(device_address, script_path)
        if not paired:
            try:
                bluetooth.remove_device(device_address)
            except:
                pass
            return False

        return paired

    def pair_using_agent(self, device_address, script_path=''):
        if not script_path:
            script_path = self.lib_path
        script_path = script_path + PAIRING_AGENT
        script = ' '.join([sys.executable, script_path, device_address])

        paired = False
        print('calling agent "' + script + '"')
        child = pexpect.spawn(script)

        while True:
            try:
                _ = child.expect(['@EOL'], timeout=None)
                split = child.before.split('SOL@')
                if len(split[0]) > 0:
                    log('Output From Pairing Agent ' + split[0])
                d = json.loads(split[1])
                return_value = d.keys()[0]
                messages = d.values()[0]
                log(['return_value = ' + return_value, 'Messages = ' + str(messages)])
                if return_value == 'PAIRING_OK':
                    paired = True
                    break
                if return_value == 'DEVICE_NOT_FOUND':
                    return False  # return early no need to call remove_device()
                if xbmc:
                    device_alias = self.get_device_property(device_address, 'Alias')
                    return_value = self.handle_agent_interaction(device_alias,
                                                                 return_value, messages)
                    if return_value:
                        if return_value == 'NO_PIN_ENTERED':
                            return False
                        send_str = self.encode_return('RETURN_VALUE', [return_value])
                        child.sendline(send_str)
            except pexpect.EOF:
                break

        return paired

    def handle_agent_interaction(self, device_alias, command, messages):
        supported_commands = ['NOTIFICATION', 'YESNO_INPUT', 'NUMERIC_INPUT']
        if command not in supported_commands:
            return None

        # This method is only called when we are running in Kodi
        dialog = xbmcgui.Dialog()
        message = ''
        #         'Bluetooth Pairing'
        heading = self.lang(32026)
        if messages[0] == 'ENTER_PIN':
            #         'Please enter the following on the device'
            message = self.lang(32027) + ' ' + messages[1]
        if messages[0] == 'AUTHORIZE_SERVICE':
            #         'Authorize Service '                 ' on device '
            message = self.lang(32027) + ' ' + messages[0] + ' ' + \
                      self.lang(32029) + ' ' + device_alias
        if messages[0] == 'REQUEST_PIN':
            #         'Enter PIN to Pair with'
            message = self.lang(32030) + ' ' + device_alias
        if messages[0] == 'CONFIRM_PASSKEY':
            #           'Confirm passkey'                      'for'
            message = self.lang(32031) + ' ' + str(messages[1]) + ' ' + \
                      self.lang(32032) + ' ' + device_alias

        if command == 'NOTIFICATION':
            dialog.notification(heading, message, time=10000, sound=False)

        if command == 'YESNO_INPUT':
            if dialog.yesno(heading, message):
                return 'YES'

            return 'NO'

        if command == 'NUMERIC_INPUT':
            value = dialog.numeric(0, message)
            if len(value) == 0:
                value = 'NO_PIN_ENTERED'

            return value

        return None
