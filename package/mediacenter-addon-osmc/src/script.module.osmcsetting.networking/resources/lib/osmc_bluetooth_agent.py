#!/usr/bin/python

from __future__ import absolute_import, print_function, unicode_literals

from optparse import OptionParser
import traceback
import dbus
import dbus.service
import dbus.mainloop.glib

try:
    from gi.repository import GObject
except ImportError:
    import gobject as GObject
import bluezutils

BUS_NAME = 'org.bluez'
AGENT_INTERFACE = 'org.bluez.Agent1'
AGENT_PATH = "/test/agent"

bus = None
device_obj = None
dev_path = None


def ask(prompt):
    try:
        return raw_input(prompt)
    except:
        return input(prompt)


def set_trusted(path, boolean):
    props = dbus.Interface(bus.get_object("org.bluez", path),
                           "org.freedesktop.DBus.Properties")
    props.Set("org.bluez.Device1", "Trusted", boolean)


def dev_connect(path):
    dev = dbus.Interface(bus.get_object("org.bluez", path),
                         "org.bluez.Device1")

    dev.Connect()


class Rejected(dbus.DBusException):
    _dbus_error_name = "org.bluez.Error.Rejected"


class Agent(dbus.service.Object):
    exit_on_release = True
    return_code = 0
    error_message = None

    def set_return(self, return_code, error_message):
        self.return_code = return_code
        self.error_message = error_message

    def set_exit_on_release(self, exit_on_release):
        self.exit_on_release = exit_on_release

    @dbus.service.method(AGENT_INTERFACE,
                         in_signature="", out_signature="")
    def Release(self):
        print("Release")
        if self.exit_on_release:
            mainloop.quit()

    @dbus.service.method(AGENT_INTERFACE,
                         in_signature="os", out_signature="")
    def AuthorizeService(self, device, uuid):
        print("AuthorizeService (%s, %s)" % (device, uuid))
        authorize = ask("Authorize connection (yes/no): ")
        if authorize == "yes":
            return
        raise Rejected("Connection rejected by user")

    @dbus.service.method(AGENT_INTERFACE,
                         in_signature="o", out_signature="s")
    def RequestPinCode(self, device):
        print("RequestPinCode (%s)" % (device))
        set_trusted(device, True)
        return ask("Enter PIN Code: ")

    @dbus.service.method(AGENT_INTERFACE,
                         in_signature="o", out_signature="u")
    def RequestPasskey(self, device):
        print("RequestPasskey (%s)" % (device))
        set_trusted(device, True)
        passkey = ask("Enter passkey: ")
        return dbus.UInt32(passkey)

    @dbus.service.method(AGENT_INTERFACE,
                         in_signature="ouq", out_signature="")
    def DisplayPasskey(self, device, passkey, entered):
        print("DisplayPasskey (%s, %06u entered %u)" %
              (device, passkey, entered))

    @dbus.service.method(AGENT_INTERFACE,
                         in_signature="os", out_signature="")
    def DisplayPinCode(self, device, pincode):
        print("DisplayPinCode (%s, %s)" % (device, pincode))

    @dbus.service.method(AGENT_INTERFACE,
                         in_signature="ou", out_signature="")
    def RequestConfirmation(self, device, passkey):
        print("RequestConfirmation (%s, %06d)" % (device, passkey))
        confirm = ask("Confirm passkey (yes/no): ")
        if (confirm == "yes"):
            set_trusted(device, True)
            return
        raise Rejected("Passkey doesn't match")

    @dbus.service.method(AGENT_INTERFACE,
                         in_signature="o", out_signature="")
    def RequestAuthorization(self, device):
        print("RequestAuthorization (%s)" % (device))
        auth = ask("Authorize? (yes/no): ")
        if (auth == "yes"):
            return
        raise Rejected("Pairing rejected")

    @dbus.service.method(AGENT_INTERFACE,
                         in_signature="", out_signature="")
    def Cancel(self):
        print("Cancel")


def pair_successful():
    print("Device paired")
    set_trusted(dev_path, True)
    try:
        dev_connect(dev_path)
    except Exception, e:
        print(traceback.format_exc())
        agent.set_return(1, "Exception Connecting")
        mainloop.quit()
        return

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

    path = "/test/agent"
    agent = Agent(bus, path)

    mainloop = GObject.MainLoop()

    obj = bus.get_object(BUS_NAME, "/org/bluez")
    manager = dbus.Interface(obj, "org.bluez.AgentManager1")
    manager.RegisterAgent(path, options.capability)

    if len(args) > 0:
        device = bluezutils.find_device(args[0],
                                        options.adapter_pattern)
        dev_path = device.object_path
        agent.set_exit_on_release(False)
        device.Pair(reply_handler=pair_successful, error_handler=pair_error,
                    timeout=options.timeout)
        device_obj = device
    else:
        manager.RequestDefaultAgent(path)

    mainloop.run()
    manager.UnregisterAgent(path)

    if agent.error_message is not None:
        print(agent.error_message)
    exit(agent.return_code)
