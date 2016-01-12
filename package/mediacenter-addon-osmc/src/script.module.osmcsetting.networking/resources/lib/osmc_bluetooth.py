import connman
import bluetooth
import osmc_systemd
import sys
import pexpect
import json

RUNNING_IN_KODI = True

# XBMC Modules
try:
    import xbmc
    import xbmcgui
    import xbmcaddon
except:
    RUNNING_IN_KODI = False


DEVICE_PATH = 'org.bluez.Device1' 
PAIRING_AGENT = 'osmc_bluetooth_agent.py'

BLUETOOTH_SERVICE = 'bluetooth.service'

PEXPECT_SOL = 'SOL@'
PEXPECT_EOL = '@EOL'


def log(message):

    try:
        message = str(message)
    except UnicodeEncodeError:
        message = message.encode('utf-8', 'ignore' )

    msg_str='OSMC_BLUETOOTH -  ' + str(message)
    if RUNNING_IN_KODI:
        xbmc.log(msg_str, level=xbmc.LOGDEBUG)
    else:
        print(msg_str)


def is_bluetooth_available():
    return connman.is_technology_available('bluetooth')


def is_bluetooth_enabled():
    connman_status = connman.is_technology_enabled('bluetooth')
    service_status = osmc_systemd.is_service_running(BLUETOOTH_SERVICE)
    adapterFound = False
    if connman_status and service_status:
        try:
            bluetooth.get_adapter()
            adapterFound = True
        except: #  catch issue where connman reports BT but Bluez can't find an adapter
            adapterFound = False
    return connman_status and service_status and adapterFound


def toggle_bluetooth_state(state):
    if state:
        if not osmc_systemd.is_service_running(BLUETOOTH_SERVICE):
            osmc_systemd.toggle_service(BLUETOOTH_SERVICE, state)
        connman.toggle_technology_state('bluetooth', state)
    else:
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


def list_trusted_devices():
    return list_devices('Trusted', True)

def list_discovered_devices():
    # assuming discovered device are non Trusted
    # (e.g. ps3 controller is never paired)
    return list_devices('Trusted', False)


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
    try:
        bluetooth.connect_device(deviceAddress)
        set_device_trusted(deviceAddress, True)
    except:
        return False
    return True

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
    print 'Attempting to pair with ' + deviceAddress
    paired = False
    paired = pair_using_agent(deviceAddress, scriptBasePath)
    if not paired:
        try:
            bluetooth.remove_device(deviceAddress)
        except:
            pass
        return False
    return paired


def pair_using_agent(deviceAddress, scriptBasePath = ''):
    script_path = scriptBasePath + PAIRING_AGENT
    script = str.join(' ', [sys.executable, script_path,deviceAddress])
    paired = False
    print 'calling agent "' + script + '"'
    child = pexpect.spawn(script)
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
                paired = True
                break
            if return_value == 'DEVICE_NOT_FOUND':
                return False  # return early no need to call remove_device()
            if RUNNING_IN_KODI:
                deviceAlias = get_device_property(deviceAddress, 'Alias')
                returnValue = handleAgentInteraction(deviceAlias, return_value , messages)
                if returnValue:
                    if returnValue == 'NO_PIN_ENTERED':
                        return False
                    sendStr = encode_return('RETURN_VALUE', [ returnValue ])
                    child.sendline(sendStr)
        except pexpect.EOF:
            break
    return paired


def handleAgentInteraction(deviceAlias, command , messages):
    supported_commands = ['NOTIFICATION', 'YESNO_INPUT', 'NUMERIC_INPUT']
    if not command in supported_commands:
        return None
    
    # This method is only called when we are running in Kodi 
    dialog = xbmcgui.Dialog()
    #         'Bluetooth Pairing'
    heading = lang(32026)
    if messages[0] == 'ENTER_PIN':
        #         'Please enter the following on the device'
        message = lang(32027) + ' ' + messages[1]
    if messages[0] == 'AUTHORIZE_SERVICE':
        #         'Authorize Service '                 ' on device ' 
        message = lang(32027) + ' ' + messages[0] + ' ' + lang(32029) + ' ' + deviceAlias
    if messages[0] == 'REQUEST_PIN':
        #         'Enter PIN to Pair with'
        message = lang(32030) + ' ' + deviceAlias
    if messages[0] == 'CONFIRM_PASSKEY':
       #           'Confirm passkey'                      'for'
        message = lang(32031)+ ' ' + str(messages[1]) + ' ' + lang(32032) + ' ' + deviceAlias

    if command == 'NOTIFICATION':
        xbmc.executebuiltin("XBMC.Notification(%s,%s,%s)" % (heading, message, "10000"))
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


def lang(id):
    addon = xbmcaddon.Addon('script.module.osmcsetting.networking')
    san =addon.getLocalizedString(id).encode( 'utf-8', 'ignore' )
    return san 

