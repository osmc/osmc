#!usr/bin/python
# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.networking

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import json
from optparse import OptionParser

import dbus
import dbus.mainloop.glib
import dbus.service

from . import bluezutils

try:
    from gi.repository import GObject
except ImportError:
    import gobject as GObject

try:
    input = raw_input
except NameError:
    pass

PEXPECT_SOL = 'SOL@'
PEXPECT_EOL = '@EOL'

BUS_NAME = 'org.bluez'
AGENT_INTERFACE = 'org.bluez.Agent1'
AGENT_PATH = "/test/agent"

bus = None
device_obj = None
dev_path = None


def return_status(result, messages):
    return_dict = {
        result: messages
    }
    print(PEXPECT_SOL + json.dumps(return_dict) + PEXPECT_EOL)


def decode_response(message):
    if message.startswith(PEXPECT_SOL):
        json_str = message.replace(PEXPECT_SOL, '').replace(PEXPECT_EOL, '')

        return_value = json.loads(json_str)
        if return_value.keys()[0] == 'RETURN_VALUE':
            return str(return_value.values()[0][0])

        return None

    return message


def set_trusted(path, boolean):
    props = dbus.Interface(bus.get_object("org.bluez", path),
                           "org.freedesktop.DBus.Properties")
    props.Set("org.bluez.Device1", "Trusted", boolean)


class Rejected(dbus.DBusException):
    _dbus_error_name = "org.bluez.Error.Rejected"


class Agent(dbus.service.Object):
    exit_on_release = True
    return_code = 9
    error_message = None

    def set_return(self, return_code, error_message):
        self.return_code = return_code
        self.error_message = error_message

    def set_exit_on_release(self, exit_on_release):
        self.exit_on_release = exit_on_release

    @dbus.service.method(AGENT_INTERFACE, in_signature="", out_signature="")
    def Release(self):
        if self.exit_on_release:
            mainloop.quit()

    @dbus.service.method(AGENT_INTERFACE, in_signature="os", out_signature="")
    def AuthorizeService(self, device, uuid):
        message_list = ['AUTHORIZE_SERVICE', device, uuid]
        return_status('YESNO_INPUT', message_list)
        return_str = input('Confirm Passkey:')
        return_value = decode_response(return_str)
        if return_value == 'YES':
            return
        raise Rejected("Connection rejected by user")

    @dbus.service.method(AGENT_INTERFACE, in_signature="o", out_signature="s")
    def RequestPinCode(self, device):
        message_list = ['REQUEST_PIN', device]
        return_status('NUMERIC_INPUT', message_list)
        return_str = input('Enter Pin: ')
        return_value = decode_response(return_str)
        pin = '0000'
        if return_value is not None:
            pin = return_value
        message_list = ['ENTER_PIN', str(pin)]
        return_status('NOTIFICATION', message_list)
        set_trusted(device, True)
        return pin

    @dbus.service.method(AGENT_INTERFACE, in_signature="o", out_signature="u")
    def RequestPasskey(self, device):
        message_list = ['REQUEST_PIN', device]
        return_status('NUMERIC_INPUT', message_list)
        return_str = input('Enter Pin: ')
        return_value = decode_response(return_str)
        pin = '0000'
        if return_value is not None:
            pin = return_value
        message_list = ['ENTER_PIN', str(pin)]
        return_status('NOTIFICATION', message_list)
        return dbus.UInt32(pin)

    @dbus.service.method(AGENT_INTERFACE, in_signature="ouq", out_signature="")
    def DisplayPasskey(self, device, passkey, entered):
        _ = device
        _ = entered
        message_list = ['ENTER_PIN', str(passkey)]
        return_status('NOTIFICATION', message_list)

    @dbus.service.method(AGENT_INTERFACE, in_signature="os", out_signature="")
    def DisplayPinCode(self, device, pincode):
        _ = device
        message_list = ['ENTER_PIN', str(pincode)]
        return_status('NOTIFICATION', message_list)

    @dbus.service.method(AGENT_INTERFACE, in_signature="ou", out_signature="")
    def RequestConfirmation(self, device, passkey):
        _ = device
        message_list = ['CONFIRM_PASSKEY', passkey]
        return_status('YESNO_INPUT', message_list)
        return_str = input('Confirm Passkey:')
        return_value = decode_response(return_str)
        if return_value == 'YES':
            return
        raise Rejected("Passkey doesn't match")

    @dbus.service.method(AGENT_INTERFACE, in_signature="o", out_signature="")
    def RequestAuthorization(self, device):
        message_list = ['AUTHORIZE_DEVICE', device]
        return_status('YESNO_INPUT', message_list)
        return_str = input('AUTHORIZE Device:')
        return_value = decode_response(return_str)
        if return_value.keys()[0] == 'RETURN_VALUE':
            if return_value == 'YES':
                return
            raise Rejected("Passkey doesn't match")


def pair_successful():
    set_trusted(dev_path, True)
    agent.set_return(0, None)
    mainloop.quit()


def pair_error(error):
    set_trusted(dev_path, False)
    err_name = error.get_dbus_name()

    if err_name == "org.freedesktop.DBus.Error.NoReply" and device_obj:
        agent.set_return(1, ' Timed out. Cancelling pairing')
        device_obj.CancelPairing()

    else:
        if error.get_dbus_message() == 'Already Exists':
            # we do not want to return an error code in this case
            agent.set_return(0, None)
        else:
            agent.set_return(1, error.get_dbus_message())

        mainloop.quit()


if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()
    parser = OptionParser()
    parser.add_option("-i", "--adapter", action="store",
                      type="string",
                      dest="adapter_pattern",
                      default=None)
    parser.add_option("-c", "--capability", action="store",
                      type="string", dest="capability",
                      default="KeyboardDisplay")
    parser.add_option("-t", "--timeout", action="store",
                      type="int", dest="timeout",
                      default=60000)
    (options, args) = parser.parse_args()

    agent = Agent(bus, AGENT_PATH)
    mainloop = GObject.MainLoop()
    obj = bus.get_object(BUS_NAME, "/org/bluez")
    manager = dbus.Interface(obj, "org.bluez.AgentManager1")
    manager.RegisterAgent(AGENT_PATH, options.capability)

    if len(args) > 0:
        try:
            _device = bluezutils.find_device(args[0],
                                             options.adapter_pattern)
        except:
            _device = None
            return_status('DEVICE_NOT_FOUND', ['Device not Found'])
            exit(1)

        dev_path = _device.object_path
        agent.set_exit_on_release(False)
        _device.Pair(reply_handler=pair_successful, error_handler=pair_error,
                     timeout=options.timeout)
        device_obj = _device

    mainloop.run()
    manager.UnregisterAgent(AGENT_PATH)

    if agent.error_message is not None:
        return_status('PAIRING_FAILED', [agent.error_message])
    else:
        return_status('PAIRING_OK', [])
