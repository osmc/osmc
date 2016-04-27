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

class BTPlayer(threading.Thread):
    bus = None
    mainloop = None
    device = None
    deviceAlias = None
    player = None
    transport = None
    connected = None
    state = None
    status = None

    def __init__(self):
        """Specify a signal handler, and find any connected media players"""
        super(BTPlayer, self).__init__()
        self.bus = dbus.SystemBus()
        self.bus.add_signal_receiver(self.playerHandler,
                bus_name="org.bluez",
                dbus_interface="org.freedesktop.DBus.Properties",
                signal_name="PropertiesChanged",
                path_keyword="path")
        self.findPlayer()

    def run(self):
        """Start monitoring bluez by running the gobject Mainloop()"""
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
        transport_path = None
        for path, interfaces in objects.iteritems():
            if PLAYER_IFACE in interfaces:
                player_path = path
                break
            if TRANSPORT_IFACE in interfaces:
                transport_path = path

        if player_path:
            self.connected = True
            self.getPlayer(player_path)
            player_properties = self.player.GetAll(PLAYER_IFACE, dbus_interface="org.freedesktop.DBus.Properties")
            if "Status" in player_properties:
                self.status = player_properties["Status"]
            if "Track" in player_properties:
                self.track = player_properties["Track"]            
        else:
            log("Could not find a player")

        if transport_path:
            self.transport = self.bus.get_object("org.bluez", transport_path)
            transport_properties = self.transport.GetAll(TRANSPORT_IFACE, dbus_interface="org.freedesktop.DBus.Properties")
            if "State" in transport_properties:
                self.state = transport_properties["State"]

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
        player_path = path.replace("/fd0", "/player0")
        if __addon__.getSetting("debug") == "true":
            log("Changed : " + str(changed))
            log("Interface : " + str(interface))
            log("Path : " + str(path))
            log("Player Path : " + str(player_path))
        if iface == "Device1":
            if "Connected" in changed:
                self.connected = changed["Connected"]
        elif iface == "MediaControl1":
            if "Connected" in changed:
                self.connected = changed["Connected"]
                if changed["Connected"]:
                    self.findPlayer()
        elif iface == "MediaTransport1":
            if "State" in changed:
                if not changed["State"] == self.state:
                    if changed["State"] == "active":
                        self.state = changed["State"]
                        xbmc.startBTPlayer(player_path);
                    else:
                        self.state = changed["State"]
                        xbmc.stopBTPlayer()
                        
    def isPlaying(self):
        return self.state == "active"

    def next(self):
        self.player.Next(dbus_interface=PLAYER_IFACE)

    def previous(self):
        self.player.Previous(dbus_interface=PLAYER_IFACE)

    def play(self):
        self.player.Play(dbus_interface=PLAYER_IFACE)

    def stop(self):
        self.player.Stop(dbus_interface=PLAYER_IFACE)
        self.receivedPosition = None;
        
    def pause(self):
        self.player.Pause(dbus_interface=PLAYER_IFACE)

    def volumeUp(self):
        self.control.VolumeUp(dbus_interface=CONTROL_IFACE)
        self.transport.VolumeUp(dbus_interface=TRANSPORT_IFACE)

    def volumeDown(self):
        self.control.VolumeDown(dbus_interface=CONTROL_IFACE)
        self.transport.VolumeDown(dbus_interface=TRANSPORT_IFACE)

    def startBTPlayer(self, macAddress):
        if "playing" in self.status:
            if xbmc.isBTPlayerActive() == 0:
                log("Start BTPlayer")
                xbmc.startBTPlayer(macAddress)

class BTPlayerMonitor(xbmc.Player):
    btPlayer = None

    def __init__( self, *args, **kwargs ):
        log("BTPlayerMonitor Started")

    def setBtPlayer(self, btPlayer):
        self.btPlayer = btPlayer
        
    def onPlayBackStopped(self):
        if self.btPlayer.isPlaying():
            self.btPlayer.stop()
            
    def onPlayBackPaused(self):
        if self.btPlayer.isPlaying():
            self.btPlayer.pause()

    def onPlayBackResumed(self):
        if xbmc.isBTPlayerActive():
            self.btPlayer.play()
    
    def onNextItem(self):
        if self.btPlayer.isPlaying():
            self.btPlayer.next()

    def onPrevItem(self):
        if (self.btPlayer.isPlaying()):
            self.btPlayer.previous()

if __name__ == "__main__":
    if __addon__.getSetting("enabled") == "true":
        log("BTPlayer Starting")
        try:
            dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
            btPlayer = BTPlayer()
            btPlayer.start()
            btPlayerMonitor = BTPlayerMonitor()
            btPlayerMonitor.setBtPlayer(btPlayer)
            monitor = xbmc.Monitor()
            while not monitor.abortRequested():
                # Sleep/wait for abort for 10 seconds
                if monitor.waitForAbort(3):
                    # Abort was requested while waiting. We should exit
                    break
                
        except Exception as e:
            print(e)
        finally:
            btPlayer.end()
        log("BTPlayer ended")
    else:
        log("BTPlayer disabled in addon settings")

                            
