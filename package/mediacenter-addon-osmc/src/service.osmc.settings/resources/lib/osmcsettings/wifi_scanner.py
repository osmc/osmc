#!/usr/bin/env python

"""
   Original work Copyright 2010 Filia Dova, Georgios Migdos
   Modified work Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of service.osmc.settings

    SPDX-License-Identifier: Apache-2.0
    See LICENSES/Apache-2.0 for more information.
"""

import subprocess
import sys
import threading
import time
from io import open

PY3 = sys.version_info.major == 3


class WifiUtilities:
    @staticmethod
    def get_wireless_interfaces():
        interfaces = []
        command = ["iwconfig"]
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        process.wait()
        (stdoutdata, stderrdata) = process.communicate()
        lines = stdoutdata.splitlines()
        for line in lines:
            search_string = "IEEE 802.11"
            if PY3:
                search_string = search_string.encode('utf-8')
            if line.find(search_string) != -1:
                interfaces.append(line.split()[0])
        return interfaces


class WifiScanner(threading.Thread):
    """
        Calls iwlist periodically and parses its output - also gets GPS data from gpsd
    """

    def __init__(self, wifi_networks, interval):

        self.wifi_networks = wifi_networks
        self.interval = interval
        self.wireless_interface = None

        self._scanning = False
        self._wireless_interface = None

        self.stop_thread = threading.Event()
        threading.Thread.__init__(self)

    def run(self):
        self.stop_thread.clear()
        self.scanning = True

        while not self.stop_thread.isSet():
            self.scan_for_wifi_networks()
            time.sleep(self.interval)

        self.scanning = False

    def stop(self):
        self.stop_thread.set()

    @property
    def scanning(self):
        return self._scanning

    @scanning.setter
    def scanning(self, value):
        self._scanning = bool(value)

    def wifi_networks_list(self):
        result = []
        for _, value in self.wifi_networks.items():
            result.append(value)
        return result

    @property
    def wireless_interface(self):
        return self._wireless_interface

    @wireless_interface.setter
    def wireless_interface(self, value):
        self._wireless_interface = value

    def scan_for_wifi_networks(self):
        interface = self.wireless_interface
        if interface is not None:
            command = ["iwlist", interface, "scanning"]
            process = subprocess.Popen(command,
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            process.wait()
            (stdoutdata, stderrdata) = process.communicate()
            self.parse_iwlist_output(stdoutdata)

    @staticmethod
    def cut_from(value, pattern):
        index = value.find(pattern)
        if index > -1:
            return value[index + len(pattern):]
        else:
            return ""

    @staticmethod
    def cut_to(value, pattern):
        index = value.find(pattern)
        if index > -1:
            return value[:index]
        else:
            return value

    def parse_iwlist_output(self, output):
        output = self.cut_from(output, "Address:")
        while output != "":
            entry = self.cut_to(output, "Address:")

            essid = ""
            mode = ""
            channel = ""
            frequency = ""
            quality = ""
            signal = ""
            noise = ""
            encryption = ""

            address = entry[1:18]

            start_index = entry.find("ESSID:\"")
            if start_index > -1:
                end_index = entry.find("\"\n", start_index)
                essid = entry[start_index + 7:end_index]

            start_index = entry.find("Mode:")
            if start_index > -1:
                end_index = entry.find("\n", start_index)
                mode = entry[start_index + 5:end_index]

            start_index = entry.find("Channel:")
            if start_index > -1:
                end_index = entry.find("\n", start_index)
                channel = entry[start_index + 8:end_index]

            start_index = entry.find("Frequency:")
            if start_index > -1:
                end_index = entry.find("\n", start_index)
                frequency = entry[start_index + 10:end_index]

            start_index = entry.find("Quality=")
            if start_index > -1:
                end_index = entry.find("Signal", start_index) - 2
                qual = eval(entry[start_index + 8:end_index] + ".0")
                if qual > 1.0:
                    qual = 1.0
                quality = str(qual)

            start_index = entry.find("Signal level:")
            if start_index > -1:
                end_index = entry.find("dBm", start_index) - 1
                signal = entry[start_index + 13:end_index]

            start_index = entry.find("Noise level=")
            if start_index > -1:
                end_index = entry.find("dBm", start_index) - 1
                noise = entry[start_index + 12:end_index]

            start_index = entry.find("Encryption key:")
            if start_index > -1:
                end_index = entry.find("\n", start_index)
                encryption = entry[start_index + 15:end_index]

            key = (address, essid)
            value = [address, essid, mode, channel, frequency,
                     quality, signal, noise, encryption]
            try:
                old_value = self.wifi_networks[key]
                quality_n = eval(quality)
                old_quality_n = eval(old_value[5])
                if quality_n > old_quality_n:
                    self.wifi_networks[key] = value
            except KeyError:
                self.wifi_networks[key] = value

            output = self.cut_from(output, "Address:")

    def export_xml(self, filename):
        lst = self.wifi_networks_list()

        with open(filename, 'w', encoding='utf-8') as out:
            out.write('<?xml version="1.0" encoding="UTF-8"?>\n')
            out.write("<networkslist>\n")

            for lst_itm in lst:
                out.write("\t<network>\n")
                out.write("\t\t<address>" + lst_itm[0] + "</address>\n")
                out.write("\t\t<essid>" + lst_itm[1] + "</essid>\n")
                out.write("\t\t<mode>" + lst_itm[2] + "</mode>\n")
                out.write("\t\t<channel>" + lst_itm[3] + "</channel>\n")
                out.write("\t\t<frequency>" + lst_itm[4] + "</frequency>\n")
                out.write("\t\t<quality>" + lst_itm[5] + "</quality>\n")
                out.write("\t\t<signal>" + lst_itm[6] + "</signal>\n")
                out.write("\t\t<noise>" + lst_itm[7] + "</noise>\n")
                out.write("\t\t<security>" + lst_itm[8] + "</security>\n")

                out.write("\t</network>\n")

            out.write("</networkslist>\n")


# If there is only one wireless interface then just use that.
# If there is more than one then create window to select the wireless interface.
WIFI_INTERFACES = WifiUtilities().get_wireless_interfaces()
print(WIFI_INTERFACES)

INTERFACE = WIFI_INTERFACES[0]
SCANNER_INTERVAL = 5
UPDATE_INTERVAL = 2
WIFI_NETWORKS = {}

scanner = WifiScanner(WIFI_NETWORKS, SCANNER_INTERVAL)

scanner.wireless_interface = INTERFACE
scanner.start()

for x in range(20):
    time.sleep(2)
    print(len(scanner.wifi_networks))

print('Scan complete')
print(scanner.wifi_networks)

for k, connection in scanner.wifi_networks.items():
    print("Name: %s   Strength: %s" %
          (connection[1], (int(float(connection[5]) * 1000.0)) / 10.0))

print('Ended')
