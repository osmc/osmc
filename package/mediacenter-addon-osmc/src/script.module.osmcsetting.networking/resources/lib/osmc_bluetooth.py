import connman
import bluetooth
import subprocess
import sys
import pexpect
import os.path
import json

RUNNING_IN_KODI = True

# XBMC Modules
try:
    import xbmc
    import xbmcgui
except:
    RUNNING_IN_KODI = False

DEVICE_PATH = 'org.bluez.Device1' 
PAIRING_AGENT = 'osmc_bluetooth_agent.py'

PEXPECT_SOL = 'SOL@'
PEXPECT_EOL = '@EOL'

def log(message):
    msg_str='OSMC_BLUETOOTH -  ' + str(message)
    if RUNNING_IN_KODI:
        xbmc.log(msg_str, level=xbmc.LOGDEBUG)
    else:
        print(msg_str)

def is_bluetooth_available():
    return connman.is_technology_available('bluetooth')


def is_bluetooth_enabled():
    return connman.is_technology_enabled('bluetooth')


def toggle_bluetooth_state(state):
    connman.toggle_technology_state('bluetooth', state)


def get_adapter_property(key):
    return bluetooth.get_adapter_property(key)


def set_adapter_property(key, value):
    bluetooth.set_adapter_property(key, value)


def is_discovering():
    return bluetooth.get_adapter_property('Discovering')


def start_discovery():
    bluetooth.start_discovery()


def stop_discovery():
    bluetooth.stop_discovery()


def list_paired_devices():
    return list_devices('Paired', True)


def list_discovered_devices():
    # assuming discovered device are non paired
    return list_devices('Paired', False)


def get_device_property(deviceAddress, key):
    return bluetooth.get_device_property(deviceAddress, key)


def set_device_property(deviceAddress, key, value):
    bluetooth.set_device_property(deviceAddress, key, value)


def is_device_paired(deviceAddress):
    return bluetooth.get_device_property(deviceAddress, 'Paired')


def remove_device(deviceAddress):
    bluetooth.remove_device(deviceAddress)


def is_device_trusted(deviceAddress):
    return bluetooth.get_device_property(deviceAddress, 'Trusted')


def set_device_trusted(deviceAddress, value):
    set_device_property(deviceAddress,'Trusted', value)


def is_device_connected(deviceAddress):
    return bluetooth.get_device_property(deviceAddress, 'Connected')


def connect_device(deviceAddress):
    bluetooth.connect_device(deviceAddress)


def disconnect_device(deviceAddress):
    bluetooth.disconnect_device(deviceAddress)


'''
 returns a dictionary with the key being the device address
   and value being a dictionary of device information
'''    
def list_devices(filterkey=None, expectedvalue=None):
    devices = {}
    managed_objects = bluetooth.get_managed_objects()
    for path in managed_objects.keys():
        if path.startswith('/org/bluez/hci') and DEVICE_PATH in managed_objects[path].keys():
                dbus_dict = managed_objects[path][DEVICE_PATH]
                device_dict = {}
                # remove dbus.String from the key
                for key in dbus_dict:
                    device_dict[str(key)] = dbus_dict[key]
                if filterkey == None or device_dict[filterkey] == expectedvalue:
                    devices[str(device_dict['Address'])] = device_dict
    return devices

def encode_return(result, messages):
    return_value = {result : messages}
    return PEXPECT_SOL+ json.dumps(return_value) + PEXPECT_EOL

def pair_device(deviceAddress, scriptBasePath = ''):
    script_path = scriptBasePath + PAIRING_AGENT
    script = str.join(' ', [sys.executable, script_path,deviceAddress])
    child = pexpect.spawn(script)
    paired = False
    while True:
        try:
            index = child.expect(['@EOL'], timeout=None)
            split = child.before.split('SOL@')
            if len(split[0]) >0:
                log('Output From Pairing Agent ' + split[0])
            d = json.loads(split[1])
            return_value = d.keys()[0]
            messages = d.values()[0]
            log(['return_value = '+ return_value, 'Messages = ' + str(messages)])
            if return_value == 'PAIRING_OK':
                paired =  True
                break
            if return_value == 'DEVICE_NOT_FOUND':
                return False  # return early no need to call remove_device()
            if RUNNING_IN_KODI:
                returnValue = handleAgentInteraction(deviceAddress, return_value , messages)
                if returnValue:
                    sendStr = encode_return('RETURN_VALUE', [ returnValue ])
                    child.sendline(sendStr)
        except pexpect.EOF:
            break
    if not paired:
        bluetooth.remove_device(deviceAddress)
        return False
    return True

def handleAgentInteraction(deviceAddress, command , messages):
    supported_commands = ['OK_DIALOGUE', 'YESNO_INPUT', 'NUMERIC_INPUT']
    if not command in supported_commands:
        return None

    dialog = xbmcgui.Dialog()
    # Needs translation strings
    heading = 'Bluetooth Pairing'
    if messages[0] == 'ENTER_PIN':
        message = 'Please enter the following on the device ' + messages[1]
    if messages[0] == 'AUTHORIZE_SERVICE':
        message = 'Authorize Sevice ' + messages[0] + ' on device ' + deviceAddress
    if messages[0] == 'REQUEST_PIN':
        message = 'Enter PIN to Pair With ' + deviceAddress
    if messages[0] == 'CONFIRM_PASSKEY':
        message = 'Confirm passkey ' +messages[0] + ' for ' + deviceAddress

    if command == 'OK_DIALOGUE':
        dialog.ok(heading, message)
    if command == 'YESNO_DIALOGUE':
        if dialog.yesno(heading, message):
            return 'YES'
        return 'NO'
    if command == 'NUMERIC_INPUT':
        return  dialog.numeric(0, message)
                    
    return None
