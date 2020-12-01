import dbus
import bluezutils
      
BLUEZ_OBJECT_PATH = 'org.bluez'
BLUEZ_ADAPTER = 'org.bluez.Adapter1'
BLUEZ_DEVICE = 'org.bluez.Device1'

bus = dbus.SystemBus()


def get_adapter_property(key, adapteraddress=None):
    adapter = get_adapter_interface(adapteraddress)
    value =  adapter.Get(BLUEZ_ADAPTER, key)
    if isinstance(value, dbus.Boolean):
        return bool(value)
    return value


def set_adapter_property(key, value, adapteraddress=None):
    adapter = get_adapter_interface(adapteraddress)
    adapter.Set(BLUEZ_ADAPTER, key, value)


def get_adapter_interface(adapteraddress=None):
    adapter_path = bluezutils.find_adapter(adapteraddress).object_path
    return dbus.Interface(bus.get_object(BLUEZ_OBJECT_PATH, adapter_path), "org.freedesktop.DBus.Properties")


def get_adapter(adapteraddress=None):
    return bluezutils.find_adapter(adapteraddress)


def start_discovery(adapteraddress=None):
    adapter = get_adapter(adapteraddress)
    adapter.StartDiscovery()


def stop_discovery(address=None):
    adapter = get_adapter(address)
    adapter.StopDiscovery()


def remove_device(deviceaddress, adapteraddress=None):
    adapter = get_adapter(adapteraddress)
    device = get_device(deviceaddress)
    adapter.RemoveDevice(device.object_path)


def get_manager():
    return dbus.Interface(bus.get_object(BLUEZ_OBJECT_PATH, "/"),"org.freedesktop.DBus.ObjectManager")


def get_managed_objects():
    return get_manager().GetManagedObjects()


def get_device(deviceaddress):
    return bluezutils.find_device(deviceaddress)


def connect_device(deviceaddress):
    device = bluezutils.find_device(deviceaddress)
    device.Connect()


def disconnect_device(deviceaddress):
    device = bluezutils.find_device(deviceaddress)
    device.Disconnect()
    

def get_device_interface(deviceaddress):
    device = get_device(deviceaddress)
    path = device.object_path
    return dbus.Interface(bus.get_object("org.bluez", path), "org.freedesktop.DBus.Properties")


def get_device_property(deviceaddress, key):
    device = get_device_interface(deviceaddress)
    value = device.Get(BLUEZ_DEVICE, key)
    if isinstance(value, dbus.Boolean):
        return bool(value)
    return value


def set_device_property(deviceaddress, key, value):
    device = get_device_interface(deviceaddress)
    device.Set(BLUEZ_DEVICE, key, value)

