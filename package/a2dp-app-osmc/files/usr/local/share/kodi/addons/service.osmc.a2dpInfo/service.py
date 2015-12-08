#!/usr/bin/python
# -*- coding: utf-8 -*-
'''

 This Program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This Program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with XBMC; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 http://www.gnu.org/copyleft/gpl.html
'''
import time
import xbmc
import xbmcaddon

import signal
import dbus
import dbus.service
import dbus.mainloop.glib
import gobject
import threading
import json

SERVICE_NAME = "org.bluez"
AGENT_IFACE = SERVICE_NAME + '.Agent1'
ADAPTER_IFACE = SERVICE_NAME + ".Adapter1"
DEVICE_IFACE = SERVICE_NAME + ".Device1"
PLAYER_IFACE = SERVICE_NAME + '.MediaPlayer1'
TRANSPORT_IFACE = SERVICE_NAME + '.MediaTransport1'

__addon__        = xbmcaddon.Addon()
__addonversion__ = __addon__.getAddonInfo('version')
__addonid__      = __addon__.getAddonInfo('id')
__addonname__    = __addon__.getAddonInfo('name')

def log(txt):
    message = '%s: %s' % (__addonname__, str(txt).encode('ascii', 'ignore'))
    xbmc.log(msg=message, level=xbmc.LOGDEBUG)

class A2DPInfo(threading.Thread):
    bus = None
    mainloop = None
    device = None
    deviceAlias = None
    player = None
    connected = None
    state = None
    status = None
    track = []

    def __init__(self):
        """Specify a signal handler, and find any connected media players"""
        super(A2DPInfo, self).__init__()

        self.bus = dbus.SystemBus()

        self.bus.add_signal_receiver(self.playerHandler,
                bus_name="org.bluez",
                dbus_interface="org.freedesktop.DBus.Properties",
                signal_name="PropertiesChanged",
                path_keyword="path")

        self.findPlayer()

    def run(self):
        """Start the BluePlayer by running the gobject Mainloop()"""
        gobject.threads_init()
        self.mainloop = gobject.MainLoop()
        self.mainloop.run()

    def end(self):
        """Stop the gobject Mainloop()"""
        if (self.mainloop):
            self.mainloop.quit();

    def findPlayer(self):
        """Find any current media players and associated device"""
        manager = dbus.Interface(self.bus.get_object("org.bluez", "/"), "org.freedesktop.DBus.ObjectManager")
        objects = manager.GetManagedObjects()

        player_path = None
        for path, interfaces in objects.iteritems():
            if PLAYER_IFACE in interfaces:
                player_path = path
                break

        if player_path:
            self.connected = True
            self.getPlayer(player_path)
            player_properties = self.player.GetAll(PLAYER_IFACE, dbus_interface="org.freedesktop.DBus.Properties")
            if "Status" in player_properties:
                self.status = player_properties["Status"]
            if "Track" in player_properties:
                self.track = player_properties["Track"]            
                
    def getPlayer(self, path):
        """Get a media player from a dbus path, and the associated device"""
        self.player = self.bus.get_object("org.bluez", path)
        device_path = self.player.Get("org.bluez.MediaPlayer1", "Device", dbus_interface="org.freedesktop.DBus.Properties")
        self.getDevice(device_path)

    def getDevice(self, path):
        """Get a device from a dbus path"""
        self.device = self.bus.get_object("org.bluez", path)
        self.deviceAlias = self.device.Get(DEVICE_IFACE, "Alias", dbus_interface="org.freedesktop.DBus.Properties")

    def playerHandler(self, interface, changed, invalidated, path):
        """Handle relevant property change signals"""
        iface = interface[interface.rfind(".") + 1:]
        if iface == "Device1":
            if "Connected" in changed:
                self.connected = changed["Connected"]
        elif iface == "MediaControl1":
            if "Connected" in changed:
                self.connected = changed["Connected"]
                if changed["Connected"]:
                    self.findPlayer()
        elif iface == "MediaPlayer1":
            if "Status" in changed:
                if not changed["Status"] == self.status:
                    self.status = changed["Status"]
#                    log("STATUS : " + str(self.status))
                    if self.status in ["stopped", "paused"]:
                        self.stopA2DP()
                    else:
                        self.trackChanged()
                    
            if "Track" in changed:
                self.track = changed["Track"]
                self.trackChanged()


    def trackChanged(self):
        if "playing" in self.status:
            artist = ""
            track  = ""
            album = ""
            if "Artist" in self.track:      artist = self.track["Artist"]
            if "Title" in self.track:       track = self.track["Title"]
            if "Album" in self.track:       album = self.track["Album"]
            self.trackIsPlaying(artist, track, album)

    def sendJSONRPC(self, method, params={}):
        try:
            payload = json.dumps({"jsonrpc": "2.0", "method": method, "params" :params,"id": 1})
            return xbmc.executeJSONRPC(payload)
        except Exception as ex:
            log("Error Sending JSON Request : " + str(json_payload) + " - "  + format(ex))

    def trackIsPlaying(self, artist, track, album=""):
        params = {"artist" : artist ,"track" : track, "album": album }
        return self.sendJSONRPC("OSMC.StartBTPlayer", params)

    def stopA2DP(self):
        jsonResponse = json.loads(self.sendJSONRPC("OSMC.BTPlayerActive"))
        if ("result" in jsonResponse and jsonResponse["result"] == "OK"):
            self.sendJSONRPC("OSMC.StopBTPlayer")


if __name__ == "__main__":
    log("A2DP Info Starting")
    try:
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        a2dpInfo = A2DPInfo()
        a2dpInfo.start()
        xbmc.log("Bluez Monitor Started", level=xbmc.LOGDEBUG)

        monitor = xbmc.Monitor()
        while not monitor.abortRequested():
            # Sleep/wait for abort for 10 seconds
            if monitor.waitForAbort(3):
                # Abort was requested while waiting. We should exit
                break
            # Would like to be able to get events when the track change buttons
            # are pressed here and if its from BTPlayer we can then pass that event
            # via AVRCP bluez dbus to change the track
            
                
    except Exception as e:
        print(e)
    finally:
        a2dpInfo.end()

    log("A2DP Info ended")
                            
