#!usr/bin/python3
# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage), 2020 OSMC (grahamh)

    This file is part of script.module.osmcsetting.networking

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import json
from optparse import OptionParser

import dbussy as dbsy
from dbussy import \
    DBUS
import ravel
import asyncio
try:
    import nest_asyncio
except ImportError:
    pass

try:
    from . import bluezutils
except ImportError:
    import bluezutils

try:
    import xbmc
except ImportError:
    xbmc = None

PEXPECT_SOL = 'SOL@'
PEXPECT_EOL = '@EOL'

BUS_NAME = 'org.bluez'
AGENT_INTERFACE = 'org.bluez.Agent1'
AGENT_PATH = "/test/agent"

bus = None
device_obj = None
dev_path = None
mainloop = None
error_message = None
osmc_bt = None
agent = None
paired = False

def return_status(result, messages):
    return_dict = {
        result: messages
    }
    print('Return status: ', return_dict)
    if xbmc:
        return osmc_bt.handle_agent_return(return_dict)
    else:
        return 'no_kodi'


def decode_response(message):
    if message.startswith(PEXPECT_SOL):
        json_str = message.replace(PEXPECT_SOL, '').replace(PEXPECT_EOL, '')

        return_value = json.loads(json_str)
        if list(return_value.keys())[0] == 'RETURN_VALUE':
            return str(list(return_value.values())[0][0])

        return None

    return message

def handle_reply(reply, action, *args):
    if reply.type == DBUS.MESSAGE_TYPE_METHOD_RETURN :
        if callable(action):
            action(*args)
        return None, None
    elif reply.type == DBUS.MESSAGE_TYPE_ERROR :
        return reply.error_name, reply.expect_objects("s")[0]
    else :
        raise ValueError("unexpected reply type %d" % reply.type)

def set_trusted(conn, trusted):
    message = dbsy.Message.new_method_call \
      (
        destination = dbsy.valid_bus_name(BUS_NAME),
        path = dbsy.valid_path(dev_path),
        iface = "org.freedesktop.DBus.Properties",
        method = "Set",
      )
    message.append_objects('ssv', "org.bluez.Device1", 'Trusted', (dbsy.DBUS.Signature('b'),trusted))
    error = None
    reply = conn.connection.send_with_reply_and_block(message, error = error)
    error_name, error_message = handle_reply(reply, None)
    if error_name:
        print('Error setting Trusted: ', error_name, error_message)

@ravel.interface(ravel.INTERFACE.SERVER, name = AGENT_INTERFACE)
class AgentInterface:

    def __init__(self, conn, capability = 'KeyboardDisplay'):
        self.conn = conn
        self.conn.register \
          (
            path = AGENT_PATH,
            fallback = True,
            interface = self
          )
        self.handle_agent_manager(True, capability)

    def close(self):
        self.handle_agent_manager(False)
        self.conn.unregister(AGENT_PATH)

    def handle_agent_manager(self, register, capability = None):
        error = None
        success = None
        message = dbsy.Message.new_method_call \
            (
               destination = dbsy.valid_bus_name(BUS_NAME),
               path = dbsy.valid_path("/org/bluez"),
               iface = "org.bluez.AgentManager1",
               method = "RegisterAgent" if register else "UnregisterAgent"
            )
        if register:
            message.append_objects('os', AGENT_PATH, capability)
            success = 'Agent registered'
        else:
            message.append_objects('o', AGENT_PATH)
            success = 'Agent unregistered'
        reply = self.conn.connection.send_with_reply_and_block(message, error = error)
        handle_reply(reply, print, success)

    @ravel.method(name = 'AuthorizeService', in_signature="os", out_signature="")
    def _AuthorizeService(self, device, uuid):
        message_list = ['AUTHORIZE_SERVICE', device, uuid]
        return_str = return_status('YESNO_INPUT', message_list)
        if return_str == 'no_kodi':
            return_str = input('Confirm Passkey: ')
        return_value = decode_response(return_str)
        if return_value == 'YES':
            return
        raise dbsy.DBusError("org.bluez.Error.Rejected", "Connection rejected by user")

    @ravel.method(name = 'RequestPinCode', in_signature="o", out_signature="s",
                  arg_keys=['device'], result_keyword='reply')
    def _RequestPinCode(self, device, reply):
        message_list = ['REQUEST_PIN', device]
        return_str = return_status('NUMERIC_INPUT', message_list)
        if return_str == 'no_kodi':
            return_str = input('Enter Pin: ')
        return_value = decode_response(return_str)
        pin = '0000'
        if return_value is not None:
            pin = return_value
        message_list = ['ENTER_PIN', str(pin)]
        return_status('NOTIFICATION', message_list)
        reply[0] = pin

    @ravel.method(name = 'RequestPasskey', in_signature="o", out_signature="u",
                  arg_keys=['device'], result_keyword='reply')
    def _RequestPasskey(self, device, reply):
        message_list = ['REQUEST_PIN', device]
        return_str = return_status('NUMERIC_INPUT', message_list)
        if return_str == 'no_kodi':
            return_str = input('Enter Passkey: ')
        return_value = decode_response(return_str)
        pin = '0000'
        if return_value is not None:
            pin = return_value
        message_list = ['ENTER_PIN', str(pin)]
        return_status('NOTIFICATION', message_list)
        reply[0] = pin

    @ravel.method(name = 'DisplayPasskey', in_signature="ouq", out_signature="")
    def _DisplayPasskey(self, device, passkey, entered):
        _ = device
        _ = entered
        message_list = ['ENTER_PIN', str(passkey)]
        return_status('NOTIFICATION', message_list)

    @ravel.method(name = 'DisplayPinCode', in_signature="os", out_signature="")
    def _DisplayPinCode(self, device, pincode):
        _ = device
        message_list = ['ENTER_PIN', str(pincode)]
        return_status('NOTIFICATION', message_list)

    @ravel.method(name = 'RequestConfirmation', in_signature="ou", out_signature="")
    def _RequestConfirmation(self, device, passkey):
        _ = device
        message_list = ['CONFIRM_PASSKEY', passkey]
        return_str = return_status('YESNO_INPUT', message_list)
        if return_str == 'no_kodi':
            return_str = input('Confirm Passkey: ')
        return_value = decode_response(return_str)
        if return_value.upper() == 'YES':
            return
        raise dbsy.DBusError("org.bluez.Error.Rejected", "Passkey doesn't match")

    @ravel.method(name = 'RequestAuthorization', in_signature="o", out_signature="")
    def _RequestAuthorization(self, device):
        message_list = ['AUTHORIZE_DEVICE', device]
        return_str = return_status('YESNO_INPUT', message_list)
        if return_str == 'no_kodi':
            return_str = input('AUTHORIZE Device: ')
        return_value = decode_response(return_str)
        if list(return_value.keys())[0] == 'RETURN_VALUE':
            if return_value == 'YES':
                return
            raise dbsy.DBusError("org.bluez.Error.Rejected", "Passkey doesn't match")

def pair_with_agent(_osmc_bt, device_uuid, adapter_pattern=None, capability='KeyboardDisplay', timeout=15000):
    global osmc_bt, mainloop, agent, dev_path, device_obj, paired
    osmc_bt = _osmc_bt
    paired = False

    try:
        device = bluezutils.find_device(device_uuid, adapter_pattern)
    except:
        device = None
        return_status('DEVICE_NOT_FOUND', ['Device not Found'])
        return False

    dev_path = device.object_path
    rvl_conn = ravel.system_bus()

    # Check if already paired
    message = dbsy.Message.new_method_call \
      (
        destination = dbsy.valid_bus_name(BUS_NAME),
        path = dbsy.valid_path(dev_path),
        iface = "org.freedesktop.DBus.Properties",
        method = "Get"
      )
    message.append_objects('ss', "org.bluez.Device1", 'Paired')
    error = None
    reply = rvl_conn.connection.send_with_reply_and_block(message, error = error)
    if error != None and reply.type == DBUS.MESSAGE_TYPE_ERROR :
        reply.set_error(error)
        result = None
        set_trusted(rvl_conn, False)
        rvl_conn = None
        return paired
    else :
        result = reply.expect_return_objects("v")[0]
        if result[0] == 'b' and result[1] == True:
            print('Already paired')
            paired = True
            set_trusted(rvl_conn, True)
            rvl_conn = None
            return paired

    # Create the agent object
    agent = AgentInterface(rvl_conn, capability)

    # This bit needed to run in Spyder3 IDE
    try:
        nest_asyncio.apply()
    except:
        pass

    # there's probably a more elegant way to do this
    try:
        mainloop = asyncio.get_running_loop()
        if mainloop:
            mainloop.stop()
    except:
        pass
    mainloop = asyncio.new_event_loop()
    rvl_conn.attach_asyncio(mainloop)

    message = dbsy.Message.new_method_call \
      (
        destination = dbsy.valid_bus_name(BUS_NAME),
        path = dbsy.valid_path(dev_path),
        iface = "org.bluez.Device1",
        method = "Pair"
      )

    async def pair(conn, message):
        print('Pairing')
        await_reply = await conn.connection.send_await_reply(message)
        print('Finished')
        return await_reply

    reply = mainloop.run_until_complete(pair(rvl_conn, message))
    error_name, error_message = handle_reply(reply, None)
    print(error_name)
    if error_name == "org.freedesktop.DBus.Error.NoReply" and device:
        error_message = 'Timed out. Cancelling pairing'
        message = dbsy.Message.new_method_call \
          (
             destination = dbsy.valid_bus_name(BUS_NAME),
             path = dbsy.valid_path(dev_path),
             iface = "org.bluez.Device1",
             method = "CancelPairing"
          )
        try:
            rvl_conn.connection.send_with_reply_and_block(message)
            set_trusted(rvl_conn, False)
        except:
            pass

    if error_message is not None:
        print('PAIRING_FAILED ' + error_message)
        return_status('PAIRING_FAILED', [error_message])
        try:
            set_trusted(rvl_conn, False)
        except:
            pass
    else:
        print('PAIRING_OK')
        return_status('PAIRING_OK', [])
        paired = True
        set_trusted(rvl_conn, True)

    agent.close()
    rvl_conn = None
    return paired

if __name__ == '__main__':
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

    if len(args) > 0:
        device_uuid = args[0]
        paired = pair_with_agent(None, device_uuid, options.adapter_pattern, options.capability, options.timeout)
