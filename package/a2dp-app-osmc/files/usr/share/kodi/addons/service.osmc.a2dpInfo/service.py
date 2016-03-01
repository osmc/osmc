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
    transport = None
    connected = None
    state = None
    status = None
    # hack to hide the fact we always get false 'playing' events
    receivedPosition = None

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
                    if self.status in ["stopped"]:
                        xbmc.stopBTPlayer()
                        # hack to hide the fact we always get false 'playing' events
                        self.receivedPosition = None

            if "Track" in changed:
                self.startBTPlayer()

            # hack to hide the fact we always get false 'playing' events
            if "Position" in changed and not self.status == 'paused':
                newPosition = changed["Position"]
                if self.receivedPosition:
                    if not self.receivedPosition == newPosition:
                        self.startBTPlayer()
                else:
                     self.receivedPosition = newPosition;

    def isPlaying(self):
        return self.status == "playing"

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

    def startBTPlayer(self):
        if "playing" in self.status:
            if xbmc.isBTPlayerActive() == 0:
                log("Start BTPlayer")
                xbmc.startBTPlayer()

class BTPlayerMonitor(xbmc.Player):
    a2dpInfo = None

    def __init__( self, *args, **kwargs ):
        log("BTPlayerMonitor Started")

    def setA2DPInfo(self, a2dpInfo):
        self.a2dpInfo = a2dpInfo
        
    def onPlayBackStopped(self):
        if self.a2dpInfo.isPlaying():
            self.a2dpInfo.stop()
            
    def onPlayBackPaused(self):
        if self.a2dpInfo.isPlaying():
            self.a2dpInfo.pause()

    def onPlayBackResumed(self):
        if xbmc.isBTPlayerActive():
            self.a2dpInfo.play()
    
    def onNextItem(self):
        if self.a2dpInfo.isPlaying():
            self.a2dpInfo.next()

    def onPrevItem(self):
        if (self.a2dpInfo.isPlaying()):
            self.a2dpInfo.previous()

if __name__ == "__main__":
    log("A2DP Info Starting")
    try:
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        a2dpInfo = A2DPInfo()
        a2dpInfo.start()
        btPlayerMonitor = BTPlayerMonitor()
        btPlayerMonitor.setA2DPInfo(a2dpInfo)
        monitor = xbmc.Monitor()
        while not monitor.abortRequested():
            # Sleep/wait for abort for 10 seconds
            if monitor.waitForAbort(3):
                # Abort was requested while waiting. We should exit
                break
                
    except Exception as e:
        print(e)
    finally:
        a2dpInfo.end()

    log("A2DP Info ended")
                            
