#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.networking

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import sys

import dbus
import dbus.mainloop.glib
import dbus.service

try:
    from gi.repository import GObject
except ImportError:
    import gobject as GObject

try:
    input = raw_input
except NameError:
    pass


class Canceled(dbus.DBusException):
    _dbus_error_name = "net.connman.Error.Canceled"


class LaunchBrowser(dbus.DBusException):
    _dbus_error_name = "net.connman.Agent.Error.LaunchBrowser"


class Agent(dbus.service.Object):
    name = None
    ssid = None
    identity = None
    passphrase = None
    wpspin = None
    username = None
    password = None

    @dbus.service.method("net.connman.Agent", in_signature='', out_signature='')
    def Release(self):
        print("Release")
        mainloop.quit()

    def input_passphrase(self):
        response = {}

        if not self.identity and not self.passphrase and not self.wpspin:
            print("Service credentials requested, type cancel to cancel")
            args = input('Answer: ')

            for arg in args.split():
                if arg.startswith("cancel"):
                    response["Error"] = arg
                if arg.startswith("Identity="):
                    identity = arg.replace("Identity=", "", 1)
                    response["Identity"] = identity
                if arg.startswith("Passphrase="):
                    passphrase = arg.replace("Passphrase=", "", 1)
                    response["Passphrase"] = passphrase
                if arg.startswith("WPS="):
                    wpspin = arg.replace("WPS=", "", 1)
                    response["WPS"] = wpspin
                    break
        else:
            if self.identity:
                response["Identity"] = self.identity
            if self.passphrase:
                response["Passphrase"] = self.passphrase
            if self.wpspin:
                response["WPS"] = self.wpspin

        return response

    def input_username(self):
        response = {}

        if not self.username and not self.password:
            print("User login requested, type cancel to cancel")
            print("or browser to login through the browser by yourself.")
            args = input('Answer: ')

            for arg in args.split():
                if arg.startswith("cancel") or arg.startswith("browser"):
                    response["Error"] = arg
                if arg.startswith("Username="):
                    username = arg.replace("Username=", "", 1)
                    response["Username"] = username
                if arg.startswith("Password="):
                    password = arg.replace("Password=", "", 1)
                    response["Password"] = password
        else:
            if self.username:
                response["Username"] = self.username
            if self.password:
                response["Password"] = self.password

        return response

    def input_hidden(self):
        response = {}

        if not self.name and not self.ssid:
            args = input('Answer ')

            for arg in args.split():
                if arg.startswith("Name="):
                    name = arg.replace("Name=", "", 1)
                    response["Name"] = name
                    break
                if arg.startswith("SSID="):
                    ssid = arg.replace("SSID", "", 1)
                    response["SSID"] = ssid
                    break

        else:
            if self.name:
                response["Name"] = self.name
            if self.ssid:
                response["SSID"] = self.ssid

        return response

    @dbus.service.method("net.connman.Agent", in_signature='oa{sv}', out_signature='a{sv}')
    def RequestInput(self, path, fields):
        print("RequestInput (%s,%s)" % (path, fields))

        response = {}

        if "Name" in fields:
            response.update(self.input_hidden())
        if "Passphrase" in fields:
            response.update(self.input_passphrase())
        if "Username" in fields:
            response.update(self.input_username())

        if "Error" in response:
            if response["Error"] == "cancel":
                raise Canceled("canceled")
            if response["Error"] == "browser":
                raise LaunchBrowser("launch browser")

        print("returning (%s)" % response)

        return response

    @dbus.service.method("net.connman.Agent", in_signature='os', out_signature='')
    def RequestBrowser(self, path, url):
        print("RequestBrowser (%s,%s)" % (path, url))

        print("Please login through the given url in a browser")
        print("Then press enter to accept or some text to cancel")

        args = input('> ')

        if len(args) > 0:
            raise Canceled("canceled")

        return

    @dbus.service.method("net.connman.Agent", in_signature='os', out_signature='')
    def ReportError(self, path, error):
        print("ReportError %s, %s" % (path, error))
        retry = input("Retry service (yes/no): ")
        if retry == "yes":
            class Retry(dbus.DBusException):
                _dbus_error_name = "net.connman.Agent.Error.Retry"

            raise Retry("retry service")

        else:
            return

    @dbus.service.method("net.connman.Agent", in_signature='', out_signature='')
    def Cancel(self):
        print("Cancel")


def argv():
    return sys.argv


if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = dbus.SystemBus()
    manager = dbus.Interface(bus.get_object('net.connman', "/"), 'net.connman.Manager')

    _path = "/test/agent"
    obj = Agent(bus, _path)

    if len(argv()) >= 2:
        for _arg in argv()[1:]:
            if _arg.startswith("fromfile"):
                with open("/tmp/preseed_data") as key_file:
                    data = key_file.read()

                lines = data.split('\n')
                if len(lines) > 0 and len(lines[0]) > 0:
                    obj.passphrase = lines[0]
                if len(lines) > 1 and len(lines[1]) > 0:
                    obj.name = lines[1]
                    obj.ssid = lines[1]

    try:
        manager.RegisterAgent(_path)
    except:
        print("Cannot register connman agent.")

    mainloop = GObject.MainLoop()
    mainloop.run()
