import connman
import bluetooth
import subprocess
import sys

DEVICE_PATH = 'org.bluez.Device1' 
PAIRING_AGENT = 'osmc_bluetooth_agent.py'


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


def pair_device(deviceAddress):
    try:
        exit_status = subprocess.call([sys.executable, PAIRING_AGENT,deviceAddress])
    except:
        bluetooth.remove_device(deviceAddress)
        return False
    if not exit_status == 0:
        # if we have had issues connecting remove the device
        bluetooth.remove_device(deviceAddress)
        return False
    return True


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

