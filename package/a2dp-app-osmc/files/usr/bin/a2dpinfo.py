#!/usr/bin/env python

# Listens to Bluez dbus for a Media connection and on change of track will
# send a JSON-RPC request with the song details to Kodi which starts
# BTPlayer (a dummy player that allows us to display the details of the
# track playing via a2dp) On Pause or stop of a song a request for BTPlayer
# to stop

# (curenntly assumes the Kodi webserver is running on 8080 with no password)

# Dependencies:
# sudo apt-get install -y python-gobject python-requests


import time
import signal
import dbus
import dbus.service
import dbus.mainloop.glib
import gobject
import requests
import json
import urllib


SERVICE_NAME = "org.bluez"
AGENT_IFACE = SERVICE_NAME + '.Agent1'
ADAPTER_IFACE = SERVICE_NAME + ".Adapter1"
DEVICE_IFACE = SERVICE_NAME + ".Device1"
PLAYER_IFACE = SERVICE_NAME + '.MediaPlayer1'
TRANSPORT_IFACE = SERVICE_NAME + '.MediaTransport1'

class A2DPInfo():
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
        gobject.threads_init()
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)


        self.bus = dbus.SystemBus()

        self.bus.add_signal_receiver(self.playerHandler,
                bus_name="org.bluez",
                dbus_interface="org.freedesktop.DBus.Properties",
                signal_name="PropertiesChanged",
                path_keyword="path")

        self.findPlayer()

    def start(self):
        """Start the BluePlayer by running the gobject Mainloop()"""
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
                self.status = (changed["Status"])
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
            self.trackPlaying(artist, track, album)

    def sendJSONRPC(self, method, params={}):
        try:
            headers = {'content-type': 'application/json'}
            #Base URL of the json RPC calls.
            xbmc_json_rpc_url = "http://localhost:8080/jsonrpc"

            payload = {"jsonrpc": "2.0", "method": method, "params" :params,"id": 1}
            url_param = urllib.urlencode({'request': json.dumps(payload)})
            response =  requests.get(xbmc_json_rpc_url + '?' + url_param,
                                     headers=headers)
            return json.loads(response.text)["result"]
        except Exception as ex:
            print "Error Sending JSON Request : " + str(payload) + " - "  + format(ex)

    def trackPlaying(self, artist, track, album=""):
        params = {"artist" : artist ,"track" : track, "album": album }
        return self.sendJSONRPC("OSMC.StartBTPlayer", params)

    def stopA2DP(self):    
        return self.sendJSONRPC("OSMC.StopBTPlayer")

if __name__ == "__main__":
    player = None
    print "A2DP Info Service Started"
    try:
        player = A2DPInfo()
        player.start()
    except Exception as ex:
        print("How embarrassing. The following error occurred {}".format(ex))
    finally:
        if player: player.end()
