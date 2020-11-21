import dbus

from . import bluezutils

BLUEZ_OBJECT_PATH = 'org.bluez'
BLUEZ_ADAPTER = 'org.bluez.Adapter1'
BLUEZ_DEVICE = 'org.bluez.Device1'

bus = dbus.SystemBus()


def get_adapter_property(key, adapter_address=None):
    adapter = get_adapter_interface(adapter_address)
    value = adapter.Get(BLUEZ_ADAPTER, key)
    if isinstance(value, dbus.Boolean):
        return bool(value)
    return value


def set_adapter_property(key, value, adapter_address=None):
    adapter = get_adapter_interface(adapter_address)
    adapter.Set(BLUEZ_ADAPTER, key, value)


def get_adapter_interface(adapter_address=None):
    adapter_path = bluezutils.find_adapter(adapter_address).object_path
    return dbus.Interface(bus.get_object(BLUEZ_OBJECT_PATH, adapter_path),
                          "org.freedesktop.DBus.Properties")


def get_adapter(adapter_address=None):
    return bluezutils.find_adapter(adapter_address)


def start_discovery(adapter_address=None):
    adapter = get_adapter(adapter_address)
    adapter.StartDiscovery()


def stop_discovery(address=None):
    adapter = get_adapter(address)
    adapter.StopDiscovery()


def remove_device(device_address, adapter_address=None):
    adapter = get_adapter(adapter_address)
    device = get_device(device_address)
    adapter.RemoveDevice(device.object_path)


def get_manager():
    return dbus.Interface(bus.get_object(BLUEZ_OBJECT_PATH, "/"),
                          "org.freedesktop.DBus.ObjectManager")


def get_managed_objects():
    return get_manager().GetManagedObjects()


def get_device(device_address):
    return bluezutils.find_device(device_address)


def connect_device(device_address):
    device = bluezutils.find_device(device_address)
    device.Connect()


def disconnect_device(device_address):
    device = bluezutils.find_device(device_address)
    device.Disconnect()


def get_device_interface(device_address):
    device = get_device(device_address)
    path = device.object_path
    return dbus.Interface(bus.get_object("org.bluez", path),
                          "org.freedesktop.DBus.Properties")


def get_device_property(device_address, key):
    device = get_device_interface(device_address)
    value = device.Get(BLUEZ_DEVICE, key)
    if isinstance(value, dbus.Boolean):
        return bool(value)
    return value


def set_device_property(device_address, key, value):
    device = get_device_interface(device_address)
    device.Set(BLUEZ_DEVICE, key, value)
